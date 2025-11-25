
#include "config.h"

#include "voice.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <new>
#include <stdlib.h>
#include <utility>
#include <vector>

#include "albyte.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "alspan.h"
#include "alstring.h"
#include "ambidefs.h"
#include "async_event.h"
#include "buffer_storage.h"
#include "context.h"
#include "cpu_caps.h"
#include "devformat.h"
#include "device.h"
#include "filters/biquad.h"
#include "filters/nfc.h"
#include "filters/splitter.h"
#include "fmt_traits.h"
#include "logging.h"
#include "mixer.h"
#include "mixer/defs.h"
#include "mixer/hrtfdefs.h"
#include "opthelpers.h"
#include "resampler_limits.h"
#include "ringbuffer.h"
#include "vector.h"
#include "voice_change.h"

struct CTag;
#ifdef HAVE_SSE
struct SSETag;
#endif
#ifdef HAVE_NEON
struct NEONTag;
#endif
struct CopyTag;


static_assert(!(sizeof(DeviceBase::MixerBufferLine)&15),
    "DeviceBase::MixerBufferLine must be a multiple of 16 bytes");
static_assert(!(MaxResamplerEdge&3), "MaxResamplerEdge is not a multiple of 4");

Resampler ResamplerDefault{Resampler::Linear};

namespace {

using uint = unsigned int;

using HrtfMixerFunc = void(*)(const float *InSamples, float2 *AccumSamples, const uint IrSize,
    const MixHrtfFilter *hrtfparams, const size_t BufferSize);
using HrtfMixerBlendFunc = void(*)(const float *InSamples, float2 *AccumSamples,
    const uint IrSize, const HrtfFilter *oldparams, const MixHrtfFilter *newparams,
    const size_t BufferSize);

HrtfMixerFunc MixHrtfSamples{MixHrtf_<CTag>};
HrtfMixerBlendFunc MixHrtfBlendSamples{MixHrtfBlend_<CTag>};

inline MixerFunc SelectMixer()
{
#ifdef HAVE_NEON
    if((CPUCapFlags&CPU_CAP_NEON))
        return Mix_<NEONTag>;
#endif
#ifdef HAVE_SSE
    if((CPUCapFlags&CPU_CAP_SSE))
        return Mix_<SSETag>;
#endif
    return Mix_<CTag>;
}

inline HrtfMixerFunc SelectHrtfMixer()
{
#ifdef HAVE_NEON
    if((CPUCapFlags&CPU_CAP_NEON))
        return MixHrtf_<NEONTag>;
#endif
#ifdef HAVE_SSE
    if((CPUCapFlags&CPU_CAP_SSE))
        return MixHrtf_<SSETag>;
#endif
    return MixHrtf_<CTag>;
}

inline HrtfMixerBlendFunc SelectHrtfBlendMixer()
{
#ifdef HAVE_NEON
    if((CPUCapFlags&CPU_CAP_NEON))
        return MixHrtfBlend_<NEONTag>;
#endif
#ifdef HAVE_SSE
    if((CPUCapFlags&CPU_CAP_SSE))
        return MixHrtfBlend_<SSETag>;
#endif
    return MixHrtfBlend_<CTag>;
}

} // namespace

void Voice::InitMixer(al::optional<std::string> resampler)
{
    if(resampler)
    {
        struct ResamplerEntry {
            const char name[16];
            const Resampler resampler;
        };
        constexpr ResamplerEntry ResamplerList[]{
            { "none", Resampler::Point },
            { "point", Resampler::Point },
            { "linear", Resampler::Linear },
            { "cubic", Resampler::Cubic },
            { "bsinc12", Resampler::BSinc12 },
            { "fast_bsinc12", Resampler::FastBSinc12 },
            { "bsinc24", Resampler::BSinc24 },
            { "fast_bsinc24", Resampler::FastBSinc24 },
        };

        const char *str{resampler->c_str()};
        if(al::strcasecmp(str, "bsinc") == 0)
        {
            WARN("Resampler option \"%s\" is deprecated, using bsinc12\n", str);
            str = "bsinc12";
        }
        else if(al::strcasecmp(str, "sinc4") == 0 || al::strcasecmp(str, "sinc8") == 0)
        {
            WARN("Resampler option \"%s\" is deprecated, using cubic\n", str);
            str = "cubic";
        }

        auto iter = std::find_if(std::begin(ResamplerList), std::end(ResamplerList),
            [str](const ResamplerEntry &entry) -> bool
            { return al::strcasecmp(str, entry.name) == 0; });
        if(iter == std::end(ResamplerList))
            ERR("Invalid resampler: %s\n", str);
        else
            ResamplerDefault = iter->resampler;
    }

    MixSamples = SelectMixer();
    MixHrtfBlendSamples = SelectHrtfBlendMixer();
    MixHrtfSamples = SelectHrtfMixer();
}


