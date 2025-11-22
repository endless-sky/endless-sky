/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2010 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <numeric>
#include <string>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "al/auxeffectslot.h"
#include "albit.h"
#include "alconfig.h"
#include "alc/context.h"
#include "almalloc.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "alspan.h"
#include "alstring.h"
#include "alu.h"
#include "core/ambdec.h"
#include "core/ambidefs.h"
#include "core/bformatdec.h"
#include "core/bs2b.h"
#include "core/devformat.h"
#include "core/front_stablizer.h"
#include "core/hrtf.h"
#include "core/logging.h"
#include "core/uhjfilter.h"
#include "device.h"
#include "opthelpers.h"


namespace {

using namespace std::placeholders;
using std::chrono::seconds;
using std::chrono::nanoseconds;

inline const char *GetLabelFromChannel(Channel channel)
{
    switch(channel)
    {
        case FrontLeft: return "front-left";
        case FrontRight: return "front-right";
        case FrontCenter: return "front-center";
        case LFE: return "lfe";
        case BackLeft: return "back-left";
        case BackRight: return "back-right";
        case BackCenter: return "back-center";
        case SideLeft: return "side-left";
        case SideRight: return "side-right";

        case TopFrontLeft: return "top-front-left";
        case TopFrontCenter: return "top-front-center";
        case TopFrontRight: return "top-front-right";
        case TopCenter: return "top-center";
        case TopBackLeft: return "top-back-left";
        case TopBackCenter: return "top-back-center";
        case TopBackRight: return "top-back-right";

        case MaxChannels: break;
    }
    return "(unknown)";
}


std::unique_ptr<FrontStablizer> CreateStablizer(const size_t outchans, const uint srate)
{
    auto stablizer = FrontStablizer::Create(outchans);
    for(auto &buf : stablizer->DelayBuf)
        std::fill(buf.begin(), buf.end(), 0.0f);

    /* Initialize band-splitting filter for the mid signal, with a crossover at
     * 5khz (could be higher).
     */
    stablizer->MidFilter.init(5000.0f / static_cast<float>(srate));

    return stablizer;
}

void AllocChannels(ALCdevice *device, const size_t main_chans, const size_t real_chans)
{
    TRACE("Channel config, Main: %zu, Real: %zu\n", main_chans, real_chans);

    /* Allocate extra channels for any post-filter output. */
    const size_t num_chans{main_chans + real_chans};

    TRACE("Allocating %zu channels, %zu bytes\n", num_chans,
        num_chans*sizeof(device->MixBuffer[0]));
    device->MixBuffer.resize(num_chans);
    al::span<FloatBufferLine> buffer{device->MixBuffer};

    device->Dry.Buffer = buffer.first(main_chans);
    buffer = buffer.subspan(main_chans);
    if(real_chans != 0)
    {
        device->RealOut.Buffer = buffer.first(real_chans);
        buffer = buffer.subspan(real_chans);
    }
    else
        device->RealOut.Buffer = device->Dry.Buffer;
}


using ChannelCoeffs = std::array<float,MaxAmbiChannels>;
enum DecoderMode : bool {
    SingleBand = false,
    DualBand = true
};

template<DecoderMode Mode, size_t N>
struct DecoderConfig;

template<size_t N>
struct DecoderConfig<SingleBand, N> {
    uint8_t mOrder{};
    bool mIs3D{};
    std::array<Channel,N> mChannels{};
    DevAmbiScaling mScaling{};
    std::array<float,MaxAmbiOrder+1> mOrderGain{};
    std::array<ChannelCoeffs,N> mCoeffs{};
};

template<size_t N>
struct DecoderConfig<DualBand, N> {
    uint8_t mOrder{};
    bool mIs3D{};
    std::array<Channel,N> mChannels{};
    DevAmbiScaling mScaling{};
    std::array<float,MaxAmbiOrder+1> mOrderGain{};
    std::array<ChannelCoeffs,N> mCoeffs{};
    std::array<float,MaxAmbiOrder+1> mOrderGainLF{};
    std::array<ChannelCoeffs,N> mCoeffsLF{};
};

template<>
struct DecoderConfig<DualBand, 0> {
    uint8_t mOrder{};
    bool mIs3D{};
    al::span<const Channel> mChannels;
    DevAmbiScaling mScaling{};
    al::span<const float> mOrderGain;
    al::span<const ChannelCoeffs> mCoeffs;
    al::span<const float> mOrderGainLF;
    al::span<const ChannelCoeffs> mCoeffsLF;

    template<size_t N>
    DecoderConfig& operator=(const DecoderConfig<SingleBand,N> &rhs) noexcept
    {
        mOrder = rhs.mOrder;
        mIs3D = rhs.mIs3D;
        mChannels = rhs.mChannels;
        mScaling = rhs.mScaling;
        mOrderGain = rhs.mOrderGain;
        mCoeffs = rhs.mCoeffs;
        mOrderGainLF = {};
        mCoeffsLF = {};
        return *this;
    }

