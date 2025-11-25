
#include "config.h"

#include "ambidefs.h"

#include <cassert>


namespace {

constexpr std::array<float,MaxAmbiOrder+1> Ambi3DDecoderHFScale{{
    1.00000000e+00f, 1.00000000e+00f
}};
constexpr std::array<float,MaxAmbiOrder+1> Ambi3DDecoderHFScale2O{{
    7.45355990e-01f, 1.00000000e+00f, 1.00000000e+00f
}};
constexpr std::array<float,MaxAmbiOrder+1> Ambi3DDecoderHFScale3O{{
    5.89792205e-01f, 8.79693856e-01f, 1.00000000e+00f, 1.00000000e+00f
}};

inline auto& GetDecoderHFScales(uint order) noexcept
{
    if(order >= 3) return Ambi3DDecoderHFScale3O;
    if(order == 2) return Ambi3DDecoderHFScale2O;
    return Ambi3DDecoderHFScale;
}

} // namespace

auto AmbiScale::GetHFOrderScales(const uint in_order, const uint out_order) noexcept
    -> std::array<float,MaxAmbiOrder+1>
{
    std::array<float,MaxAmbiOrder+1> ret{};

    assert(out_order >= in_order);

    const auto &target = GetDecoderHFScales(out_order);
    const auto &input = GetDecoderHFScales(in_order);

    for(size_t i{0};i < in_order+1;++i)
        ret[i] = input[i] / target[i];

    return ret;
}
