#ifndef CORE_FMT_TRAITS_H
#define CORE_FMT_TRAITS_H

#include <stddef.h>
#include <stdint.h>

#include "albyte.h"
#include "buffer_storage.h"


namespace al {

extern const int16_t muLawDecompressionTable[256];
extern const int16_t aLawDecompressionTable[256];


template<FmtType T>
struct FmtTypeTraits { };

template<>
struct FmtTypeTraits<FmtUByte> {
    using Type = uint8_t;

    template<typename OutT>
    static constexpr inline OutT to(const Type val) noexcept
    { return val*OutT{1.0/128.0} - OutT{1.0}; }
};
template<>
struct FmtTypeTraits<FmtShort> {
    using Type = int16_t;

    template<typename OutT>
    static constexpr inline OutT to(const Type val) noexcept { return val*OutT{1.0/32768.0}; }
};
template<>
struct FmtTypeTraits<FmtFloat> {
    using Type = float;

    template<typename OutT>
    static constexpr inline OutT to(const Type val) noexcept { return val; }
};
template<>
struct FmtTypeTraits<FmtDouble> {
    using Type = double;

    template<typename OutT>
    static constexpr inline OutT to(const Type val) noexcept { return static_cast<OutT>(val); }
};
template<>
struct FmtTypeTraits<FmtMulaw> {
    using Type = uint8_t;

    template<typename OutT>
    static constexpr inline OutT to(const Type val) noexcept
    { return muLawDecompressionTable[val] * OutT{1.0/32768.0}; }
};
template<>
struct FmtTypeTraits<FmtAlaw> {
    using Type = uint8_t;

    template<typename OutT>
    static constexpr inline OutT to(const Type val) noexcept
    { return aLawDecompressionTable[val] * OutT{1.0/32768.0}; }
};


template<FmtType SrcType, typename DstT>
inline void LoadSampleArray(DstT *RESTRICT dst, const al::byte *src, const size_t srcstep,
    const size_t samples) noexcept
{
    using TypeTraits = FmtTypeTraits<SrcType>;
    using SampleType = typename TypeTraits::Type;

    const SampleType *RESTRICT ssrc{reinterpret_cast<const SampleType*>(src)};
    for(size_t i{0u};i < samples;i++)
        dst[i] = TypeTraits::template to<DstT>(ssrc[i*srcstep]);
}

} // namespace al

#endif /* CORE_FMT_TRAITS_H */