namespace {

void SendSourceStoppedEvent(ContextBase *context, uint id)
{
    RingBuffer *ring{context->mAsyncEvents.get()};
    auto evt_vec = ring->getWriteVector();
    if(evt_vec.first.len < 1) return;

    AsyncEvent *evt{al::construct_at(reinterpret_cast<AsyncEvent*>(evt_vec.first.buf),
        AsyncEvent::SourceStateChange)};
    evt->u.srcstate.id = id;
    evt->u.srcstate.state = AsyncEvent::SrcState::Stop;

    ring->writeAdvance(1);
}


const float *DoFilters(BiquadFilter &lpfilter, BiquadFilter &hpfilter, float *dst,
    const al::span<const float> src, int type)
{
    switch(type)
    {
    case AF_None:
        lpfilter.clear();
        hpfilter.clear();
        break;

    case AF_LowPass:
        lpfilter.process(src, dst);
        hpfilter.clear();
        return dst;
    case AF_HighPass:
        lpfilter.clear();
        hpfilter.process(src, dst);
        return dst;

    case AF_BandPass:
        DualBiquad{lpfilter, hpfilter}.process(src, dst);
        return dst;
    }
    return src.data();
}


template<FmtType Type>
inline void LoadSamples(const al::span<float*> dstSamples, const size_t dstOffset,
    const al::byte *src, const size_t srcOffset, const FmtChannels srcChans, const size_t srcStep,
    const size_t samples) noexcept
{
    constexpr size_t sampleSize{sizeof(typename al::FmtTypeTraits<Type>::Type)};
    auto s = src + srcOffset*srcStep*sampleSize;
    if(srcChans == FmtUHJ2 || srcChans == FmtSuperStereo)
    {
        al::LoadSampleArray<Type>(dstSamples[0]+dstOffset, s, srcStep, samples);
        al::LoadSampleArray<Type>(dstSamples[1]+dstOffset, s+sampleSize, srcStep, samples);
        std::fill_n(dstSamples[2]+dstOffset, samples, 0.0f);
    }
    else
    {
        for(auto *dst : dstSamples)
        {
            al::LoadSampleArray<Type>(dst+dstOffset, s, srcStep, samples);
            s += sampleSize;
        }
    }
}

void LoadSamples(const al::span<float*> dstSamples, const size_t dstOffset, const al::byte *src,
    const size_t srcOffset, const FmtType srcType, const FmtChannels srcChans,
    const size_t srcStep, const size_t samples) noexcept
{
#define HANDLE_FMT(T) case T:                                                 \
    LoadSamples<T>(dstSamples, dstOffset, src, srcOffset, srcChans, srcStep,  \
        samples);                                                             \
    break

    switch(srcType)
    {
    HANDLE_FMT(FmtUByte);
    HANDLE_FMT(FmtShort);
    HANDLE_FMT(FmtFloat);
    HANDLE_FMT(FmtDouble);
    HANDLE_FMT(FmtMulaw);
    HANDLE_FMT(FmtAlaw);
    }
#undef HANDLE_FMT
}

void LoadBufferStatic(VoiceBufferItem *buffer, VoiceBufferItem *bufferLoopItem,
    const size_t dataPosInt, const FmtType sampleType, const FmtChannels sampleChannels,
    const size_t srcStep, const size_t samplesToLoad, const al::span<float*> voiceSamples)
{
    const uint loopStart{buffer->mLoopStart};
    const uint loopEnd{buffer->mLoopEnd};
    ASSUME(loopEnd > loopStart);

    /* If current pos is beyond the loop range, do not loop */
    if(!bufferLoopItem || dataPosInt >= loopEnd)
    {
        /* Load what's left to play from the buffer */
        const size_t remaining{minz(samplesToLoad, buffer->mSampleLen-dataPosInt)};
        LoadSamples(voiceSamples, 0, buffer->mSamples, dataPosInt, sampleType, sampleChannels,
            srcStep, remaining);

        if(const size_t toFill{samplesToLoad - remaining})
        {
            for(auto *chanbuffer : voiceSamples)
            {
                auto srcsamples = chanbuffer + remaining - 1;
                std::fill_n(srcsamples + 1, toFill, *srcsamples);
            }
        }
    }
    else
    {
        /* Load what's left of this loop iteration */
        const size_t remaining{minz(samplesToLoad, loopEnd-dataPosInt)};
        LoadSamples(voiceSamples, 0, buffer->mSamples, dataPosInt, sampleType, sampleChannels,
            srcStep, remaining);

        /* Load repeats of the loop to fill the buffer. */
        const auto loopSize = static_cast<size_t>(loopEnd - loopStart);
        size_t samplesLoaded{remaining};
        while(const size_t toFill{minz(samplesToLoad - samplesLoaded, loopSize)})
        {
            LoadSamples(voiceSamples, samplesLoaded, buffer->mSamples, loopStart, sampleType,
                sampleChannels, srcStep, toFill);
            samplesLoaded += toFill;
        }
    }
}

void LoadBufferCallback(VoiceBufferItem *buffer, const size_t numCallbackSamples,
    const FmtType sampleType, const FmtChannels sampleChannels, const size_t srcStep,
    const size_t samplesToLoad, const al::span<float*> voiceSamples)
{
    /* Load what's left to play from the buffer */
    const size_t remaining{minz(samplesToLoad, numCallbackSamples)};
    LoadSamples(voiceSamples, 0, buffer->mSamples, 0, sampleType, sampleChannels, srcStep,
        remaining);

    if(const size_t toFill{samplesToLoad - remaining})
    {
        for(auto *chanbuffer : voiceSamples)
        {
            auto srcsamples = chanbuffer + remaining - 1;
            std::fill_n(srcsamples + 1, toFill, *srcsamples);
        }
    }
}

void LoadBufferQueue(VoiceBufferItem *buffer, VoiceBufferItem *bufferLoopItem,
    size_t dataPosInt, const FmtType sampleType, const FmtChannels sampleChannels,
    const size_t srcStep, const size_t samplesToLoad, const al::span<float*> voiceSamples)
{
    /* Crawl the buffer queue to fill in the temp buffer */
    size_t samplesLoaded{0};
    while(buffer && samplesLoaded != samplesToLoad)
    {
        if(dataPosInt >= buffer->mSampleLen)
        {
            dataPosInt -= buffer->mSampleLen;
            buffer = buffer->mNext.load(std::memory_order_acquire);
            if(!buffer) buffer = bufferLoopItem;
            continue;
        }

        const size_t remaining{minz(samplesToLoad-samplesLoaded, buffer->mSampleLen-dataPosInt)};
        LoadSamples(voiceSamples, samplesLoaded, buffer->mSamples, dataPosInt, sampleType,
            sampleChannels, srcStep, remaining);

        samplesLoaded += remaining;
        if(samplesLoaded == samplesToLoad)
            break;

        dataPosInt = 0;
        buffer = buffer->mNext.load(std::memory_order_acquire);
        if(!buffer) buffer = bufferLoopItem;
    }
    if(const size_t toFill{samplesToLoad - samplesLoaded})
    {
        size_t chanidx{0};
        for(auto *chanbuffer : voiceSamples)
        {
            auto srcsamples = chanbuffer + samplesLoaded - 1;
            std::fill_n(srcsamples + 1, toFill, *srcsamples);
            ++chanidx;
        }
    }
}


void DoHrtfMix(const float *samples, const uint DstBufferSize, DirectParams &parms,
    const float TargetGain, const uint Counter, uint OutPos, const bool IsPlaying,
    DeviceBase *Device)
{
    const uint IrSize{Device->mIrSize};
    auto &HrtfSamples = Device->HrtfSourceData;
    auto &AccumSamples = Device->HrtfAccumData;

    /* Copy the HRTF history and new input samples into a temp buffer. */
    auto src_iter = std::copy(parms.Hrtf.History.begin(), parms.Hrtf.History.end(),
        std::begin(HrtfSamples));
    std::copy_n(samples, DstBufferSize, src_iter);
    /* Copy the last used samples back into the history buffer for later. */
    if(likely(IsPlaying))
        std::copy_n(std::begin(HrtfSamples) + DstBufferSize, parms.Hrtf.History.size(),
            parms.Hrtf.History.begin());

    /* If fading and this is the first mixing pass, fade between the IRs. */
    uint fademix{0u};
    if(Counter && OutPos == 0)
    {
        fademix = minu(DstBufferSize, Counter);

        float gain{TargetGain};

        /* The new coefficients need to fade in completely since they're
         * replacing the old ones. To keep the gain fading consistent,
         * interpolate between the old and new target gains given how much of
         * the fade time this mix handles.
         */
        if(Counter > fademix)
        {
            const float a{static_cast<float>(fademix) / static_cast<float>(Counter)};
            gain = lerpf(parms.Hrtf.Old.Gain, TargetGain, a);
        }

        MixHrtfFilter hrtfparams{
            parms.Hrtf.Target.Coeffs,
            parms.Hrtf.Target.Delay,
            0.0f, gain / static_cast<float>(fademix)};
        MixHrtfBlendSamples(HrtfSamples, AccumSamples+OutPos, IrSize, &parms.Hrtf.Old, &hrtfparams,
            fademix);

        /* Update the old parameters with the result. */
        parms.Hrtf.Old = parms.Hrtf.Target;
        parms.Hrtf.Old.Gain = gain;
        OutPos += fademix;
    }

    if(fademix < DstBufferSize)
    {
        const uint todo{DstBufferSize - fademix};
        float gain{TargetGain};

        /* Interpolate the target gain if the gain fading lasts longer than
         * this mix.
         */
        if(Counter > DstBufferSize)
        {
            const float a{static_cast<float>(todo) / static_cast<float>(Counter-fademix)};
            gain = lerpf(parms.Hrtf.Old.Gain, TargetGain, a);
        }

        MixHrtfFilter hrtfparams{
            parms.Hrtf.Target.Coeffs,
            parms.Hrtf.Target.Delay,
            parms.Hrtf.Old.Gain,
            (gain - parms.Hrtf.Old.Gain) / static_cast<float>(todo)};
        MixHrtfSamples(HrtfSamples+fademix, AccumSamples+OutPos, IrSize, &hrtfparams, todo);

        /* Store the now-current gain for next time. */
        parms.Hrtf.Old.Gain = gain;
    }
}

void DoNfcMix(const al::span<const float> samples, FloatBufferLine *OutBuffer, DirectParams &parms,
    const float *TargetGains, const uint Counter, const uint OutPos, DeviceBase *Device)
{
    using FilterProc = void (NfcFilter::*)(const al::span<const float>, float*);
    static constexpr FilterProc NfcProcess[MaxAmbiOrder+1]{
        nullptr, &NfcFilter::process1, &NfcFilter::process2, &NfcFilter::process3};

    float *CurrentGains{parms.Gains.Current.data()};
    MixSamples(samples, {OutBuffer, 1u}, CurrentGains, TargetGains, Counter, OutPos);
    ++OutBuffer;
    ++CurrentGains;
    ++TargetGains;

    const al::span<float> nfcsamples{Device->NfcSampleData, samples.size()};
    size_t order{1};
    while(const size_t chancount{Device->NumChannelsPerOrder[order]})
    {
        (parms.NFCtrlFilter.*NfcProcess[order])(samples, nfcsamples.data());
        MixSamples(nfcsamples, {OutBuffer, chancount}, CurrentGains, TargetGains, Counter, OutPos);
        OutBuffer += chancount;
        CurrentGains += chancount;
        TargetGains += chancount;
        if(++order == MaxAmbiOrder+1)
            break;
    }
}

} // namespace

