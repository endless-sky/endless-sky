#include "config.h"

#include <xmmintrin.h>

#include <cmath>
#include <limits>

#include "alnumeric.h"
#include "core/bsinc_defs.h"
#include "defs.h"
#include "hrtfbase.h"

struct SSETag;
struct BSincTag;
struct FastBSincTag;


#if defined(__GNUC__) && !defined(__clang__) && !defined(__SSE__)
#pragma GCC target("sse")
#endif

namespace {

constexpr uint FracPhaseBitDiff{MixerFracBits - BSincPhaseBits};
constexpr uint FracPhaseDiffOne{1 << FracPhaseBitDiff};

#define MLA4(x, y, z) _mm_add_ps(x, _mm_mul_ps(y, z))

inline void ApplyCoeffs(float2 *RESTRICT Values, const size_t IrSize, const ConstHrirSpan Coeffs,
    const float left, const float right)
{
    const __m128 lrlr{_mm_setr_ps(left, right, left, right)};

    ASSUME(IrSize >= MinIrLength);
    /* This isn't technically correct to test alignment, but it's true for
     * systems that support SSE, which is the only one that needs to know the
     * alignment of Values (which alternates between 8- and 16-byte aligned).
     */
    if(!(reinterpret_cast<uintptr_t>(Values)&15))
    {
        for(size_t i{0};i < IrSize;i += 2)
        {
            const __m128 coeffs{_mm_load_ps(&Coeffs[i][0])};
            __m128 vals{_mm_load_ps(&Values[i][0])};
            vals = MLA4(vals, lrlr, coeffs);
            _mm_store_ps(&Values[i][0], vals);
        }
    }
    else
    {
        __m128 imp0, imp1;
        __m128 coeffs{_mm_load_ps(&Coeffs[0][0])};
        __m128 vals{_mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<__m64*>(&Values[0][0]))};
        imp0 = _mm_mul_ps(lrlr, coeffs);
        vals = _mm_add_ps(imp0, vals);
        _mm_storel_pi(reinterpret_cast<__m64*>(&Values[0][0]), vals);
        size_t td{((IrSize+1)>>1) - 1};
        size_t i{1};
        do {
            coeffs = _mm_load_ps(&Coeffs[i+1][0]);
            vals = _mm_load_ps(&Values[i][0]);
            imp1 = _mm_mul_ps(lrlr, coeffs);
            imp0 = _mm_shuffle_ps(imp0, imp1, _MM_SHUFFLE(1, 0, 3, 2));
            vals = _mm_add_ps(imp0, vals);
            _mm_store_ps(&Values[i][0], vals);
            imp0 = imp1;
            i += 2;
        } while(--td);
        vals = _mm_loadl_pi(vals, reinterpret_cast<__m64*>(&Values[i][0]));
        imp0 = _mm_movehl_ps(imp0, imp0);
        vals = _mm_add_ps(imp0, vals);
        _mm_storel_pi(reinterpret_cast<__m64*>(&Values[i][0]), vals);
    }
}

} // namespace

template<>
float *Resample_<BSincTag,SSETag>(const InterpState *state, float *RESTRICT src, uint frac,
    uint increment, const al::span<float> dst)
{
    const float *const filter{state->bsinc.filter};
    const __m128 sf4{_mm_set1_ps(state->bsinc.sf)};
    const size_t m{state->bsinc.m};
    ASSUME(m > 0);

    src -= state->bsinc.l;
    for(float &out_sample : dst)
    {
        // Calculate the phase index and factor.
        const uint pi{frac >> FracPhaseBitDiff};
        const float pf{static_cast<float>(frac & (FracPhaseDiffOne-1)) * (1.0f/FracPhaseDiffOne)};

        // Apply the scale and phase interpolated filter.
        __m128 r4{_mm_setzero_ps()};
        {
            const __m128 pf4{_mm_set1_ps(pf)};
            const float *RESTRICT fil{filter + m*pi*2};
            const float *RESTRICT phd{fil + m};
            const float *RESTRICT scd{fil + BSincPhaseCount*2*m};
            const float *RESTRICT spd{scd + m};
            size_t td{m >> 2};
            size_t j{0u};

            do {
                /* f = ((fil + sf*scd) + pf*(phd + sf*spd)) */
                const __m128 f4 = MLA4(
                    MLA4(_mm_load_ps(&fil[j]), sf4, _mm_load_ps(&scd[j])),
                    pf4, MLA4(_mm_load_ps(&phd[j]), sf4, _mm_load_ps(&spd[j])));
                /* r += f*src */
                r4 = MLA4(r4, f4, _mm_loadu_ps(&src[j]));
                j += 4;
            } while(--td);
        }
        r4 = _mm_add_ps(r4, _mm_shuffle_ps(r4, r4, _MM_SHUFFLE(0, 1, 2, 3)));
        r4 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
        out_sample = _mm_cvtss_f32(r4);

        frac += increment;
        src  += frac>>MixerFracBits;
        frac &= MixerFracMask;
    }
    return dst.data();
}

