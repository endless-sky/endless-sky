/**
 * OpenAL cross platform audio library
 * Copyright (C) 2013 by Mike Gorchak
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
#include <climits>
#include <cstdlib>
#include <iterator>

#include "alc/effects/base.h"
#include "almalloc.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "alspan.h"
#include "core/bufferline.h"
#include "core/context.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/mixer.h"
#include "core/mixer/defs.h"
#include "core/resampler_limits.h"
#include "intrusive_ptr.h"
#include "opthelpers.h"
#include "vector.h"


namespace {

using uint = unsigned int;

#define MAX_UPDATE_SAMPLES 256

struct ChorusState final : public EffectState {
    al::vector<float,16> mSampleBuffer;
    uint mOffset{0};

    uint mLfoOffset{0};
    uint mLfoRange{1};
    float mLfoScale{0.0f};
    uint mLfoDisp{0};

    /* Gains for left and right sides */
    struct {
        float Current[MAX_OUTPUT_CHANNELS]{};
        float Target[MAX_OUTPUT_CHANNELS]{};
    } mGains[2];

    /* effect parameters */
    ChorusWaveform mWaveform{};
    int mDelay{0};
    float mDepth{0.0f};
    float mFeedback{0.0f};

    void getTriangleDelays(uint (*delays)[MAX_UPDATE_SAMPLES], const size_t todo);
    void getSinusoidDelays(uint (*delays)[MAX_UPDATE_SAMPLES], const size_t todo);

    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(ChorusState)
};

void ChorusState::deviceUpdate(const DeviceBase *Device, const Buffer&)
{
    constexpr float max_delay{maxf(ChorusMaxDelay, FlangerMaxDelay)};

    const auto frequency = static_cast<float>(Device->Frequency);
    const size_t maxlen{NextPowerOf2(float2uint(max_delay*2.0f*frequency) + 1u)};
    if(maxlen != mSampleBuffer.size())
        al::vector<float,16>(maxlen).swap(mSampleBuffer);

    std::fill(mSampleBuffer.begin(), mSampleBuffer.end(), 0.0f);
    for(auto &e : mGains)
    {
        std::fill(std::begin(e.Current), std::end(e.Current), 0.0f);
        std::fill(std::begin(e.Target), std::end(e.Target), 0.0f);
    }
}

void ChorusState::update(const ContextBase *Context, const EffectSlot *Slot,
    const EffectProps *props, const EffectTarget target)
{
    constexpr int mindelay{(MaxResamplerPadding>>1) << MixerFracBits};

    /* The LFO depth is scaled to be relative to the sample delay. Clamp the
     * delay and depth to allow enough padding for resampling.
     */
    const DeviceBase *device{Context->mDevice};
    const auto frequency = static_cast<float>(device->Frequency);

    mWaveform = props->Chorus.Waveform;

    mDelay = maxi(float2int(props->Chorus.Delay*frequency*MixerFracOne + 0.5f), mindelay);
    mDepth = minf(props->Chorus.Depth * static_cast<float>(mDelay),
        static_cast<float>(mDelay - mindelay));

    mFeedback = props->Chorus.Feedback;

    /* Gains for left and right sides */
    const auto lcoeffs = CalcDirectionCoeffs({-1.0f, 0.0f, 0.0f}, 0.0f);
    const auto rcoeffs = CalcDirectionCoeffs({ 1.0f, 0.0f, 0.0f}, 0.0f);

    mOutTarget = target.Main->Buffer;
    ComputePanGains(target.Main, lcoeffs.data(), Slot->Gain, mGains[0].Target);
    ComputePanGains(target.Main, rcoeffs.data(), Slot->Gain, mGains[1].Target);

    float rate{props->Chorus.Rate};
    if(!(rate > 0.0f))
    {
        mLfoOffset = 0;
        mLfoRange = 1;
        mLfoScale = 0.0f;
        mLfoDisp = 0;
    }
    else
    {
        /* Calculate LFO coefficient (number of samples per cycle). Limit the
         * max range to avoid overflow when calculating the displacement.
         */
        uint lfo_range{float2uint(minf(frequency/rate + 0.5f, float{INT_MAX/360 - 180}))};

        mLfoOffset = mLfoOffset * lfo_range / mLfoRange;
        mLfoRange = lfo_range;
        switch(mWaveform)
        {
        case ChorusWaveform::Triangle:
            mLfoScale = 4.0f / static_cast<float>(mLfoRange);
            break;
        case ChorusWaveform::Sinusoid:
            mLfoScale = al::numbers::pi_v<float>*2.0f / static_cast<float>(mLfoRange);
            break;
        }

        /* Calculate lfo phase displacement */
        int phase{props->Chorus.Phase};
        if(phase < 0) phase = 360 + phase;
        mLfoDisp = (mLfoRange*static_cast<uint>(phase) + 180) / 360;
    }
}


