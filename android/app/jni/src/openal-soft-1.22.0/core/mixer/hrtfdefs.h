#ifndef CORE_MIXER_HRTFDEFS_H
#define CORE_MIXER_HRTFDEFS_H

#include <array>

#include "alspan.h"
#include "core/ambidefs.h"
#include "core/bufferline.h"
#include "core/filters/splitter.h"


using float2 = std::array<float,2>;
using ubyte = unsigned char;
using ubyte2 = std::array<ubyte,2>;
using ushort = unsigned short;
using uint = unsigned int;
using uint2 = std::array<uint,2>;

constexpr uint HrtfHistoryBits{6};
constexpr uint HrtfHistoryLength{1 << HrtfHistoryBits};
constexpr uint HrtfHistoryMask{HrtfHistoryLength - 1};

constexpr uint HrirBits{7};
constexpr uint HrirLength{1 << HrirBits};
constexpr uint HrirMask{HrirLength - 1};

constexpr uint MinIrLength{8};

using HrirArray = std::array<float2,HrirLength>;
using HrirSpan = al::span<float2,HrirLength>;
using ConstHrirSpan = al::span<const float2,HrirLength>;

struct MixHrtfFilter {
    const ConstHrirSpan Coeffs;
    uint2 Delay;
    float Gain;
    float GainStep;
};

struct HrtfFilter {
    alignas(16) HrirArray Coeffs;
    uint2 Delay;
    float Gain;
};


struct HrtfChannelState {
    BandSplitter mSplitter;
    float mHfScale{};
    alignas(16) HrirArray mCoeffs{};
};

#endif /* CORE_MIXER_HRTFDEFS_H */