template<>
float *Resample_<FastBSincTag,SSETag>(const InterpState *state, float *RESTRICT src, uint frac,
    uint increment, const al::span<float> dst)
{
    const float *const filter{state->bsinc.filter};
    const size_t m{state->bsinc.m};
    ASSUME(m > 0);

    src -= state->bsinc.l;
    for(float &out_sample : dst)
    {
        // Calculate the phase index and factor.
        const uint pi{frac >> FracPhaseBitDiff};
        const float pf{static_cast<float>(frac & (FracPhaseDiffOne-1)) * (1.0f/FracPhaseDiffOne)};

        // Apply the phase interpolated filter.
        __m128 r4{_mm_setzero_ps()};
        {
            const __m128 pf4{_mm_set1_ps(pf)};
            const float *RESTRICT fil{filter + m*pi*2};
            const float *RESTRICT phd{fil + m};
            size_t td{m >> 2};
            size_t j{0u};

            do {
                /* f = fil + pf*phd */
                const __m128 f4 = MLA4(_mm_load_ps(&fil[j]), pf4, _mm_load_ps(&phd[j]));
                /* r += f*src */
                r4 = MLA4(r4, f4, _mm_loadu_ps(&src[j]));
                j += 4;
            } while(--td);
        }
        r4 = _mm_add_ps(r4, _mm_shuffle_ps(r4, r4, _MM_SHUFFLE(0, 1, 2, 3)));
        r4 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
        out_sample = _mm_cvtss_f32(r4);

        frac += increment;
        src  += frac>>MixerFracBits;
        frac &= MixerFracMask;
    }
    return dst.data();
}


template<>
void MixHrtf_<SSETag>(const float *InSamples, float2 *AccumSamples, const uint IrSize,
    const MixHrtfFilter *hrtfparams, const size_t BufferSize)
{ MixHrtfBase<ApplyCoeffs>(InSamples, AccumSamples, IrSize, hrtfparams, BufferSize); }

template<>
void MixHrtfBlend_<SSETag>(const float *InSamples, float2 *AccumSamples, const uint IrSize,
    const HrtfFilter *oldparams, const MixHrtfFilter *newparams, const size_t BufferSize)
{
    MixHrtfBlendBase<ApplyCoeffs>(InSamples, AccumSamples, IrSize, oldparams, newparams,
        BufferSize);
}

template<>
void MixDirectHrtf_<SSETag>(const FloatBufferSpan LeftOut, const FloatBufferSpan RightOut,
    const al::span<const FloatBufferLine> InSamples, float2 *AccumSamples,
    float *TempBuf, HrtfChannelState *ChanState, const size_t IrSize, const size_t BufferSize)
{
    MixDirectHrtfBase<ApplyCoeffs>(LeftOut, RightOut, InSamples, AccumSamples, TempBuf, ChanState,
        IrSize, BufferSize);
}


template<>
void Mix_<SSETag>(const al::span<const float> InSamples, const al::span<FloatBufferLine> OutBuffer,
    float *CurrentGains, const float *TargetGains, const size_t Counter, const size_t OutPos)
{
    const float delta{(Counter > 0) ? 1.0f / static_cast<float>(Counter) : 0.0f};
    const auto min_len = minz(Counter, InSamples.size());
    const auto aligned_len = minz((min_len+3) & ~size_t{3}, InSamples.size()) - min_len;

    for(FloatBufferLine &output : OutBuffer)
    {
        float *RESTRICT dst{al::assume_aligned<16>(output.data()+OutPos)};
        float gain{*CurrentGains};
        const float step{(*TargetGains-gain) * delta};

        size_t pos{0};
        if(!(std::abs(step) > std::numeric_limits<float>::epsilon()))
            gain = *TargetGains;
        else
        {
            float step_count{0.0f};
            /* Mix with applying gain steps in aligned multiples of 4. */
            if(size_t todo{min_len >> 2})
            {
                const __m128 four4{_mm_set1_ps(4.0f)};
                const __m128 step4{_mm_set1_ps(step)};
                const __m128 gain4{_mm_set1_ps(gain)};
                __m128 step_count4{_mm_setr_ps(0.0f, 1.0f, 2.0f, 3.0f)};
                do {
                    const __m128 val4{_mm_load_ps(&InSamples[pos])};
                    __m128 dry4{_mm_load_ps(&dst[pos])};

                    /* dry += val * (gain + step*step_count) */
                    dry4 = MLA4(dry4, val4, MLA4(gain4, step4, step_count4));

                    _mm_store_ps(&dst[pos], dry4);
                    step_count4 = _mm_add_ps(step_count4, four4);
                    pos += 4;
                } while(--todo);
                /* NOTE: step_count4 now represents the next four counts after
                 * the last four mixed samples, so the lowest element
                 * represents the next step count to apply.
                 */
                step_count = _mm_cvtss_f32(step_count4);
            }
            /* Mix with applying left over gain steps that aren't aligned multiples of 4. */
            for(size_t leftover{min_len&3};leftover;++pos,--leftover)
            {
                dst[pos] += InSamples[pos] * (gain + step*step_count);
                step_count += 1.0f;
            }
            if(pos == Counter)
                gain = *TargetGains;
            else
                gain += step*step_count;

            /* Mix until pos is aligned with 4 or the mix is done. */
            for(size_t leftover{aligned_len&3};leftover;++pos,--leftover)
                dst[pos] += InSamples[pos] * gain;
        }
        *CurrentGains = gain;
        ++CurrentGains;
        ++TargetGains;

        if(!(std::abs(gain) > GainSilenceThreshold))
            continue;
        if(size_t todo{(InSamples.size()-pos) >> 2})
        {
            const __m128 gain4{_mm_set1_ps(gain)};
            do {
                const __m128 val4{_mm_load_ps(&InSamples[pos])};
                __m128 dry4{_mm_load_ps(&dst[pos])};
                dry4 = _mm_add_ps(dry4, _mm_mul_ps(val4, gain4));
                _mm_store_ps(&dst[pos], dry4);
                pos += 4;
            } while(--todo);
        }
        for(size_t leftover{(InSamples.size()-pos)&3};leftover;++pos,--leftover)
            dst[pos] += InSamples[pos] * gain;
    }
}
