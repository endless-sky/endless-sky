#ifndef CORE_FRONT_STABLIZER_H
#define CORE_FRONT_STABLIZER_H

#include <array>
#include <memory>

#include "almalloc.h"
#include "bufferline.h"
#include "filters/splitter.h"


struct FrontStablizer {
    static constexpr size_t DelayLength{256u};

    FrontStablizer(size_t numchans) : DelayBuf{numchans} { }

    alignas(16) std::array<float,BufferLineSize + DelayLength> Side{};
    alignas(16) std::array<float,BufferLineSize + DelayLength> MidDirect{};
    alignas(16) std::array<float,DelayLength> MidDelay{};

    alignas(16) std::array<float,BufferLineSize + DelayLength> TempBuf{};

    BandSplitter MidFilter;
    alignas(16) FloatBufferLine MidLF{};
    alignas(16) FloatBufferLine MidHF{};

    using DelayLine = std::array<float,DelayLength>;
    al::FlexArray<DelayLine,16> DelayBuf;

    static std::unique_ptr<FrontStablizer> Create(size_t numchans)
    { return std::unique_ptr<FrontStablizer>{new(FamCount(numchans)) FrontStablizer{numchans}}; }

    DEF_FAM_NEWDEL(FrontStablizer, DelayBuf)
};

#endif /* CORE_FRONT_STABLIZER_H */
