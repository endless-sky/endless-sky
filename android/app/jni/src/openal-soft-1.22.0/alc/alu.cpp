/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
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

#include "alu.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdint.h>
#include <utility>

#include "almalloc.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "alspan.h"
#include "alstring.h"
#include "atomic.h"
#include "core/ambidefs.h"
#include "core/async_event.h"
#include "core/bformatdec.h"
#include "core/bs2b.h"
#include "core/bsinc_defs.h"
#include "core/bsinc_tables.h"
#include "core/bufferline.h"
#include "core/buffer_storage.h"
#include "core/context.h"
#include "core/cpu_caps.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effects/base.h"
#include "core/effectslot.h"
#include "core/filters/biquad.h"
#include "core/filters/nfc.h"
#include "core/fpu_ctrl.h"
#include "core/hrtf.h"
#include "core/mastering.h"
#include "core/mixer.h"
#include "core/mixer/defs.h"
#include "core/mixer/hrtfdefs.h"
#include "core/resampler_limits.h"
#include "core/uhjfilter.h"
#include "core/voice.h"
#include "core/voice_change.h"
#include "intrusive_ptr.h"
#include "opthelpers.h"
#include "ringbuffer.h"
#include "strutils.h"
#include "threads.h"
#include "vecmat.h"
#include "vector.h"

struct CTag;
#ifdef HAVE_SSE
struct SSETag;
#endif
#ifdef HAVE_SSE2
struct SSE2Tag;
#endif
#ifdef HAVE_SSE4_1
struct SSE4Tag;
#endif
#ifdef HAVE_NEON
struct NEONTag;
#endif
struct PointTag;
struct LerpTag;
struct CubicTag;
struct BSincTag;
struct FastBSincTag;


static_assert(!(MaxResamplerPadding&1), "MaxResamplerPadding is not a multiple of two");


namespace {

using uint = unsigned int;

constexpr uint MaxPitch{10};

static_assert((BufferLineSize-1)/MaxPitch > 0, "MaxPitch is too large for BufferLineSize!");
static_assert((INT_MAX>>MixerFracBits)/MaxPitch > BufferLineSize,
    "MaxPitch and/or BufferLineSize are too large for MixerFracBits!");

using namespace std::placeholders;

float InitConeScale()
{
    float ret{1.0f};
    if(auto optval = al::getenv("__ALSOFT_HALF_ANGLE_CONES"))
    {
        if(al::strcasecmp(optval->c_str(), "true") == 0
            || strtol(optval->c_str(), nullptr, 0) == 1)
            ret *= 0.5f;
    }
    return ret;
}
/* Cone scalar */
const float ConeScale{InitConeScale()};

/* Localized scalars for mono sources (initialized in aluInit, after
 * configuration is loaded).
 */
float XScale{1.0f};
float YScale{1.0f};
float ZScale{1.0f};

} // namespace

namespace {

struct ChanMap {
    Channel channel;
    float angle;
    float elevation;
};

using HrtfDirectMixerFunc = void(*)(const FloatBufferSpan LeftOut, const FloatBufferSpan RightOut,
    const al::span<const FloatBufferLine> InSamples, float2 *AccumSamples, float *TempBuf,
    HrtfChannelState *ChanState, const size_t IrSize, const size_t BufferSize);

HrtfDirectMixerFunc MixDirectHrtf{MixDirectHrtf_<CTag>};

inline HrtfDirectMixerFunc SelectHrtfMixer(void)
{
#ifdef HAVE_NEON
    if((CPUCapFlags&CPU_CAP_NEON))
        return MixDirectHrtf_<NEONTag>;
#endif
#ifdef HAVE_SSE
    if((CPUCapFlags&CPU_CAP_SSE))
        return MixDirectHrtf_<SSETag>;
#endif

    return MixDirectHrtf_<CTag>;
}


inline void BsincPrepare(const uint increment, BsincState *state, const BSincTable *table)
{
    size_t si{BSincScaleCount - 1};
    float sf{0.0f};

    if(increment > MixerFracOne)
    {
        sf = MixerFracOne/static_cast<float>(increment) - table->scaleBase;
        sf = maxf(0.0f, BSincScaleCount*sf*table->scaleRange - 1.0f);
        si = float2uint(sf);
        /* The interpolation factor is fit to this diagonally-symmetric curve
         * to reduce the transition ripple caused by interpolating different
         * scales of the sinc function.
         */
        sf = 1.0f - std::cos(std::asin(sf - static_cast<float>(si)));
    }

    state->sf = sf;
    state->m = table->m[si];
    state->l = (state->m/2) - 1;
    state->filter = table->Tab + table->filterOffset[si];
}

inline ResamplerFunc SelectResampler(Resampler resampler, uint increment)
{
    switch(resampler)
    {
    case Resampler::Point:
        return Resample_<PointTag,CTag>;
    case Resampler::Linear:
#ifdef HAVE_NEON
        if((CPUCapFlags&CPU_CAP_NEON))
            return Resample_<LerpTag,NEONTag>;
#endif
#ifdef HAVE_SSE4_1
        if((CPUCapFlags&CPU_CAP_SSE4_1))
            return Resample_<LerpTag,SSE4Tag>;
#endif
#ifdef HAVE_SSE2
        if((CPUCapFlags&CPU_CAP_SSE2))
            return Resample_<LerpTag,SSE2Tag>;
#endif
        return Resample_<LerpTag,CTag>;
    case Resampler::Cubic:
        return Resample_<CubicTag,CTag>;
    case Resampler::BSinc12:
    case Resampler::BSinc24:
        if(increment <= MixerFracOne)
        {
            /* fall-through */
        case Resampler::FastBSinc12:
        case Resampler::FastBSinc24:
#ifdef HAVE_NEON
            if((CPUCapFlags&CPU_CAP_NEON))
                return Resample_<FastBSincTag,NEONTag>;
#endif
#ifdef HAVE_SSE
            if((CPUCapFlags&CPU_CAP_SSE))
                return Resample_<FastBSincTag,SSETag>;
#endif
            return Resample_<FastBSincTag,CTag>;
        }
#ifdef HAVE_NEON
        if((CPUCapFlags&CPU_CAP_NEON))
            return Resample_<BSincTag,NEONTag>;
#endif
#ifdef HAVE_SSE
        if((CPUCapFlags&CPU_CAP_SSE))
            return Resample_<BSincTag,SSETag>;
#endif
        return Resample_<BSincTag,CTag>;
    }

    return Resample_<PointTag,CTag>;
}

} // namespace

void aluInit(CompatFlagBitset flags)
{
    MixDirectHrtf = SelectHrtfMixer();
    XScale = flags.test(CompatFlags::ReverseX) ? -1.0f : 1.0f;
    YScale = flags.test(CompatFlags::ReverseY) ? -1.0f : 1.0f;
    ZScale = flags.test(CompatFlags::ReverseZ) ? -1.0f : 1.0f;
}


ResamplerFunc PrepareResampler(Resampler resampler, uint increment, InterpState *state)
{
    switch(resampler)
    {
    case Resampler::Point:
    case Resampler::Linear:
    case Resampler::Cubic:
        break;
    case Resampler::FastBSinc12:
    case Resampler::BSinc12:
        BsincPrepare(increment, &state->bsinc, &bsinc12);
        break;
    case Resampler::FastBSinc24:
    case Resampler::BSinc24:
        BsincPrepare(increment, &state->bsinc, &bsinc24);
        break;
    }
    return SelectResampler(resampler, increment);
}


void DeviceBase::ProcessHrtf(const size_t SamplesToDo)
{
    /* HRTF is stereo output only. */
    const uint lidx{RealOut.ChannelIndex[FrontLeft]};
    const uint ridx{RealOut.ChannelIndex[FrontRight]};

    MixDirectHrtf(RealOut.Buffer[lidx], RealOut.Buffer[ridx], Dry.Buffer, HrtfAccumData,
        mHrtfState->mTemp.data(), mHrtfState->mChannels.data(), mHrtfState->mIrSize, SamplesToDo);
}

void DeviceBase::ProcessAmbiDec(const size_t SamplesToDo)
{
    AmbiDecoder->process(RealOut.Buffer, Dry.Buffer.data(), SamplesToDo);
}

void DeviceBase::ProcessAmbiDecStablized(const size_t SamplesToDo)
{
    /* Decode with front image stablization. */
    const uint lidx{RealOut.ChannelIndex[FrontLeft]};
    const uint ridx{RealOut.ChannelIndex[FrontRight]};
    const uint cidx{RealOut.ChannelIndex[FrontCenter]};

    AmbiDecoder->processStablize(RealOut.Buffer, Dry.Buffer.data(), lidx, ridx, cidx,
        SamplesToDo);
}

void DeviceBase::ProcessUhj(const size_t SamplesToDo)
{
    /* UHJ is stereo output only. */
    const uint lidx{RealOut.ChannelIndex[FrontLeft]};
    const uint ridx{RealOut.ChannelIndex[FrontRight]};

    /* Encode to stereo-compatible 2-channel UHJ output. */
    mUhjEncoder->encode(RealOut.Buffer[lidx].data(), RealOut.Buffer[ridx].data(),
        Dry.Buffer.data(), SamplesToDo);
}

void DeviceBase::ProcessBs2b(const size_t SamplesToDo)
{
    /* First, decode the ambisonic mix to the "real" output. */
    AmbiDecoder->process(RealOut.Buffer, Dry.Buffer.data(), SamplesToDo);

    /* BS2B is stereo output only. */
    const uint lidx{RealOut.ChannelIndex[FrontLeft]};
    const uint ridx{RealOut.ChannelIndex[FrontRight]};

    /* Now apply the BS2B binaural/crossfeed filter. */
    bs2b_cross_feed(Bs2b.get(), RealOut.Buffer[lidx].data(), RealOut.Buffer[ridx].data(),
        SamplesToDo);
}