void Voice::mix(const State vstate, ContextBase *Context, const uint SamplesToDo)
{
    static constexpr std::array<float,MAX_OUTPUT_CHANNELS> SilentTarget{};

    ASSUME(SamplesToDo > 0);

    /* Get voice info */
    uint DataPosInt{mPosition.load(std::memory_order_relaxed)};
    uint DataPosFrac{mPositionFrac.load(std::memory_order_relaxed)};
    VoiceBufferItem *BufferListItem{mCurrentBuffer.load(std::memory_order_relaxed)};
    VoiceBufferItem *BufferLoopItem{mLoopBuffer.load(std::memory_order_relaxed)};
    const uint increment{mStep};
    if UNLIKELY(increment < 1)
    {
        /* If the voice is supposed to be stopping but can't be mixed, just
         * stop it before bailing.
         */
        if(vstate == Stopping)
            mPlayState.store(Stopped, std::memory_order_release);
        return;
    }

    DeviceBase *Device{Context->mDevice};
    const uint NumSends{Device->NumAuxSends};

    ResamplerFunc Resample{(increment == MixerFracOne && DataPosFrac == 0) ?
                           Resample_<CopyTag,CTag> : mResampler};

    uint Counter{mFlags.test(VoiceIsFading) ? SamplesToDo : 0};
    if(!Counter)
    {
        /* No fading, just overwrite the old/current params. */
        for(auto &chandata : mChans)
        {
            {
                DirectParams &parms = chandata.mDryParams;
                if(!mFlags.test(VoiceHasHrtf))
                    parms.Gains.Current = parms.Gains.Target;
                else
                    parms.Hrtf.Old = parms.Hrtf.Target;
            }
            for(uint send{0};send < NumSends;++send)
            {
                if(mSend[send].Buffer.empty())
                    continue;

                SendParams &parms = chandata.mWetParams[send];
                parms.Gains.Current = parms.Gains.Target;
            }
        }
    }
    else if UNLIKELY(!BufferListItem)
        Counter = std::min(Counter, 64u);

    std::array<float*,DeviceBase::MixerChannelsMax> SamplePointers;
    const al::span<float*> MixingSamples{SamplePointers.data(), mChans.size()};
    auto offset_bufferline = [](DeviceBase::MixerBufferLine &bufline) noexcept -> float*
    { return bufline.data() + MaxResamplerEdge; };
    std::transform(Device->mSampleData.end() - mChans.size(), Device->mSampleData.end(),
        MixingSamples.begin(), offset_bufferline);

    const uint PostPadding{MaxResamplerEdge +
        (mDecoder ? uint{UhjDecoder::sFilterDelay} : 0u)};
    uint buffers_done{0u};
    uint OutPos{0u};
    do {
        /* Figure out how many buffer samples will be needed */
        uint DstBufferSize{SamplesToDo - OutPos};
        uint SrcBufferSize;

        if(increment <= MixerFracOne)
        {
            /* Calculate the last written dst sample pos. */
            uint64_t DataSize64{DstBufferSize - 1};
            /* Calculate the last read src sample pos. */
            DataSize64 = (DataSize64*increment + DataPosFrac) >> MixerFracBits;
            /* +1 to get the src sample count, include padding. */
            DataSize64 += 1 + PostPadding;

            /* Result is guaranteed to be <= BufferLineSize+PostPadding since
             * we won't use more src samples than dst samples+padding.
             */
            SrcBufferSize = static_cast<uint>(DataSize64);
        }
        else
        {
            uint64_t DataSize64{DstBufferSize};
            /* Calculate the end src sample pos, include padding. */
            DataSize64 = (DataSize64*increment + DataPosFrac) >> MixerFracBits;
            DataSize64 += PostPadding;

            if(DataSize64 <= DeviceBase::MixerLineSize - MaxResamplerEdge)
                SrcBufferSize = static_cast<uint>(DataSize64);
            else
            {
                /* If the source size got saturated, we can't fill the desired
                 * dst size. Figure out how many samples we can actually mix.
                 */
                SrcBufferSize = DeviceBase::MixerLineSize - MaxResamplerEdge;

                DataSize64 = SrcBufferSize - PostPadding;
                DataSize64 = ((DataSize64<<MixerFracBits) - DataPosFrac) / increment;
                if(DataSize64 < DstBufferSize)
                {
                    /* Some mixers require being 16-byte aligned, so also limit
                     * to a multiple of 4 samples to maintain alignment.
                     */
                    DstBufferSize = static_cast<uint>(DataSize64) & ~3u;
                    /* If the voice is stopping, only one mixing iteration will
                     * be done, so ensure it fades out completely this mix.
                     */
                    if(unlikely(vstate == Stopping))
                        Counter = std::min(Counter, DstBufferSize);
                }
                ASSUME(DstBufferSize > 0);
            }
        }

        if(unlikely(!BufferListItem))
        {
            const size_t srcOffset{(increment*DstBufferSize + DataPosFrac)>>MixerFracBits};
            auto prevSamples = mPrevSamples.data();
            SrcBufferSize = SrcBufferSize - PostPadding + MaxResamplerEdge;
            for(auto *chanbuffer : MixingSamples)
            {
                auto srcend = std::copy_n(prevSamples->data(), MaxResamplerPadding,
                    chanbuffer-MaxResamplerEdge);

                /* When loading from a voice that ended prematurely, only take
                 * the samples that get closest to 0 amplitude. This helps
                 * certain sounds fade out better.
                 */
                auto abs_lt = [](const float lhs, const float rhs) noexcept -> bool
                { return std::abs(lhs) < std::abs(rhs); };
                auto srciter = std::min_element(chanbuffer, srcend, abs_lt);

                std::fill(srciter+1, chanbuffer + SrcBufferSize, *srciter);

                std::copy_n(chanbuffer-MaxResamplerEdge+srcOffset, prevSamples->size(),
                    prevSamples->data());
                ++prevSamples;
            }
        }
        else
        {
            auto prevSamples = mPrevSamples.data();
            for(auto *chanbuffer : MixingSamples)
            {
                std::copy_n(prevSamples->data(), MaxResamplerEdge, chanbuffer-MaxResamplerEdge);
                ++prevSamples;
            }
            if(mFlags.test(VoiceIsStatic))
                LoadBufferStatic(BufferListItem, BufferLoopItem, DataPosInt, mFmtType,
                    mFmtChannels, mFrameStep, SrcBufferSize, MixingSamples);
            else if(mFlags.test(VoiceIsCallback))
            {
                if(!mFlags.test(VoiceCallbackStopped) && SrcBufferSize > mNumCallbackSamples)
                {
                    const size_t byteOffset{mNumCallbackSamples*mFrameSize};
                    const size_t needBytes{SrcBufferSize*mFrameSize - byteOffset};

                    const int gotBytes{BufferListItem->mCallback(BufferListItem->mUserData,
                        &BufferListItem->mSamples[byteOffset], static_cast<int>(needBytes))};
                    if(gotBytes < 0)
                        mFlags.set(VoiceCallbackStopped);
                    else if(static_cast<uint>(gotBytes) < needBytes)
                    {
                        mFlags.set(VoiceCallbackStopped);
                        mNumCallbackSamples += static_cast<uint>(gotBytes) / mFrameSize;
                    }
                    else
                        mNumCallbackSamples = SrcBufferSize;
                }
                LoadBufferCallback(BufferListItem, mNumCallbackSamples, mFmtType, mFmtChannels,
                    mFrameStep, SrcBufferSize, MixingSamples);
            }
            else
                LoadBufferQueue(BufferListItem, BufferLoopItem, DataPosInt, mFmtType, mFmtChannels,
                    mFrameStep, SrcBufferSize, MixingSamples);

            const size_t srcOffset{(increment*DstBufferSize + DataPosFrac)>>MixerFracBits};
            if(mDecoder)
            {
                SrcBufferSize = SrcBufferSize - PostPadding + MaxResamplerEdge;
                ((*mDecoder).*mDecoderFunc)(MixingSamples, SrcBufferSize,
                    srcOffset * likely(vstate == Playing));
            }
            /* Store the last source samples used for next time. */
            if(likely(vstate == Playing))
            {
                prevSamples = mPrevSamples.data();
                for(auto *chanbuffer : MixingSamples)
                {
                    /* Store the last source samples used for next time. */
                    std::copy_n(chanbuffer-MaxResamplerEdge+srcOffset, prevSamples->size(),
                        prevSamples->data());
                    ++prevSamples;
                }
            }
        }

        auto voiceSamples = MixingSamples.begin();
        for(auto &chandata : mChans)
        {
            /* Resample, then apply ambisonic upsampling as needed. */
            float *ResampledData{Resample(&mResampleState, *voiceSamples, DataPosFrac, increment,
                {Device->ResampledData, DstBufferSize})};
            ++voiceSamples;

            if(mFlags.test(VoiceIsAmbisonic))
                chandata.mAmbiSplitter.processScale({ResampledData, DstBufferSize},
                    chandata.mAmbiHFScale, chandata.mAmbiLFScale);

            /* Now filter and mix to the appropriate outputs. */
            const al::span<float,BufferLineSize> FilterBuf{Device->FilteredData};
            {
                DirectParams &parms = chandata.mDryParams;
                const float *samples{DoFilters(parms.LowPass, parms.HighPass, FilterBuf.data(),
                    {ResampledData, DstBufferSize}, mDirect.FilterType)};

                if(mFlags.test(VoiceHasHrtf))
                {
                    const float TargetGain{parms.Hrtf.Target.Gain * likely(vstate == Playing)};
                    DoHrtfMix(samples, DstBufferSize, parms, TargetGain, Counter, OutPos,
                        (vstate == Playing), Device);
                }
                else
                {
                    const float *TargetGains{likely(vstate == Playing) ? parms.Gains.Target.data()
                        : SilentTarget.data()};
                    if(mFlags.test(VoiceHasNfc))
                        DoNfcMix({samples, DstBufferSize}, mDirect.Buffer.data(), parms,
                            TargetGains, Counter, OutPos, Device);
                    else
                        MixSamples({samples, DstBufferSize}, mDirect.Buffer,
                            parms.Gains.Current.data(), TargetGains, Counter, OutPos);
                }
            }

            for(uint send{0};send < NumSends;++send)
            {
                if(mSend[send].Buffer.empty())
                    continue;

                SendParams &parms = chandata.mWetParams[send];
                const float *samples{DoFilters(parms.LowPass, parms.HighPass, FilterBuf.data(),
                    {ResampledData, DstBufferSize}, mSend[send].FilterType)};

                const float *TargetGains{likely(vstate == Playing) ? parms.Gains.Target.data()
                    : SilentTarget.data()};
                MixSamples({samples, DstBufferSize}, mSend[send].Buffer,
                    parms.Gains.Current.data(), TargetGains, Counter, OutPos);
            }
        }
        /* If the voice is stopping, we're now done. */
        if(unlikely(vstate == Stopping))
            break;

        /* Update positions */
        DataPosFrac += increment*DstBufferSize;
        const uint SrcSamplesDone{DataPosFrac>>MixerFracBits};
        DataPosInt  += SrcSamplesDone;
        DataPosFrac &= MixerFracMask;

        OutPos += DstBufferSize;
        Counter = maxu(DstBufferSize, Counter) - DstBufferSize;

        if(unlikely(!BufferListItem))
        {
            /* Do nothing extra when there's no buffers. */
        }
        else if(mFlags.test(VoiceIsStatic))
        {
            if(BufferLoopItem)
            {
                /* Handle looping static source */
                const uint LoopStart{BufferListItem->mLoopStart};
                const uint LoopEnd{BufferListItem->mLoopEnd};
                if(DataPosInt >= LoopEnd)
                {
                    assert(LoopEnd > LoopStart);
                    DataPosInt = ((DataPosInt-LoopStart)%(LoopEnd-LoopStart)) + LoopStart;
                }
            }
            else
            {
                /* Handle non-looping static source */
                if(DataPosInt >= BufferListItem->mSampleLen)
                {
                    BufferListItem = nullptr;
                    break;
                }
            }
        }
        else if(mFlags.test(VoiceIsCallback))
        {
            /* Handle callback buffer source */
            if(SrcSamplesDone < mNumCallbackSamples)
            {
                const size_t byteOffset{SrcSamplesDone*mFrameSize};
                const size_t byteEnd{mNumCallbackSamples*mFrameSize};
                al::byte *data{BufferListItem->mSamples};
                std::copy(data+byteOffset, data+byteEnd, data);
                mNumCallbackSamples -= SrcSamplesDone;
            }
            else
            {
                BufferListItem = nullptr;
                mNumCallbackSamples = 0;
            }
        }
        else
        {
            /* Handle streaming source */
            do {
                if(BufferListItem->mSampleLen > DataPosInt)
                    break;

                DataPosInt -= BufferListItem->mSampleLen;

                ++buffers_done;
                BufferListItem = BufferListItem->mNext.load(std::memory_order_relaxed);
                if(!BufferListItem) BufferListItem = BufferLoopItem;
            } while(BufferListItem);
        }
    } while(OutPos < SamplesToDo);

    mFlags.set(VoiceIsFading);

    /* Don't update positions and buffers if we were stopping. */
    if(unlikely(vstate == Stopping))
    {
        mPlayState.store(Stopped, std::memory_order_release);
        return;
    }

    /* Capture the source ID in case it's reset for stopping. */
    const uint SourceID{mSourceID.load(std::memory_order_relaxed)};

    /* Update voice info */
    mPosition.store(DataPosInt, std::memory_order_relaxed);
    mPositionFrac.store(DataPosFrac, std::memory_order_relaxed);
    mCurrentBuffer.store(BufferListItem, std::memory_order_relaxed);
    if(!BufferListItem)
    {
        mLoopBuffer.store(nullptr, std::memory_order_relaxed);
        mSourceID.store(0u, std::memory_order_relaxed);
    }
    std::atomic_thread_fence(std::memory_order_release);

    /* Send any events now, after the position/buffer info was updated. */
    const uint enabledevt{Context->mEnabledEvts.load(std::memory_order_acquire)};
    if(buffers_done > 0 && (enabledevt&AsyncEvent::BufferCompleted))
    {
        RingBuffer *ring{Context->mAsyncEvents.get()};
        auto evt_vec = ring->getWriteVector();
        if(evt_vec.first.len > 0)
        {
            AsyncEvent *evt{al::construct_at(reinterpret_cast<AsyncEvent*>(evt_vec.first.buf),
                AsyncEvent::BufferCompleted)};
            evt->u.bufcomp.id = SourceID;
            evt->u.bufcomp.count = buffers_done;
            ring->writeAdvance(1);
        }
    }

    if(!BufferListItem)
    {
        /* If the voice just ended, set it to Stopping so the next render
         * ensures any residual noise fades to 0 amplitude.
         */
        mPlayState.store(Stopping, std::memory_order_release);
        if((enabledevt&AsyncEvent::SourceStateChange))
            SendSourceStoppedEvent(Context, SourceID);
    }
}

