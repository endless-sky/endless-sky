#ifndef PHASE_SHIFTER_H
#define PHASE_SHIFTER_H

#ifdef HAVE_SSE_INTRINSICS
#include <xmmintrin.h>
#elif defined(HAVE_NEON)
#include <arm_neon.h>
#endif

#include <array>
#include <stddef.h>

#include "alcomplex.h"
#include "alspan.h"


/* Implements a wide-band +90 degree phase-shift. Note that this should be
 * given one sample less of a delay (FilterSize/2 - 1) compared to the direct
 * signal delay (FilterSize/2) to properly align.
 */
template<size_t FilterSize>
struct PhaseShifterT {
    static_assert(FilterSize >= 16, "FilterSize needs to be at least 16");
    static_assert((FilterSize&(FilterSize-1)) == 0, "FilterSize needs to be power-of-two");

    alignas(16) std::array<float,FilterSize/2> mCoeffs{};

    /* Some notes on this filter construction.
     *
     * A wide-band phase-shift filter needs a delay to maintain linearity. A
     * dirac impulse in the center of a time-domain buffer represents a filter
     * passing all frequencies through as-is with a pure delay. Converting that
     * to the frequency domain, adjusting the phase of each frequency bin by
     * +90 degrees, then converting back to the time domain, results in a FIR
     * filter that applies a +90 degree wide-band phase-shift.
     *
     * A particularly notable aspect of the time-domain filter response is that
     * every other coefficient is 0. This allows doubling the effective size of
     * the filter, by storing only the non-0 coefficients and double-stepping
     * over the input to apply it.
     *
     * Additionally, the resulting filter is independent of the sample rate.
     * The same filter can be applied regardless of the device's sample rate
     * and achieve the same effect.
     */
    PhaseShifterT()
    {
        using complex_d = std::complex<double>;
        constexpr size_t fft_size{FilterSize};
        constexpr size_t half_size{fft_size / 2};

        auto fftBuffer = std::make_unique<complex_d[]>(fft_size);
        std::fill_n(fftBuffer.get(), fft_size, complex_d{});
        fftBuffer[half_size] = 1.0;

        forward_fft({fftBuffer.get(), fft_size});
        for(size_t i{0};i < half_size+1;++i)
            fftBuffer[i] = complex_d{-fftBuffer[i].imag(), fftBuffer[i].real()};
        for(size_t i{half_size+1};i < fft_size;++i)
            fftBuffer[i] = std::conj(fftBuffer[fft_size - i]);
        inverse_fft({fftBuffer.get(), fft_size});

        auto fftiter = fftBuffer.get() + half_size + (FilterSize/2 - 1);
        for(float &coeff : mCoeffs)
        {
            coeff = static_cast<float>(fftiter->real() / double{fft_size});
            fftiter -= 2;
        }
    }

    void process(al::span<float> dst, const float *RESTRICT src) const;
    void processAccum(al::span<float> dst, const float *RESTRICT src) const;

private:
#if defined(HAVE_NEON)
    /* There doesn't seem to be NEON intrinsics to do this kind of stipple
     * shuffling, so there's two custom methods for it.
     */
    static auto shuffle_2020(float32x4_t a, float32x4_t b)
    {
        float32x4_t ret{vmovq_n_f32(vgetq_lane_f32(a, 0))};
        ret = vsetq_lane_f32(vgetq_lane_f32(a, 2), ret, 1);
        ret = vsetq_lane_f32(vgetq_lane_f32(b, 0), ret, 2);
        ret = vsetq_lane_f32(vgetq_lane_f32(b, 2), ret, 3);
        return ret;
    }
    static auto shuffle_3131(float32x4_t a, float32x4_t b)
    {
        float32x4_t ret{vmovq_n_f32(vgetq_lane_f32(a, 1))};
        ret = vsetq_lane_f32(vgetq_lane_f32(a, 3), ret, 1);
        ret = vsetq_lane_f32(vgetq_lane_f32(b, 1), ret, 2);
        ret = vsetq_lane_f32(vgetq_lane_f32(b, 3), ret, 3);
        return ret;
    }
    static auto unpacklo(float32x4_t a, float32x4_t b)
    {
        float32x2x2_t result{vzip_f32(vget_low_f32(a), vget_low_f32(b))};
        return vcombine_f32(result.val[0], result.val[1]);
    }
    static auto unpackhi(float32x4_t a, float32x4_t b)
    {
        float32x2x2_t result{vzip_f32(vget_high_f32(a), vget_high_f32(b))};
        return vcombine_f32(result.val[0], result.val[1]);
    }
    static auto load4(float32_t a, float32_t b, float32_t c, float32_t d)
    {
        float32x4_t ret{vmovq_n_f32(a)};
        ret = vsetq_lane_f32(b, ret, 1);
        ret = vsetq_lane_f32(c, ret, 2);
        ret = vsetq_lane_f32(d, ret, 3);
        return ret;
    }
#endif
};

