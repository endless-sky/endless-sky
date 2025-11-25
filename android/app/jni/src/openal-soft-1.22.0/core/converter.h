#ifndef CORE_CONVERTER_H
#define CORE_CONVERTER_H

#include <cstddef>
#include <memory>

#include "almalloc.h"
#include "devformat.h"
#include "mixer/defs.h"

using uint = unsigned int;


struct SampleConverter {
    DevFmtType mSrcType{};
    DevFmtType mDstType{};
    uint mSrcTypeSize{};
    uint mDstTypeSize{};

    int mSrcPrepCount{};

    uint mFracOffset{};
    uint mIncrement{};
    InterpState mState{};
    ResamplerFunc mResample{};

    alignas(16) float mSrcSamples[BufferLineSize]{};
    alignas(16) float mDstSamples[BufferLineSize]{};

    struct ChanSamples {
        alignas(16) float PrevSamples[MaxResamplerPadding];
    };
    al::FlexArray<ChanSamples> mChan;

    SampleConverter(size_t numchans) : mChan{numchans} { }

    uint convert(const void **src, uint *srcframes, void *dst, uint dstframes);
    uint availableOut(uint srcframes) const;

    DEF_FAM_NEWDEL(SampleConverter, mChan)
};
using SampleConverterPtr = std::unique_ptr<SampleConverter>;

SampleConverterPtr CreateSampleConverter(DevFmtType srcType, DevFmtType dstType, size_t numchans,
    uint srcRate, uint dstRate, Resampler resampler);


struct ChannelConverter {
    DevFmtType mSrcType{};
    uint mSrcStep{};
    uint mChanMask{};
    DevFmtChannels mDstChans{};

    bool is_active() const noexcept { return mChanMask != 0; }

    void convert(const void *src, float *dst, uint frames) const;
};

#endif /* CORE_CONVERTER_H */
