#ifndef COMMON_ALNUMBERS_H
#define COMMON_ALNUMBERS_H

#include <utility>

namespace al {

namespace numbers {

namespace detail_ {
    template<typename T>
    using as_fp = std::enable_if_t<std::is_floating_point<T>::value, T>;
} // detail_

template<typename T>
static constexpr auto pi_v = detail_::as_fp<T>(3.141592653589793238462643383279502884L);

template<typename T>
static constexpr auto inv_pi_v = detail_::as_fp<T>(0.318309886183790671537767526745028724L);

template<typename T>
static constexpr auto sqrt2_v = detail_::as_fp<T>(1.414213562373095048801688724209698079L);

template<typename T>
static constexpr auto sqrt3_v = detail_::as_fp<T>(1.732050807568877293527446341505872367L);

static constexpr auto pi = pi_v<double>;
static constexpr auto inv_pi = inv_pi_v<double>;
static constexpr auto sqrt2 = sqrt2_v<double>;
static constexpr auto sqrt3 = sqrt3_v<double>;

} // namespace numbers

} // namespace al

#endif /* COMMON_ALNUMBERS_H */