template<size_t S>
inline void PhaseShifterT<S>::process(al::span<float> dst, const float *RESTRICT src) const
{
#ifdef HAVE_SSE_INTRINSICS
    if(size_t todo{dst.size()>>1})
    {
        auto *out = reinterpret_cast<__m64*>(dst.data());
        do {
            __m128 r04{_mm_setzero_ps()};
            __m128 r14{_mm_setzero_ps()};
            for(size_t j{0};j < mCoeffs.size();j+=4)
            {
                const __m128 coeffs{_mm_load_ps(&mCoeffs[j])};
                const __m128 s0{_mm_loadu_ps(&src[j*2])};
                const __m128 s1{_mm_loadu_ps(&src[j*2 + 4])};

                __m128 s{_mm_shuffle_ps(s0, s1, _MM_SHUFFLE(2, 0, 2, 0))};
                r04 = _mm_add_ps(r04, _mm_mul_ps(s, coeffs));

                s = _mm_shuffle_ps(s0, s1, _MM_SHUFFLE(3, 1, 3, 1));
                r14 = _mm_add_ps(r14, _mm_mul_ps(s, coeffs));
            }
            src += 2;

            __m128 r4{_mm_add_ps(_mm_unpackhi_ps(r04, r14), _mm_unpacklo_ps(r04, r14))};
            r4 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));

            _mm_storel_pi(out, r4);
            ++out;
        } while(--todo);
    }
    if((dst.size()&1))
    {
        __m128 r4{_mm_setzero_ps()};
        for(size_t j{0};j < mCoeffs.size();j+=4)
        {
            const __m128 coeffs{_mm_load_ps(&mCoeffs[j])};
            const __m128 s{_mm_setr_ps(src[j*2], src[j*2 + 2], src[j*2 + 4], src[j*2 + 6])};
            r4 = _mm_add_ps(r4, _mm_mul_ps(s, coeffs));
        }
        r4 = _mm_add_ps(r4, _mm_shuffle_ps(r4, r4, _MM_SHUFFLE(0, 1, 2, 3)));
        r4 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));

        dst.back() = _mm_cvtss_f32(r4);
    }

#elif defined(HAVE_NEON)

    size_t pos{0};
    if(size_t todo{dst.size()>>1})
    {
        do {
            float32x4_t r04{vdupq_n_f32(0.0f)};
            float32x4_t r14{vdupq_n_f32(0.0f)};
            for(size_t j{0};j < mCoeffs.size();j+=4)
            {
                const float32x4_t coeffs{vld1q_f32(&mCoeffs[j])};
                const float32x4_t s0{vld1q_f32(&src[j*2])};
                const float32x4_t s1{vld1q_f32(&src[j*2 + 4])};

                r04 = vmlaq_f32(r04, shuffle_2020(s0, s1), coeffs);
                r14 = vmlaq_f32(r14, shuffle_3131(s0, s1), coeffs);
            }
            src += 2;

            float32x4_t r4{vaddq_f32(unpackhi(r04, r14), unpacklo(r04, r14))};
            float32x2_t r2{vadd_f32(vget_low_f32(r4), vget_high_f32(r4))};

            vst1_f32(&dst[pos], r2);
            pos += 2;
        } while(--todo);
    }
    if((dst.size()&1))
    {
        float32x4_t r4{vdupq_n_f32(0.0f)};
        for(size_t j{0};j < mCoeffs.size();j+=4)
        {
            const float32x4_t coeffs{vld1q_f32(&mCoeffs[j])};
            const float32x4_t s{load4(src[j*2], src[j*2 + 2], src[j*2 + 4], src[j*2 + 6])};
            r4 = vmlaq_f32(r4, s, coeffs);
        }
        r4 = vaddq_f32(r4, vrev64q_f32(r4));
        dst[pos] = vget_lane_f32(vadd_f32(vget_low_f32(r4), vget_high_f32(r4)), 0);
    }

