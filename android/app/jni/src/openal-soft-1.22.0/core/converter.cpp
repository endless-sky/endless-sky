
#include "config.h"

#include "converter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits.h>

#include "albit.h"
#include "albyte.h"
#include "alnumeric.h"
#include "fpu_ctrl.h"

struct CTag;
struct CopyTag;


namespace {

constexpr uint MaxPitch{10};

static_assert((BufferLineSize-1)/MaxPitch > 0, "MaxPitch is too large for BufferLineSize!");
static_assert((INT_MAX>>MixerFracBits)/MaxPitch > BufferLineSize,
    "MaxPitch and/or BufferLineSize are too large for MixerFracBits!");

/* Base template left undefined. Should be marked =delete, but Clang 3.8.1
 * chokes on that given the inline specializations.
 */
template<DevFmtType T>
inline float LoadSample(DevFmtType_t<T> val) noexcept;

template<> inline float LoadSample<DevFmtByte>(DevFmtType_t<DevFmtByte> val) noexcept
{ return val * (1.0f/128.0f); }
template<> inline float LoadSample<DevFmtShort>(DevFmtType_t<DevFmtShort> val) noexcept
{ return val * (1.0f/32768.0f); }
template<> inline float LoadSample<DevFmtInt>(DevFmtType_t<DevFmtInt> val) noexcept
{ return static_cast<float>(val) * (1.0f/2147483648.0f); }
template<> inline float LoadSample<DevFmtFloat>(DevFmtType_t<DevFmtFloat> val) noexcept
{ return val; }

template<> inline float LoadSample<DevFmtUByte>(DevFmtType_t<DevFmtUByte> val) noexcept
{ return LoadSample<DevFmtByte>(static_cast<int8_t>(val - 128)); }
template<> inline float LoadSample<DevFmtUShort>(DevFmtType_t<DevFmtUShort> val) noexcept
{ return LoadSample<DevFmtShort>(static_cast<int16_t>(val - 32768)); }
template<> inline float LoadSample<DevFmtUInt>(DevFmtType_t<DevFmtUInt> val) noexcept
{ return LoadSample<DevFmtInt>(static_cast<int32_t>(val - 2147483648u)); }


template<DevFmtType T>
inline void LoadSampleArray(float *RESTRICT dst, const void *src, const size_t srcstep,
    const size_t samples) noexcept
{
    const DevFmtType_t<T> *ssrc = static_cast<const DevFmtType_t<T>*>(src);
    for(size_t i{0u};i < samples;i++)
        dst[i] = LoadSample<T>(ssrc[i*srcstep]);
}

void LoadSamples(float *dst, const void *src, const size_t srcstep, const DevFmtType srctype,
    const size_t samples) noexcept
{
#define HANDLE_FMT(T)                                                         \
    case T: LoadSampleArray<T>(dst, src, srcstep, samples); break
    switch(srctype)
    {
        HANDLE_FMT(DevFmtByte);
        HANDLE_FMT(DevFmtUByte);
        HANDLE_FMT(DevFmtShort);
        HANDLE_FMT(DevFmtUShort);
        HANDLE_FMT(DevFmtInt);
        HANDLE_FMT(DevFmtUInt);
        HANDLE_FMT(DevFmtFloat);
    }
#undef HANDLE_FMT
}


template<DevFmtType T>
inline DevFmtType_t<T> StoreSample(float) noexcept;

template<> inline float StoreSample<DevFmtFloat>(float val) noexcept
{ return val; }
template<> inline int32_t StoreSample<DevFmtInt>(float val) noexcept
{ return fastf2i(clampf(val*2147483648.0f, -2147483648.0f, 2147483520.0f)); }
template<> inline int16_t StoreSample<DevFmtShort>(float val) noexcept
{ return static_cast<int16_t>(fastf2i(clampf(val*32768.0f, -32768.0f, 32767.0f))); }
template<> inline int8_t StoreSample<DevFmtByte>(float val) noexcept
{ return static_cast<int8_t>(fastf2i(clampf(val*128.0f, -128.0f, 127.0f))); }

/* Define unsigned output variations. */
template<> inline uint32_t StoreSample<DevFmtUInt>(float val) noexcept
{ return static_cast<uint32_t>(StoreSample<DevFmtInt>(val)) + 2147483648u; }
template<> inline uint16_t StoreSample<DevFmtUShort>(float val) noexcept
{ return static_cast<uint16_t>(StoreSample<DevFmtShort>(val) + 32768); }
template<> inline uint8_t StoreSample<DevFmtUByte>(float val) noexcept
{ return static_cast<uint8_t>(StoreSample<DevFmtByte>(val) + 128); }

template<DevFmtType T>
inline void StoreSampleArray(void *dst, const float *RESTRICT src, const size_t dststep,
    const size_t samples) noexcept
{
    DevFmtType_t<T> *sdst = static_cast<DevFmtType_t<T>*>(dst);
    for(size_t i{0u};i < samples;i++)
        sdst[i*dststep] = StoreSample<T>(src[i]);
}


void StoreSamples(void *dst, const float *src, const size_t dststep, const DevFmtType dsttype,
    const size_t samples) noexcept
{
#define HANDLE_FMT(T)                                                         \
    case T: StoreSampleArray<T>(dst, src, dststep, samples); break
    switch(dsttype)
    {
        HANDLE_FMT(DevFmtByte);
        HANDLE_FMT(DevFmtUByte);
        HANDLE_FMT(DevFmtShort);
        HANDLE_FMT(DevFmtUShort);
        HANDLE_FMT(DevFmtInt);
        HANDLE_FMT(DevFmtUInt);
        HANDLE_FMT(DevFmtFloat);
    }
#undef HANDLE_FMT
}


template<DevFmtType T>
void Mono2Stereo(float *RESTRICT dst, const void *src, const size_t frames) noexcept
{
    const DevFmtType_t<T> *ssrc = static_cast<const DevFmtType_t<T>*>(src);
    for(size_t i{0u};i < frames;i++)
        dst[i*2 + 1] = dst[i*2 + 0] = LoadSample<T>(ssrc[i]) * 0.707106781187f;
}

template<DevFmtType T>
void Multi2Mono(uint chanmask, const size_t step, const float scale, float *RESTRICT dst,
    const void *src, const size_t frames) noexcept
{
    const DevFmtType_t<T> *ssrc = static_cast<const DevFmtType_t<T>*>(src);
    std::fill_n(dst, frames, 0.0f);
    for(size_t c{0};chanmask;++c)
    {
        if LIKELY((chanmask&1))
        {
            for(size_t i{0u};i < frames;i++)
                dst[i] += LoadSample<T>(ssrc[i*step + c]);
        }
        chanmask >>= 1;
    }
    for(size_t i{0u};i < frames;i++)
        dst[i] *= scale;
}

} // namespace