namespace {

/* This RNG method was created based on the math found in opusdec. It's quick,
 * and starting with a seed value of 22222, is suitable for generating
 * whitenoise.
 */
inline uint dither_rng(uint *seed) noexcept
{
    *seed = (*seed * 96314165) + 907633515;
    return *seed;
}


inline auto& GetAmbiScales(AmbiScaling scaletype) noexcept
{
    switch(scaletype)
    {
    case AmbiScaling::FuMa: return AmbiScale::FromFuMa();
    case AmbiScaling::SN3D: return AmbiScale::FromSN3D();
    case AmbiScaling::UHJ: return AmbiScale::FromUHJ();
    case AmbiScaling::N3D: break;
    }
    return AmbiScale::FromN3D();
}

inline auto& GetAmbiLayout(AmbiLayout layouttype) noexcept
{
    if(layouttype == AmbiLayout::FuMa) return AmbiIndex::FromFuMa();
    return AmbiIndex::FromACN();
}

inline auto& GetAmbi2DLayout(AmbiLayout layouttype) noexcept
{
    if(layouttype == AmbiLayout::FuMa) return AmbiIndex::FromFuMa2D();
    return AmbiIndex::FromACN2D();
}


bool CalcContextParams(ContextBase *ctx)
{
    ContextProps *props{ctx->mParams.ContextUpdate.exchange(nullptr, std::memory_order_acq_rel)};
    if(!props) return false;

    const alu::Vector pos{props->Position[0], props->Position[1], props->Position[2], 1.0f};
    ctx->mParams.Position = pos;

    /* AT then UP */
    alu::Vector N{props->OrientAt[0], props->OrientAt[1], props->OrientAt[2], 0.0f};
    N.normalize();
    alu::Vector V{props->OrientUp[0], props->OrientUp[1], props->OrientUp[2], 0.0f};
    V.normalize();
    /* Build and normalize right-vector */
    alu::Vector U{N.cross_product(V)};
    U.normalize();

    const alu::Matrix rot{
        U[0], V[0], -N[0], 0.0,
        U[1], V[1], -N[1], 0.0,
        U[2], V[2], -N[2], 0.0,
         0.0,  0.0,   0.0, 1.0};
    const alu::Vector vel{props->Velocity[0], props->Velocity[1], props->Velocity[2], 0.0};

    ctx->mParams.Matrix = rot;
    ctx->mParams.Velocity = rot * vel;

    ctx->mParams.Gain = props->Gain * ctx->mGainBoost;
    ctx->mParams.MetersPerUnit = props->MetersPerUnit;
    ctx->mParams.AirAbsorptionGainHF = props->AirAbsorptionGainHF;

    ctx->mParams.DopplerFactor = props->DopplerFactor;
    ctx->mParams.SpeedOfSound = props->SpeedOfSound * props->DopplerVelocity;

    ctx->mParams.SourceDistanceModel = props->SourceDistanceModel;
    ctx->mParams.mDistanceModel = props->mDistanceModel;

    AtomicReplaceHead(ctx->mFreeContextProps, props);
    return true;
}

bool CalcEffectSlotParams(EffectSlot *slot, EffectSlot **sorted_slots, ContextBase *context)
{
    EffectSlotProps *props{slot->Update.exchange(nullptr, std::memory_order_acq_rel)};
    if(!props) return false;

    /* If the effect slot target changed, clear the first sorted entry to force
     * a re-sort.
     */
    if(slot->Target != props->Target)
        *sorted_slots = nullptr;
    slot->Gain = props->Gain;
    slot->AuxSendAuto = props->AuxSendAuto;
    slot->Target = props->Target;
    slot->EffectType = props->Type;
    slot->mEffectProps = props->Props;
    if(props->Type == EffectSlotType::Reverb || props->Type == EffectSlotType::EAXReverb)
    {
        slot->RoomRolloff = props->Props.Reverb.RoomRolloffFactor;
        slot->DecayTime = props->Props.Reverb.DecayTime;
        slot->DecayLFRatio = props->Props.Reverb.DecayLFRatio;
        slot->DecayHFRatio = props->Props.Reverb.DecayHFRatio;
        slot->DecayHFLimit = props->Props.Reverb.DecayHFLimit;
        slot->AirAbsorptionGainHF = props->Props.Reverb.AirAbsorptionGainHF;
    }
    else
    {
        slot->RoomRolloff = 0.0f;
        slot->DecayTime = 0.0f;
        slot->DecayLFRatio = 0.0f;
        slot->DecayHFRatio = 0.0f;
        slot->DecayHFLimit = false;
        slot->AirAbsorptionGainHF = 1.0f;
    }

    EffectState *state{props->State.release()};
    EffectState *oldstate{slot->mEffectState};
    slot->mEffectState = state;

    /* Only release the old state if it won't get deleted, since we can't be
     * deleting/freeing anything in the mixer.
     */
    if(!oldstate->releaseIfNoDelete())
    {
        /* Otherwise, if it would be deleted send it off with a release event. */
        RingBuffer *ring{context->mAsyncEvents.get()};
        auto evt_vec = ring->getWriteVector();
        if LIKELY(evt_vec.first.len > 0)
        {
            AsyncEvent *evt{al::construct_at(reinterpret_cast<AsyncEvent*>(evt_vec.first.buf),
                AsyncEvent::ReleaseEffectState)};
            evt->u.mEffectState = oldstate;
            ring->writeAdvance(1);
        }
        else
        {
            /* If writing the event failed, the queue was probably full. Store
             * the old state in the property object where it can eventually be
             * cleaned up sometime later (not ideal, but better than blocking
             * or leaking).
             */
            props->State.reset(oldstate);
        }
    }

    AtomicReplaceHead(context->mFreeEffectslotProps, props);

    EffectTarget output;
    if(EffectSlot *target{slot->Target})
        output = EffectTarget{&target->Wet, nullptr};
    else
    {
        DeviceBase *device{context->mDevice};
        output = EffectTarget{&device->Dry, &device->RealOut};
    }
    state->update(context, slot, &slot->mEffectProps, output);
    return true;
}


/* Scales the given azimuth toward the side (+/- pi/2 radians) for positions in
 * front.
 */
inline float ScaleAzimuthFront(float azimuth, float scale)
{
    const float abs_azi{std::fabs(azimuth)};
    if(!(abs_azi >= al::numbers::pi_v<float>*0.5f))
        return std::copysign(minf(abs_azi*scale, al::numbers::pi_v<float>*0.5f), azimuth);
    return azimuth;
}

/* Wraps the given value in radians to stay between [-pi,+pi] */
inline float WrapRadians(float r)
{
    static constexpr float Pi{al::numbers::pi_v<float>};
    static constexpr float Pi2{Pi*2.0f};
    if(r >  Pi) return std::fmod(Pi+r, Pi2) - Pi;
    if(r < -Pi) return Pi - std::fmod(Pi-r, Pi2);
    return r;
}

/* Begin ambisonic rotation helpers.
 *
 * Rotating first-order B-Format just needs a straight-forward X/Y/Z rotation
 * matrix. Higher orders, however, are more complicated. The method implemented
 * here is a recursive algorithm (the rotation for first-order is used to help
 * generate the second-order rotation, which helps generate the third-order
 * rotation, etc).
 *
 * Adapted from
 * <https://github.com/polarch/Spherical-Harmonic-Transform/blob/master/getSHrotMtx.m>,
 * provided under the BSD 3-Clause license.
 *
 * Copyright (c) 2015, Archontis Politis
 * Copyright (c) 2019, Christopher Robinson
 *
 * The u, v, and w coefficients used for generating higher-order rotations are
 * precomputed since they're constant. The second-order coefficients are
 * followed by the third-order coefficients, etc.
 */
struct RotatorCoeffs {
    float u, v, w;

    template<size_t N0, size_t N1>
    static std::array<RotatorCoeffs,N0+N1> ConcatArrays(const std::array<RotatorCoeffs,N0> &lhs,
        const std::array<RotatorCoeffs,N1> &rhs)
    {
        std::array<RotatorCoeffs,N0+N1> ret;
        auto iter = std::copy(lhs.cbegin(), lhs.cend(), ret.begin());
        std::copy(rhs.cbegin(), rhs.cend(), iter);
        return ret;
    }

