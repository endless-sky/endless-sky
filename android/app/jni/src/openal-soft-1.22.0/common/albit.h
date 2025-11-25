#ifndef AL_BIT_H
#define AL_BIT_H

#include <cstdint>
#include <limits>
#include <type_traits>
#if !defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64))
#include <intrin.h>
#endif

namespace al {

#ifdef __BYTE_ORDER__
enum class endian {
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};

#else

/* This doesn't support mixed-endian. */
namespace detail_ {
constexpr bool IsLittleEndian() noexcept
{
    static_assert(sizeof(char) < sizeof(int), "char is too big");

    constexpr int test_val{1};
    return static_cast<const char&>(test_val) ? true : false;
}
} // namespace detail_

enum class endian {
    big = 0,
    little = 1,
    native = detail_::IsLittleEndian() ? little : big
};
#endif


/* Define popcount (population count/count 1 bits) and countr_zero (count
 * trailing zero bits, starting from the lsb) methods, for various integer
 * types.
 */
#ifdef __GNUC__

namespace detail_ {
    inline int popcount(unsigned long long val) noexcept { return __builtin_popcountll(val); }
    inline int popcount(unsigned long val) noexcept { return __builtin_popcountl(val); }
    inline int popcount(unsigned int val) noexcept { return __builtin_popcount(val); }

    inline int countr_zero(unsigned long long val) noexcept { return __builtin_ctzll(val); }
    inline int countr_zero(unsigned long val) noexcept { return __builtin_ctzl(val); }
    inline int countr_zero(unsigned int val) noexcept { return __builtin_ctz(val); }
} // namespace detail_

template<typename T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value,
int> popcount(T v) noexcept { return detail_::popcount(v); }

template<typename T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value,
int> countr_zero(T val) noexcept
{ return val ? detail_::countr_zero(val) : std::numeric_limits<T>::digits; }

#else

/* There be black magics here. The popcount method is derived from
 * https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
 * while the ctz-utilizing-popcount algorithm is shown here
 * http://www.hackersdelight.org/hdcodetxt/ntz.c.txt
 * as the ntz2 variant. These likely aren't the most efficient methods, but
 * they're good enough if the GCC built-ins aren't available.
 */
namespace detail_ {
    template<typename T, size_t = std::numeric_limits<T>::digits>
    struct fast_utype { };
    template<typename T>
    struct fast_utype<T,8> { using type = std::uint_fast8_t; };
    template<typename T>
    struct fast_utype<T,16> { using type = std::uint_fast16_t; };
    template<typename T>
    struct fast_utype<T,32> { using type = std::uint_fast32_t; };
    template<typename T>
    struct fast_utype<T,64> { using type = std::uint_fast64_t; };

    template<typename T>
    constexpr T repbits(unsigned char bits) noexcept
    {
        T ret{bits};
        for(size_t i{1};i < sizeof(T);++i)
            ret = (ret<<8) | bits;
        return ret;
    }
} // namespace detail_

template<typename T>
constexpr std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value,
int> popcount(T val) noexcept
{
    using fast_type = typename detail_::fast_utype<T>::type;
    constexpr fast_type b01010101{detail_::repbits<fast_type>(0x55)};
    constexpr fast_type b00110011{detail_::repbits<fast_type>(0x33)};
    constexpr fast_type b00001111{detail_::repbits<fast_type>(0x0f)};
    constexpr fast_type b00000001{detail_::repbits<fast_type>(0x01)};

    fast_type v{fast_type{val} - ((fast_type{val} >> 1) & b01010101)};
    v = (v & b00110011) + ((v >> 2) & b00110011);
    v = (v + (v >> 4)) & b00001111;
    return static_cast<int>(((v * b00000001) >> ((sizeof(T)-1)*8)) & 0xff);
}

#ifdef _WIN32

template<typename T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
    && std::numeric_limits<T>::digits <= 32,
int> countr_zero(T v)
{
    unsigned long idx{std::numeric_limits<T>::digits};
    _BitScanForward(&idx, static_cast<uint32_t>(v));
    return static_cast<int>(idx);
}

template<typename T>
inline std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value
    && 32 < std::numeric_limits<T>::digits && std::numeric_limits<T>::digits <= 64,
int> countr_zero(T v)
{
    unsigned long idx{std::numeric_limits<T>::digits};
#ifdef _WIN64
    _BitScanForward64(&idx, v);
#else
    if(!_BitScanForward(&idx, static_cast<uint32_t>(v)))
    {
        if(_BitScanForward(&idx, static_cast<uint32_t>(v>>32)))
            idx += 32;
    }
#endif /* _WIN64 */
    return static_cast<int>(idx);
}

#else

template<typename T>
constexpr std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value,
int> countr_zero(T value)
{ return popcount(static_cast<T>(~value & (value - 1))); }

#endif
#endif

} // namespace al

#endif /* AL_BIT_H */