#else

    for(float &output : dst)
    {
        float ret{0.0f};
        for(size_t j{0};j < mCoeffs.size();++j)
            ret += src[j*2] * mCoeffs[j];

        output = ret;
        ++src;
    }
#endif
}

template<size_t S>
inline void PhaseShifterT<S>::processAccum(al::span<float> dst, const float *RESTRICT src) const
{
#ifdef HAVE_SSE_INTRINSICS
    if(size_t todo{dst.size()>>1})
    {
        auto *out = reinterpret_cast<__m64*>(dst.data());
        do {
            __m128 r04{_mm_setzero_ps()};
            __m128 r14{_mm_setzero_ps()};
            for(size_t j{0};j < mCoeffs.size();j+=4)
            {
                const __m128 coeffs{_mm_load_ps(&mCoeffs[j])};
                const __m128 s0{_mm_loadu_ps(&src[j*2])};
                const __m128 s1{_mm_loadu_ps(&src[j*2 + 4])};

                __m128 s{_mm_shuffle_ps(s0, s1, _MM_SHUFFLE(2, 0, 2, 0))};
                r04 = _mm_add_ps(r04, _mm_mul_ps(s, coeffs));

                s = _mm_shuffle_ps(s0, s1, _MM_SHUFFLE(3, 1, 3, 1));
                r14 = _mm_add_ps(r14, _mm_mul_ps(s, coeffs));
            }
            src += 2;

            __m128 r4{_mm_add_ps(_mm_unpackhi_ps(r04, r14), _mm_unpacklo_ps(r04, r14))};
            r4 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));

            _mm_storel_pi(out, _mm_add_ps(_mm_loadl_pi(_mm_undefined_ps(), out), r4));
            ++out;
        } while(--todo);
    }
    if((dst.size()&1))
    {
        __m128 r4{_mm_setzero_ps()};
        for(size_t j{0};j < mCoeffs.size();j+=4)
        {
            const __m128 coeffs{_mm_load_ps(&mCoeffs[j])};
            const __m128 s{_mm_setr_ps(src[j*2], src[j*2 + 2], src[j*2 + 4], src[j*2 + 6])};
            r4 = _mm_add_ps(r4, _mm_mul_ps(s, coeffs));
        }
        r4 = _mm_add_ps(r4, _mm_shuffle_ps(r4, r4, _MM_SHUFFLE(0, 1, 2, 3)));
        r4 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));

        dst.back() += _mm_cvtss_f32(r4);
    }

#elif defined(HAVE_NEON)

    size_t pos{0};
    if(size_t todo{dst.size()>>1})
    {
        do {
            float32x4_t r04{vdupq_n_f32(0.0f)};
            float32x4_t r14{vdupq_n_f32(0.0f)};
            for(size_t j{0};j < mCoeffs.size();j+=4)
            {
                const float32x4_t coeffs{vld1q_f32(&mCoeffs[j])};
                const float32x4_t s0{vld1q_f32(&src[j*2])};
                const float32x4_t s1{vld1q_f32(&src[j*2 + 4])};

                r04 = vmlaq_f32(r04, shuffle_2020(s0, s1), coeffs);
                r14 = vmlaq_f32(r14, shuffle_3131(s0, s1), coeffs);
            }
            src += 2;

            float32x4_t r4{vaddq_f32(unpackhi(r04, r14), unpacklo(r04, r14))};
            float32x2_t r2{vadd_f32(vget_low_f32(r4), vget_high_f32(r4))};

            vst1_f32(&dst[pos], vadd_f32(vld1_f32(&dst[pos]), r2));
            pos += 2;
        } while(--todo);
    }
    if((dst.size()&1))
    {
        float32x4_t r4{vdupq_n_f32(0.0f)};
        for(size_t j{0};j < mCoeffs.size();j+=4)
        {
            const float32x4_t coeffs{vld1q_f32(&mCoeffs[j])};
            const float32x4_t s{load4(src[j*2], src[j*2 + 2], src[j*2 + 4], src[j*2 + 6])};
            r4 = vmlaq_f32(r4, s, coeffs);
        }
        r4 = vaddq_f32(r4, vrev64q_f32(r4));
        dst[pos] += vget_lane_f32(vadd_f32(vget_low_f32(r4), vget_high_f32(r4)), 0);
    }

#else

    for(float &output : dst)
    {
        float ret{0.0f};
        for(size_t j{0};j < mCoeffs.size();++j)
            ret += src[j*2] * mCoeffs[j];

        output += ret;
        ++src;
    }
#endif
}

#endif /* PHASE_SHIFTER_H */