SampleConverterPtr CreateSampleConverter(DevFmtType srcType, DevFmtType dstType, size_t numchans,
    uint srcRate, uint dstRate, Resampler resampler)
{
    if(numchans < 1 || srcRate < 1 || dstRate < 1)
        return nullptr;

    SampleConverterPtr converter{new(FamCount(numchans)) SampleConverter{numchans}};
    converter->mSrcType = srcType;
    converter->mDstType = dstType;
    converter->mSrcTypeSize = BytesFromDevFmt(srcType);
    converter->mDstTypeSize = BytesFromDevFmt(dstType);

    converter->mSrcPrepCount = 0;
    converter->mFracOffset = 0;

    /* Have to set the mixer FPU mode since that's what the resampler code expects. */
    FPUCtl mixer_mode{};
    auto step = static_cast<uint>(
        mind(srcRate*double{MixerFracOne}/dstRate + 0.5, MaxPitch*MixerFracOne));
    converter->mIncrement = maxu(step, 1);
    if(converter->mIncrement == MixerFracOne)
        converter->mResample = Resample_<CopyTag,CTag>;
    else
        converter->mResample = PrepareResampler(resampler, converter->mIncrement,
            &converter->mState);

    return converter;
}

uint SampleConverter::availableOut(uint srcframes) const
{
    int prepcount{mSrcPrepCount};
    if(prepcount < 0)
    {
        /* Negative prepcount means we need to skip that many input samples. */
        if(static_cast<uint>(-prepcount) >= srcframes)
            return 0;
        srcframes -= static_cast<uint>(-prepcount);
        prepcount = 0;
    }

    if(srcframes < 1)
    {
        /* No output samples if there's no input samples. */
        return 0;
    }

    if(prepcount < MaxResamplerPadding
        && static_cast<uint>(MaxResamplerPadding - prepcount) >= srcframes)
    {
        /* Not enough input samples to generate an output sample. */
        return 0;
    }

    auto DataSize64 = static_cast<uint64_t>(prepcount);
    DataSize64 += srcframes;
    DataSize64 -= MaxResamplerPadding;
    DataSize64 <<= MixerFracBits;
    DataSize64 -= mFracOffset;

    /* If we have a full prep, we can generate at least one sample. */
    return static_cast<uint>(clampu64((DataSize64 + mIncrement-1)/mIncrement, 1,
        std::numeric_limits<int>::max()));
}