    template<size_t N>
    DecoderConfig& operator=(const DecoderConfig<DualBand,N> &rhs) noexcept
    {
        mOrder = rhs.mOrder;
        mIs3D = rhs.mIs3D;
        mChannels = rhs.mChannels;
        mScaling = rhs.mScaling;
        mOrderGain = rhs.mOrderGain;
        mCoeffs = rhs.mCoeffs;
        mOrderGainLF = rhs.mOrderGainLF;
        mCoeffsLF = rhs.mCoeffsLF;
        return *this;
    }
};
using DecoderView = DecoderConfig<DualBand, 0>;


void InitNearFieldCtrl(ALCdevice *device, float ctrl_dist, uint order, bool is3d)
{
    static const uint chans_per_order2d[MaxAmbiOrder+1]{ 1, 2, 2, 2 };
    static const uint chans_per_order3d[MaxAmbiOrder+1]{ 1, 3, 5, 7 };

    /* NFC is only used when AvgSpeakerDist is greater than 0. */
    if(!device->getConfigValueBool("decoder", "nfc", 0) || !(ctrl_dist > 0.0f))
        return;

    device->AvgSpeakerDist = clampf(ctrl_dist, 0.1f, 10.0f);
    TRACE("Using near-field reference distance: %.2f meters\n", device->AvgSpeakerDist);

    const float w1{SpeedOfSoundMetersPerSec /
        (device->AvgSpeakerDist * static_cast<float>(device->Frequency))};
    device->mNFCtrlFilter.init(w1);

    auto iter = std::copy_n(is3d ? chans_per_order3d : chans_per_order2d, order+1u,
        std::begin(device->NumChannelsPerOrder));
    std::fill(iter, std::end(device->NumChannelsPerOrder), 0u);
}

void InitDistanceComp(ALCdevice *device, const al::span<const Channel> channels,
    const al::span<const float,MAX_OUTPUT_CHANNELS> dists)
{
    const float maxdist{std::accumulate(std::begin(dists), std::end(dists), 0.0f, maxf)};

    if(!device->getConfigValueBool("decoder", "distance-comp", 1) || !(maxdist > 0.0f))
        return;

    const auto distSampleScale = static_cast<float>(device->Frequency) / SpeedOfSoundMetersPerSec;
    std::vector<DistanceComp::ChanData> ChanDelay;
    ChanDelay.reserve(device->RealOut.Buffer.size());
    size_t total{0u};
    for(size_t chidx{0};chidx < channels.size();++chidx)
    {
        const Channel ch{channels[chidx]};
        const uint idx{device->RealOut.ChannelIndex[ch]};
        if(idx == INVALID_CHANNEL_INDEX)
            continue;

        const float distance{dists[chidx]};

        /* Distance compensation only delays in steps of the sample rate. This
         * is a bit less accurate since the delay time falls to the nearest
         * sample time, but it's far simpler as it doesn't have to deal with
         * phase offsets. This means at 48khz, for instance, the distance delay
         * will be in steps of about 7 millimeters.
         */
        float delay{std::floor((maxdist - distance)*distSampleScale + 0.5f)};
        if(delay > float{MAX_DELAY_LENGTH-1})
        {
            ERR("Delay for channel %u (%s) exceeds buffer length (%f > %d)\n", idx,
                GetLabelFromChannel(ch), delay, MAX_DELAY_LENGTH-1);
            delay = float{MAX_DELAY_LENGTH-1};
        }

        ChanDelay.resize(maxz(ChanDelay.size(), idx+1));
        ChanDelay[idx].Length = static_cast<uint>(delay);
        ChanDelay[idx].Gain = distance / maxdist;
        TRACE("Channel %s distance comp: %u samples, %f gain\n", GetLabelFromChannel(ch),
            ChanDelay[idx].Length, ChanDelay[idx].Gain);

        /* Round up to the next 4th sample, so each channel buffer starts
         * 16-byte aligned.
         */
        total += RoundUp(ChanDelay[idx].Length, 4);
    }

    if(total > 0)
    {
        auto chandelays = DistanceComp::Create(total);

        ChanDelay[0].Buffer = chandelays->mSamples.data();
        auto set_bufptr = [](const DistanceComp::ChanData &last, const DistanceComp::ChanData &cur)
            -> DistanceComp::ChanData
        {
            DistanceComp::ChanData ret{cur};
            ret.Buffer = last.Buffer + RoundUp(last.Length, 4);
            return ret;
        };
        std::partial_sum(ChanDelay.begin(), ChanDelay.end(), chandelays->mChannels.begin(),
            set_bufptr);
        device->ChannelDelays = std::move(chandelays);
    }
}


inline auto& GetAmbiScales(DevAmbiScaling scaletype) noexcept
{
    if(scaletype == DevAmbiScaling::FuMa) return AmbiScale::FromFuMa();
    if(scaletype == DevAmbiScaling::SN3D) return AmbiScale::FromSN3D();
    return AmbiScale::FromN3D();
}

inline auto& GetAmbiLayout(DevAmbiLayout layouttype) noexcept
{
    if(layouttype == DevAmbiLayout::FuMa) return AmbiIndex::FromFuMa();
    return AmbiIndex::FromACN();
}


DecoderView MakeDecoderView(ALCdevice *device, const AmbDecConf *conf,
    DecoderConfig<DualBand, MAX_OUTPUT_CHANNELS> &decoder)
{
    DecoderView ret{};

    decoder.mOrder = (conf->ChanMask > Ambi2OrderMask) ? uint8_t{3} :
        (conf->ChanMask > Ambi1OrderMask) ? uint8_t{2} : uint8_t{1};
    decoder.mIs3D = (conf->ChanMask&AmbiPeriphonicMask) != 0;

    switch(conf->CoeffScale)
    {
    case AmbDecScale::N3D: decoder.mScaling = DevAmbiScaling::N3D; break;
    case AmbDecScale::SN3D: decoder.mScaling = DevAmbiScaling::SN3D; break;
    case AmbDecScale::FuMa: decoder.mScaling = DevAmbiScaling::FuMa; break;
    }

    std::copy_n(std::begin(conf->HFOrderGain),
        std::min(al::size(conf->HFOrderGain), al::size(decoder.mOrderGain)),
        std::begin(decoder.mOrderGain));
    std::copy_n(std::begin(conf->LFOrderGain),
        std::min(al::size(conf->LFOrderGain), al::size(decoder.mOrderGainLF)),
        std::begin(decoder.mOrderGainLF));

    std::array<uint8_t,MaxAmbiChannels> idx_map{};
    if(decoder.mIs3D)
    {
        uint flags{conf->ChanMask};
        auto elem = idx_map.begin();
        while(flags)
        {
            int acn{al::countr_zero(flags)};
            flags &= ~(1u<<acn);

            *elem = static_cast<uint8_t>(acn);
            ++elem;
        }
    }
    else
    {
        uint flags{conf->ChanMask};
        auto elem = idx_map.begin();
        while(flags)
        {
            int acn{al::countr_zero(flags)};
            flags &= ~(1u<<acn);

            switch(acn)
            {
            case 0: *elem = 0; break;
            case 1: *elem = 1; break;
            case 3: *elem = 2; break;
            case 4: *elem = 3; break;
            case 8: *elem = 4; break;
            case 9: *elem = 5; break;
            case 15: *elem = 6; break;
            default: return ret;
            }
            ++elem;
        }
    }
    const auto num_coeffs = static_cast<uint>(al::popcount(conf->ChanMask));
    const auto hfmatrix = conf->HFMatrix;
    const auto lfmatrix = conf->LFMatrix;

    uint chan_count{0};
    using const_speaker_span = al::span<const AmbDecConf::SpeakerConf>;
    for(auto &speaker : const_speaker_span{conf->Speakers.get(), conf->NumSpeakers})
    {
        /* NOTE: AmbDec does not define any standard speaker names, however
         * for this to work we have to by able to find the output channel
         * the speaker definition corresponds to. Therefore, OpenAL Soft
         * requires these channel labels to be recognized:
         *
         * LF = Front left
         * RF = Front right
         * LS = Side left
         * RS = Side right
         * LB = Back left
         * RB = Back right
         * CE = Front center
         * CB = Back center
         *
         * Additionally, surround51 will acknowledge back speakers for side
         * channels, to avoid issues with an ambdec expecting 5.1 to use the
         * back channels.
         */
        Channel ch{};
        if(speaker.Name == "LF")
            ch = FrontLeft;
        else if(speaker.Name == "RF")
            ch = FrontRight;
        else if(speaker.Name == "CE")
            ch = FrontCenter;
        else if(speaker.Name == "LS")
            ch = SideLeft;
        else if(speaker.Name == "RS")
            ch = SideRight;
        else if(speaker.Name == "LB")
            ch = (device->FmtChans == DevFmtX51) ? SideLeft : BackLeft;
        else if(speaker.Name == "RB")
            ch = (device->FmtChans == DevFmtX51) ? SideRight : BackRight;
        else if(speaker.Name == "CB")
            ch = BackCenter;
        else
        {
            ERR("AmbDec speaker label \"%s\" not recognized\n", speaker.Name.c_str());
            continue;
        }

        decoder.mChannels[chan_count] = ch;
        for(size_t src{0};src < num_coeffs;++src)
        {
            const size_t dst{idx_map[src]};
            decoder.mCoeffs[chan_count][dst] = hfmatrix[chan_count][src];
        }
        if(conf->FreqBands > 1)
        {
            for(size_t src{0};src < num_coeffs;++src)
            {
                const size_t dst{idx_map[src]};
                decoder.mCoeffsLF[chan_count][dst] = lfmatrix[chan_count][src];
            }
        }
        ++chan_count;
    }

    if(chan_count > 0)
    {
        ret.mOrder = decoder.mOrder;
        ret.mIs3D = decoder.mIs3D;
        ret.mScaling = decoder.mScaling;
        ret.mChannels = {decoder.mChannels.data(), chan_count};
        ret.mOrderGain = decoder.mOrderGain;
        ret.mCoeffs = {decoder.mCoeffs.data(), chan_count};
        if(conf->FreqBands > 1)
        {
            ret.mOrderGainLF = decoder.mOrderGainLF;
            ret.mCoeffsLF = {decoder.mCoeffsLF.data(), chan_count};
        }
    }
    return ret;
}

constexpr DecoderConfig<SingleBand, 1> MonoConfig{
    0, false, {{FrontCenter}},
    DevAmbiScaling::N3D,
    {{1.0f}},
    {{ {{1.0f}} }}
};
constexpr DecoderConfig<SingleBand, 2> StereoConfig{
    1, false, {{FrontLeft, FrontRight}},
    DevAmbiScaling::N3D,
    {{1.0f, 1.0f}},
    {{
        {{5.00000000e-1f,  2.88675135e-1f,  5.52305643e-2f}},
        {{5.00000000e-1f, -2.88675135e-1f,  5.52305643e-2f}},
    }}
};
constexpr DecoderConfig<DualBand, 4> QuadConfig{
    2, false, {{BackLeft, FrontLeft, FrontRight, BackRight}},
    DevAmbiScaling::N3D,
    /*HF*/{{1.15470054e+0f, 1.00000000e+0f, 5.77350269e-1f}},
    {{
        {{2.50000000e-1f,  2.04124145e-1f, -2.04124145e-1f, -1.29099445e-1f, 0.00000000e+0f}},
        {{2.50000000e-1f,  2.04124145e-1f,  2.04124145e-1f,  1.29099445e-1f, 0.00000000e+0f}},
        {{2.50000000e-1f, -2.04124145e-1f,  2.04124145e-1f, -1.29099445e-1f, 0.00000000e+0f}},
        {{2.50000000e-1f, -2.04124145e-1f, -2.04124145e-1f,  1.29099445e-1f, 0.00000000e+0f}},
    }},
    /*LF*/{{1.00000000e+0f, 1.00000000e+0f, 1.00000000e+0f}},
    {{
        {{2.50000000e-1f,  2.04124145e-1f, -2.04124145e-1f, -1.29099445e-1f, 0.00000000e+0f}},
        {{2.50000000e-1f,  2.04124145e-1f,  2.04124145e-1f,  1.29099445e-1f, 0.00000000e+0f}},
        {{2.50000000e-1f, -2.04124145e-1f,  2.04124145e-1f, -1.29099445e-1f, 0.00000000e+0f}},
        {{2.50000000e-1f, -2.04124145e-1f, -2.04124145e-1f,  1.29099445e-1f, 0.00000000e+0f}},
    }}
};
constexpr DecoderConfig<DualBand, 5> X51Config{
    2, false, {{SideLeft, FrontLeft, FrontCenter, FrontRight, SideRight}},
    DevAmbiScaling::FuMa,
    /*HF*/{{1.00000000e+0f, 1.00000000e+0f, 1.00000000e+0f}},
    {{
        {{5.67316000e-1f,  4.22920000e-1f, -3.15495000e-1f, -6.34490000e-2f, -2.92380000e-2f}},
        {{3.68584000e-1f,  2.72349000e-1f,  3.21616000e-1f,  1.92645000e-1f,  4.82600000e-2f}},
        {{1.83579000e-1f,  0.00000000e+0f,  1.99588000e-1f,  0.00000000e+0f,  9.62820000e-2f}},
        {{3.68584000e-1f, -2.72349000e-1f,  3.21616000e-1f, -1.92645000e-1f,  4.82600000e-2f}},
        {{5.67316000e-1f, -4.22920000e-1f, -3.15495000e-1f,  6.34490000e-2f, -2.92380000e-2f}},
    }},
    /*LF*/{{1.00000000e+0f, 1.00000000e+0f, 1.00000000e+0f}},
    {{
        {{4.90109850e-1f,  3.77305010e-1f, -3.73106990e-1f, -1.25914530e-1f,  1.45133000e-2f}},
        {{1.49085730e-1f,  3.03561680e-1f,  1.53290060e-1f,  2.45112480e-1f, -1.50753130e-1f}},
        {{1.37654920e-1f,  0.00000000e+0f,  4.49417940e-1f,  0.00000000e+0f,  2.57844070e-1f}},
        {{1.49085730e-1f, -3.03561680e-1f,  1.53290060e-1f, -2.45112480e-1f, -1.50753130e-1f}},
        {{4.90109850e-1f, -3.77305010e-1f, -3.73106990e-1f,  1.25914530e-1f,  1.45133000e-2f}},
    }}
};
constexpr DecoderConfig<SingleBand, 5> X61Config{
    2, false, {{SideLeft, FrontLeft, FrontRight, SideRight, BackCenter}},
    DevAmbiScaling::N3D,
    {{1.0f, 1.0f, 1.0f}},
    {{
        {{2.04460341e-1f,  2.17177926e-1f, -4.39996780e-2f, -2.60790269e-2f, -6.87239792e-2f}},
        {{1.58923161e-1f,  9.21772680e-2f,  1.59658796e-1f,  6.66278083e-2f,  3.84686854e-2f}},
        {{1.58923161e-1f, -9.21772680e-2f,  1.59658796e-1f, -6.66278083e-2f,  3.84686854e-2f}},
        {{2.04460341e-1f, -2.17177926e-1f, -4.39996780e-2f,  2.60790269e-2f, -6.87239792e-2f}},
        {{2.50001688e-1f,  0.00000000e+0f, -2.50000094e-1f,  0.00000000e+0f,  6.05133395e-2f}},
    }}
};
constexpr DecoderConfig<DualBand, 6> X71Config{
    3, false, {{BackLeft, SideLeft, FrontLeft, FrontRight, SideRight, BackRight}},
    DevAmbiScaling::N3D,
    /*HF*/{{1.22474487e+0f, 1.13151672e+0f, 8.66025404e-1f, 4.68689571e-1f}},
    {{
        {{1.66666667e-1f,  9.62250449e-2f, -1.66666667e-1f, -1.49071198e-1f,  8.60662966e-2f,  7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f,  1.92450090e-1f,  0.00000000e+0f,  0.00000000e+0f, -1.72132593e-1f, -7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f,  9.62250449e-2f,  1.66666667e-1f,  1.49071198e-1f,  8.60662966e-2f,  7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f, -9.62250449e-2f,  1.66666667e-1f, -1.49071198e-1f,  8.60662966e-2f, -7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f, -1.92450090e-1f,  0.00000000e+0f,  0.00000000e+0f, -1.72132593e-1f,  7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f, -9.62250449e-2f, -1.66666667e-1f,  1.49071198e-1f,  8.60662966e-2f, -7.96819073e-2f, 0.00000000e+0f}},
    }},
    /*LF*/{{1.00000000e+0f, 1.00000000e+0f, 1.00000000e+0f, 1.00000000e+0f}},
    {{
        {{1.66666667e-1f,  9.62250449e-2f, -1.66666667e-1f, -1.49071198e-1f,  8.60662966e-2f,  7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f,  1.92450090e-1f,  0.00000000e+0f,  0.00000000e+0f, -1.72132593e-1f, -7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f,  9.62250449e-2f,  1.66666667e-1f,  1.49071198e-1f,  8.60662966e-2f,  7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f, -9.62250449e-2f,  1.66666667e-1f, -1.49071198e-1f,  8.60662966e-2f, -7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f, -1.92450090e-1f,  0.00000000e+0f,  0.00000000e+0f, -1.72132593e-1f,  7.96819073e-2f, 0.00000000e+0f}},
        {{1.66666667e-1f, -9.62250449e-2f, -1.66666667e-1f,  1.49071198e-1f,  8.60662966e-2f, -7.96819073e-2f, 0.00000000e+0f}},
    }}
};

void InitPanning(ALCdevice *device, const bool hqdec=false, const bool stablize=false,
    DecoderView decoder={})
{
    if(!decoder.mOrder)
    {
        switch(device->FmtChans)
        {
        case DevFmtMono: decoder = MonoConfig; break;
        case DevFmtStereo: decoder = StereoConfig; break;
        case DevFmtQuad: decoder = QuadConfig; break;
        case DevFmtX51: decoder = X51Config; break;
        case DevFmtX61: decoder = X61Config; break;
        case DevFmtX71: decoder = X71Config; break;
        case DevFmtAmbi3D:
            auto&& acnmap = GetAmbiLayout(device->mAmbiLayout);
            auto&& n3dscale = GetAmbiScales(device->mAmbiScale);

            /* For DevFmtAmbi3D, the ambisonic order is already set. */
            const size_t count{AmbiChannelsFromOrder(device->mAmbiOrder)};
            std::transform(acnmap.begin(), acnmap.begin()+count, std::begin(device->Dry.AmbiMap),
                [&n3dscale](const uint8_t &acn) noexcept -> BFChannelConfig
                { return BFChannelConfig{1.0f/n3dscale[acn], acn}; });
            AllocChannels(device, count, 0);

            float nfc_delay{device->configValue<float>("decoder", "nfc-ref-delay").value_or(0.0f)};
            if(nfc_delay > 0.0f)
                InitNearFieldCtrl(device, nfc_delay * SpeedOfSoundMetersPerSec, device->mAmbiOrder,
                    true);
            return;
        }
    }

    const bool dual_band{hqdec && !decoder.mCoeffsLF.empty()};
    al::vector<ChannelDec> chancoeffs, chancoeffslf;
    for(size_t i{0u};i < decoder.mChannels.size();++i)
    {
        const uint idx{GetChannelIdxByName(device->RealOut, decoder.mChannels[i])};
        if(idx == INVALID_CHANNEL_INDEX)
        {
            ERR("Failed to find %s channel in device\n",
                GetLabelFromChannel(decoder.mChannels[i]));
            continue;
        }

        chancoeffs.resize(maxz(chancoeffs.size(), idx+1u), ChannelDec{});
        al::span<float,MaxAmbiChannels> coeffs{chancoeffs[idx]};
        size_t ambichan{0};
        for(uint o{0};o < decoder.mOrder+1u;++o)
        {
            const float order_gain{decoder.mOrderGain[o]};
            const size_t order_max{decoder.mIs3D ? AmbiChannelsFromOrder(o) :
                Ambi2DChannelsFromOrder(o)};
            for(;ambichan < order_max;++ambichan)
                coeffs[ambichan] = decoder.mCoeffs[i][ambichan] * order_gain;
        }
        if(!dual_band)
            continue;

        chancoeffslf.resize(maxz(chancoeffslf.size(), idx+1u), ChannelDec{});
        coeffs = chancoeffslf[idx];
        ambichan = 0;
        for(uint o{0};o < decoder.mOrder+1u;++o)
        {
            const float order_gain{decoder.mOrderGainLF[o]};
            const size_t order_max{decoder.mIs3D ? AmbiChannelsFromOrder(o) :
                Ambi2DChannelsFromOrder(o)};
            for(;ambichan < order_max;++ambichan)
                coeffs[ambichan] = decoder.mCoeffsLF[i][ambichan] * order_gain;
        }
    }

    /* For non-DevFmtAmbi3D, set the ambisonic order. */
    device->mAmbiOrder = decoder.mOrder;

    const size_t ambicount{decoder.mIs3D ? AmbiChannelsFromOrder(decoder.mOrder) :
        Ambi2DChannelsFromOrder(decoder.mOrder)};
    const al::span<const uint8_t> acnmap{decoder.mIs3D ? AmbiIndex::FromACN().data() :
        AmbiIndex::FromACN2D().data(), ambicount};
    auto&& coeffscale = GetAmbiScales(decoder.mScaling);
    std::transform(acnmap.begin(), acnmap.end(), std::begin(device->Dry.AmbiMap),
        [&coeffscale](const uint8_t &acn) noexcept
        { return BFChannelConfig{1.0f/coeffscale[acn], acn}; });
    AllocChannels(device, ambicount, device->channelsFromFmt());

    std::unique_ptr<FrontStablizer> stablizer;
    if(stablize)
    {
        /* Only enable the stablizer if the decoder does not output to the
         * front-center channel.
         */
        const auto cidx = device->RealOut.ChannelIndex[FrontCenter];
        bool hasfc{false};
        if(cidx < chancoeffs.size())
        {
            for(const auto &coeff : chancoeffs[cidx])
                hasfc |= coeff != 0.0f;
        }
        if(!hasfc && cidx < chancoeffslf.size())
        {
            for(const auto &coeff : chancoeffslf[cidx])
                hasfc |= coeff != 0.0f;
        }
        if(!hasfc)
        {
            stablizer = CreateStablizer(device->channelsFromFmt(), device->Frequency);
            TRACE("Front stablizer enabled\n");
        }
    }

    TRACE("Enabling %s-band %s-order%s ambisonic decoder\n",
        !dual_band ? "single" : "dual",
        (decoder.mOrder > 2) ? "third" :
        (decoder.mOrder > 1) ? "second" : "first",
        decoder.mIs3D ? " periphonic" : "");
    device->AmbiDecoder = BFormatDec::Create(ambicount, chancoeffs, chancoeffslf,
        device->mXOverFreq/static_cast<float>(device->Frequency), std::move(stablizer));
}

void InitHrtfPanning(ALCdevice *device)
{
    constexpr float Deg180{al::numbers::pi_v<float>};
    constexpr float Deg_90{Deg180 / 2.0f /* 90 degrees*/};
    constexpr float Deg_45{Deg_90 / 2.0f /* 45 degrees*/};
    constexpr float Deg135{Deg_45 * 3.0f /*135 degrees*/};
    constexpr float Deg_35{6.154797087e-01f /* 35~ 36 degrees*/};
    constexpr float Deg_69{1.205932499e+00f /* 69~ 70 degrees*/};
    constexpr float Deg111{1.935660155e+00f /*110~111 degrees*/};
    constexpr float Deg_21{3.648638281e-01f /* 20~ 21 degrees*/};
    static const AngularPoint AmbiPoints1O[]{
        { EvRadians{ Deg_35}, AzRadians{-Deg_45} },
        { EvRadians{ Deg_35}, AzRadians{-Deg135} },
        { EvRadians{ Deg_35}, AzRadians{ Deg_45} },
        { EvRadians{ Deg_35}, AzRadians{ Deg135} },
        { EvRadians{-Deg_35}, AzRadians{-Deg_45} },
        { EvRadians{-Deg_35}, AzRadians{-Deg135} },
        { EvRadians{-Deg_35}, AzRadians{ Deg_45} },
        { EvRadians{-Deg_35}, AzRadians{ Deg135} },
    }, AmbiPoints2O[]{
        { EvRadians{   0.0f}, AzRadians{   0.0f} },
        { EvRadians{   0.0f}, AzRadians{ Deg180} },
        { EvRadians{   0.0f}, AzRadians{-Deg_90} },
        { EvRadians{   0.0f}, AzRadians{ Deg_90} },
        { EvRadians{ Deg_90}, AzRadians{   0.0f} },
        { EvRadians{-Deg_90}, AzRadians{   0.0f} },
        { EvRadians{ Deg_35}, AzRadians{-Deg_45} },
        { EvRadians{ Deg_35}, AzRadians{-Deg135} },
        { EvRadians{ Deg_35}, AzRadians{ Deg_45} },
        { EvRadians{ Deg_35}, AzRadians{ Deg135} },
        { EvRadians{-Deg_35}, AzRadians{-Deg_45} },
        { EvRadians{-Deg_35}, AzRadians{-Deg135} },
        { EvRadians{-Deg_35}, AzRadians{ Deg_45} },
        { EvRadians{-Deg_35}, AzRadians{ Deg135} },
    }, AmbiPoints3O[]{
        { EvRadians{ Deg_69}, AzRadians{-Deg_90} },
        { EvRadians{ Deg_69}, AzRadians{ Deg_90} },
        { EvRadians{-Deg_69}, AzRadians{-Deg_90} },
        { EvRadians{-Deg_69}, AzRadians{ Deg_90} },
        { EvRadians{   0.0f}, AzRadians{-Deg_69} },
        { EvRadians{   0.0f}, AzRadians{-Deg111} },
        { EvRadians{   0.0f}, AzRadians{ Deg_69} },
        { EvRadians{   0.0f}, AzRadians{ Deg111} },
        { EvRadians{ Deg_21}, AzRadians{   0.0f} },
        { EvRadians{ Deg_21}, AzRadians{ Deg180} },
        { EvRadians{-Deg_21}, AzRadians{   0.0f} },
        { EvRadians{-Deg_21}, AzRadians{ Deg180} },
        { EvRadians{ Deg_35}, AzRadians{-Deg_45} },
        { EvRadians{ Deg_35}, AzRadians{-Deg135} },
        { EvRadians{ Deg_35}, AzRadians{ Deg_45} },
        { EvRadians{ Deg_35}, AzRadians{ Deg135} },
        { EvRadians{-Deg_35}, AzRadians{-Deg_45} },
        { EvRadians{-Deg_35}, AzRadians{-Deg135} },
        { EvRadians{-Deg_35}, AzRadians{ Deg_45} },
        { EvRadians{-Deg_35}, AzRadians{ Deg135} },
    };
    static const float AmbiMatrix1O[][MaxAmbiChannels]{
        { 1.250000000e-01f,  1.250000000e-01f,  1.250000000e-01f,  1.250000000e-01f },
        { 1.250000000e-01f,  1.250000000e-01f,  1.250000000e-01f, -1.250000000e-01f },
        { 1.250000000e-01f, -1.250000000e-01f,  1.250000000e-01f,  1.250000000e-01f },
        { 1.250000000e-01f, -1.250000000e-01f,  1.250000000e-01f, -1.250000000e-01f },
        { 1.250000000e-01f,  1.250000000e-01f, -1.250000000e-01f,  1.250000000e-01f },
        { 1.250000000e-01f,  1.250000000e-01f, -1.250000000e-01f, -1.250000000e-01f },
        { 1.250000000e-01f, -1.250000000e-01f, -1.250000000e-01f,  1.250000000e-01f },
        { 1.250000000e-01f, -1.250000000e-01f, -1.250000000e-01f, -1.250000000e-01f },
    }, AmbiMatrix2O[][MaxAmbiChannels]{
        { 7.142857143e-02f,  0.000000000e+00f,  0.000000000e+00f,  1.237179148e-01f,  0.000000000e+00f,  0.000000000e+00f, -7.453559925e-02f,  0.000000000e+00f,  1.290994449e-01f, },
        { 7.142857143e-02f,  0.000000000e+00f,  0.000000000e+00f, -1.237179148e-01f,  0.000000000e+00f,  0.000000000e+00f, -7.453559925e-02f,  0.000000000e+00f,  1.290994449e-01f, },
        { 7.142857143e-02f,  1.237179148e-01f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f, -7.453559925e-02f,  0.000000000e+00f, -1.290994449e-01f, },
        { 7.142857143e-02f, -1.237179148e-01f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f, -7.453559925e-02f,  0.000000000e+00f, -1.290994449e-01f, },
        { 7.142857143e-02f,  0.000000000e+00f,  1.237179148e-01f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f,  1.490711985e-01f,  0.000000000e+00f,  0.000000000e+00f, },
        { 7.142857143e-02f,  0.000000000e+00f, -1.237179148e-01f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f,  1.490711985e-01f,  0.000000000e+00f,  0.000000000e+00f, },
        { 7.142857143e-02f,  7.142857143e-02f,  7.142857143e-02f,  7.142857143e-02f,  9.682458366e-02f,  9.682458366e-02f,  0.000000000e+00f,  9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f,  7.142857143e-02f,  7.142857143e-02f, -7.142857143e-02f, -9.682458366e-02f,  9.682458366e-02f,  0.000000000e+00f, -9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f, -7.142857143e-02f,  7.142857143e-02f,  7.142857143e-02f, -9.682458366e-02f, -9.682458366e-02f,  0.000000000e+00f,  9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f, -7.142857143e-02f,  7.142857143e-02f, -7.142857143e-02f,  9.682458366e-02f, -9.682458366e-02f,  0.000000000e+00f, -9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f,  7.142857143e-02f, -7.142857143e-02f,  7.142857143e-02f,  9.682458366e-02f, -9.682458366e-02f,  0.000000000e+00f, -9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f,  7.142857143e-02f, -7.142857143e-02f, -7.142857143e-02f, -9.682458366e-02f, -9.682458366e-02f,  0.000000000e+00f,  9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f, -7.142857143e-02f, -7.142857143e-02f,  7.142857143e-02f, -9.682458366e-02f,  9.682458366e-02f,  0.000000000e+00f, -9.682458366e-02f,  0.000000000e+00f, },
        { 7.142857143e-02f, -7.142857143e-02f, -7.142857143e-02f, -7.142857143e-02f,  9.682458366e-02f,  9.682458366e-02f,  0.000000000e+00f,  9.682458366e-02f,  0.000000000e+00f, },
    }, AmbiMatrix3O[][MaxAmbiChannels]{
        { 5.000000000e-02f,  3.090169944e-02f,  8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f,  6.454972244e-02f,  9.045084972e-02f,  0.000000000e+00f, -1.232790000e-02f, -1.256118221e-01f,  0.000000000e+00f,  1.126112056e-01f,  7.944389175e-02f,  0.000000000e+00f,  2.421151497e-02f,  0.000000000e+00f, },
        { 5.000000000e-02f, -3.090169944e-02f,  8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f, -6.454972244e-02f,  9.045084972e-02f,  0.000000000e+00f, -1.232790000e-02f,  1.256118221e-01f,  0.000000000e+00f, -1.126112056e-01f,  7.944389175e-02f,  0.000000000e+00f,  2.421151497e-02f,  0.000000000e+00f, },
        { 5.000000000e-02f,  3.090169944e-02f, -8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f, -6.454972244e-02f,  9.045084972e-02f,  0.000000000e+00f, -1.232790000e-02f, -1.256118221e-01f,  0.000000000e+00f,  1.126112056e-01f, -7.944389175e-02f,  0.000000000e+00f, -2.421151497e-02f,  0.000000000e+00f, },
        { 5.000000000e-02f, -3.090169944e-02f, -8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f,  6.454972244e-02f,  9.045084972e-02f,  0.000000000e+00f, -1.232790000e-02f,  1.256118221e-01f,  0.000000000e+00f, -1.126112056e-01f, -7.944389175e-02f,  0.000000000e+00f, -2.421151497e-02f,  0.000000000e+00f, },
        { 5.000000000e-02f,  8.090169944e-02f,  0.000000000e+00f,  3.090169944e-02f,  6.454972244e-02f,  0.000000000e+00f, -5.590169944e-02f,  0.000000000e+00f, -7.216878365e-02f, -7.763237543e-02f,  0.000000000e+00f, -2.950836627e-02f,  0.000000000e+00f, -1.497759251e-01f,  0.000000000e+00f, -7.763237543e-02f, },
        { 5.000000000e-02f,  8.090169944e-02f,  0.000000000e+00f, -3.090169944e-02f, -6.454972244e-02f,  0.000000000e+00f, -5.590169944e-02f,  0.000000000e+00f, -7.216878365e-02f, -7.763237543e-02f,  0.000000000e+00f, -2.950836627e-02f,  0.000000000e+00f,  1.497759251e-01f,  0.000000000e+00f,  7.763237543e-02f, },
        { 5.000000000e-02f, -8.090169944e-02f,  0.000000000e+00f,  3.090169944e-02f, -6.454972244e-02f,  0.000000000e+00f, -5.590169944e-02f,  0.000000000e+00f, -7.216878365e-02f,  7.763237543e-02f,  0.000000000e+00f,  2.950836627e-02f,  0.000000000e+00f, -1.497759251e-01f,  0.000000000e+00f, -7.763237543e-02f, },
        { 5.000000000e-02f, -8.090169944e-02f,  0.000000000e+00f, -3.090169944e-02f,  6.454972244e-02f,  0.000000000e+00f, -5.590169944e-02f,  0.000000000e+00f, -7.216878365e-02f,  7.763237543e-02f,  0.000000000e+00f,  2.950836627e-02f,  0.000000000e+00f,  1.497759251e-01f,  0.000000000e+00f,  7.763237543e-02f, },
        { 5.000000000e-02f,  0.000000000e+00f,  3.090169944e-02f,  8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f, -3.454915028e-02f,  6.454972244e-02f,  8.449668365e-02f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f,  3.034486645e-02f, -6.779013272e-02f,  1.659481923e-01f,  4.797944664e-02f, },
        { 5.000000000e-02f,  0.000000000e+00f,  3.090169944e-02f, -8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f, -3.454915028e-02f, -6.454972244e-02f,  8.449668365e-02f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f,  3.034486645e-02f,  6.779013272e-02f,  1.659481923e-01f, -4.797944664e-02f, },
        { 5.000000000e-02f,  0.000000000e+00f, -3.090169944e-02f,  8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f, -3.454915028e-02f, -6.454972244e-02f,  8.449668365e-02f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f, -3.034486645e-02f, -6.779013272e-02f, -1.659481923e-01f,  4.797944664e-02f, },
        { 5.000000000e-02f,  0.000000000e+00f, -3.090169944e-02f, -8.090169944e-02f,  0.000000000e+00f,  0.000000000e+00f, -3.454915028e-02f,  6.454972244e-02f,  8.449668365e-02f,  0.000000000e+00f,  0.000000000e+00f,  0.000000000e+00f, -3.034486645e-02f,  6.779013272e-02f, -1.659481923e-01f, -4.797944664e-02f, },
        { 5.000000000e-02f,  5.000000000e-02f,  5.000000000e-02f,  5.000000000e-02f,  6.454972244e-02f,  6.454972244e-02f,  0.000000000e+00f,  6.454972244e-02f,  0.000000000e+00f,  1.016220987e-01f,  6.338656910e-02f, -1.092600649e-02f, -7.364853795e-02f,  1.011266756e-01f, -7.086833869e-02f, -1.482646439e-02f, },
        { 5.000000000e-02f,  5.000000000e-02f,  5.000000000e-02f, -5.000000000e-02f, -6.454972244e-02f,  6.454972244e-02f,  0.000000000e+00f, -6.454972244e-02f,  0.000000000e+00f,  1.016220987e-01f, -6.338656910e-02f, -1.092600649e-02f, -7.364853795e-02f, -1.011266756e-01f, -7.086833869e-02f,  1.482646439e-02f, },
        { 5.000000000e-02f, -5.000000000e-02f,  5.000000000e-02f,  5.000000000e-02f, -6.454972244e-02f, -6.454972244e-02f,  0.000000000e+00f,  6.454972244e-02f,  0.000000000e+00f, -1.016220987e-01f, -6.338656910e-02f,  1.092600649e-02f, -7.364853795e-02f,  1.011266756e-01f, -7.086833869e-02f, -1.482646439e-02f, },
        { 5.000000000e-02f, -5.000000000e-02f,  5.000000000e-02f, -5.000000000e-02f,  6.454972244e-02f, -6.454972244e-02f,  0.000000000e+00f, -6.454972244e-02f,  0.000000000e+00f, -1.016220987e-01f,  6.338656910e-02f,  1.092600649e-02f, -7.364853795e-02f, -1.011266756e-01f, -7.086833869e-02f,  1.482646439e-02f, },
        { 5.000000000e-02f,  5.000000000e-02f, -5.000000000e-02f,  5.000000000e-02f,  6.454972244e-02f, -6.454972244e-02f,  0.000000000e+00f, -6.454972244e-02f,  0.000000000e+00f,  1.016220987e-01f, -6.338656910e-02f, -1.092600649e-02f,  7.364853795e-02f,  1.011266756e-01f,  7.086833869e-02f, -1.482646439e-02f, },
        { 5.000000000e-02f,  5.000000000e-02f, -5.000000000e-02f, -5.000000000e-02f, -6.454972244e-02f, -6.454972244e-02f,  0.000000000e+00f,  6.454972244e-02f,  0.000000000e+00f,  1.016220987e-01f,  6.338656910e-02f, -1.092600649e-02f,  7.364853795e-02f, -1.011266756e-01f,  7.086833869e-02f,  1.482646439e-02f, },
        { 5.000000000e-02f, -5.000000000e-02f, -5.000000000e-02f,  5.000000000e-02f, -6.454972244e-02f,  6.454972244e-02f,  0.000000000e+00f, -6.454972244e-02f,  0.000000000e+00f, -1.016220987e-01f,  6.338656910e-02f,  1.092600649e-02f,  7.364853795e-02f,  1.011266756e-01f,  7.086833869e-02f, -1.482646439e-02f, },
        { 5.000000000e-02f, -5.000000000e-02f, -5.000000000e-02f, -5.000000000e-02f,  6.454972244e-02f,  6.454972244e-02f,  0.000000000e+00f,  6.454972244e-02f,  0.000000000e+00f, -1.016220987e-01f, -6.338656910e-02f,  1.092600649e-02f,  7.364853795e-02f, -1.011266756e-01f,  7.086833869e-02f,  1.482646439e-02f, },
    };
    static const float AmbiOrderHFGain1O[MaxAmbiOrder+1]{
        /*ENRGY*/ 2.000000000e+00f, 1.154700538e+00f
    }, AmbiOrderHFGain2O[MaxAmbiOrder+1]{
        /*ENRGY 2.357022604e+00f, 1.825741858e+00f, 9.428090416e-01f*/
        /*AMP   1.000000000e+00f, 7.745966692e-01f, 4.000000000e-01f*/
        /*RMS*/ 9.128709292e-01f, 7.071067812e-01f, 3.651483717e-01f
    }, AmbiOrderHFGain3O[MaxAmbiOrder+1]{
        /*ENRGY 1.865086714e+00f, 1.606093894e+00f, 1.142055301e+00f, 5.683795528e-01f*/
        /*AMP   1.000000000e+00f, 8.611363116e-01f, 6.123336207e-01f, 3.047469850e-01f*/
        /*RMS*/ 8.340921354e-01f, 7.182670250e-01f, 5.107426573e-01f, 2.541870634e-01f
    };

    static_assert(al::size(AmbiPoints1O) == al::size(AmbiMatrix1O), "First-Order Ambisonic HRTF mismatch");
    static_assert(al::size(AmbiPoints2O) == al::size(AmbiMatrix2O), "Second-Order Ambisonic HRTF mismatch");
    static_assert(al::size(AmbiPoints3O) == al::size(AmbiMatrix3O), "Third-Order Ambisonic HRTF mismatch");

    /* A 700hz crossover frequency provides tighter sound imaging at the sweet
     * spot with ambisonic decoding, as the distance between the ears is closer
     * to half this frequency wavelength, which is the optimal point where the
     * response should change between optimizing phase vs volume. Normally this
     * tighter imaging is at the cost of a smaller sweet spot, but since the
     * listener is fixed in the center of the HRTF responses for the decoder,
     * we don't have to worry about ever being out of the sweet spot.
     *
     * A better option here may be to have the head radius as part of the HRTF
     * data set and calculate the optimal crossover frequency from that.
     */
    device->mXOverFreq = 700.0f;

    /* Don't bother with HOA when using full HRTF rendering. Nothing needs it,
     * and it eases the CPU/memory load.
     */
    device->mRenderMode = RenderMode::Hrtf;
    uint ambi_order{1};
    if(auto modeopt = device->configValue<std::string>(nullptr, "hrtf-mode"))
    {
        struct HrtfModeEntry {
            char name[8];
            RenderMode mode;
            uint order;
        };
        static const HrtfModeEntry hrtf_modes[]{
            { "full", RenderMode::Hrtf, 1 },
            { "ambi1", RenderMode::Normal, 1 },
            { "ambi2", RenderMode::Normal, 2 },
            { "ambi3", RenderMode::Normal, 3 },
        };

        const char *mode{modeopt->c_str()};
        if(al::strcasecmp(mode, "basic") == 0)
        {
            ERR("HRTF mode \"%s\" deprecated, substituting \"%s\"\n", mode, "ambi2");
            mode = "ambi2";
        }

        auto match_entry = [mode](const HrtfModeEntry &entry) -> bool
        { return al::strcasecmp(mode, entry.name) == 0; };
        auto iter = std::find_if(std::begin(hrtf_modes), std::end(hrtf_modes), match_entry);
        if(iter == std::end(hrtf_modes))
            ERR("Unexpected hrtf-mode: %s\n", mode);
        else
        {
            device->mRenderMode = iter->mode;
            ambi_order = iter->order;
        }
    }
    TRACE("%u%s order %sHRTF rendering enabled, using \"%s\"\n", ambi_order,
        (((ambi_order%100)/10) == 1) ? "th" :
        ((ambi_order%10) == 1) ? "st" :
        ((ambi_order%10) == 2) ? "nd" :
        ((ambi_order%10) == 3) ? "rd" : "th",
        (device->mRenderMode == RenderMode::Hrtf) ? "+ Full " : "",
        device->mHrtfName.c_str());

    al::span<const AngularPoint> AmbiPoints{AmbiPoints1O};
    const float (*AmbiMatrix)[MaxAmbiChannels]{AmbiMatrix1O};
    al::span<const float,MaxAmbiOrder+1> AmbiOrderHFGain{AmbiOrderHFGain1O};
    if(ambi_order >= 3)
    {
        AmbiPoints = AmbiPoints3O;
        AmbiMatrix = AmbiMatrix3O;
        AmbiOrderHFGain = AmbiOrderHFGain3O;
    }
    else if(ambi_order == 2)
    {
        AmbiPoints = AmbiPoints2O;
        AmbiMatrix = AmbiMatrix2O;
        AmbiOrderHFGain = AmbiOrderHFGain2O;
    }
    device->mAmbiOrder = ambi_order;

    const size_t count{AmbiChannelsFromOrder(ambi_order)};
    std::transform(AmbiIndex::FromACN().begin(), AmbiIndex::FromACN().begin()+count,
        std::begin(device->Dry.AmbiMap),
        [](const uint8_t &index) noexcept { return BFChannelConfig{1.0f, index}; }
    );
    AllocChannels(device, count, device->channelsFromFmt());

    HrtfStore *Hrtf{device->mHrtf.get()};
    auto hrtfstate = DirectHrtfState::Create(count);
    hrtfstate->build(Hrtf, device->mIrSize, AmbiPoints, AmbiMatrix, device->mXOverFreq,
        AmbiOrderHFGain);
    device->mHrtfState = std::move(hrtfstate);

    InitNearFieldCtrl(device, Hrtf->field[0].distance, ambi_order, true);
}

void InitUhjPanning(ALCdevice *device)
{
    /* UHJ is always 2D first-order. */
    constexpr size_t count{Ambi2DChannelsFromOrder(1)};

    device->mAmbiOrder = 1;

    auto acnmap_begin = AmbiIndex::FromFuMa().begin();
    std::transform(acnmap_begin, acnmap_begin + count, std::begin(device->Dry.AmbiMap),
        [](const uint8_t &acn) noexcept -> BFChannelConfig
        { return BFChannelConfig{1.0f/AmbiScale::FromUHJ()[acn], acn}; });
    AllocChannels(device, count, device->channelsFromFmt());
}

} // namespace