    template<int l, int num_elems=l*2+1>
    static std::array<RotatorCoeffs,num_elems*num_elems> GenCoeffs()
    {
        std::array<RotatorCoeffs,num_elems*num_elems> ret{};
        auto coeffs = ret.begin();

        for(int m{-l};m <= l;++m)
        {
            for(int n{-l};n <= l;++n)
            {
                // compute u,v,w terms of Eq.8.1 (Table I)
                const bool d{m == 0}; // the delta function d_m0
                const float denom{static_cast<float>((std::abs(n) == l) ?
                    (2*l) * (2*l - 1) : (l*l - n*n))};

                const int abs_m{std::abs(m)};
                coeffs->u = std::sqrt(static_cast<float>(l*l - m*m)/denom);
                coeffs->v = std::sqrt(static_cast<float>(l+abs_m-1) * static_cast<float>(l+abs_m) /
                    denom) * (1.0f+d) * (1.0f - 2.0f*d) * 0.5f;
                coeffs->w = std::sqrt(static_cast<float>(l-abs_m-1) * static_cast<float>(l-abs_m) /
                    denom) * (1.0f-d) * -0.5f;
                ++coeffs;
            }
        }

        return ret;
    }
};
const auto RotatorCoeffArray = RotatorCoeffs::ConcatArrays(RotatorCoeffs::GenCoeffs<2>(),
    RotatorCoeffs::GenCoeffs<3>());

/**
 * Given the matrix, pre-filled with the (zeroth- and) first-order rotation
 * coefficients, this fills in the coefficients for the higher orders up to and
 * including the given order. The matrix is in ACN layout.
 */
void AmbiRotator(std::array<std::array<float,MaxAmbiChannels>,MaxAmbiChannels> &matrix,
    const int order)
{
    /* Don't do anything for < 2nd order. */
    if(order < 2) return;

    auto P = [](const int i, const int l, const int a, const int n, const size_t last_band,
        const std::array<std::array<float,MaxAmbiChannels>,MaxAmbiChannels> &R)
    {
        const float ri1{ R[static_cast<uint>(i+2)][ 1+2]};
        const float rim1{R[static_cast<uint>(i+2)][-1+2]};
        const float ri0{ R[static_cast<uint>(i+2)][ 0+2]};

        auto vec = R[static_cast<uint>(a+l-1) + last_band].cbegin() + last_band;
        if(n == -l)
            return ri1*vec[0] + rim1*vec[static_cast<uint>(l-1)*size_t{2}];
        if(n == l)
            return ri1*vec[static_cast<uint>(l-1)*size_t{2}] - rim1*vec[0];
        return ri0*vec[static_cast<uint>(n+l-1)];
    };

    auto U = [P](const int l, const int m, const int n, const size_t last_band,
        const std::array<std::array<float,MaxAmbiChannels>,MaxAmbiChannels> &R)
    {
        return P(0, l, m, n, last_band, R);
    };
    auto V = [P](const int l, const int m, const int n, const size_t last_band,
        const std::array<std::array<float,MaxAmbiChannels>,MaxAmbiChannels> &R)
    {
        using namespace al::numbers;
        if(m > 0)
        {
            const bool d{m == 1};
            const float p0{P( 1, l,  m-1, n, last_band, R)};
            const float p1{P(-1, l, -m+1, n, last_band, R)};
            return d ? p0*sqrt2_v<float> : (p0 - p1);
        }
        const bool d{m == -1};
        const float p0{P( 1, l,  m+1, n, last_band, R)};
        const float p1{P(-1, l, -m-1, n, last_band, R)};
        return d ? p1*sqrt2_v<float> : (p0 + p1);
    };
    auto W = [P](const int l, const int m, const int n, const size_t last_band,
        const std::array<std::array<float,MaxAmbiChannels>,MaxAmbiChannels> &R)
    {
        assert(m != 0);
        if(m > 0)
        {
            const float p0{P( 1, l,  m+1, n, last_band, R)};
            const float p1{P(-1, l, -m-1, n, last_band, R)};
            return p0 + p1;
        }
        const float p0{P( 1, l,  m-1, n, last_band, R)};
        const float p1{P(-1, l, -m+1, n, last_band, R)};
        return p0 - p1;
    };

    // compute rotation matrix of each subsequent band recursively
    auto coeffs = RotatorCoeffArray.cbegin();
    size_t band_idx{4}, last_band{1};
    for(int l{2};l <= order;++l)
    {
        size_t y{band_idx};
        for(int m{-l};m <= l;++m,++y)
        {
            size_t x{band_idx};
            for(int n{-l};n <= l;++n,++x)
            {
                float r{0.0f};

                // computes Eq.8.1
                const float u{coeffs->u};
                if(u != 0.0f) r += u * U(l, m, n, last_band, matrix);
                const float v{coeffs->v};
                if(v != 0.0f) r += v * V(l, m, n, last_band, matrix);
                const float w{coeffs->w};
                if(w != 0.0f) r += w * W(l, m, n, last_band, matrix);

                matrix[y][x] = r;
                ++coeffs;
            }
        }
        last_band = band_idx;
        band_idx += static_cast<uint>(l)*size_t{2} + 1;
    }
}
/* End ambisonic rotation helpers. */


constexpr float Deg2Rad(float x) noexcept
{ return static_cast<float>(al::numbers::pi / 180.0 * x); }

struct GainTriplet { float Base, HF, LF; };

void CalcPanningAndFilters(Voice *voice, const float xpos, const float ypos, const float zpos,
    const float Distance, const float Spread, const GainTriplet &DryGain,
    const al::span<const GainTriplet,MAX_SENDS> WetGain, EffectSlot *(&SendSlots)[MAX_SENDS],
    const VoiceProps *props, const ContextParams &Context, const DeviceBase *Device)
{
    static constexpr ChanMap MonoMap[1]{
        { FrontCenter, 0.0f, 0.0f }
    }, RearMap[2]{
        { BackLeft,  Deg2Rad(-150.0f), Deg2Rad(0.0f) },
        { BackRight, Deg2Rad( 150.0f), Deg2Rad(0.0f) }
    }, QuadMap[4]{
        { FrontLeft,  Deg2Rad( -45.0f), Deg2Rad(0.0f) },
        { FrontRight, Deg2Rad(  45.0f), Deg2Rad(0.0f) },
        { BackLeft,   Deg2Rad(-135.0f), Deg2Rad(0.0f) },
        { BackRight,  Deg2Rad( 135.0f), Deg2Rad(0.0f) }
    }, X51Map[6]{
        { FrontLeft,   Deg2Rad( -30.0f), Deg2Rad(0.0f) },
        { FrontRight,  Deg2Rad(  30.0f), Deg2Rad(0.0f) },
        { FrontCenter, Deg2Rad(   0.0f), Deg2Rad(0.0f) },
        { LFE, 0.0f, 0.0f },
        { SideLeft,    Deg2Rad(-110.0f), Deg2Rad(0.0f) },
        { SideRight,   Deg2Rad( 110.0f), Deg2Rad(0.0f) }
    }, X61Map[7]{
        { FrontLeft,   Deg2Rad(-30.0f), Deg2Rad(0.0f) },
        { FrontRight,  Deg2Rad( 30.0f), Deg2Rad(0.0f) },
        { FrontCenter, Deg2Rad(  0.0f), Deg2Rad(0.0f) },
        { LFE, 0.0f, 0.0f },
        { BackCenter,  Deg2Rad(180.0f), Deg2Rad(0.0f) },
        { SideLeft,    Deg2Rad(-90.0f), Deg2Rad(0.0f) },
        { SideRight,   Deg2Rad( 90.0f), Deg2Rad(0.0f) }
    }, X71Map[8]{
        { FrontLeft,   Deg2Rad( -30.0f), Deg2Rad(0.0f) },
        { FrontRight,  Deg2Rad(  30.0f), Deg2Rad(0.0f) },
        { FrontCenter, Deg2Rad(   0.0f), Deg2Rad(0.0f) },
        { LFE, 0.0f, 0.0f },
        { BackLeft,    Deg2Rad(-150.0f), Deg2Rad(0.0f) },
        { BackRight,   Deg2Rad( 150.0f), Deg2Rad(0.0f) },
        { SideLeft,    Deg2Rad( -90.0f), Deg2Rad(0.0f) },
        { SideRight,   Deg2Rad(  90.0f), Deg2Rad(0.0f) }
    };

    ChanMap StereoMap[2]{
        { FrontLeft,  Deg2Rad(-30.0f), Deg2Rad(0.0f) },
        { FrontRight, Deg2Rad( 30.0f), Deg2Rad(0.0f) }
    };

    const auto Frequency = static_cast<float>(Device->Frequency);
    const uint NumSends{Device->NumAuxSends};

    const size_t num_channels{voice->mChans.size()};
    ASSUME(num_channels > 0);

    for(auto &chandata : voice->mChans)
    {
        chandata.mDryParams.Hrtf.Target = HrtfFilter{};
        chandata.mDryParams.Gains.Target.fill(0.0f);
        std::for_each(chandata.mWetParams.begin(), chandata.mWetParams.begin()+NumSends,
            [](SendParams &params) -> void { params.Gains.Target.fill(0.0f); });
    }

    DirectMode DirectChannels{props->DirectChannels};
    const ChanMap *chans{nullptr};
    switch(voice->mFmtChannels)
    {
    case FmtMono:
        chans = MonoMap;
        /* Mono buffers are never played direct. */
        DirectChannels = DirectMode::Off;
        break;

    case FmtStereo:
        if(DirectChannels == DirectMode::Off)
        {
            /* Convert counter-clockwise to clock-wise, and wrap between
             * [-pi,+pi].
             */
            StereoMap[0].angle = WrapRadians(-props->StereoPan[0]);
            StereoMap[1].angle = WrapRadians(-props->StereoPan[1]);
        }
        chans = StereoMap;
        break;

    case FmtRear: chans = RearMap; break;
    case FmtQuad: chans = QuadMap; break;
    case FmtX51: chans = X51Map; break;
    case FmtX61: chans = X61Map; break;
    case FmtX71: chans = X71Map; break;

    case FmtBFormat2D:
    case FmtBFormat3D:
    case FmtUHJ2:
    case FmtUHJ3:
    case FmtUHJ4:
    case FmtSuperStereo:
        DirectChannels = DirectMode::Off;
        break;
    }

    voice->mFlags.reset(VoiceHasHrtf).reset(VoiceHasNfc);
    if(auto *decoder{voice->mDecoder.get()})
        decoder->mWidthControl = minf(props->EnhWidth, 0.7f);

    if(IsAmbisonic(voice->mFmtChannels))
    {
        /* Special handling for B-Format and UHJ sources. */

        if(Device->AvgSpeakerDist > 0.0f && voice->mFmtChannels != FmtUHJ2
            && voice->mFmtChannels != FmtSuperStereo)
        {
            if(!(Distance > std::numeric_limits<float>::epsilon()))
            {
                /* NOTE: The NFCtrlFilters were created with a w0 of 0, which
                 * is what we want for FOA input. The first channel may have
                 * been previously re-adjusted if panned, so reset it.
                 */
                voice->mChans[0].mDryParams.NFCtrlFilter.adjust(0.0f);
            }
            else
            {
                /* Clamp the distance for really close sources, to prevent
                 * excessive bass.
                 */
                const float mdist{maxf(Distance, Device->AvgSpeakerDist/4.0f)};
                const float w0{SpeedOfSoundMetersPerSec / (mdist * Frequency)};

                /* Only need to adjust the first channel of a B-Format source. */
                voice->mChans[0].mDryParams.NFCtrlFilter.adjust(w0);
            }

            voice->mFlags.set(VoiceHasNfc);
        }

        /* Panning a B-Format sound toward some direction is easy. Just pan the
         * first (W) channel as a normal mono sound. The angular spread is used
         * as a directional scalar to blend between full coverage and full
         * panning.
         */
        const float coverage{!(Distance > std::numeric_limits<float>::epsilon()) ? 1.0f :
            (al::numbers::inv_pi_v<float>/2.0f * Spread)};

        auto calc_coeffs = [xpos,ypos,zpos](RenderMode mode)
        {
            if(mode != RenderMode::Pairwise)
                return CalcDirectionCoeffs({xpos, ypos, zpos}, 0.0f);

            /* Clamp Y, in case rounding errors caused it to end up outside
             * of -1...+1.
             */
            const float ev{std::asin(clampf(ypos, -1.0f, 1.0f))};
            /* Negate Z for right-handed coords with -Z in front. */
            const float az{std::atan2(xpos, -zpos)};

            /* A scalar of 1.5 for plain stereo results in +/-60 degrees
             * being moved to +/-90 degrees for direct right and left
             * speaker responses.
             */
            return CalcAngleCoeffs(ScaleAzimuthFront(az, 1.5f), ev, 0.0f);
        };
        auto coeffs = calc_coeffs(Device->mRenderMode);
        std::transform(coeffs.begin()+1, coeffs.end(), coeffs.begin()+1,
            std::bind(std::multiplies<float>{}, _1, 1.0f-coverage));

        /* NOTE: W needs to be scaled according to channel scaling. */
        auto&& scales = GetAmbiScales(voice->mAmbiScaling);
        ComputePanGains(&Device->Dry, coeffs.data(), DryGain.Base*scales[0],
            voice->mChans[0].mDryParams.Gains.Target);
        for(uint i{0};i < NumSends;i++)
        {
            if(const EffectSlot *Slot{SendSlots[i]})
                ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base*scales[0],
                    voice->mChans[0].mWetParams[i].Gains.Target);
        }

        if(coverage > 0.0f)
        {
            /* Local B-Format sources have their XYZ channels rotated according
             * to the orientation.
             */
            /* AT then UP */
            alu::Vector N{props->OrientAt[0], props->OrientAt[1], props->OrientAt[2], 0.0f};
            N.normalize();
            alu::Vector V{props->OrientUp[0], props->OrientUp[1], props->OrientUp[2], 0.0f};
            V.normalize();
            if(!props->HeadRelative)
            {
                N = Context.Matrix * N;
                V = Context.Matrix * V;
            }
            /* Build and normalize right-vector */
            alu::Vector U{N.cross_product(V)};
            U.normalize();

            /* Build a rotation matrix. Manually fill the zeroth- and first-
             * order elements, then construct the rotation for the higher
             * orders.
             */
            std::array<std::array<float,MaxAmbiChannels>,MaxAmbiChannels> shrot{};
            shrot[0][0] = 1.0f;
            shrot[1][1] =  U[0]; shrot[1][2] = -V[0]; shrot[1][3] = -N[0];
            shrot[2][1] = -U[1]; shrot[2][2] =  V[1]; shrot[2][3] =  N[1];
            shrot[3][1] =  U[2]; shrot[3][2] = -V[2]; shrot[3][3] = -N[2];
            AmbiRotator(shrot, static_cast<int>(minu(voice->mAmbiOrder, Device->mAmbiOrder)));

            /* Convert the rotation matrix for input ordering and scaling, and
             * whether input is 2D or 3D.
             */
            const uint8_t *index_map{Is2DAmbisonic(voice->mFmtChannels) ?
                GetAmbi2DLayout(voice->mAmbiLayout).data() :
                GetAmbiLayout(voice->mAmbiLayout).data()};

            static const uint8_t ChansPerOrder[MaxAmbiOrder+1]{1, 3, 5, 7,};
            static const uint8_t OrderOffset[MaxAmbiOrder+1]{0, 1, 4, 9,};
            for(size_t c{1};c < num_channels;c++)
            {
                const size_t acn{index_map[c]};
                const size_t order{AmbiIndex::OrderFromChannel()[acn]};
                const size_t tocopy{ChansPerOrder[order]};
                const size_t offset{OrderOffset[order]};
                const float scale{scales[acn] * coverage};
                auto in = shrot.cbegin() + offset;

                coeffs = std::array<float,MaxAmbiChannels>{};
                for(size_t x{0};x < tocopy;++x)
                    coeffs[offset+x] = in[x][acn] * scale;

                ComputePanGains(&Device->Dry, coeffs.data(), DryGain.Base,
                    voice->mChans[c].mDryParams.Gains.Target);

                for(uint i{0};i < NumSends;i++)
                {
                    if(const EffectSlot *Slot{SendSlots[i]})
                        ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base,
                            voice->mChans[c].mWetParams[i].Gains.Target);
                }
            }
        }
    }
    else if(DirectChannels != DirectMode::Off && !Device->RealOut.RemixMap.empty())
    {
        /* Direct source channels always play local. Skip the virtual channels
         * and write inputs to the matching real outputs.
         */
        voice->mDirect.Buffer = Device->RealOut.Buffer;

        for(size_t c{0};c < num_channels;c++)
        {
            uint idx{GetChannelIdxByName(Device->RealOut, chans[c].channel)};
            if(idx != INVALID_CHANNEL_INDEX)
                voice->mChans[c].mDryParams.Gains.Target[idx] = DryGain.Base;
            else if(DirectChannels == DirectMode::RemixMismatch)
            {
                auto match_channel = [chans,c](const InputRemixMap &map) noexcept -> bool
                { return chans[c].channel == map.channel; };
                auto remap = std::find_if(Device->RealOut.RemixMap.cbegin(),
                    Device->RealOut.RemixMap.cend(), match_channel);
                if(remap != Device->RealOut.RemixMap.cend())
                {
                    for(const auto &target : remap->targets)
                    {
                        idx = GetChannelIdxByName(Device->RealOut, target.channel);
                        if(idx != INVALID_CHANNEL_INDEX)
                            voice->mChans[c].mDryParams.Gains.Target[idx] = DryGain.Base *
                                target.mix;
                    }
                }
            }
        }

        /* Auxiliary sends still use normal channel panning since they mix to
         * B-Format, which can't channel-match.
         */
        for(size_t c{0};c < num_channels;c++)
        {
            const auto coeffs = CalcAngleCoeffs(chans[c].angle, chans[c].elevation, 0.0f);

            for(uint i{0};i < NumSends;i++)
            {
                if(const EffectSlot *Slot{SendSlots[i]})
                    ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base,
                        voice->mChans[c].mWetParams[i].Gains.Target);
            }
        }
    }
    else if(Device->mRenderMode == RenderMode::Hrtf)
    {
        /* Full HRTF rendering. Skip the virtual channels and render to the
         * real outputs.
         */
        voice->mDirect.Buffer = Device->RealOut.Buffer;

        if(Distance > std::numeric_limits<float>::epsilon())
        {
            const float ev{std::asin(clampf(ypos, -1.0f, 1.0f))};
            const float az{std::atan2(xpos, -zpos)};

            /* Get the HRIR coefficients and delays just once, for the given
             * source direction.
             */
            GetHrtfCoeffs(Device->mHrtf.get(), ev, az, Distance, Spread,
                voice->mChans[0].mDryParams.Hrtf.Target.Coeffs,
                voice->mChans[0].mDryParams.Hrtf.Target.Delay);
            voice->mChans[0].mDryParams.Hrtf.Target.Gain = DryGain.Base;

            /* Remaining channels use the same results as the first. */
            for(size_t c{1};c < num_channels;c++)
            {
                /* Skip LFE */
                if(chans[c].channel == LFE) continue;
                voice->mChans[c].mDryParams.Hrtf.Target = voice->mChans[0].mDryParams.Hrtf.Target;
            }

            /* Calculate the directional coefficients once, which apply to all
             * input channels of the source sends.
             */
            const auto coeffs = CalcDirectionCoeffs({xpos, ypos, zpos}, Spread);

            for(size_t c{0};c < num_channels;c++)
            {
                /* Skip LFE */
                if(chans[c].channel == LFE)
                    continue;
                for(uint i{0};i < NumSends;i++)
                {
                    if(const EffectSlot *Slot{SendSlots[i]})
                        ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base,
                            voice->mChans[c].mWetParams[i].Gains.Target);
                }
            }
        }
        else
        {
            /* Local sources on HRTF play with each channel panned to its
             * relative location around the listener, providing "virtual
             * speaker" responses.
             */
            for(size_t c{0};c < num_channels;c++)
            {
                /* Skip LFE */
                if(chans[c].channel == LFE)
                    continue;

                /* Get the HRIR coefficients and delays for this channel
                 * position.
                 */
                GetHrtfCoeffs(Device->mHrtf.get(), chans[c].elevation, chans[c].angle,
                    std::numeric_limits<float>::infinity(), Spread,
                    voice->mChans[c].mDryParams.Hrtf.Target.Coeffs,
                    voice->mChans[c].mDryParams.Hrtf.Target.Delay);
                voice->mChans[c].mDryParams.Hrtf.Target.Gain = DryGain.Base;

                /* Normal panning for auxiliary sends. */
                const auto coeffs = CalcAngleCoeffs(chans[c].angle, chans[c].elevation, Spread);

                for(uint i{0};i < NumSends;i++)
                {
                    if(const EffectSlot *Slot{SendSlots[i]})
                        ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base,
                            voice->mChans[c].mWetParams[i].Gains.Target);
                }
            }
        }

        voice->mFlags.set(VoiceHasHrtf);
    }
    else
    {
        /* Non-HRTF rendering. Use normal panning to the output. */

        if(Distance > std::numeric_limits<float>::epsilon())
        {
            /* Calculate NFC filter coefficient if needed. */
            if(Device->AvgSpeakerDist > 0.0f)
            {
                /* Clamp the distance for really close sources, to prevent
                 * excessive bass.
                 */
                const float mdist{maxf(Distance, Device->AvgSpeakerDist/4.0f)};
                const float w0{SpeedOfSoundMetersPerSec / (mdist * Frequency)};

                /* Adjust NFC filters. */
                for(size_t c{0};c < num_channels;c++)
                    voice->mChans[c].mDryParams.NFCtrlFilter.adjust(w0);

                voice->mFlags.set(VoiceHasNfc);
            }

            /* Calculate the directional coefficients once, which apply to all
             * input channels.
             */
            auto calc_coeffs = [xpos,ypos,zpos,Spread](RenderMode mode)
            {
                if(mode != RenderMode::Pairwise)
                    return CalcDirectionCoeffs({xpos, ypos, zpos}, Spread);
                const float ev{std::asin(clampf(ypos, -1.0f, 1.0f))};
                const float az{std::atan2(xpos, -zpos)};
                return CalcAngleCoeffs(ScaleAzimuthFront(az, 1.5f), ev, Spread);
            };
            const auto coeffs = calc_coeffs(Device->mRenderMode);

            for(size_t c{0};c < num_channels;c++)
            {
                /* Special-case LFE */
                if(chans[c].channel == LFE)
                {
                    if(Device->Dry.Buffer.data() == Device->RealOut.Buffer.data())
                    {
                        const uint idx{GetChannelIdxByName(Device->RealOut, chans[c].channel)};
                        if(idx != INVALID_CHANNEL_INDEX)
                            voice->mChans[c].mDryParams.Gains.Target[idx] = DryGain.Base;
                    }
                    continue;
                }

                ComputePanGains(&Device->Dry, coeffs.data(), DryGain.Base,
                    voice->mChans[c].mDryParams.Gains.Target);
                for(uint i{0};i < NumSends;i++)
                {
                    if(const EffectSlot *Slot{SendSlots[i]})
                        ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base,
                            voice->mChans[c].mWetParams[i].Gains.Target);
                }
            }
        }
        else
        {
            if(Device->AvgSpeakerDist > 0.0f)
            {
                /* If the source distance is 0, simulate a plane-wave by using
                 * infinite distance, which results in a w0 of 0.
                 */
                static constexpr float w0{0.0f};
                for(size_t c{0};c < num_channels;c++)
                    voice->mChans[c].mDryParams.NFCtrlFilter.adjust(w0);

                voice->mFlags.set(VoiceHasNfc);
            }

            for(size_t c{0};c < num_channels;c++)
            {
                /* Special-case LFE */
                if(chans[c].channel == LFE)
                {
                    if(Device->Dry.Buffer.data() == Device->RealOut.Buffer.data())
                    {
                        const uint idx{GetChannelIdxByName(Device->RealOut, chans[c].channel)};
                        if(idx != INVALID_CHANNEL_INDEX)
                            voice->mChans[c].mDryParams.Gains.Target[idx] = DryGain.Base;
                    }
                    continue;
                }

                const auto coeffs = CalcAngleCoeffs((Device->mRenderMode == RenderMode::Pairwise)
                    ? ScaleAzimuthFront(chans[c].angle, 3.0f) : chans[c].angle,
                    chans[c].elevation, Spread);

                ComputePanGains(&Device->Dry, coeffs.data(), DryGain.Base,
                    voice->mChans[c].mDryParams.Gains.Target);
                for(uint i{0};i < NumSends;i++)
                {
                    if(const EffectSlot *Slot{SendSlots[i]})
                        ComputePanGains(&Slot->Wet, coeffs.data(), WetGain[i].Base,
                            voice->mChans[c].mWetParams[i].Gains.Target);
                }
            }
        }
    }

    {
        const float hfNorm{props->Direct.HFReference / Frequency};
        const float lfNorm{props->Direct.LFReference / Frequency};

        voice->mDirect.FilterType = AF_None;
        if(DryGain.HF != 1.0f) voice->mDirect.FilterType |= AF_LowPass;
        if(DryGain.LF != 1.0f) voice->mDirect.FilterType |= AF_HighPass;

        auto &lowpass = voice->mChans[0].mDryParams.LowPass;
        auto &highpass = voice->mChans[0].mDryParams.HighPass;
        lowpass.setParamsFromSlope(BiquadType::HighShelf, hfNorm, DryGain.HF, 1.0f);
        highpass.setParamsFromSlope(BiquadType::LowShelf, lfNorm, DryGain.LF, 1.0f);
        for(size_t c{1};c < num_channels;c++)
        {
            voice->mChans[c].mDryParams.LowPass.copyParamsFrom(lowpass);
            voice->mChans[c].mDryParams.HighPass.copyParamsFrom(highpass);
        }
    }
    for(uint i{0};i < NumSends;i++)
    {
        const float hfNorm{props->Send[i].HFReference / Frequency};
        const float lfNorm{props->Send[i].LFReference / Frequency};

        voice->mSend[i].FilterType = AF_None;
        if(WetGain[i].HF != 1.0f) voice->mSend[i].FilterType |= AF_LowPass;
        if(WetGain[i].LF != 1.0f) voice->mSend[i].FilterType |= AF_HighPass;

        auto &lowpass = voice->mChans[0].mWetParams[i].LowPass;
        auto &highpass = voice->mChans[0].mWetParams[i].HighPass;
        lowpass.setParamsFromSlope(BiquadType::HighShelf, hfNorm, WetGain[i].HF, 1.0f);
        highpass.setParamsFromSlope(BiquadType::LowShelf, lfNorm, WetGain[i].LF, 1.0f);
        for(size_t c{1};c < num_channels;c++)
        {
            voice->mChans[c].mWetParams[i].LowPass.copyParamsFrom(lowpass);
            voice->mChans[c].mWetParams[i].HighPass.copyParamsFrom(highpass);
        }
    }
}