uint SampleConverter::convert(const void **src, uint *srcframes, void *dst, uint dstframes)
{
    const uint SrcFrameSize{static_cast<uint>(mChan.size()) * mSrcTypeSize};
    const uint DstFrameSize{static_cast<uint>(mChan.size()) * mDstTypeSize};
    const uint increment{mIncrement};
    auto SamplesIn = static_cast<const al::byte*>(*src);
    uint NumSrcSamples{*srcframes};

    FPUCtl mixer_mode{};
    uint pos{0};
    while(pos < dstframes && NumSrcSamples > 0)
    {
        int prepcount{mSrcPrepCount};
        if(prepcount < 0)
        {
            /* Negative prepcount means we need to skip that many input samples. */
            if(static_cast<uint>(-prepcount) >= NumSrcSamples)
            {
                mSrcPrepCount = static_cast<int>(NumSrcSamples) + prepcount;
                NumSrcSamples = 0;
                break;
            }
            SamplesIn += SrcFrameSize*static_cast<uint>(-prepcount);
            NumSrcSamples -= static_cast<uint>(-prepcount);
            mSrcPrepCount = 0;
            continue;
        }
        const uint toread{minu(NumSrcSamples, BufferLineSize - MaxResamplerPadding)};

        if(prepcount < MaxResamplerPadding
            && static_cast<uint>(MaxResamplerPadding - prepcount) >= toread)
        {
            /* Not enough input samples to generate an output sample. Store
             * what we're given for later.
             */
            for(size_t chan{0u};chan < mChan.size();chan++)
                LoadSamples(&mChan[chan].PrevSamples[prepcount], SamplesIn + mSrcTypeSize*chan,
                    mChan.size(), mSrcType, toread);

            mSrcPrepCount = prepcount + static_cast<int>(toread);
            NumSrcSamples = 0;
            break;
        }

        float *RESTRICT SrcData{mSrcSamples};
        float *RESTRICT DstData{mDstSamples};
        uint DataPosFrac{mFracOffset};
        auto DataSize64 = static_cast<uint64_t>(prepcount);
        DataSize64 += toread;
        DataSize64 -= MaxResamplerPadding;
        DataSize64 <<= MixerFracBits;
        DataSize64 -= DataPosFrac;

        /* If we have a full prep, we can generate at least one sample. */
        auto DstSize = static_cast<uint>(
            clampu64((DataSize64 + increment-1)/increment, 1, BufferLineSize));
        DstSize = minu(DstSize, dstframes-pos);

        for(size_t chan{0u};chan < mChan.size();chan++)
        {
            const al::byte *SrcSamples{SamplesIn + mSrcTypeSize*chan};
            al::byte *DstSamples = static_cast<al::byte*>(dst) + mDstTypeSize*chan;

            /* Load the previous samples into the source data first, then the
             * new samples from the input buffer.
             */
            std::copy_n(mChan[chan].PrevSamples, prepcount, SrcData);
            LoadSamples(SrcData + prepcount, SrcSamples, mChan.size(), mSrcType, toread);

            /* Store as many prep samples for next time as possible, given the
             * number of output samples being generated.
             */
            uint SrcDataEnd{(DstSize*increment + DataPosFrac)>>MixerFracBits};
            if(SrcDataEnd >= static_cast<uint>(prepcount)+toread)
                std::fill(std::begin(mChan[chan].PrevSamples),
                    std::end(mChan[chan].PrevSamples), 0.0f);
            else
            {
                const size_t len{minz(al::size(mChan[chan].PrevSamples),
                    static_cast<uint>(prepcount)+toread-SrcDataEnd)};
                std::copy_n(SrcData+SrcDataEnd, len, mChan[chan].PrevSamples);
                std::fill(std::begin(mChan[chan].PrevSamples)+len,
                    std::end(mChan[chan].PrevSamples), 0.0f);
            }

            /* Now resample, and store the result in the output buffer. */
            const float *ResampledData{mResample(&mState, SrcData+(MaxResamplerPadding>>1),
                DataPosFrac, increment, {DstData, DstSize})};

            StoreSamples(DstSamples, ResampledData, mChan.size(), mDstType, DstSize);
        }

        /* Update the number of prep samples still available, as well as the
         * fractional offset.
         */
        DataPosFrac += increment*DstSize;
        mSrcPrepCount = mini(prepcount + static_cast<int>(toread - (DataPosFrac>>MixerFracBits)),
            MaxResamplerPadding);
        mFracOffset = DataPosFrac & MixerFracMask;

        /* Update the src and dst pointers in case there's still more to do. */
        SamplesIn += SrcFrameSize*(DataPosFrac>>MixerFracBits);
        NumSrcSamples -= minu(NumSrcSamples, (DataPosFrac>>MixerFracBits));

        dst = static_cast<al::byte*>(dst) + DstFrameSize*DstSize;
        pos += DstSize;
    }

    *src = SamplesIn;
    *srcframes = NumSrcSamples;

    return pos;
}