void ChorusState::getTriangleDelays(uint (*delays)[MAX_UPDATE_SAMPLES], const size_t todo)
{
    const uint lfo_range{mLfoRange};
    const float lfo_scale{mLfoScale};
    const float depth{mDepth};
    const int delay{mDelay};

    ASSUME(lfo_range > 0);
    ASSUME(todo > 0);

    uint offset{mLfoOffset};
    auto gen_lfo = [&offset,lfo_range,lfo_scale,depth,delay]() -> uint
    {
        offset = (offset+1)%lfo_range;
        const float offset_norm{static_cast<float>(offset) * lfo_scale};
        return static_cast<uint>(fastf2i((1.0f-std::abs(2.0f-offset_norm)) * depth) + delay);
    };
    std::generate_n(delays[0], todo, gen_lfo);

    offset = (mLfoOffset+mLfoDisp) % lfo_range;
    std::generate_n(delays[1], todo, gen_lfo);

    mLfoOffset = static_cast<uint>(mLfoOffset+todo) % lfo_range;
}

void ChorusState::getSinusoidDelays(uint (*delays)[MAX_UPDATE_SAMPLES], const size_t todo)
{
    const uint lfo_range{mLfoRange};
    const float lfo_scale{mLfoScale};
    const float depth{mDepth};
    const int delay{mDelay};

    ASSUME(lfo_range > 0);
    ASSUME(todo > 0);

    uint offset{mLfoOffset};
    auto gen_lfo = [&offset,lfo_range,lfo_scale,depth,delay]() -> uint
    {
        offset = (offset+1)%lfo_range;
        const float offset_norm{static_cast<float>(offset) * lfo_scale};
        return static_cast<uint>(fastf2i(std::sin(offset_norm)*depth) + delay);
    };
    std::generate_n(delays[0], todo, gen_lfo);

    offset = (mLfoOffset+mLfoDisp) % lfo_range;
    std::generate_n(delays[1], todo, gen_lfo);

    mLfoOffset = static_cast<uint>(mLfoOffset+todo) % lfo_range;
}

void ChorusState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    const size_t bufmask{mSampleBuffer.size()-1};
    const float feedback{mFeedback};
    const uint avgdelay{(static_cast<uint>(mDelay) + (MixerFracOne>>1)) >> MixerFracBits};
    float *RESTRICT delaybuf{mSampleBuffer.data()};
    uint offset{mOffset};

    for(size_t base{0u};base < samplesToDo;)
    {
        const size_t todo{minz(MAX_UPDATE_SAMPLES, samplesToDo-base)};

        uint moddelays[2][MAX_UPDATE_SAMPLES];
        if(mWaveform == ChorusWaveform::Sinusoid)
            getSinusoidDelays(moddelays, todo);
        else /*if(mWaveform == ChorusWaveform::Triangle)*/
            getTriangleDelays(moddelays, todo);

        alignas(16) float temps[2][MAX_UPDATE_SAMPLES];
        for(size_t i{0u};i < todo;++i)
        {
            // Feed the buffer's input first (necessary for delays < 1).
            delaybuf[offset&bufmask] = samplesIn[0][base+i];

            // Tap for the left output.
            uint delay{offset - (moddelays[0][i]>>MixerFracBits)};
            float mu{static_cast<float>(moddelays[0][i]&MixerFracMask) * (1.0f/MixerFracOne)};
            temps[0][i] = cubic(delaybuf[(delay+1) & bufmask], delaybuf[(delay  ) & bufmask],
                delaybuf[(delay-1) & bufmask], delaybuf[(delay-2) & bufmask], mu);

            // Tap for the right output.
            delay = offset - (moddelays[1][i]>>MixerFracBits);
            mu = static_cast<float>(moddelays[1][i]&MixerFracMask) * (1.0f/MixerFracOne);
            temps[1][i] = cubic(delaybuf[(delay+1) & bufmask], delaybuf[(delay  ) & bufmask],
                delaybuf[(delay-1) & bufmask], delaybuf[(delay-2) & bufmask], mu);

            // Accumulate feedback from the average delay of the taps.
            delaybuf[offset&bufmask] += delaybuf[(offset-avgdelay) & bufmask] * feedback;
            ++offset;
        }

        for(size_t c{0};c < 2;++c)
            MixSamples({temps[c], todo}, samplesOut, mGains[c].Current, mGains[c].Target,
                samplesToDo-base, base);

        base += todo;
    }

    mOffset = offset;
}


struct ChorusStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new ChorusState{}}; }
};


/* Flanger is basically a chorus with a really short delay. They can both use
 * the same processing functions, so piggyback flanger on the chorus functions.
 */
struct FlangerStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new ChorusState{}}; }
};

} // namespace

EffectStateFactory *ChorusStateFactory_getFactory()
{
    static ChorusStateFactory ChorusFactory{};
    return &ChorusFactory;
}

EffectStateFactory *FlangerStateFactory_getFactory()
{
    static FlangerStateFactory FlangerFactory{};
    return &FlangerFactory;
}