void CalcNonAttnSourceParams(Voice *voice, const VoiceProps *props, const ContextBase *context)
{
    const DeviceBase *Device{context->mDevice};
    EffectSlot *SendSlots[MAX_SENDS];

    voice->mDirect.Buffer = Device->Dry.Buffer;
    for(uint i{0};i < Device->NumAuxSends;i++)
    {
        SendSlots[i] = props->Send[i].Slot;
        if(!SendSlots[i] || SendSlots[i]->EffectType == EffectSlotType::None)
        {
            SendSlots[i] = nullptr;
            voice->mSend[i].Buffer = {};
        }
        else
            voice->mSend[i].Buffer = SendSlots[i]->Wet.Buffer;
    }

    /* Calculate the stepping value */
    const auto Pitch = static_cast<float>(voice->mFrequency) /
        static_cast<float>(Device->Frequency) * props->Pitch;
    if(Pitch > float{MaxPitch})
        voice->mStep = MaxPitch<<MixerFracBits;
    else
        voice->mStep = maxu(fastf2u(Pitch * MixerFracOne), 1);
    voice->mResampler = PrepareResampler(props->mResampler, voice->mStep, &voice->mResampleState);

    /* Calculate gains */
    GainTriplet DryGain;
    DryGain.Base  = minf(clampf(props->Gain, props->MinGain, props->MaxGain) * props->Direct.Gain *
        context->mParams.Gain, GainMixMax);
    DryGain.HF = props->Direct.GainHF;
    DryGain.LF = props->Direct.GainLF;
    GainTriplet WetGain[MAX_SENDS];
    for(uint i{0};i < Device->NumAuxSends;i++)
    {
        WetGain[i].Base = minf(clampf(props->Gain, props->MinGain, props->MaxGain) *
            props->Send[i].Gain * context->mParams.Gain, GainMixMax);
        WetGain[i].HF = props->Send[i].GainHF;
        WetGain[i].LF = props->Send[i].GainLF;
    }

    CalcPanningAndFilters(voice, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, DryGain, WetGain, SendSlots, props,
        context->mParams, Device);
}

