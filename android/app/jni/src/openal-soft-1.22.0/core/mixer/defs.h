#ifndef CORE_MIXER_DEFS_H
#define CORE_MIXER_DEFS_H

#include <array>
#include <stdlib.h>

#include "alspan.h"
#include "core/bufferline.h"
#include "core/resampler_limits.h"

struct HrtfChannelState;
struct HrtfFilter;
struct MixHrtfFilter;

using uint = unsigned int;
using float2 = std::array<float,2>;


constexpr int MixerFracBits{12};
constexpr int MixerFracOne{1 << MixerFracBits};
constexpr int MixerFracMask{MixerFracOne - 1};

constexpr float GainSilenceThreshold{0.00001f}; /* -100dB */


enum class Resampler {
    Point,
    Linear,
    Cubic,
    FastBSinc12,
    BSinc12,
    FastBSinc24,
    BSinc24,

    Max = BSinc24
};

/* Interpolator state. Kind of a misnomer since the interpolator itself is
 * stateless. This just keeps it from having to recompute scale-related
 * mappings for every sample.
 */
struct BsincState {
    float sf; /* Scale interpolation factor. */
    uint m; /* Coefficient count. */
    uint l; /* Left coefficient offset. */
    /* Filter coefficients, followed by the phase, scale, and scale-phase
     * delta coefficients. Starting at phase index 0, each subsequent phase
     * index follows contiguously.
     */
    const float *filter;
};

union InterpState {
    BsincState bsinc;
};

using ResamplerFunc = float*(*)(const InterpState *state, float *RESTRICT src, uint frac,
    uint increment, const al::span<float> dst);

ResamplerFunc PrepareResampler(Resampler resampler, uint increment, InterpState *state);


template<typename TypeTag, typename InstTag>
float *Resample_(const InterpState *state, float *RESTRICT src, uint frac, uint increment,
    const al::span<float> dst);

template<typename InstTag>
void Mix_(const al::span<const float> InSamples, const al::span<FloatBufferLine> OutBuffer,
    float *CurrentGains, const float *TargetGains, const size_t Counter, const size_t OutPos);

template<typename InstTag>
void MixHrtf_(const float *InSamples, float2 *AccumSamples, const uint IrSize,
    const MixHrtfFilter *hrtfparams, const size_t BufferSize);
template<typename InstTag>
void MixHrtfBlend_(const float *InSamples, float2 *AccumSamples, const uint IrSize,
    const HrtfFilter *oldparams, const MixHrtfFilter *newparams, const size_t BufferSize);
template<typename InstTag>
void MixDirectHrtf_(const FloatBufferSpan LeftOut, const FloatBufferSpan RightOut,
    const al::span<const FloatBufferLine> InSamples, float2 *AccumSamples,
    float *TempBuf, HrtfChannelState *ChanState, const size_t IrSize, const size_t BufferSize);

/* Vectorized resampler helpers */
template<size_t N>
inline void InitPosArrays(uint frac, uint increment, uint (&frac_arr)[N], uint (&pos_arr)[N])
{
    pos_arr[0] = 0;
    frac_arr[0] = frac;
    for(size_t i{1};i < N;i++)
    {
        const uint frac_tmp{frac_arr[i-1] + increment};
        pos_arr[i] = pos_arr[i-1] + (frac_tmp>>MixerFracBits);
        frac_arr[i] = frac_tmp&MixerFracMask;
    }
}

#endif /* CORE_MIXER_DEFS_H */
