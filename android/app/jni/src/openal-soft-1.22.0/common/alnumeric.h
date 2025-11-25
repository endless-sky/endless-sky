#ifndef AL_NUMERIC_H
#define AL_NUMERIC_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#ifdef HAVE_INTRIN_H
#include <intrin.h>
#endif
#ifdef HAVE_SSE_INTRINSICS
#include <xmmintrin.h>
#endif

#include "opthelpers.h"


inline constexpr int64_t operator "" _i64(unsigned long long int n) noexcept { return static_cast<int64_t>(n); }
inline constexpr uint64_t operator "" _u64(unsigned long long int n) noexcept { return static_cast<uint64_t>(n); }


constexpr inline float minf(float a, float b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline float maxf(float a, float b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline float clampf(float val, float min, float max) noexcept
{ return minf(max, maxf(min, val)); }

constexpr inline double mind(double a, double b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline double maxd(double a, double b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline double clampd(double val, double min, double max) noexcept
{ return mind(max, maxd(min, val)); }

constexpr inline unsigned int minu(unsigned int a, unsigned int b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline unsigned int maxu(unsigned int a, unsigned int b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline unsigned int clampu(unsigned int val, unsigned int min, unsigned int max) noexcept
{ return minu(max, maxu(min, val)); }

constexpr inline int mini(int a, int b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline int maxi(int a, int b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline int clampi(int val, int min, int max) noexcept
{ return mini(max, maxi(min, val)); }

constexpr inline int64_t mini64(int64_t a, int64_t b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline int64_t maxi64(int64_t a, int64_t b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline int64_t clampi64(int64_t val, int64_t min, int64_t max) noexcept
{ return mini64(max, maxi64(min, val)); }

constexpr inline uint64_t minu64(uint64_t a, uint64_t b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline uint64_t maxu64(uint64_t a, uint64_t b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline uint64_t clampu64(uint64_t val, uint64_t min, uint64_t max) noexcept
{ return minu64(max, maxu64(min, val)); }

constexpr inline size_t minz(size_t a, size_t b) noexcept
{ return ((a > b) ? b : a); }
constexpr inline size_t maxz(size_t a, size_t b) noexcept
{ return ((a > b) ? a : b); }
constexpr inline size_t clampz(size_t val, size_t min, size_t max) noexcept
{ return minz(max, maxz(min, val)); }


constexpr inline float lerpf(float val1, float val2, float mu) noexcept
{ return val1 + (val2-val1)*mu; }
constexpr inline float cubic(float val1, float val2, float val3, float val4, float mu) noexcept
{
    const float mu2{mu*mu}, mu3{mu2*mu};
    const float a0{-0.5f*mu3 +       mu2 + -0.5f*mu};
    const float a1{ 1.5f*mu3 + -2.5f*mu2            + 1.0f};
    const float a2{-1.5f*mu3 +  2.0f*mu2 +  0.5f*mu};
    const float a3{ 0.5f*mu3 + -0.5f*mu2};
    return val1*a0 + val2*a1 + val3*a2 + val4*a3;
}


/** Find the next power-of-2 for non-power-of-2 numbers. */
inline uint32_t NextPowerOf2(uint32_t value) noexcept
{
    if(value > 0)
    {
        value--;
        value |= value>>1;
        value |= value>>2;
        value |= value>>4;
        value |= value>>8;
        value |= value>>16;
    }
    return value+1;
}

/** Round up a value to the next multiple. */
inline size_t RoundUp(size_t value, size_t r) noexcept
{
    value += r-1;
    return value - (value%r);
}


/**
 * Fast float-to-int conversion. No particular rounding mode is assumed; the
 * IEEE-754 default is round-to-nearest with ties-to-even, though an app could
 * change it on its own threads. On some systems, a truncating conversion may
 * always be the fastest method.
 */
inline int fastf2i(float f) noexcept
{
#if defined(HAVE_SSE_INTRINSICS)
    return _mm_cvt_ss2si(_mm_set_ss(f));

#elif defined(_MSC_VER) && defined(_M_IX86_FP)

    int i;
    __asm fld f
    __asm fistp i
    return i;

#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))

    int i;
#ifdef __SSE_MATH__
    __asm__("cvtss2si %1, %0" : "=r"(i) : "x"(f));
#else
    __asm__ __volatile__("fistpl %0" : "=m"(i) : "t"(f) : "st");
#endif
    return i;

#else

    return static_cast<int>(f);
#endif
}
inline unsigned int fastf2u(float f) noexcept
{ return static_cast<unsigned int>(fastf2i(f)); }

/** Converts float-to-int using standard behavior (truncation). */
inline int float2int(float f) noexcept
{
#if defined(HAVE_SSE_INTRINSICS)
    return _mm_cvtt_ss2si(_mm_set_ss(f));

#elif (defined(_MSC_VER) && defined(_M_IX86_FP) && _M_IX86_FP == 0) \
    || ((defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__)) \
        && !defined(__SSE_MATH__))
    int sign, shift, mant;
    union {
        float f;
        int i;
    } conv;

    conv.f = f;
    sign = (conv.i>>31) | 1;
    shift = ((conv.i>>23)&0xff) - (127+23);

    /* Over/underflow */
    if UNLIKELY(shift >= 31 || shift < -23)
        return 0;

    mant = (conv.i&0x7fffff) | 0x800000;
    if LIKELY(shift < 0)
        return (mant >> -shift) * sign;
    return (mant << shift) * sign;

#else

    return static_cast<int>(f);
#endif
}
inline unsigned int float2uint(float f) noexcept
{ return static_cast<unsigned int>(float2int(f)); }

/** Converts double-to-int using standard behavior (truncation). */
inline int double2int(double d) noexcept
{
#if defined(HAVE_SSE_INTRINSICS)
    return _mm_cvttsd_si32(_mm_set_sd(d));

#elif (defined(_MSC_VER) && defined(_M_IX86_FP) && _M_IX86_FP < 2) \
    || ((defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__)) \
        && !defined(__SSE2_MATH__))
    int sign, shift;
    int64_t mant;
    union {
        double d;
        int64_t i64;
    } conv;

    conv.d = d;
    sign = (conv.i64 >> 63) | 1;
    shift = ((conv.i64 >> 52) & 0x7ff) - (1023 + 52);

    /* Over/underflow */
    if UNLIKELY(shift >= 63 || shift < -52)
        return 0;

    mant = (conv.i64 & 0xfffffffffffff_i64) | 0x10000000000000_i64;
    if LIKELY(shift < 0)
        return (int)(mant >> -shift) * sign;
    return (int)(mant << shift) * sign;

#else

    return static_cast<int>(d);
#endif
}

/**
 * Rounds a float to the nearest integral value, according to the current
 * rounding mode. This is essentially an inlined version of rintf, although
 * makes fewer promises (e.g. -0 or -0.25 rounded to 0 may result in +0).
 */
inline float fast_roundf(float f) noexcept
{
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__)) \
    && !defined(__SSE_MATH__)

    float out;
    __asm__ __volatile__("frndint" : "=t"(out) : "0"(f));
    return out;

#elif (defined(__GNUC__) || defined(__clang__)) && defined(__aarch64__)

    float out;
    __asm__ volatile("frintx %s0, %s1" : "=w"(out) : "w"(f));
    return out;

#else

    /* Integral limit, where sub-integral precision is not available for
     * floats.
     */
    static const float ilim[2]{
         8388608.0f /*  0x1.0p+23 */,
        -8388608.0f /* -0x1.0p+23 */
    };
    unsigned int sign, expo;
    union {
        float f;
        unsigned int i;
    } conv;

    conv.f = f;
    sign = (conv.i>>31)&0x01;
    expo = (conv.i>>23)&0xff;

    if UNLIKELY(expo >= 150/*+23*/)
    {
        /* An exponent (base-2) of 23 or higher is incapable of sub-integral
         * precision, so it's already an integral value. We don't need to worry
         * about infinity or NaN here.
         */
        return f;
    }
    /* Adding the integral limit to the value (with a matching sign) forces a
     * result that has no sub-integral precision, and is consequently forced to
     * round to an integral value. Removing the integral limit then restores
     * the initial value rounded to the integral. The compiler should not
     * optimize this out because of non-associative rules on floating-point
     * math (as long as you don't use -fassociative-math,
     * -funsafe-math-optimizations, -ffast-math, or -Ofast, in which case this
     * may break).
     */
    f += ilim[sign];
    return f - ilim[sign];
#endif
}


template<typename T>
constexpr const T& clamp(const T& value, const T& min_value, const T& max_value) noexcept
{
    return std::min(std::max(value, min_value), max_value);
}

// Converts level (mB) to gain.
inline float level_mb_to_gain(float x)
{
    if(x <= -10'000.0f)
        return 0.0f;
    return std::pow(10.0f, x / 2'000.0f);
}

// Converts gain to level (mB).
inline float gain_to_level_mb(float x)
{
    if (x <= 0.0f)
        return -10'000.0f;
    return maxf(std::log10(x) * 2'000.0f, -10'000.0f);
}

#endif /* AL_NUMERIC_H */