void aluInitRenderer(ALCdevice *device, int hrtf_id, al::optional<StereoEncoding> stereomode)
{
    /* Hold the HRTF the device last used, in case it's used again. */
    HrtfStorePtr old_hrtf{std::move(device->mHrtf)};

    device->mHrtfState = nullptr;
    device->mHrtf = nullptr;
    device->mIrSize = 0;
    device->mHrtfName.clear();
    device->mXOverFreq = 400.0f;
    device->mRenderMode = RenderMode::Normal;

    if(device->FmtChans != DevFmtStereo)
    {
        old_hrtf = nullptr;
        if(stereomode && *stereomode == StereoEncoding::Hrtf)
            device->mHrtfStatus = ALC_HRTF_UNSUPPORTED_FORMAT_SOFT;

        const char *layout{nullptr};
        switch(device->FmtChans)
        {
        case DevFmtQuad: layout = "quad"; break;
        case DevFmtX51: layout = "surround51"; break;
        case DevFmtX61: layout = "surround61"; break;
        case DevFmtX71: layout = "surround71"; break;
        /* Mono, Stereo, and Ambisonics output don't use custom decoders. */
        case DevFmtMono:
        case DevFmtStereo:
        case DevFmtAmbi3D:
            break;
        }

        std::unique_ptr<DecoderConfig<DualBand,MAX_OUTPUT_CHANNELS>> decoder_store;
        DecoderView decoder{};
        float speakerdists[MaxChannels]{};
        auto load_config = [device,&decoder_store,&decoder,&speakerdists](const char *config)
        {
            AmbDecConf conf{};
            if(auto err = conf.load(config))
            {
                ERR("Failed to load layout file %s\n", config);
                ERR("  %s\n", err->c_str());
            }
            else if(conf.NumSpeakers > MAX_OUTPUT_CHANNELS)
                ERR("Unsupported decoder speaker count %zu (max %d)\n", conf.NumSpeakers,
                    MAX_OUTPUT_CHANNELS);
            else if(conf.ChanMask > Ambi3OrderMask)
                ERR("Unsupported decoder channel mask 0x%04x (max 0x%x)\n", conf.ChanMask,
                    Ambi3OrderMask);
            else
            {
                device->mXOverFreq = clampf(conf.XOverFreq, 100.0f, 1000.0f);

                decoder_store = std::make_unique<DecoderConfig<DualBand,MAX_OUTPUT_CHANNELS>>();
                decoder = MakeDecoderView(device, &conf, *decoder_store);
                for(size_t i{0};i < decoder.mChannels.size();++i)
                    speakerdists[i] = conf.Speakers[i].Distance;
            }
        };
        if(layout)
        {
            if(auto decopt = device->configValue<std::string>("decoder", layout))
                load_config(decopt->c_str());
        }

        /* Enable the stablizer only for formats that have front-left, front-
         * right, and front-center outputs.
         */
        const bool stablize{device->RealOut.ChannelIndex[FrontCenter] != INVALID_CHANNEL_INDEX
            && device->RealOut.ChannelIndex[FrontLeft] != INVALID_CHANNEL_INDEX
            && device->RealOut.ChannelIndex[FrontRight] != INVALID_CHANNEL_INDEX
            && device->getConfigValueBool(nullptr, "front-stablizer", 0) != 0};
        const bool hqdec{device->getConfigValueBool("decoder", "hq-mode", 1) != 0};
        InitPanning(device, hqdec, stablize, decoder);
        if(decoder.mOrder > 0)
        {
            float accum_dist{0.0f}, spkr_count{0.0f};
            for(auto dist : speakerdists)
            {
                if(dist > 0.0f)
                {
                    accum_dist += dist;
                    spkr_count += 1.0f;
                }
            }
            if(spkr_count > 0)
            {
                InitNearFieldCtrl(device, accum_dist / spkr_count, decoder.mOrder, decoder.mIs3D);
                InitDistanceComp(device, decoder.mChannels, speakerdists);
            }
        }
        if(auto *ambidec{device->AmbiDecoder.get()})
        {
            device->PostProcess = ambidec->hasStablizer() ? &ALCdevice::ProcessAmbiDecStablized
                : &ALCdevice::ProcessAmbiDec;
        }
        return;
    }


    /* If HRTF is explicitly requested, or if there's no explicit request and
     * the device is headphones, try to enable it.
     */
    if(stereomode.value_or(StereoEncoding::Default) == StereoEncoding::Hrtf
        || (!stereomode && device->Flags.test(DirectEar)))
    {
        if(device->mHrtfList.empty())
            device->enumerateHrtfs();

        if(hrtf_id >= 0 && static_cast<uint>(hrtf_id) < device->mHrtfList.size())
        {
            const std::string &hrtfname = device->mHrtfList[static_cast<uint>(hrtf_id)];
            if(HrtfStorePtr hrtf{GetLoadedHrtf(hrtfname, device->Frequency)})
            {
                device->mHrtf = std::move(hrtf);
                device->mHrtfName = hrtfname;
            }
        }

        if(!device->mHrtf)
        {
            for(const auto &hrtfname : device->mHrtfList)
            {
                if(HrtfStorePtr hrtf{GetLoadedHrtf(hrtfname, device->Frequency)})
                {
                    device->mHrtf = std::move(hrtf);
                    device->mHrtfName = hrtfname;
                    break;
                }
            }
        }

        if(device->mHrtf)
        {
            old_hrtf = nullptr;

            HrtfStore *hrtf{device->mHrtf.get()};
            device->mIrSize = hrtf->irSize;
            if(auto hrtfsizeopt = device->configValue<uint>(nullptr, "hrtf-size"))
            {
                if(*hrtfsizeopt > 0 && *hrtfsizeopt < device->mIrSize)
                    device->mIrSize = maxu(*hrtfsizeopt, MinIrLength);
            }

            InitHrtfPanning(device);
            device->PostProcess = &ALCdevice::ProcessHrtf;
            device->mHrtfStatus = ALC_HRTF_ENABLED_SOFT;
            return;
        }
    }
    old_hrtf = nullptr;

    if(stereomode.value_or(StereoEncoding::Default) == StereoEncoding::Uhj)
    {
        device->mUhjEncoder = std::make_unique<UhjEncoder>();
        TRACE("UHJ enabled\n");
        InitUhjPanning(device);
        device->PostProcess = &ALCdevice::ProcessUhj;
        return;
    }

    device->mRenderMode = RenderMode::Pairwise;
    if(device->Type != DeviceType::Loopback)
    {
        if(auto cflevopt = device->configValue<int>(nullptr, "cf_level"))
        {
            if(*cflevopt > 0 && *cflevopt <= 6)
            {
                device->Bs2b = std::make_unique<bs2b>();
                bs2b_set_params(device->Bs2b.get(), *cflevopt,
                    static_cast<int>(device->Frequency));
                TRACE("BS2B enabled\n");
                InitPanning(device);
                device->PostProcess = &ALCdevice::ProcessBs2b;
                return;
            }
        }
    }

    TRACE("Stereo rendering\n");
    InitPanning(device);
    device->PostProcess = &ALCdevice::ProcessAmbiDec;
}


