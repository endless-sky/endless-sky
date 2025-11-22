#ifndef CORE_MIXER_HRTFBASE_H
#define CORE_MIXER_HRTFBASE_H

#include <algorithm>
#include <cmath>

#include "almalloc.h"
#include "hrtfdefs.h"
#include "opthelpers.h"


using uint = unsigned int;

using ApplyCoeffsT = void(&)(float2 *RESTRICT Values, const size_t irSize,
    const ConstHrirSpan Coeffs, const float left, const float right);

template<ApplyCoeffsT ApplyCoeffs>
inline void MixHrtfBase(const float *InSamples, float2 *RESTRICT AccumSamples, const size_t IrSize,
    const MixHrtfFilter *hrtfparams, const size_t BufferSize)
{
    ASSUME(BufferSize > 0);

    const ConstHrirSpan Coeffs{hrtfparams->Coeffs};
    const float gainstep{hrtfparams->GainStep};
    const float gain{hrtfparams->Gain};

    size_t ldelay{HrtfHistoryLength - hrtfparams->Delay[0]};
    size_t rdelay{HrtfHistoryLength - hrtfparams->Delay[1]};
    float stepcount{0.0f};
    for(size_t i{0u};i < BufferSize;++i)
    {
        const float g{gain + gainstep*stepcount};
        const float left{InSamples[ldelay++] * g};
        const float right{InSamples[rdelay++] * g};
        ApplyCoeffs(AccumSamples+i, IrSize, Coeffs, left, right);

        stepcount += 1.0f;
    }
}

template<ApplyCoeffsT ApplyCoeffs>
inline void MixHrtfBlendBase(const float *InSamples, float2 *RESTRICT AccumSamples,
    const size_t IrSize, const HrtfFilter *oldparams, const MixHrtfFilter *newparams,
    const size_t BufferSize)
{
    ASSUME(BufferSize > 0);

    const ConstHrirSpan OldCoeffs{oldparams->Coeffs};
    const float oldGainStep{oldparams->Gain / static_cast<float>(BufferSize)};
    const ConstHrirSpan NewCoeffs{newparams->Coeffs};
    const float newGainStep{newparams->GainStep};

    if LIKELY(oldparams->Gain > GainSilenceThreshold)
    {
        size_t ldelay{HrtfHistoryLength - oldparams->Delay[0]};
        size_t rdelay{HrtfHistoryLength - oldparams->Delay[1]};
        auto stepcount = static_cast<float>(BufferSize);
        for(size_t i{0u};i < BufferSize;++i)
        {
            const float g{oldGainStep*stepcount};
            const float left{InSamples[ldelay++] * g};
            const float right{InSamples[rdelay++] * g};
            ApplyCoeffs(AccumSamples+i, IrSize, OldCoeffs, left, right);

            stepcount -= 1.0f;
        }
    }

    if LIKELY(newGainStep*static_cast<float>(BufferSize) > GainSilenceThreshold)
    {
        size_t ldelay{HrtfHistoryLength+1 - newparams->Delay[0]};
        size_t rdelay{HrtfHistoryLength+1 - newparams->Delay[1]};
        float stepcount{1.0f};
        for(size_t i{1u};i < BufferSize;++i)
        {
            const float g{newGainStep*stepcount};
            const float left{InSamples[ldelay++] * g};
            const float right{InSamples[rdelay++] * g};
            ApplyCoeffs(AccumSamples+i, IrSize, NewCoeffs, left, right);

            stepcount += 1.0f;
        }
    }
}

template<ApplyCoeffsT ApplyCoeffs>
inline void MixDirectHrtfBase(const FloatBufferSpan LeftOut, const FloatBufferSpan RightOut,
    const al::span<const FloatBufferLine> InSamples, float2 *RESTRICT AccumSamples,
    float *TempBuf, HrtfChannelState *ChanState, const size_t IrSize, const size_t BufferSize)
{
    ASSUME(BufferSize > 0);

    for(const FloatBufferLine &input : InSamples)
    {
        /* For dual-band processing, the signal needs extra scaling applied to
         * the high frequency response. The band-splitter applies this scaling
         * with a consistent phase shift regardless of the scale amount.
         */
        ChanState->mSplitter.processHfScale({input.data(), BufferSize}, TempBuf,
            ChanState->mHfScale);

        /* Now apply the HRIR coefficients to this channel. */
        const float *RESTRICT tempbuf{al::assume_aligned<16>(TempBuf)};
        const ConstHrirSpan Coeffs{ChanState->mCoeffs};
        for(size_t i{0u};i < BufferSize;++i)
        {
            const float insample{tempbuf[i]};
            ApplyCoeffs(AccumSamples+i, IrSize, Coeffs, insample, insample);
        }

        ++ChanState;
    }

    /* Add the HRTF signal to the existing "direct" signal. */
    float *RESTRICT left{al::assume_aligned<16>(LeftOut.data())};
    float *RESTRICT right{al::assume_aligned<16>(RightOut.data())};
    for(size_t i{0u};i < BufferSize;++i)
        left[i]  += AccumSamples[i][0];
    for(size_t i{0u};i < BufferSize;++i)
        right[i] += AccumSamples[i][1];

    /* Copy the new in-progress accumulation values to the front and clear the
     * following samples for the next mix.
     */
    auto accum_iter = std::copy_n(AccumSamples+BufferSize, HrirLength, AccumSamples);
    std::fill_n(accum_iter, BufferSize, float2{});
}

#endif /* CORE_MIXER_HRTFBASE_H */