void ChannelConverter::convert(const void *src, float *dst, uint frames) const
{
    if(mDstChans == DevFmtMono)
    {
        const float scale{std::sqrt(1.0f / static_cast<float>(al::popcount(mChanMask)))};
        switch(mSrcType)
        {
#define HANDLE_FMT(T) case T: Multi2Mono<T>(mChanMask, mSrcStep, scale, dst, src, frames); break
        HANDLE_FMT(DevFmtByte);
        HANDLE_FMT(DevFmtUByte);
        HANDLE_FMT(DevFmtShort);
        HANDLE_FMT(DevFmtUShort);
        HANDLE_FMT(DevFmtInt);
        HANDLE_FMT(DevFmtUInt);
        HANDLE_FMT(DevFmtFloat);
#undef HANDLE_FMT
        }
    }
    else if(mChanMask == 0x1 && mDstChans == DevFmtStereo)
    {
        switch(mSrcType)
        {
#define HANDLE_FMT(T) case T: Mono2Stereo<T>(dst, src, frames); break
        HANDLE_FMT(DevFmtByte);
        HANDLE_FMT(DevFmtUByte);
        HANDLE_FMT(DevFmtShort);
        HANDLE_FMT(DevFmtUShort);
        HANDLE_FMT(DevFmtInt);
        HANDLE_FMT(DevFmtUInt);
        HANDLE_FMT(DevFmtFloat);
#undef HANDLE_FMT
        }
    }
}