void CalcAttnSourceParams(Voice *voice, const VoiceProps *props, const ContextBase *context)
{
    const DeviceBase *Device{context->mDevice};
    const uint NumSends{Device->NumAuxSends};

    /* Set mixing buffers and get send parameters. */
    voice->mDirect.Buffer = Device->Dry.Buffer;
    EffectSlot *SendSlots[MAX_SENDS];
    uint UseDryAttnForRoom{0};
    for(uint i{0};i < NumSends;i++)
    {
        SendSlots[i] = props->Send[i].Slot;
        if(!SendSlots[i] || SendSlots[i]->EffectType == EffectSlotType::None)
            SendSlots[i] = nullptr;
        else if(!SendSlots[i]->AuxSendAuto)
        {
            /* If the slot's auxiliary send auto is off, the data sent to the
             * effect slot is the same as the dry path, sans filter effects.
             */
            UseDryAttnForRoom |= 1u<<i;
        }

        if(!SendSlots[i])
            voice->mSend[i].Buffer = {};
        else
            voice->mSend[i].Buffer = SendSlots[i]->Wet.Buffer;
    }

    /* Transform source to listener space (convert to head relative) */
    alu::Vector Position{props->Position[0], props->Position[1], props->Position[2], 1.0f};
    alu::Vector Velocity{props->Velocity[0], props->Velocity[1], props->Velocity[2], 0.0f};
    alu::Vector Direction{props->Direction[0], props->Direction[1], props->Direction[2], 0.0f};
    if(!props->HeadRelative)
    {
        /* Transform source vectors */
        Position = context->mParams.Matrix * (Position - context->mParams.Position);
        Velocity = context->mParams.Matrix * Velocity;
        Direction = context->mParams.Matrix * Direction;
    }
    else
    {
        /* Offset the source velocity to be relative of the listener velocity */
        Velocity += context->mParams.Velocity;
    }

    const bool directional{Direction.normalize() > 0.0f};
    alu::Vector ToSource{Position[0], Position[1], Position[2], 0.0f};
    const float Distance{ToSource.normalize()};

    /* Calculate distance attenuation */
    float ClampedDist{Distance};
    float DryGainBase{props->Gain};
    float WetGainBase{props->Gain};

    switch(context->mParams.SourceDistanceModel ? props->mDistanceModel
        : context->mParams.mDistanceModel)
    {
        case DistanceModel::InverseClamped:
            if(props->MaxDistance < props->RefDistance) break;
            ClampedDist = clampf(ClampedDist, props->RefDistance, props->MaxDistance);
            /*fall-through*/
        case DistanceModel::Inverse:
            if(props->RefDistance > 0.0f)
            {
                float dist{lerpf(props->RefDistance, ClampedDist, props->RolloffFactor)};
                if(dist > 0.0f) DryGainBase *= props->RefDistance / dist;

                dist = lerpf(props->RefDistance, ClampedDist, props->RoomRolloffFactor);
                if(dist > 0.0f) WetGainBase *= props->RefDistance / dist;
            }
            break;

        case DistanceModel::LinearClamped:
            if(props->MaxDistance < props->RefDistance) break;
            ClampedDist = clampf(ClampedDist, props->RefDistance, props->MaxDistance);
            /*fall-through*/
        case DistanceModel::Linear:
            if(props->MaxDistance != props->RefDistance)
            {
                float attn{(ClampedDist-props->RefDistance) /
                    (props->MaxDistance-props->RefDistance) * props->RolloffFactor};
                DryGainBase *= maxf(1.0f - attn, 0.0f);

                attn = (ClampedDist-props->RefDistance) /
                    (props->MaxDistance-props->RefDistance) * props->RoomRolloffFactor;
                WetGainBase *= maxf(1.0f - attn, 0.0f);
            }
            break;

        case DistanceModel::ExponentClamped:
            if(props->MaxDistance < props->RefDistance) break;
            ClampedDist = clampf(ClampedDist, props->RefDistance, props->MaxDistance);
            /*fall-through*/
        case DistanceModel::Exponent:
            if(ClampedDist > 0.0f && props->RefDistance > 0.0f)
            {
                const float dist_ratio{ClampedDist/props->RefDistance};
                DryGainBase *= std::pow(dist_ratio, -props->RolloffFactor);
                WetGainBase *= std::pow(dist_ratio, -props->RoomRolloffFactor);
            }
            break;

        case DistanceModel::Disable:
            break;
    }

    /* Calculate directional soundcones */
    float ConeHF{1.0f}, WetConeHF{1.0f};
    if(directional && props->InnerAngle < 360.0f)
    {
        static constexpr float Rad2Deg{static_cast<float>(180.0 / al::numbers::pi)};
        const float Angle{Rad2Deg*2.0f * std::acos(-Direction.dot_product(ToSource)) * ConeScale};

        float ConeGain{1.0f};
        if(Angle >= props->OuterAngle)
        {
            ConeGain = props->OuterGain;
            ConeHF = lerpf(1.0f, props->OuterGainHF, props->DryGainHFAuto);
        }
        else if(Angle >= props->InnerAngle)
        {
            const float scale{(Angle-props->InnerAngle) / (props->OuterAngle-props->InnerAngle)};
            ConeGain = lerpf(1.0f, props->OuterGain, scale);
            ConeHF = lerpf(1.0f, props->OuterGainHF, scale * props->DryGainHFAuto);
        }

        DryGainBase *= ConeGain;
        WetGainBase *= lerpf(1.0f, ConeGain, props->WetGainAuto);

        WetConeHF = lerpf(1.0f, ConeHF, props->WetGainHFAuto);
    }

    /* Apply gain and frequency filters */
    DryGainBase = clampf(DryGainBase, props->MinGain, props->MaxGain) * context->mParams.Gain;
    WetGainBase = clampf(WetGainBase, props->MinGain, props->MaxGain) * context->mParams.Gain;

    GainTriplet DryGain{};
    DryGain.Base = minf(DryGainBase * props->Direct.Gain, GainMixMax);
    DryGain.HF = ConeHF * props->Direct.GainHF;
    DryGain.LF = props->Direct.GainLF;
    GainTriplet WetGain[MAX_SENDS]{};
    for(uint i{0};i < NumSends;i++)
    {
        /* If this effect slot's Auxiliary Send Auto is off, then use the dry
         * path distance and cone attenuation, otherwise use the wet (room)
         * path distance and cone attenuation. The send filter is used instead
         * of the direct filter, regardless.
         */
        const bool use_room{!(UseDryAttnForRoom&(1u<<i))};
        const float gain{use_room ? WetGainBase : DryGainBase};
        WetGain[i].Base = minf(gain * props->Send[i].Gain, GainMixMax);
        WetGain[i].HF = (use_room ? WetConeHF : ConeHF) * props->Send[i].GainHF;
        WetGain[i].LF = props->Send[i].GainLF;
    }

    /* Distance-based air absorption and initial send decay. */
    if(likely(Distance > props->RefDistance))
    {
        const float distance_base{(Distance-props->RefDistance) * props->RolloffFactor};
        const float absorption{distance_base * context->mParams.MetersPerUnit *
            props->AirAbsorptionFactor};
        if(absorption > std::numeric_limits<float>::epsilon())
        {
            const float hfattn{std::pow(context->mParams.AirAbsorptionGainHF, absorption)};
            DryGain.HF *= hfattn;
            for(uint i{0u};i < NumSends;++i)
                WetGain[i].HF *= hfattn;
        }

        /* If the source's Auxiliary Send Filter Gain Auto is off, no extra
         * adjustment is applied to the send gains.
         */
        for(uint i{props->WetGainAuto ? 0u : NumSends};i < NumSends;++i)
        {
            if(!SendSlots[i])
                continue;

            auto calc_attenuation = [](float distance, float refdist, float rolloff) noexcept
            {
                const float dist{lerpf(refdist, distance, rolloff)};
                if(dist > refdist) return refdist / dist;
                return 1.0f;
            };

            /* The reverb effect's room rolloff factor always applies to an
             * inverse distance rolloff model.
             */
            WetGain[i].Base *= calc_attenuation(Distance, props->RefDistance,
                SendSlots[i]->RoomRolloff);

            /* If this effect slot's Auxiliary Send Auto is off, don't apply
             * the automatic initial reverb decay (should the reverb's room
             * rolloff still apply?).
             */
            if(!SendSlots[i]->AuxSendAuto)
                continue;

            GainTriplet DecayDistance;
            /* Calculate the distances to where this effect's decay reaches
             * -60dB.
             */
            DecayDistance.Base = SendSlots[i]->DecayTime * SpeedOfSoundMetersPerSec;
            DecayDistance.LF = DecayDistance.Base * SendSlots[i]->DecayLFRatio;
            DecayDistance.HF = DecayDistance.Base * SendSlots[i]->DecayHFRatio;
            if(SendSlots[i]->DecayHFLimit)
            {
                const float airAbsorption{SendSlots[i]->AirAbsorptionGainHF};
                if(airAbsorption < 1.0f)
                {
                    /* Calculate the distance to where this effect's air
                     * absorption reaches -60dB, and limit the effect's HF
                     * decay distance (so it doesn't take any longer to decay
                     * than the air would allow).
                     */
                    static constexpr float log10_decaygain{-3.0f/*std::log10(ReverbDecayGain)*/};
                    const float absorb_dist{log10_decaygain / std::log10(airAbsorption)};
                    DecayDistance.HF = minf(absorb_dist, DecayDistance.HF);
                }
            }

            const float baseAttn = calc_attenuation(Distance, props->RefDistance,
                props->RolloffFactor);

            /* Apply a decay-time transformation to the wet path, based on the
             * source distance. The initial decay of the reverb effect is
             * calculated and applied to the wet path.
             */
            const float fact{distance_base / DecayDistance.Base};
            const float gain{std::pow(ReverbDecayGain, fact)*(1.0f-baseAttn) + baseAttn};
            WetGain[i].Base *= gain;

            if(gain > 0.0f)
            {
                const float hffact{distance_base / DecayDistance.HF};
                const float gainhf{std::pow(ReverbDecayGain, hffact)*(1.0f-baseAttn) + baseAttn};
                WetGain[i].HF *= minf(gainhf/gain, 1.0f);
                const float lffact{distance_base / DecayDistance.LF};
                const float gainlf{std::pow(ReverbDecayGain, lffact)*(1.0f-baseAttn) + baseAttn};
                WetGain[i].LF *= minf(gainlf/gain, 1.0f);
            }
        }
    }


    /* Initial source pitch */
    float Pitch{props->Pitch};

    /* Calculate velocity-based doppler effect */
    float DopplerFactor{props->DopplerFactor * context->mParams.DopplerFactor};
    if(DopplerFactor > 0.0f)
    {
        const alu::Vector &lvelocity = context->mParams.Velocity;
        float vss{Velocity.dot_product(ToSource) * -DopplerFactor};
        float vls{lvelocity.dot_product(ToSource) * -DopplerFactor};

        const float SpeedOfSound{context->mParams.SpeedOfSound};
        if(!(vls < SpeedOfSound))
        {
            /* Listener moving away from the source at the speed of sound.
             * Sound waves can't catch it.
             */
            Pitch = 0.0f;
        }
        else if(!(vss < SpeedOfSound))
        {
            /* Source moving toward the listener at the speed of sound. Sound
             * waves bunch up to extreme frequencies.
             */
            Pitch = std::numeric_limits<float>::infinity();
        }
        else
        {
            /* Source and listener movement is nominal. Calculate the proper
             * doppler shift.
             */
            Pitch *= (SpeedOfSound-vls) / (SpeedOfSound-vss);
        }
    }

    /* Adjust pitch based on the buffer and output frequencies, and calculate
     * fixed-point stepping value.
     */
    Pitch *= static_cast<float>(voice->mFrequency) / static_cast<float>(Device->Frequency);
    if(Pitch > float{MaxPitch})
        voice->mStep = MaxPitch<<MixerFracBits;
    else
        voice->mStep = maxu(fastf2u(Pitch * MixerFracOne), 1);
    voice->mResampler = PrepareResampler(props->mResampler, voice->mStep, &voice->mResampleState);

    float spread{0.0f};
    if(props->Radius > Distance)
        spread = al::numbers::pi_v<float>*2.0f - Distance/props->Radius*al::numbers::pi_v<float>;
    else if(Distance > 0.0f)
        spread = std::asin(props->Radius/Distance) * 2.0f;

    CalcPanningAndFilters(voice, ToSource[0]*XScale, ToSource[1]*YScale, ToSource[2]*ZScale,
        Distance*context->mParams.MetersPerUnit, spread, DryGain, WetGain, SendSlots, props,
        context->mParams, Device);
}