void aluInitEffectPanning(EffectSlot *slot, ALCcontext *context)
{
    DeviceBase *device{context->mDevice};
    const size_t count{AmbiChannelsFromOrder(device->mAmbiOrder)};

    auto wetbuffer_iter = context->mWetBuffers.end();
    if(slot->mWetBuffer)
    {
        /* If the effect slot already has a wet buffer attached, allocate a new
         * one in its place.
         */
        wetbuffer_iter = context->mWetBuffers.begin();
        for(;wetbuffer_iter != context->mWetBuffers.end();++wetbuffer_iter)
        {
            if(wetbuffer_iter->get() == slot->mWetBuffer)
            {
                slot->mWetBuffer = nullptr;
                slot->Wet.Buffer = {};

                *wetbuffer_iter = WetBufferPtr{new(FamCount(count)) WetBuffer{count}};

                break;
            }
        }
    }
    if(wetbuffer_iter == context->mWetBuffers.end())
    {
        /* Otherwise, search for an unused wet buffer. */
        wetbuffer_iter = context->mWetBuffers.begin();
        for(;wetbuffer_iter != context->mWetBuffers.end();++wetbuffer_iter)
        {
            if(!(*wetbuffer_iter)->mInUse)
                break;
        }
        if(wetbuffer_iter == context->mWetBuffers.end())
        {
            /* Otherwise, allocate a new one to use. */
            context->mWetBuffers.emplace_back(WetBufferPtr{new(FamCount(count)) WetBuffer{count}});
            wetbuffer_iter = context->mWetBuffers.end()-1;
        }
    }
    WetBuffer *wetbuffer{slot->mWetBuffer = wetbuffer_iter->get()};
    wetbuffer->mInUse = true;

    auto acnmap_begin = AmbiIndex::FromACN().begin();
    auto iter = std::transform(acnmap_begin, acnmap_begin + count, slot->Wet.AmbiMap.begin(),
        [](const uint8_t &acn) noexcept -> BFChannelConfig
        { return BFChannelConfig{1.0f, acn}; });
    std::fill(iter, slot->Wet.AmbiMap.end(), BFChannelConfig{});
    slot->Wet.Buffer = wetbuffer->mBuffer;
}
