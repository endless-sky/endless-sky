#ifndef OPTHELPERS_H
#define OPTHELPERS_H

#include <cstdint>
#include <utility>


#ifdef __has_builtin
#define HAS_BUILTIN __has_builtin
#else
#define HAS_BUILTIN(x) (0)
#endif

#ifdef __GNUC__
#define force_inline [[gnu::always_inline]]
#elif defined(_MSC_VER)
#define force_inline __forceinline
#else
#define force_inline inline
#endif

#if defined(__GNUC__) || HAS_BUILTIN(__builtin_expect)
/* likely() optimizes for the case where the condition is true. The condition
 * is not required to be true, but it can result in more optimal code for the
 * true path at the expense of a less optimal false path.
 */
template<typename T>
force_inline constexpr bool likely(T&& expr) noexcept
{ return __builtin_expect(static_cast<bool>(std::forward<T>(expr)), true); }
/* The opposite of likely(), optimizing for the case where the condition is
 * false.
 */
template<typename T>
force_inline constexpr bool unlikely(T&& expr) noexcept
{ return __builtin_expect(static_cast<bool>(std::forward<T>(expr)), false); }

#else

template<typename T>
force_inline constexpr bool likely(T&& expr) noexcept
{ return static_cast<bool>(std::forward<T>(expr)); }
template<typename T>
force_inline constexpr bool unlikely(T&& expr) noexcept
{ return static_cast<bool>(std::forward<T>(expr)); }
#endif
#define LIKELY(x) (likely(x))
#define UNLIKELY(x) (unlikely(x))

#if HAS_BUILTIN(__builtin_assume)
/* Unlike LIKELY, ASSUME requires the condition to be true or else it invokes
 * undefined behavior. It's essentially an assert without actually checking the
 * condition at run-time, allowing for stronger optimizations than LIKELY.
 */
#define ASSUME __builtin_assume
#elif defined(_MSC_VER)
#define ASSUME __assume
#elif defined(__GNUC__)
#define ASSUME(x) do { if(x) break; __builtin_unreachable(); } while(0)
#else
#define ASSUME(x) ((void)0)
#endif

namespace al {

template<std::size_t alignment, typename T>
force_inline constexpr auto assume_aligned(T *ptr) noexcept
{
#ifdef __cpp_lib_assume_aligned
    return std::assume_aligned<alignment,T>(ptr);
#elif defined(__clang__) || (defined(__GNUC__) && !defined(__ICC))
    return static_cast<T*>(__builtin_assume_aligned(ptr, alignment));
#elif defined(_MSC_VER)
    constexpr std::size_t alignment_mask{(1<<alignment) - 1};
    if((reinterpret_cast<std::uintptr_t>(ptr)&alignment_mask) == 0)
        return ptr;
    __assume(0);
#elif defined(__ICC)
    __assume_aligned(ptr, alignment);
    return ptr;
#else
    return ptr;
#endif
}

} // namespace al

#endif /* OPTHELPERS_H */