void CalcSourceParams(Voice *voice, ContextBase *context, bool force)
{
    VoicePropsItem *props{voice->mUpdate.exchange(nullptr, std::memory_order_acq_rel)};
    if(!props && !force) return;

    if(props)
    {
        voice->mProps = *props;

        AtomicReplaceHead(context->mFreeVoiceProps, props);
    }

    if((voice->mProps.DirectChannels != DirectMode::Off && voice->mFmtChannels != FmtMono
            && !IsAmbisonic(voice->mFmtChannels))
        || voice->mProps.mSpatializeMode == SpatializeMode::Off
        || (voice->mProps.mSpatializeMode==SpatializeMode::Auto && voice->mFmtChannels != FmtMono))
        CalcNonAttnSourceParams(voice, &voice->mProps, context);
    else
        CalcAttnSourceParams(voice, &voice->mProps, context);
}


void SendSourceStateEvent(ContextBase *context, uint id, VChangeState state)
{
    RingBuffer *ring{context->mAsyncEvents.get()};
    auto evt_vec = ring->getWriteVector();
    if(evt_vec.first.len < 1) return;

    AsyncEvent *evt{al::construct_at(reinterpret_cast<AsyncEvent*>(evt_vec.first.buf),
        AsyncEvent::SourceStateChange)};
    evt->u.srcstate.id = id;
    switch(state)
    {
    case VChangeState::Reset:
        evt->u.srcstate.state = AsyncEvent::SrcState::Reset;
        break;
    case VChangeState::Stop:
        evt->u.srcstate.state = AsyncEvent::SrcState::Stop;
        break;
    case VChangeState::Play:
        evt->u.srcstate.state = AsyncEvent::SrcState::Play;
        break;
    case VChangeState::Pause:
        evt->u.srcstate.state = AsyncEvent::SrcState::Pause;
        break;
    /* Shouldn't happen. */
    case VChangeState::Restart:
        ASSUME(0);
    }

    ring->writeAdvance(1);
}