void Voice::prepare(DeviceBase *device)
{
    /* Even if storing really high order ambisonics, we only mix channels for
     * orders up to the device order. The rest are simply dropped.
     */
    uint num_channels{(mFmtChannels == FmtUHJ2 || mFmtChannels == FmtSuperStereo) ? 3 :
        ChannelsFromFmt(mFmtChannels, minu(mAmbiOrder, device->mAmbiOrder))};
    if(unlikely(num_channels > device->mSampleData.size()))
    {
        ERR("Unexpected channel count: %u (limit: %zu, %d:%d)\n", num_channels,
            device->mSampleData.size(), mFmtChannels, mAmbiOrder);
        num_channels = static_cast<uint>(device->mSampleData.size());
    }
    if(mChans.capacity() > 2 && num_channels < mChans.capacity())
    {
        decltype(mChans){}.swap(mChans);
        decltype(mPrevSamples){}.swap(mPrevSamples);
    }
    mChans.reserve(maxu(2, num_channels));
    mChans.resize(num_channels);
    mPrevSamples.reserve(maxu(2, num_channels));
    mPrevSamples.resize(num_channels);

    if(IsUHJ(mFmtChannels))
    {
        mDecoder = std::make_unique<UhjDecoder>();
        mDecoderFunc = (mFmtChannels == FmtSuperStereo) ? &UhjDecoder::decodeStereo
            : &UhjDecoder::decode;
    }
    else
    {
        mDecoder = nullptr;
        mDecoderFunc = nullptr;
    }

    /* Clear the stepping value explicitly so the mixer knows not to mix this
     * until the update gets applied.
     */
    mStep = 0;

    /* Make sure the sample history is cleared. */
    std::fill(mPrevSamples.begin(), mPrevSamples.end(), HistoryLine{});

    /* Don't need to set the VoiceIsAmbisonic flag if the device is not higher
     * order than the voice. No HF scaling is necessary to mix it.
     */
    if(mAmbiOrder && device->mAmbiOrder > mAmbiOrder)
    {
        const uint8_t *OrderFromChan{Is2DAmbisonic(mFmtChannels) ?
            AmbiIndex::OrderFrom2DChannel().data() : AmbiIndex::OrderFromChannel().data()};
        const auto scales = AmbiScale::GetHFOrderScales(mAmbiOrder, device->mAmbiOrder);

        const BandSplitter splitter{device->mXOverFreq / static_cast<float>(device->Frequency)};
        for(auto &chandata : mChans)
        {
            chandata.mAmbiHFScale = scales[*(OrderFromChan++)];
            chandata.mAmbiLFScale = 1.0f;
            chandata.mAmbiSplitter = splitter;
            chandata.mDryParams = DirectParams{};
            chandata.mDryParams.NFCtrlFilter = device->mNFCtrlFilter;
            std::fill_n(chandata.mWetParams.begin(), device->NumAuxSends, SendParams{});
        }
        /* 2-channel UHJ needs different shelf filters. However, we can't just
         * use different shelf filters after mixing it and with any old speaker
         * setup the user has. To make this work, we apply the expected shelf
         * filters for decoding UHJ2 to quad (only needs LF scaling), and act
         * as if those 4 quad channels are encoded right back onto first-order
         * B-Format, which then upsamples to higher order as normal (only needs
         * HF scaling).
         *
         * This isn't perfect, but without an entirely separate and limited
         * UHJ2 path, it's better than nothing.
         */
        if(mFmtChannels == FmtUHJ2)
        {
            mChans[0].mAmbiLFScale = 0.661f;
            mChans[1].mAmbiLFScale = 1.293f;
            mChans[2].mAmbiLFScale = 1.293f;
        }
        mFlags.set(VoiceIsAmbisonic);
    }
    else if(mFmtChannels == FmtUHJ2 && !device->mUhjEncoder)
    {
        /* 2-channel UHJ with first-order output also needs the shelf filter
         * correction applied, except with UHJ output (UHJ2->B-Format->UHJ2 is
         * identity, so don't mess with it).
         */
        const BandSplitter splitter{device->mXOverFreq / static_cast<float>(device->Frequency)};
        for(auto &chandata : mChans)
        {
            chandata.mAmbiHFScale = 1.0f;
            chandata.mAmbiLFScale = 1.0f;
            chandata.mAmbiSplitter = splitter;
            chandata.mDryParams = DirectParams{};
            chandata.mDryParams.NFCtrlFilter = device->mNFCtrlFilter;
            std::fill_n(chandata.mWetParams.begin(), device->NumAuxSends, SendParams{});
        }
        mChans[0].mAmbiLFScale = 0.661f;
        mChans[1].mAmbiLFScale = 1.293f;
        mChans[2].mAmbiLFScale = 1.293f;
        mFlags.set(VoiceIsAmbisonic);
    }
    else
    {
        for(auto &chandata : mChans)
        {
            chandata.mDryParams = DirectParams{};
            chandata.mDryParams.NFCtrlFilter = device->mNFCtrlFilter;
            std::fill_n(chandata.mWetParams.begin(), device->NumAuxSends, SendParams{});
        }
        mFlags.reset(VoiceIsAmbisonic);
    }
}