void ProcessVoiceChanges(ContextBase *ctx)
{
    VoiceChange *cur{ctx->mCurrentVoiceChange.load(std::memory_order_acquire)};
    VoiceChange *next{cur->mNext.load(std::memory_order_acquire)};
    if(!next) return;

    const uint enabledevt{ctx->mEnabledEvts.load(std::memory_order_acquire)};
    do {
        cur = next;

        bool sendevt{false};
        if(cur->mState == VChangeState::Reset || cur->mState == VChangeState::Stop)
        {
            if(Voice *voice{cur->mVoice})
            {
                voice->mCurrentBuffer.store(nullptr, std::memory_order_relaxed);
                voice->mLoopBuffer.store(nullptr, std::memory_order_relaxed);
                /* A source ID indicates the voice was playing or paused, which
                 * gets a reset/stop event.
                 */
                sendevt = voice->mSourceID.exchange(0u, std::memory_order_relaxed) != 0u;
                Voice::State oldvstate{Voice::Playing};
                voice->mPlayState.compare_exchange_strong(oldvstate, Voice::Stopping,
                    std::memory_order_relaxed, std::memory_order_acquire);
                voice->mPendingChange.store(false, std::memory_order_release);
            }
            /* Reset state change events are always sent, even if the voice is
             * already stopped or even if there is no voice.
             */
            sendevt |= (cur->mState == VChangeState::Reset);
        }
        else if(cur->mState == VChangeState::Pause)
        {
            Voice *voice{cur->mVoice};
            Voice::State oldvstate{Voice::Playing};
            sendevt = voice->mPlayState.compare_exchange_strong(oldvstate, Voice::Stopping,
                std::memory_order_release, std::memory_order_acquire);
        }
        else if(cur->mState == VChangeState::Play)
        {
            /* NOTE: When playing a voice, sending a source state change event
             * depends if there's an old voice to stop and if that stop is
             * successful. If there is no old voice, a playing event is always
             * sent. If there is an old voice, an event is sent only if the
             * voice is already stopped.
             */
            if(Voice *oldvoice{cur->mOldVoice})
            {
                oldvoice->mCurrentBuffer.store(nullptr, std::memory_order_relaxed);
                oldvoice->mLoopBuffer.store(nullptr, std::memory_order_relaxed);
                oldvoice->mSourceID.store(0u, std::memory_order_relaxed);
                Voice::State oldvstate{Voice::Playing};
                sendevt = !oldvoice->mPlayState.compare_exchange_strong(oldvstate, Voice::Stopping,
                    std::memory_order_relaxed, std::memory_order_acquire);
                oldvoice->mPendingChange.store(false, std::memory_order_release);
            }
            else
                sendevt = true;

            Voice *voice{cur->mVoice};
            voice->mPlayState.store(Voice::Playing, std::memory_order_release);
        }
        else if(cur->mState == VChangeState::Restart)
        {
            /* Restarting a voice never sends a source change event. */
            Voice *oldvoice{cur->mOldVoice};
            oldvoice->mCurrentBuffer.store(nullptr, std::memory_order_relaxed);
            oldvoice->mLoopBuffer.store(nullptr, std::memory_order_relaxed);
            /* If there's no sourceID, the old voice finished so don't start
             * the new one at its new offset.
             */
            if(oldvoice->mSourceID.exchange(0u, std::memory_order_relaxed) != 0u)
            {
                /* Otherwise, set the voice to stopping if it's not already (it
                 * might already be, if paused), and play the new voice as
                 * appropriate.
                 */
                Voice::State oldvstate{Voice::Playing};
                oldvoice->mPlayState.compare_exchange_strong(oldvstate, Voice::Stopping,
                    std::memory_order_relaxed, std::memory_order_acquire);

                Voice *voice{cur->mVoice};
                voice->mPlayState.store((oldvstate == Voice::Playing) ? Voice::Playing
                    : Voice::Stopped, std::memory_order_release);
            }
            oldvoice->mPendingChange.store(false, std::memory_order_release);
        }
        if(sendevt && (enabledevt&AsyncEvent::SourceStateChange))
            SendSourceStateEvent(ctx, cur->mSourceID, cur->mState);

        next = cur->mNext.load(std::memory_order_acquire);
    } while(next);
    ctx->mCurrentVoiceChange.store(cur, std::memory_order_release);
}

void ProcessParamUpdates(ContextBase *ctx, const EffectSlotArray &slots,
    const al::span<Voice*> voices)
{
    ProcessVoiceChanges(ctx);

    IncrementRef(ctx->mUpdateCount);
    if LIKELY(!ctx->mHoldUpdates.load(std::memory_order_acquire))
    {
        bool force{CalcContextParams(ctx)};
        auto sorted_slots = const_cast<EffectSlot**>(slots.data() + slots.size());
        for(EffectSlot *slot : slots)
            force |= CalcEffectSlotParams(slot, sorted_slots, ctx);

        for(Voice *voice : voices)
        {
            /* Only update voices that have a source. */
            if(voice->mSourceID.load(std::memory_order_relaxed) != 0)
                CalcSourceParams(voice, ctx, force);
        }
    }
    IncrementRef(ctx->mUpdateCount);
}

void ProcessContexts(DeviceBase *device, const uint SamplesToDo)
{
    ASSUME(SamplesToDo > 0);

    for(ContextBase *ctx : *device->mContexts.load(std::memory_order_acquire))
    {
        const EffectSlotArray &auxslots = *ctx->mActiveAuxSlots.load(std::memory_order_acquire);
        const al::span<Voice*> voices{ctx->getVoicesSpanAcquired()};

        /* Process pending propery updates for objects on the context. */
        ProcessParamUpdates(ctx, auxslots, voices);

        /* Clear auxiliary effect slot mixing buffers. */
        for(EffectSlot *slot : auxslots)
        {
            for(auto &buffer : slot->Wet.Buffer)
                buffer.fill(0.0f);
        }

        /* Process voices that have a playing source. */
        for(Voice *voice : voices)
        {
            const Voice::State vstate{voice->mPlayState.load(std::memory_order_acquire)};
            if(vstate != Voice::Stopped && vstate != Voice::Pending)
                voice->mix(vstate, ctx, SamplesToDo);
        }

        /* Process effects. */
        if(const size_t num_slots{auxslots.size()})
        {
            auto slots = auxslots.data();
            auto slots_end = slots + num_slots;

            /* Sort the slots into extra storage, so that effect slots come
             * before their effect slot target (or their targets' target).
             */
            const al::span<EffectSlot*> sorted_slots{const_cast<EffectSlot**>(slots_end),
                num_slots};
            /* Skip sorting if it has already been done. */
            if(!sorted_slots[0])
            {
                /* First, copy the slots to the sorted list, then partition the
                 * sorted list so that all slots without a target slot go to
                 * the end.
                 */
                std::copy(slots, slots_end, sorted_slots.begin());
                auto split_point = std::partition(sorted_slots.begin(), sorted_slots.end(),
                    [](const EffectSlot *slot) noexcept -> bool
                    { return slot->Target != nullptr; });
                /* There must be at least one slot without a slot target. */
                assert(split_point != sorted_slots.end());

                /* Simple case: no more than 1 slot has a target slot. Either
                 * all slots go right to the output, or the remaining one must
                 * target an already-partitioned slot.
                 */
                if(split_point - sorted_slots.begin() > 1)
                {
                    /* At least two slots target other slots. Starting from the
                     * back of the sorted list, continue partitioning the front
                     * of the list given each target until all targets are
                     * accounted for. This ensures all slots without a target
                     * go last, all slots directly targeting those last slots
                     * go second-to-last, all slots directly targeting those
                     * second-last slots go third-to-last, etc.
                     */
                    auto next_target = sorted_slots.end();
                    do {
                        /* This shouldn't happen, but if there's unsorted slots
                         * left that don't target any sorted slots, they can't
                         * contribute to the output, so leave them.
                         */
                        if UNLIKELY(next_target == split_point)
                            break;

                        --next_target;
                        split_point = std::partition(sorted_slots.begin(), split_point,
                            [next_target](const EffectSlot *slot) noexcept -> bool
                            { return slot->Target != *next_target; });
                    } while(split_point - sorted_slots.begin() > 1);
                }
            }

            for(const EffectSlot *slot : sorted_slots)
            {
                EffectState *state{slot->mEffectState};
                state->process(SamplesToDo, slot->Wet.Buffer, state->mOutTarget);
            }
        }

        /* Signal the event handler if there are any events to read. */
        RingBuffer *ring{ctx->mAsyncEvents.get()};
        if(ring->readSpace() > 0)
            ctx->mEventSem.post();
    }
}


void ApplyDistanceComp(const al::span<FloatBufferLine> Samples, const size_t SamplesToDo,
    const DistanceComp::ChanData *distcomp)
{
    ASSUME(SamplesToDo > 0);

    for(auto &chanbuffer : Samples)
    {
        const float gain{distcomp->Gain};
        const size_t base{distcomp->Length};
        float *distbuf{al::assume_aligned<16>(distcomp->Buffer)};
        ++distcomp;

        if(base < 1)
            continue;

        float *inout{al::assume_aligned<16>(chanbuffer.data())};
        auto inout_end = inout + SamplesToDo;
        if LIKELY(SamplesToDo >= base)
        {
            auto delay_end = std::rotate(inout, inout_end - base, inout_end);
            std::swap_ranges(inout, delay_end, distbuf);
        }
        else
        {
            auto delay_start = std::swap_ranges(inout, inout_end, distbuf);
            std::rotate(distbuf, delay_start, distbuf + base);
        }
        std::transform(inout, inout_end, inout, std::bind(std::multiplies<float>{}, _1, gain));
    }
}

void ApplyDither(const al::span<FloatBufferLine> Samples, uint *dither_seed,
    const float quant_scale, const size_t SamplesToDo)
{
    ASSUME(SamplesToDo > 0);

    /* Dithering. Generate whitenoise (uniform distribution of random values
     * between -1 and +1) and add it to the sample values, after scaling up to
     * the desired quantization depth amd before rounding.
     */
    const float invscale{1.0f / quant_scale};
    uint seed{*dither_seed};
    auto dither_sample = [&seed,invscale,quant_scale](const float sample) noexcept -> float
    {
        float val{sample * quant_scale};
        uint rng0{dither_rng(&seed)};
        uint rng1{dither_rng(&seed)};
        val += static_cast<float>(rng0*(1.0/UINT_MAX) - rng1*(1.0/UINT_MAX));
        return fast_roundf(val) * invscale;
    };
    for(FloatBufferLine &inout : Samples)
        std::transform(inout.begin(), inout.begin()+SamplesToDo, inout.begin(), dither_sample);
    *dither_seed = seed;
}


/* Base template left undefined. Should be marked =delete, but Clang 3.8.1
 * chokes on that given the inline specializations.
 */
template<typename T>
inline T SampleConv(float) noexcept;

template<> inline float SampleConv(float val) noexcept
{ return val; }
template<> inline int32_t SampleConv(float val) noexcept
{
    /* Floats have a 23-bit mantissa, plus an implied 1 bit and a sign bit.
     * This means a normalized float has at most 25 bits of signed precision.
     * When scaling and clamping for a signed 32-bit integer, these following
     * values are the best a float can give.
     */
    return fastf2i(clampf(val*2147483648.0f, -2147483648.0f, 2147483520.0f));
}
template<> inline int16_t SampleConv(float val) noexcept
{ return static_cast<int16_t>(fastf2i(clampf(val*32768.0f, -32768.0f, 32767.0f))); }
template<> inline int8_t SampleConv(float val) noexcept
{ return static_cast<int8_t>(fastf2i(clampf(val*128.0f, -128.0f, 127.0f))); }

/* Define unsigned output variations. */
template<> inline uint32_t SampleConv(float val) noexcept
{ return static_cast<uint32_t>(SampleConv<int32_t>(val)) + 2147483648u; }
template<> inline uint16_t SampleConv(float val) noexcept
{ return static_cast<uint16_t>(SampleConv<int16_t>(val) + 32768); }
template<> inline uint8_t SampleConv(float val) noexcept
{ return static_cast<uint8_t>(SampleConv<int8_t>(val) + 128); }

template<DevFmtType T>
void Write(const al::span<const FloatBufferLine> InBuffer, void *OutBuffer, const size_t Offset,
    const size_t SamplesToDo, const size_t FrameStep)
{
    ASSUME(FrameStep > 0);
    ASSUME(SamplesToDo > 0);

    DevFmtType_t<T> *outbase{static_cast<DevFmtType_t<T>*>(OutBuffer) + Offset*FrameStep};
    size_t c{0};
    for(const FloatBufferLine &inbuf : InBuffer)
    {
        DevFmtType_t<T> *out{outbase++};
        auto conv_sample = [FrameStep,&out](const float s) noexcept -> void
        {
            *out = SampleConv<DevFmtType_t<T>>(s);
            out += FrameStep;
        };
        std::for_each(inbuf.begin(), inbuf.begin()+SamplesToDo, conv_sample);
        ++c;
    }
    if(const size_t extra{FrameStep - c})
    {
        const auto silence = SampleConv<DevFmtType_t<T>>(0.0f);
        for(size_t i{0};i < SamplesToDo;++i)
        {
            std::fill_n(outbase, extra, silence);
            outbase += FrameStep;
        }
    }
}

} // namespace

uint DeviceBase::renderSamples(const uint numSamples)
{
    const uint samplesToDo{minu(numSamples, BufferLineSize)};

    /* Clear main mixing buffers. */
    for(FloatBufferLine &buffer : MixBuffer)
        buffer.fill(0.0f);

    /* Increment the mix count at the start (lsb should now be 1). */
    IncrementRef(MixCount);

    /* Process and mix each context's sources and effects. */
    ProcessContexts(this, samplesToDo);

    /* Increment the clock time. Every second's worth of samples is converted
     * and added to clock base so that large sample counts don't overflow
     * during conversion. This also guarantees a stable conversion.
     */
    SamplesDone += samplesToDo;
    ClockBase += std::chrono::seconds{SamplesDone / Frequency};
    SamplesDone %= Frequency;

    /* Increment the mix count at the end (lsb should now be 0). */
    IncrementRef(MixCount);

    /* Apply any needed post-process for finalizing the Dry mix to the RealOut
     * (Ambisonic decode, UHJ encode, etc).
     */
    postProcess(samplesToDo);

    /* Apply compression, limiting sample amplitude if needed or desired. */
    if(Limiter) Limiter->process(samplesToDo, RealOut.Buffer.data());

    /* Apply delays and attenuation for mismatched speaker distances. */
    if(ChannelDelays)
        ApplyDistanceComp(RealOut.Buffer, samplesToDo, ChannelDelays->mChannels.data());

    /* Apply dithering. The compressor should have left enough headroom for the
     * dither noise to not saturate.
     */
    if(DitherDepth > 0.0f)
        ApplyDither(RealOut.Buffer, &DitherSeed, DitherDepth, samplesToDo);

    return samplesToDo;
}

void DeviceBase::renderSamples(const al::span<float*> outBuffers, const uint numSamples)
{
    FPUCtl mixer_mode{};
    uint total{0};
    while(const uint todo{numSamples - total})
    {
        const uint samplesToDo{renderSamples(todo)};

        auto *srcbuf = RealOut.Buffer.data();
        for(auto *dstbuf : outBuffers)
        {
            std::copy_n(srcbuf->data(), samplesToDo, dstbuf + total);
            ++srcbuf;
        }

        total += samplesToDo;
    }
}

void DeviceBase::renderSamples(void *outBuffer, const uint numSamples, const size_t frameStep)
{
    FPUCtl mixer_mode{};
    uint total{0};
    while(const uint todo{numSamples - total})
    {
        const uint samplesToDo{renderSamples(todo)};

        if LIKELY(outBuffer)
        {
            /* Finally, interleave and convert samples, writing to the device's
             * output buffer.
             */
            switch(FmtType)
            {
#define HANDLE_WRITE(T) case T:                                               \
    Write<T>(RealOut.Buffer, outBuffer, total, samplesToDo, frameStep); break;
            HANDLE_WRITE(DevFmtByte)
            HANDLE_WRITE(DevFmtUByte)
            HANDLE_WRITE(DevFmtShort)
            HANDLE_WRITE(DevFmtUShort)
            HANDLE_WRITE(DevFmtInt)
            HANDLE_WRITE(DevFmtUInt)
            HANDLE_WRITE(DevFmtFloat)
#undef HANDLE_WRITE
            }
        }

        total += samplesToDo;
    }
}

void DeviceBase::handleDisconnect(const char *msg, ...)
{
    if(!Connected.exchange(false, std::memory_order_acq_rel))
        return;

    AsyncEvent evt{AsyncEvent::Disconnected};

    va_list args;
    va_start(args, msg);
    int msglen{vsnprintf(evt.u.disconnect.msg, sizeof(evt.u.disconnect.msg), msg, args)};
    va_end(args);

    if(msglen < 0 || static_cast<size_t>(msglen) >= sizeof(evt.u.disconnect.msg))
        evt.u.disconnect.msg[sizeof(evt.u.disconnect.msg)-1] = 0;

    IncrementRef(MixCount);
    for(ContextBase *ctx : *mContexts.load())
    {
        const uint enabledevt{ctx->mEnabledEvts.load(std::memory_order_acquire)};
        if((enabledevt&AsyncEvent::Disconnected))
        {
            RingBuffer *ring{ctx->mAsyncEvents.get()};
            auto evt_data = ring->getWriteVector().first;
            if(evt_data.len > 0)
            {
                al::construct_at(reinterpret_cast<AsyncEvent*>(evt_data.buf), evt);
                ring->writeAdvance(1);
                ctx->mEventSem.post();
            }
        }

        if(!ctx->mStopVoicesOnDisconnect)
        {
            ProcessVoiceChanges(ctx);
            continue;
        }

        auto voicelist = ctx->getVoicesSpanAcquired();
        auto stop_voice = [](Voice *voice) -> void
        {
            voice->mCurrentBuffer.store(nullptr, std::memory_order_relaxed);
            voice->mLoopBuffer.store(nullptr, std::memory_order_relaxed);
            voice->mSourceID.store(0u, std::memory_order_relaxed);
            voice->mPlayState.store(Voice::Stopped, std::memory_order_release);
        };
        std::for_each(voicelist.begin(), voicelist.end(), stop_voice);
    }
    IncrementRef(MixCount);
}
