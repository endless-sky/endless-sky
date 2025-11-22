/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include "source.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <utility>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "AL/efx.h"

#include "albit.h"
#include "alc/alu.h"
#include "alc/backends/base.h"
#include "alc/context.h"
#include "alc/device.h"
#include "alc/inprogext.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "alspan.h"
#include "atomic.h"
#include "auxeffectslot.h"
#include "buffer.h"
#include "core/ambidefs.h"
#include "core/bformatdec.h"
#include "core/except.h"
#include "core/filters/nfc.h"
#include "core/filters/splitter.h"
#include "core/logging.h"
#include "core/voice_change.h"
#include "event.h"
#include "filter.h"
#include "opthelpers.h"
#include "ringbuffer.h"
#include "threads.h"

#ifdef ALSOFT_EAX
#include "eax_exception.h"
#endif // ALSOFT_EAX

namespace {

using namespace std::placeholders;
using std::chrono::nanoseconds;

Voice *GetSourceVoice(ALsource *source, ALCcontext *context)
{
    auto voicelist = context->getVoicesSpan();
    ALuint idx{source->VoiceIdx};
    if(idx < voicelist.size())
    {
        ALuint sid{source->id};
        Voice *voice = voicelist[idx];
        if(voice->mSourceID.load(std::memory_order_acquire) == sid)
            return voice;
    }
    source->VoiceIdx = INVALID_VOICE_IDX;
    return nullptr;
}


void UpdateSourceProps(const ALsource *source, Voice *voice, ALCcontext *context)
{
    /* Get an unused property container, or allocate a new one as needed. */
    VoicePropsItem *props{context->mFreeVoiceProps.load(std::memory_order_acquire)};
    if(!props)
    {
        context->allocVoiceProps();
        props = context->mFreeVoiceProps.load(std::memory_order_acquire);
    }
    VoicePropsItem *next;
    do {
        next = props->next.load(std::memory_order_relaxed);
    } while(unlikely(context->mFreeVoiceProps.compare_exchange_weak(props, next,
        std::memory_order_acq_rel, std::memory_order_acquire) == false));

    props->Pitch = source->Pitch;
    props->Gain = source->Gain;
    props->OuterGain = source->OuterGain;
    props->MinGain = source->MinGain;
    props->MaxGain = source->MaxGain;
    props->InnerAngle = source->InnerAngle;
    props->OuterAngle = source->OuterAngle;
    props->RefDistance = source->RefDistance;
    props->MaxDistance = source->MaxDistance;
    props->RolloffFactor = source->RolloffFactor
#ifdef ALSOFT_EAX
        + source->RolloffFactor2
#endif
    ;
    props->Position = source->Position;
    props->Velocity = source->Velocity;
    props->Direction = source->Direction;
    props->OrientAt = source->OrientAt;
    props->OrientUp = source->OrientUp;
    props->HeadRelative = source->HeadRelative;
    props->mDistanceModel = source->mDistanceModel;
    props->mResampler = source->mResampler;
    props->DirectChannels = source->DirectChannels;
    props->mSpatializeMode = source->mSpatialize;

    props->DryGainHFAuto = source->DryGainHFAuto;
    props->WetGainAuto = source->WetGainAuto;
    props->WetGainHFAuto = source->WetGainHFAuto;
    props->OuterGainHF = source->OuterGainHF;

    props->AirAbsorptionFactor = source->AirAbsorptionFactor;
    props->RoomRolloffFactor = source->RoomRolloffFactor;
    props->DopplerFactor = source->DopplerFactor;

    props->StereoPan = source->StereoPan;

    props->Radius = source->Radius;
    props->EnhWidth = source->EnhWidth;

    props->Direct.Gain = source->Direct.Gain;
    props->Direct.GainHF = source->Direct.GainHF;
    props->Direct.HFReference = source->Direct.HFReference;
    props->Direct.GainLF = source->Direct.GainLF;
    props->Direct.LFReference = source->Direct.LFReference;

    auto copy_send = [](const ALsource::SendData &srcsend) noexcept -> VoiceProps::SendData
    {
        VoiceProps::SendData ret{};
        ret.Slot = srcsend.Slot ? &srcsend.Slot->mSlot : nullptr;
        ret.Gain = srcsend.Gain;
        ret.GainHF = srcsend.GainHF;
        ret.HFReference = srcsend.HFReference;
        ret.GainLF = srcsend.GainLF;
        ret.LFReference = srcsend.LFReference;
        return ret;
    };
    std::transform(source->Send.cbegin(), source->Send.cend(), props->Send, copy_send);
    if(!props->Send[0].Slot && context->mDefaultSlot)
        props->Send[0].Slot = &context->mDefaultSlot->mSlot;

    /* Set the new container for updating internal parameters. */
    props = voice->mUpdate.exchange(props, std::memory_order_acq_rel);
    if(props)
    {
        /* If there was an unused update container, put it back in the
         * freelist.
         */
        AtomicReplaceHead(context->mFreeVoiceProps, props);
    }
}

/* GetSourceSampleOffset
 *
 * Gets the current read offset for the given Source, in 32.32 fixed-point
 * samples. The offset is relative to the start of the queue (not the start of
 * the current buffer).
 */
int64_t GetSourceSampleOffset(ALsource *Source, ALCcontext *context, nanoseconds *clocktime)
{
    ALCdevice *device{context->mALDevice.get()};
    const VoiceBufferItem *Current{};
    uint64_t readPos{};
    ALuint refcount;
    Voice *voice;

    do {
        refcount = device->waitForMix();
        *clocktime = GetDeviceClockTime(device);
        voice = GetSourceVoice(Source, context);
        if(voice)
        {
            Current = voice->mCurrentBuffer.load(std::memory_order_relaxed);

            readPos  = uint64_t{voice->mPosition.load(std::memory_order_relaxed)} << 32;
            readPos |= uint64_t{voice->mPositionFrac.load(std::memory_order_relaxed)} <<
                       (32-MixerFracBits);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
    } while(refcount != device->MixCount.load(std::memory_order_relaxed));

    if(!voice)
        return 0;

    for(auto &item : Source->mQueue)
    {
        if(&item == Current) break;
        readPos += uint64_t{item.mSampleLen} << 32;
    }
    return static_cast<int64_t>(minu64(readPos, 0x7fffffffffffffff_u64));
}

/* GetSourceSecOffset
 *
 * Gets the current read offset for the given Source, in seconds. The offset is
 * relative to the start of the queue (not the start of the current buffer).
 */
double GetSourceSecOffset(ALsource *Source, ALCcontext *context, nanoseconds *clocktime)
{
    ALCdevice *device{context->mALDevice.get()};
    const VoiceBufferItem *Current{};
    uint64_t readPos{};
    ALuint refcount;
    Voice *voice;

    do {
        refcount = device->waitForMix();
        *clocktime = GetDeviceClockTime(device);
        voice = GetSourceVoice(Source, context);
        if(voice)
        {
            Current = voice->mCurrentBuffer.load(std::memory_order_relaxed);

            readPos  = uint64_t{voice->mPosition.load(std::memory_order_relaxed)} << MixerFracBits;
            readPos |= voice->mPositionFrac.load(std::memory_order_relaxed);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
    } while(refcount != device->MixCount.load(std::memory_order_relaxed));

    if(!voice)
        return 0.0f;

    const ALbuffer *BufferFmt{nullptr};
    auto BufferList = Source->mQueue.cbegin();
    while(BufferList != Source->mQueue.cend() && std::addressof(*BufferList) != Current)
    {
        if(!BufferFmt) BufferFmt = BufferList->mBuffer;
        readPos += uint64_t{BufferList->mSampleLen} << MixerFracBits;
        ++BufferList;
    }
    while(BufferList != Source->mQueue.cend() && !BufferFmt)
    {
        BufferFmt = BufferList->mBuffer;
        ++BufferList;
    }
    ASSUME(BufferFmt != nullptr);

    return static_cast<double>(readPos) / double{MixerFracOne} / BufferFmt->mSampleRate;
}

/* GetSourceOffset
 *
 * Gets the current read offset for the given Source, in the appropriate format
 * (Bytes, Samples or Seconds). The offset is relative to the start of the
 * queue (not the start of the current buffer).
 */
double GetSourceOffset(ALsource *Source, ALenum name, ALCcontext *context)
{
    ALCdevice *device{context->mALDevice.get()};
    const VoiceBufferItem *Current{};
    ALuint readPos{};
    ALuint readPosFrac{};
    ALuint refcount;
    Voice *voice;

    do {
        refcount = device->waitForMix();
        voice = GetSourceVoice(Source, context);
        if(voice)
        {
            Current = voice->mCurrentBuffer.load(std::memory_order_relaxed);

            readPos = voice->mPosition.load(std::memory_order_relaxed);
            readPosFrac = voice->mPositionFrac.load(std::memory_order_relaxed);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
    } while(refcount != device->MixCount.load(std::memory_order_relaxed));

    if(!voice)
        return 0.0;

    const ALbuffer *BufferFmt{nullptr};
    auto BufferList = Source->mQueue.cbegin();
    while(BufferList != Source->mQueue.cend() && std::addressof(*BufferList) != Current)
    {
        if(!BufferFmt) BufferFmt = BufferList->mBuffer;
        readPos += BufferList->mSampleLen;
        ++BufferList;
    }
    while(BufferList != Source->mQueue.cend() && !BufferFmt)
    {
        BufferFmt = BufferList->mBuffer;
        ++BufferList;
    }
    ASSUME(BufferFmt != nullptr);

    double offset{};
    switch(name)
    {
    case AL_SEC_OFFSET:
        offset = (readPos + readPosFrac/double{MixerFracOne}) / BufferFmt->mSampleRate;
        break;

    case AL_SAMPLE_OFFSET:
        offset = readPos + readPosFrac/double{MixerFracOne};
        break;

    case AL_BYTE_OFFSET:
        if(BufferFmt->OriginalType == UserFmtIMA4)
        {
            ALuint FrameBlockSize{BufferFmt->OriginalAlign};
            ALuint align{(BufferFmt->OriginalAlign-1)/2 + 4};
            ALuint BlockSize{align * BufferFmt->channelsFromFmt()};

            /* Round down to nearest ADPCM block */
            offset = static_cast<double>(readPos / FrameBlockSize * BlockSize);
        }
        else if(BufferFmt->OriginalType == UserFmtMSADPCM)
        {
            ALuint FrameBlockSize{BufferFmt->OriginalAlign};
            ALuint align{(FrameBlockSize-2)/2 + 7};
            ALuint BlockSize{align * BufferFmt->channelsFromFmt()};

            /* Round down to nearest ADPCM block */
            offset = static_cast<double>(readPos / FrameBlockSize * BlockSize);
        }
        else
        {
            const ALuint FrameSize{BufferFmt->frameSizeFromFmt()};
            offset = static_cast<double>(readPos * FrameSize);
        }
        break;
    }
    return offset;
}

/* GetSourceLength
 *
 * Gets the length of the given Source's buffer queue, in the appropriate
 * format (Bytes, Samples or Seconds).
 */
double GetSourceLength(const ALsource *source, ALenum name)
{
    uint64_t length{0};
    const ALbuffer *BufferFmt{nullptr};
    for(auto &listitem : source->mQueue)
    {
        if(!BufferFmt)
            BufferFmt = listitem.mBuffer;
        length += listitem.mSampleLen;
    }
    if(length == 0)
        return 0.0;

    ASSUME(BufferFmt != nullptr);
    switch(name)
    {
    case AL_SEC_LENGTH_SOFT:
        return static_cast<double>(length) / BufferFmt->mSampleRate;

    case AL_SAMPLE_LENGTH_SOFT:
        return static_cast<double>(length);

    case AL_BYTE_LENGTH_SOFT:
        if(BufferFmt->OriginalType == UserFmtIMA4)
        {
            ALuint FrameBlockSize{BufferFmt->OriginalAlign};
            ALuint align{(BufferFmt->OriginalAlign-1)/2 + 4};
            ALuint BlockSize{align * BufferFmt->channelsFromFmt()};

            /* Round down to nearest ADPCM block */
            return static_cast<double>(length / FrameBlockSize) * BlockSize;
        }
        else if(BufferFmt->OriginalType == UserFmtMSADPCM)
        {
            ALuint FrameBlockSize{BufferFmt->OriginalAlign};
            ALuint align{(FrameBlockSize-2)/2 + 7};
            ALuint BlockSize{align * BufferFmt->channelsFromFmt()};

            /* Round down to nearest ADPCM block */
            return static_cast<double>(length / FrameBlockSize) * BlockSize;
        }
        return static_cast<double>(length) * BufferFmt->frameSizeFromFmt();
    }
    return 0.0;
}


struct VoicePos {
    ALuint pos, frac;
    ALbufferQueueItem *bufferitem;
};

/**
 * GetSampleOffset
 *
 * Retrieves the voice position, fixed-point fraction, and bufferlist item
 * using the givem offset type and offset. If the offset is out of range,
 * returns an empty optional.
 */
al::optional<VoicePos> GetSampleOffset(al::deque<ALbufferQueueItem> &BufferList, ALenum OffsetType,
    double Offset)
{
    /* Find the first valid Buffer in the Queue */
    const ALbuffer *BufferFmt{nullptr};
    for(auto &item : BufferList)
    {
        BufferFmt = item.mBuffer;
        if(BufferFmt) break;
    }
    if(!BufferFmt || BufferFmt->mCallback)
        return al::nullopt;

    /* Get sample frame offset */
    ALuint offset{0u}, frac{0u};
    double dbloff, dblfrac;
    switch(OffsetType)
    {
    case AL_SEC_OFFSET:
        dblfrac = std::modf(Offset*BufferFmt->mSampleRate, &dbloff);
        offset = static_cast<ALuint>(mind(dbloff, std::numeric_limits<ALuint>::max()));
        frac = static_cast<ALuint>(mind(dblfrac*MixerFracOne, MixerFracOne-1.0));
        break;

    case AL_SAMPLE_OFFSET:
        dblfrac = std::modf(Offset, &dbloff);
        offset = static_cast<ALuint>(mind(dbloff, std::numeric_limits<ALuint>::max()));
        frac = static_cast<ALuint>(mind(dblfrac*MixerFracOne, MixerFracOne-1.0));
        break;

    case AL_BYTE_OFFSET:
        /* Determine the ByteOffset (and ensure it is block aligned) */
        offset = static_cast<ALuint>(Offset);
        if(BufferFmt->OriginalType == UserFmtIMA4)
        {
            const ALuint align{(BufferFmt->OriginalAlign-1)/2 + 4};
            offset /= align * BufferFmt->channelsFromFmt();
            offset *= BufferFmt->OriginalAlign;
        }
        else if(BufferFmt->OriginalType == UserFmtMSADPCM)
        {
            const ALuint align{(BufferFmt->OriginalAlign-2)/2 + 7};
            offset /= align * BufferFmt->channelsFromFmt();
            offset *= BufferFmt->OriginalAlign;
        }
        else
            offset /= BufferFmt->frameSizeFromFmt();
        frac = 0;
        break;
    }

    /* Find the bufferlist item this offset belongs to. */
    ALuint totalBufferLen{0u};
    for(auto &item : BufferList)
    {
        if(totalBufferLen > offset)
            break;
        if(item.mSampleLen > offset-totalBufferLen)
        {
            /* Offset is in this buffer */
            return VoicePos{offset-totalBufferLen, frac, &item};
        }
        totalBufferLen += item.mSampleLen;
    }

    /* Offset is out of range of the queue */
    return al::nullopt;
}


void InitVoice(Voice *voice, ALsource *source, ALbufferQueueItem *BufferList, ALCcontext *context,
    ALCdevice *device)
{
    voice->mLoopBuffer.store(source->Looping ? &source->mQueue.front() : nullptr,
        std::memory_order_relaxed);

    ALbuffer *buffer{BufferList->mBuffer};
    voice->mFrequency = buffer->mSampleRate;
    voice->mFmtChannels =
        (buffer->mChannels == FmtStereo && source->mStereoMode == SourceStereo::Enhanced) ?
        FmtSuperStereo : buffer->mChannels;
    voice->mFmtType = buffer->mType;
    voice->mFrameStep = buffer->channelsFromFmt();
    voice->mFrameSize = buffer->frameSizeFromFmt();
    voice->mAmbiLayout = IsUHJ(voice->mFmtChannels) ? AmbiLayout::FuMa : buffer->mAmbiLayout;
    voice->mAmbiScaling = IsUHJ(voice->mFmtChannels) ? AmbiScaling::UHJ : buffer->mAmbiScaling;
    voice->mAmbiOrder = (voice->mFmtChannels == FmtSuperStereo) ? 1 : buffer->mAmbiOrder;

    if(buffer->mCallback) voice->mFlags.set(VoiceIsCallback);
    else if(source->SourceType == AL_STATIC) voice->mFlags.set(VoiceIsStatic);
    voice->mNumCallbackSamples = 0;

    voice->prepare(device);

    source->mPropsDirty = false;
    UpdateSourceProps(source, voice, context);

    voice->mSourceID.store(source->id, std::memory_order_release);
}


VoiceChange *GetVoiceChanger(ALCcontext *ctx)
{
    VoiceChange *vchg{ctx->mVoiceChangeTail};
    if UNLIKELY(vchg == ctx->mCurrentVoiceChange.load(std::memory_order_acquire))
    {
        ctx->allocVoiceChanges();
        vchg = ctx->mVoiceChangeTail;
    }

    ctx->mVoiceChangeTail = vchg->mNext.exchange(nullptr, std::memory_order_relaxed);

    return vchg;
}

void SendVoiceChanges(ALCcontext *ctx, VoiceChange *tail)
{
    ALCdevice *device{ctx->mALDevice.get()};

    VoiceChange *oldhead{ctx->mCurrentVoiceChange.load(std::memory_order_acquire)};
    while(VoiceChange *next{oldhead->mNext.load(std::memory_order_relaxed)})
        oldhead = next;
    oldhead->mNext.store(tail, std::memory_order_release);

    const bool connected{device->Connected.load(std::memory_order_acquire)};
    device->waitForMix();
    if UNLIKELY(!connected)
    {
        if(ctx->mStopVoicesOnDisconnect.load(std::memory_order_acquire))
        {
            /* If the device is disconnected and voices are stopped, just
             * ignore all pending changes.
             */
            VoiceChange *cur{ctx->mCurrentVoiceChange.load(std::memory_order_acquire)};
            while(VoiceChange *next{cur->mNext.load(std::memory_order_acquire)})
            {
                cur = next;
                if(Voice *voice{cur->mVoice})
                    voice->mSourceID.store(0, std::memory_order_relaxed);
            }
            ctx->mCurrentVoiceChange.store(cur, std::memory_order_release);
        }
    }
}


bool SetVoiceOffset(Voice *oldvoice, const VoicePos &vpos, ALsource *source, ALCcontext *context,
    ALCdevice *device)
{
    /* First, get a free voice to start at the new offset. */
    auto voicelist = context->getVoicesSpan();
    Voice *newvoice{};
    ALuint vidx{0};
    for(Voice *voice : voicelist)
    {
        if(voice->mPlayState.load(std::memory_order_acquire) == Voice::Stopped
            && voice->mSourceID.load(std::memory_order_relaxed) == 0u
            && voice->mPendingChange.load(std::memory_order_relaxed) == false)
        {
            newvoice = voice;
            break;
        }
        ++vidx;
    }
    if(unlikely(!newvoice))
    {
        auto &allvoices = *context->mVoices.load(std::memory_order_relaxed);
        if(allvoices.size() == voicelist.size())
            context->allocVoices(1);
        context->mActiveVoiceCount.fetch_add(1, std::memory_order_release);
        voicelist = context->getVoicesSpan();

        vidx = 0;
        for(Voice *voice : voicelist)
        {
            if(voice->mPlayState.load(std::memory_order_acquire) == Voice::Stopped
                && voice->mSourceID.load(std::memory_order_relaxed) == 0u
                && voice->mPendingChange.load(std::memory_order_relaxed) == false)
            {
                newvoice = voice;
                break;
            }
            ++vidx;
        }
        ASSUME(newvoice != nullptr);
    }

    /* Initialize the new voice and set its starting offset.
     * TODO: It might be better to have the VoiceChange processing copy the old
     * voice's mixing parameters (and pending update) insead of initializing it
     * all here. This would just need to set the minimum properties to link the
     * voice to the source and its position-dependent properties (including the
     * fading flag).
     */
    newvoice->mPlayState.store(Voice::Pending, std::memory_order_relaxed);
    newvoice->mPosition.store(vpos.pos, std::memory_order_relaxed);
    newvoice->mPositionFrac.store(vpos.frac, std::memory_order_relaxed);
    newvoice->mCurrentBuffer.store(vpos.bufferitem, std::memory_order_relaxed);
    newvoice->mFlags.reset();
    if(vpos.pos > 0 || vpos.frac > 0 || vpos.bufferitem != &source->mQueue.front())
        newvoice->mFlags.set(VoiceIsFading);
    InitVoice(newvoice, source, vpos.bufferitem, context, device);
    source->VoiceIdx = vidx;

    /* Set the old voice as having a pending change, and send it off with the
     * new one with a new offset voice change.
     */
    oldvoice->mPendingChange.store(true, std::memory_order_relaxed);

    VoiceChange *vchg{GetVoiceChanger(context)};
    vchg->mOldVoice = oldvoice;
    vchg->mVoice = newvoice;
    vchg->mSourceID = source->id;
    vchg->mState = VChangeState::Restart;
    SendVoiceChanges(context, vchg);

    /* If the old voice still has a sourceID, it's still active and the change-
     * over will work on the next update.
     */
    if LIKELY(oldvoice->mSourceID.load(std::memory_order_acquire) != 0u)
        return true;

    /* Otherwise, if the new voice's state is not pending, the change-over
     * already happened.
     */
    if(newvoice->mPlayState.load(std::memory_order_acquire) != Voice::Pending)
        return true;

    /* Otherwise, wait for any current mix to finish and check one last time. */
    device->waitForMix();
    if(newvoice->mPlayState.load(std::memory_order_acquire) != Voice::Pending)
        return true;
    /* The change-over failed because the old voice stopped before the new
     * voice could start at the new offset. Let go of the new voice and have
     * the caller store the source offset since it's stopped.
     */
    newvoice->mCurrentBuffer.store(nullptr, std::memory_order_relaxed);
    newvoice->mLoopBuffer.store(nullptr, std::memory_order_relaxed);
    newvoice->mSourceID.store(0u, std::memory_order_relaxed);
    newvoice->mPlayState.store(Voice::Stopped, std::memory_order_relaxed);
    return false;
}


/**
 * Returns if the last known state for the source was playing or paused. Does
 * not sync with the mixer voice.
 */
inline bool IsPlayingOrPaused(ALsource *source)
{ return source->state == AL_PLAYING || source->state == AL_PAUSED; }

/**
 * Returns an updated source state using the matching voice's status (or lack
 * thereof).
 */
inline ALenum GetSourceState(ALsource *source, Voice *voice)
{
    if(!voice && source->state == AL_PLAYING)
        source->state = AL_STOPPED;
    return source->state;
}


bool EnsureSources(ALCcontext *context, size_t needed)
{
    size_t count{std::accumulate(context->mSourceList.cbegin(), context->mSourceList.cend(),
        size_t{0},
        [](size_t cur, const SourceSubList &sublist) noexcept -> size_t
        { return cur + static_cast<ALuint>(al::popcount(sublist.FreeMask)); })};

    while(needed > count)
    {
        if UNLIKELY(context->mSourceList.size() >= 1<<25)
            return false;

        context->mSourceList.emplace_back();
        auto sublist = context->mSourceList.end() - 1;
        sublist->FreeMask = ~0_u64;
        sublist->Sources = static_cast<ALsource*>(al_calloc(alignof(ALsource), sizeof(ALsource)*64));
        if UNLIKELY(!sublist->Sources)
        {
            context->mSourceList.pop_back();
            return false;
        }
        count += 64;
    }
    return true;
}

ALsource *AllocSource(ALCcontext *context)
{
    auto sublist = std::find_if(context->mSourceList.begin(), context->mSourceList.end(),
        [](const SourceSubList &entry) noexcept -> bool
        { return entry.FreeMask != 0; });
    auto lidx = static_cast<ALuint>(std::distance(context->mSourceList.begin(), sublist));
    auto slidx = static_cast<ALuint>(al::countr_zero(sublist->FreeMask));
    ASSUME(slidx < 64);

    ALsource *source{al::construct_at(sublist->Sources + slidx)};

    /* Add 1 to avoid source ID 0. */
    source->id = ((lidx<<6) | slidx) + 1;

    context->mNumSources += 1;
    sublist->FreeMask &= ~(1_u64 << slidx);

    return source;
}

void FreeSource(ALCcontext *context, ALsource *source)
{
    const ALuint id{source->id - 1};
    const size_t lidx{id >> 6};
    const ALuint slidx{id & 0x3f};

    if(Voice *voice{GetSourceVoice(source, context)})
    {
        VoiceChange *vchg{GetVoiceChanger(context)};

        voice->mPendingChange.store(true, std::memory_order_relaxed);
        vchg->mVoice = voice;
        vchg->mSourceID = source->id;
        vchg->mState = VChangeState::Stop;

        SendVoiceChanges(context, vchg);
    }

    al::destroy_at(source);

    context->mSourceList[lidx].FreeMask |= 1_u64 << slidx;
    context->mNumSources--;
}


inline ALsource *LookupSource(ALCcontext *context, ALuint id) noexcept
{
    const size_t lidx{(id-1) >> 6};
    const ALuint slidx{(id-1) & 0x3f};

    if UNLIKELY(lidx >= context->mSourceList.size())
        return nullptr;
    SourceSubList &sublist{context->mSourceList[lidx]};
    if UNLIKELY(sublist.FreeMask & (1_u64 << slidx))
        return nullptr;
    return sublist.Sources + slidx;
}

inline ALbuffer *LookupBuffer(ALCdevice *device, ALuint id) noexcept
{
    const size_t lidx{(id-1) >> 6};
    const ALuint slidx{(id-1) & 0x3f};

    if UNLIKELY(lidx >= device->BufferList.size())
        return nullptr;
    BufferSubList &sublist = device->BufferList[lidx];
    if UNLIKELY(sublist.FreeMask & (1_u64 << slidx))
        return nullptr;
    return sublist.Buffers + slidx;
}

inline ALfilter *LookupFilter(ALCdevice *device, ALuint id) noexcept
{
    const size_t lidx{(id-1) >> 6};
    const ALuint slidx{(id-1) & 0x3f};

    if UNLIKELY(lidx >= device->FilterList.size())
        return nullptr;
    FilterSubList &sublist = device->FilterList[lidx];
    if UNLIKELY(sublist.FreeMask & (1_u64 << slidx))
        return nullptr;
    return sublist.Filters + slidx;
}

inline ALeffectslot *LookupEffectSlot(ALCcontext *context, ALuint id) noexcept
{
    const size_t lidx{(id-1) >> 6};
    const ALuint slidx{(id-1) & 0x3f};

    if UNLIKELY(lidx >= context->mEffectSlotList.size())
        return nullptr;
    EffectSlotSubList &sublist{context->mEffectSlotList[lidx]};
    if UNLIKELY(sublist.FreeMask & (1_u64 << slidx))
        return nullptr;
    return sublist.EffectSlots + slidx;
}


al::optional<SourceStereo> StereoModeFromEnum(ALenum mode)
{
    switch(mode)
    {
    case AL_NORMAL_SOFT: return al::make_optional(SourceStereo::Normal);
    case AL_SUPER_STEREO_SOFT: return al::make_optional(SourceStereo::Enhanced);
    }
    WARN("Unsupported stereo mode: 0x%04x\n", mode);
    return al::nullopt;
}
ALenum EnumFromStereoMode(SourceStereo mode)
{
    switch(mode)
    {
    case SourceStereo::Normal: return AL_NORMAL_SOFT;
    case SourceStereo::Enhanced: return AL_SUPER_STEREO_SOFT;
    }
    throw std::runtime_error{"Invalid SourceStereo: "+std::to_string(int(mode))};
}

al::optional<SpatializeMode> SpatializeModeFromEnum(ALenum mode)
{
    switch(mode)
    {
    case AL_FALSE: return al::make_optional(SpatializeMode::Off);
    case AL_TRUE: return al::make_optional(SpatializeMode::On);
    case AL_AUTO_SOFT: return al::make_optional(SpatializeMode::Auto);
    }
    WARN("Unsupported spatialize mode: 0x%04x\n", mode);
    return al::nullopt;
}
ALenum EnumFromSpatializeMode(SpatializeMode mode)
{
    switch(mode)
    {
    case SpatializeMode::Off: return AL_FALSE;
    case SpatializeMode::On: return AL_TRUE;
    case SpatializeMode::Auto: return AL_AUTO_SOFT;
    }
    throw std::runtime_error{"Invalid SpatializeMode: "+std::to_string(int(mode))};
}

al::optional<DirectMode> DirectModeFromEnum(ALenum mode)
{
    switch(mode)
    {
    case AL_FALSE: return al::make_optional(DirectMode::Off);
    case AL_DROP_UNMATCHED_SOFT: return al::make_optional(DirectMode::DropMismatch);
    case AL_REMIX_UNMATCHED_SOFT: return al::make_optional(DirectMode::RemixMismatch);
    }
    WARN("Unsupported direct mode: 0x%04x\n", mode);
    return al::nullopt;
}
ALenum EnumFromDirectMode(DirectMode mode)
{
    switch(mode)
    {
    case DirectMode::Off: return AL_FALSE;
    case DirectMode::DropMismatch: return AL_DROP_UNMATCHED_SOFT;
    case DirectMode::RemixMismatch: return AL_REMIX_UNMATCHED_SOFT;
    }
    throw std::runtime_error{"Invalid DirectMode: "+std::to_string(int(mode))};
}

al::optional<DistanceModel> DistanceModelFromALenum(ALenum model)
{
    switch(model)
    {
    case AL_NONE: return al::make_optional(DistanceModel::Disable);
    case AL_INVERSE_DISTANCE: return al::make_optional(DistanceModel::Inverse);
    case AL_INVERSE_DISTANCE_CLAMPED: return al::make_optional(DistanceModel::InverseClamped);
    case AL_LINEAR_DISTANCE: return al::make_optional(DistanceModel::Linear);
    case AL_LINEAR_DISTANCE_CLAMPED: return al::make_optional(DistanceModel::LinearClamped);
    case AL_EXPONENT_DISTANCE: return al::make_optional(DistanceModel::Exponent);
    case AL_EXPONENT_DISTANCE_CLAMPED: return al::make_optional(DistanceModel::ExponentClamped);
    }
    return al::nullopt;
}
ALenum ALenumFromDistanceModel(DistanceModel model)
{
    switch(model)
    {
    case DistanceModel::Disable: return AL_NONE;
    case DistanceModel::Inverse: return AL_INVERSE_DISTANCE;
    case DistanceModel::InverseClamped: return AL_INVERSE_DISTANCE_CLAMPED;
    case DistanceModel::Linear: return AL_LINEAR_DISTANCE;
    case DistanceModel::LinearClamped: return AL_LINEAR_DISTANCE_CLAMPED;
    case DistanceModel::Exponent: return AL_EXPONENT_DISTANCE;
    case DistanceModel::ExponentClamped: return AL_EXPONENT_DISTANCE_CLAMPED;
    }
    throw std::runtime_error{"Unexpected distance model "+std::to_string(static_cast<int>(model))};
}

enum SourceProp : ALenum {
    srcPitch = AL_PITCH,
    srcGain = AL_GAIN,
    srcMinGain = AL_MIN_GAIN,
    srcMaxGain = AL_MAX_GAIN,
    srcMaxDistance = AL_MAX_DISTANCE,
    srcRolloffFactor = AL_ROLLOFF_FACTOR,
    srcDopplerFactor = AL_DOPPLER_FACTOR,
    srcConeOuterGain = AL_CONE_OUTER_GAIN,
    srcSecOffset = AL_SEC_OFFSET,
    srcSampleOffset = AL_SAMPLE_OFFSET,
    srcByteOffset = AL_BYTE_OFFSET,
    srcConeInnerAngle = AL_CONE_INNER_ANGLE,
    srcConeOuterAngle = AL_CONE_OUTER_ANGLE,
    srcRefDistance = AL_REFERENCE_DISTANCE,

    srcPosition = AL_POSITION,
    srcVelocity = AL_VELOCITY,
    srcDirection = AL_DIRECTION,

    srcSourceRelative = AL_SOURCE_RELATIVE,
    srcLooping = AL_LOOPING,
    srcBuffer = AL_BUFFER,
    srcSourceState = AL_SOURCE_STATE,
    srcBuffersQueued = AL_BUFFERS_QUEUED,
    srcBuffersProcessed = AL_BUFFERS_PROCESSED,
    srcSourceType = AL_SOURCE_TYPE,

    /* ALC_EXT_EFX */
    srcConeOuterGainHF = AL_CONE_OUTER_GAINHF,
    srcAirAbsorptionFactor = AL_AIR_ABSORPTION_FACTOR,
    srcRoomRolloffFactor =  AL_ROOM_ROLLOFF_FACTOR,
    srcDirectFilterGainHFAuto = AL_DIRECT_FILTER_GAINHF_AUTO,
    srcAuxSendFilterGainAuto = AL_AUXILIARY_SEND_FILTER_GAIN_AUTO,
    srcAuxSendFilterGainHFAuto = AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO,
    srcDirectFilter = AL_DIRECT_FILTER,
    srcAuxSendFilter = AL_AUXILIARY_SEND_FILTER,

    /* AL_SOFT_direct_channels */
    srcDirectChannelsSOFT = AL_DIRECT_CHANNELS_SOFT,

    /* AL_EXT_source_distance_model */
    srcDistanceModel = AL_DISTANCE_MODEL,

    /* AL_SOFT_source_latency */
    srcSampleOffsetLatencySOFT = AL_SAMPLE_OFFSET_LATENCY_SOFT,
    srcSecOffsetLatencySOFT = AL_SEC_OFFSET_LATENCY_SOFT,

    /* AL_EXT_STEREO_ANGLES */
    srcAngles = AL_STEREO_ANGLES,

    /* AL_EXT_SOURCE_RADIUS */
    srcRadius = AL_SOURCE_RADIUS,

    /* AL_EXT_BFORMAT */
    srcOrientation = AL_ORIENTATION,

    /* AL_SOFT_source_length */
    srcByteLength = AL_BYTE_LENGTH_SOFT,
    srcSampleLength = AL_SAMPLE_LENGTH_SOFT,
    srcSecLength = AL_SEC_LENGTH_SOFT,

    /* AL_SOFT_source_resampler */
    srcResampler = AL_SOURCE_RESAMPLER_SOFT,

    /* AL_SOFT_source_spatialize */
    srcSpatialize = AL_SOURCE_SPATIALIZE_SOFT,

    /* ALC_SOFT_device_clock */
    srcSampleOffsetClockSOFT = AL_SAMPLE_OFFSET_CLOCK_SOFT,
    srcSecOffsetClockSOFT = AL_SEC_OFFSET_CLOCK_SOFT,

    /* AL_SOFT_UHJ */
    srcStereoMode = AL_STEREO_MODE_SOFT,
    srcSuperStereoWidth = AL_SUPER_STEREO_WIDTH_SOFT,
};


constexpr size_t MaxValues{6u};

ALuint FloatValsByProp(ALenum prop)
{
    switch(static_cast<SourceProp>(prop))
    {
    case AL_PITCH:
    case AL_GAIN:
    case AL_MIN_GAIN:
    case AL_MAX_GAIN:
    case AL_MAX_DISTANCE:
    case AL_ROLLOFF_FACTOR:
    case AL_DOPPLER_FACTOR:
    case AL_CONE_OUTER_GAIN:
    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
    case AL_CONE_INNER_ANGLE:
    case AL_CONE_OUTER_ANGLE:
    case AL_REFERENCE_DISTANCE:
    case AL_CONE_OUTER_GAINHF:
    case AL_AIR_ABSORPTION_FACTOR:
    case AL_ROOM_ROLLOFF_FACTOR:
    case AL_DIRECT_FILTER_GAINHF_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
    case AL_DIRECT_CHANNELS_SOFT:
    case AL_DISTANCE_MODEL:
    case AL_SOURCE_RELATIVE:
    case AL_LOOPING:
    case AL_SOURCE_STATE:
    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
    case AL_SOURCE_TYPE:
    case AL_SOURCE_RADIUS:
    case AL_SOURCE_RESAMPLER_SOFT:
    case AL_SOURCE_SPATIALIZE_SOFT:
    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_SEC_LENGTH_SOFT:
    case AL_STEREO_MODE_SOFT:
    case AL_SUPER_STEREO_WIDTH_SOFT:
        return 1;

    case AL_STEREO_ANGLES:
        return 2;

    case AL_POSITION:
    case AL_VELOCITY:
    case AL_DIRECTION:
        return 3;

    case AL_ORIENTATION:
        return 6;

    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
        break; /* Double only */

    case AL_BUFFER:
    case AL_DIRECT_FILTER:
    case AL_AUXILIARY_SEND_FILTER:
        break; /* i/i64 only */
    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        break; /* i64 only */
    }
    return 0;
}
ALuint DoubleValsByProp(ALenum prop)
{
    switch(static_cast<SourceProp>(prop))
    {
    case AL_PITCH:
    case AL_GAIN:
    case AL_MIN_GAIN:
    case AL_MAX_GAIN:
    case AL_MAX_DISTANCE:
    case AL_ROLLOFF_FACTOR:
    case AL_DOPPLER_FACTOR:
    case AL_CONE_OUTER_GAIN:
    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
    case AL_CONE_INNER_ANGLE:
    case AL_CONE_OUTER_ANGLE:
    case AL_REFERENCE_DISTANCE:
    case AL_CONE_OUTER_GAINHF:
    case AL_AIR_ABSORPTION_FACTOR:
    case AL_ROOM_ROLLOFF_FACTOR:
    case AL_DIRECT_FILTER_GAINHF_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
    case AL_DIRECT_CHANNELS_SOFT:
    case AL_DISTANCE_MODEL:
    case AL_SOURCE_RELATIVE:
    case AL_LOOPING:
    case AL_SOURCE_STATE:
    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
    case AL_SOURCE_TYPE:
    case AL_SOURCE_RADIUS:
    case AL_SOURCE_RESAMPLER_SOFT:
    case AL_SOURCE_SPATIALIZE_SOFT:
    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_SEC_LENGTH_SOFT:
    case AL_STEREO_MODE_SOFT:
    case AL_SUPER_STEREO_WIDTH_SOFT:
        return 1;

    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
    case AL_STEREO_ANGLES:
        return 2;

    case AL_POSITION:
    case AL_VELOCITY:
    case AL_DIRECTION:
        return 3;

    case AL_ORIENTATION:
        return 6;

    case AL_BUFFER:
    case AL_DIRECT_FILTER:
    case AL_AUXILIARY_SEND_FILTER:
        break; /* i/i64 only */
    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        break; /* i64 only */
    }
    return 0;
}


void SetSourcefv(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<const float> values);
void SetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<const int> values);
void SetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<const int64_t> values);

#define CHECKSIZE(v, s) do { \
    if LIKELY((v).size() == (s) || (v).size() == MaxValues) break;            \
    Context->setError(AL_INVALID_ENUM,                                        \
        "Property 0x%04x expects %d value(s), got %zu", prop, (s),            \
        (v).size());                                                          \
    return;                                                                   \
} while(0)
#define CHECKVAL(x) do {                                                      \
    if LIKELY(x) break;                                                       \
    Context->setError(AL_INVALID_VALUE, "Value out of range");                \
    return;                                                                   \
} while(0)

void UpdateSourceProps(ALsource *source, ALCcontext *context)
{
    if(!context->mDeferUpdates)
    {
        if(Voice *voice{GetSourceVoice(source, context)})
        {
            UpdateSourceProps(source, voice, context);
            return;
        }
    }
    source->mPropsDirty = true;
}
#ifdef ALSOFT_EAX
void CommitAndUpdateSourceProps(ALsource *source, ALCcontext *context)
{
    if(!context->mDeferUpdates)
    {
        if(source->eax_is_initialized())
            source->eax_commit();
        if(Voice *voice{GetSourceVoice(source, context)})
        {
            UpdateSourceProps(source, voice, context);
            return;
        }
    }
    source->mPropsDirty = true;
}

#else

inline void CommitAndUpdateSourceProps(ALsource *source, ALCcontext *context)
{ UpdateSourceProps(source, context); }
#endif


void SetSourcefv(ALsource *Source, ALCcontext *Context, SourceProp prop,
    const al::span<const float> values)
{
    int ival;

    switch(prop)
    {
    case AL_SEC_LENGTH_SOFT:
    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
        /* Query only */
        SETERR_RETURN(Context, AL_INVALID_OPERATION,,
            "Setting read-only source property 0x%04x", prop);

    case AL_PITCH:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->Pitch = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_CONE_INNER_ANGLE:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 360.0f);

        Source->InnerAngle = values[0];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_CONE_OUTER_ANGLE:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 360.0f);

        Source->OuterAngle = values[0];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_GAIN:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->Gain = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_MAX_DISTANCE:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->MaxDistance = values[0];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_ROLLOFF_FACTOR:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->RolloffFactor = values[0];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_REFERENCE_DISTANCE:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->RefDistance = values[0];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_MIN_GAIN:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->MinGain = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_MAX_GAIN:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        Source->MaxGain = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_CONE_OUTER_GAIN:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 1.0f);

        Source->OuterGain = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_CONE_OUTER_GAINHF:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 1.0f);

        Source->OuterGainHF = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_AIR_ABSORPTION_FACTOR:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 10.0f);

        Source->AirAbsorptionFactor = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_ROOM_ROLLOFF_FACTOR:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 10.0f);

        Source->RoomRolloffFactor = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_DOPPLER_FACTOR:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 1.0f);

        Source->DopplerFactor = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f);

        if(Voice *voice{GetSourceVoice(Source, Context)})
        {
            auto vpos = GetSampleOffset(Source->mQueue, prop, values[0]);
            if(!vpos) SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid offset");

            if(SetVoiceOffset(voice, *vpos, Source, Context, Context->mALDevice.get()))
                return;
        }
        Source->OffsetType = prop;
        Source->Offset = values[0];
        return;

    case AL_SOURCE_RADIUS:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && std::isfinite(values[0]));

        Source->Radius = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_SUPER_STEREO_WIDTH_SOFT:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0.0f && values[0] <= 1.0f);

        Source->EnhWidth = values[0];
        return UpdateSourceProps(Source, Context);

    case AL_STEREO_ANGLES:
        CHECKSIZE(values, 2);
        CHECKVAL(std::isfinite(values[0]) && std::isfinite(values[1]));

        Source->StereoPan[0] = values[0];
        Source->StereoPan[1] = values[1];
        return UpdateSourceProps(Source, Context);


    case AL_POSITION:
        CHECKSIZE(values, 3);
        CHECKVAL(std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]));

        Source->Position[0] = values[0];
        Source->Position[1] = values[1];
        Source->Position[2] = values[2];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_VELOCITY:
        CHECKSIZE(values, 3);
        CHECKVAL(std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]));

        Source->Velocity[0] = values[0];
        Source->Velocity[1] = values[1];
        Source->Velocity[2] = values[2];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_DIRECTION:
        CHECKSIZE(values, 3);
        CHECKVAL(std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]));

        Source->Direction[0] = values[0];
        Source->Direction[1] = values[1];
        Source->Direction[2] = values[2];
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_ORIENTATION:
        CHECKSIZE(values, 6);
        CHECKVAL(std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2])
            && std::isfinite(values[3]) && std::isfinite(values[4]) && std::isfinite(values[5]));

        Source->OrientAt[0] = values[0];
        Source->OrientAt[1] = values[1];
        Source->OrientAt[2] = values[2];
        Source->OrientUp[0] = values[3];
        Source->OrientUp[1] = values[4];
        Source->OrientUp[2] = values[5];
        return UpdateSourceProps(Source, Context);


    case AL_SOURCE_RELATIVE:
    case AL_LOOPING:
    case AL_SOURCE_STATE:
    case AL_SOURCE_TYPE:
    case AL_DISTANCE_MODEL:
    case AL_DIRECT_FILTER_GAINHF_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
    case AL_DIRECT_CHANNELS_SOFT:
    case AL_SOURCE_RESAMPLER_SOFT:
    case AL_SOURCE_SPATIALIZE_SOFT:
    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_STEREO_MODE_SOFT:
        CHECKSIZE(values, 1);
        ival = static_cast<int>(values[0]);
        return SetSourceiv(Source, Context, prop, {&ival, 1u});

    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
        CHECKSIZE(values, 1);
        ival = static_cast<int>(static_cast<ALuint>(values[0]));
        return SetSourceiv(Source, Context, prop, {&ival, 1u});

    case AL_BUFFER:
    case AL_DIRECT_FILTER:
    case AL_AUXILIARY_SEND_FILTER:
    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    Context->setError(AL_INVALID_ENUM, "Invalid source float property 0x%04x", prop);
    return;
}

void SetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop,
    const al::span<const int> values)
{
    ALCdevice *device{Context->mALDevice.get()};
    ALeffectslot *slot{nullptr};
    al::deque<ALbufferQueueItem> oldlist;
    std::unique_lock<std::mutex> slotlock;
    float fvals[6];

    switch(prop)
    {
    case AL_SOURCE_STATE:
    case AL_SOURCE_TYPE:
    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
        /* Query only */
        SETERR_RETURN(Context, AL_INVALID_OPERATION,,
            "Setting read-only source property 0x%04x", prop);

    case AL_SOURCE_RELATIVE:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] == AL_FALSE || values[0] == AL_TRUE);

        Source->HeadRelative = values[0] != AL_FALSE;
        return CommitAndUpdateSourceProps(Source, Context);

    case AL_LOOPING:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] == AL_FALSE || values[0] == AL_TRUE);

        Source->Looping = values[0] != AL_FALSE;
        if(Voice *voice{GetSourceVoice(Source, Context)})
        {
            if(Source->Looping)
                voice->mLoopBuffer.store(&Source->mQueue.front(), std::memory_order_release);
            else
                voice->mLoopBuffer.store(nullptr, std::memory_order_release);

            /* If the source is playing, wait for the current mix to finish to
             * ensure it isn't currently looping back or reaching the end.
             */
            device->waitForMix();
        }
        return;

    case AL_BUFFER:
        CHECKSIZE(values, 1);
        {
            const ALenum state{GetSourceState(Source, GetSourceVoice(Source, Context))};
            if(state == AL_PLAYING || state == AL_PAUSED)
                SETERR_RETURN(Context, AL_INVALID_OPERATION,,
                    "Setting buffer on playing or paused source %u", Source->id);
        }
        if(values[0])
        {
            std::lock_guard<std::mutex> _{device->BufferLock};
            ALbuffer *buffer{LookupBuffer(device, static_cast<ALuint>(values[0]))};
            if(!buffer)
                SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid buffer ID %u",
                    static_cast<ALuint>(values[0]));
            if(buffer->MappedAccess && !(buffer->MappedAccess&AL_MAP_PERSISTENT_BIT_SOFT))
                SETERR_RETURN(Context, AL_INVALID_OPERATION,,
                    "Setting non-persistently mapped buffer %u", buffer->id);
            if(buffer->mCallback && ReadRef(buffer->ref) != 0)
                SETERR_RETURN(Context, AL_INVALID_OPERATION,,
                    "Setting already-set callback buffer %u", buffer->id);

            /* Add the selected buffer to a one-item queue */
            al::deque<ALbufferQueueItem> newlist;
            newlist.emplace_back();
            newlist.back().mCallback = buffer->mCallback;
            newlist.back().mUserData = buffer->mUserData;
            newlist.back().mSampleLen = buffer->mSampleLen;
            newlist.back().mLoopStart = buffer->mLoopStart;
            newlist.back().mLoopEnd = buffer->mLoopEnd;
            newlist.back().mSamples = buffer->mData.data();
            newlist.back().mBuffer = buffer;
            IncrementRef(buffer->ref);

            /* Source is now Static */
            Source->SourceType = AL_STATIC;
            Source->mQueue.swap(oldlist);
            Source->mQueue.swap(newlist);
        }
        else
        {
            /* Source is now Undetermined */
            Source->SourceType = AL_UNDETERMINED;
            Source->mQueue.swap(oldlist);
        }

        /* Delete all elements in the previous queue */
        for(auto &item : oldlist)
        {
            if(ALbuffer *buffer{item.mBuffer})
                DecrementRef(buffer->ref);
        }
        return;

    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0);

        if(Voice *voice{GetSourceVoice(Source, Context)})
        {
            auto vpos = GetSampleOffset(Source->mQueue, prop, values[0]);
            if(!vpos) SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid source offset");

            if(SetVoiceOffset(voice, *vpos, Source, Context, device))
                return;
        }
        Source->OffsetType = prop;
        Source->Offset = values[0];
        return;

    case AL_DIRECT_FILTER:
        CHECKSIZE(values, 1);
        if(values[0])
        {
            std::lock_guard<std::mutex> _{device->FilterLock};
            ALfilter *filter{LookupFilter(device, static_cast<ALuint>(values[0]))};
            if(!filter)
                SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid filter ID %u",
                    static_cast<ALuint>(values[0]));
            Source->Direct.Gain = filter->Gain;
            Source->Direct.GainHF = filter->GainHF;
            Source->Direct.HFReference = filter->HFReference;
            Source->Direct.GainLF = filter->GainLF;
            Source->Direct.LFReference = filter->LFReference;
        }
        else
        {
            Source->Direct.Gain = 1.0f;
            Source->Direct.GainHF = 1.0f;
            Source->Direct.HFReference = LOWPASSFREQREF;
            Source->Direct.GainLF = 1.0f;
            Source->Direct.LFReference = HIGHPASSFREQREF;
        }
        return UpdateSourceProps(Source, Context);

    case AL_DIRECT_FILTER_GAINHF_AUTO:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] == AL_FALSE || values[0] == AL_TRUE);

        Source->DryGainHFAuto = values[0] != AL_FALSE;
        return UpdateSourceProps(Source, Context);

    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] == AL_FALSE || values[0] == AL_TRUE);

        Source->WetGainAuto = values[0] != AL_FALSE;
        return UpdateSourceProps(Source, Context);

    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] == AL_FALSE || values[0] == AL_TRUE);

        Source->WetGainHFAuto = values[0] != AL_FALSE;
        return UpdateSourceProps(Source, Context);

    case AL_DIRECT_CHANNELS_SOFT:
        CHECKSIZE(values, 1);
        if(auto mode = DirectModeFromEnum(values[0]))
        {
            Source->DirectChannels = *mode;
            return UpdateSourceProps(Source, Context);
        }
        Context->setError(AL_INVALID_VALUE, "Unsupported AL_DIRECT_CHANNELS_SOFT: 0x%04x\n",
            values[0]);
        return;

    case AL_DISTANCE_MODEL:
        CHECKSIZE(values, 1);
        if(auto model = DistanceModelFromALenum(values[0]))
        {
            Source->mDistanceModel = *model;
            if(Context->mSourceDistanceModel)
                UpdateSourceProps(Source, Context);
            return;
        }
        Context->setError(AL_INVALID_VALUE, "Distance model out of range: 0x%04x", values[0]);
        return;

    case AL_SOURCE_RESAMPLER_SOFT:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] >= 0 && values[0] <= static_cast<int>(Resampler::Max));

        Source->mResampler = static_cast<Resampler>(values[0]);
        return UpdateSourceProps(Source, Context);

    case AL_SOURCE_SPATIALIZE_SOFT:
        CHECKSIZE(values, 1);
        if(auto mode = SpatializeModeFromEnum(values[0]))
        {
            Source->mSpatialize = *mode;
            return UpdateSourceProps(Source, Context);
        }
        Context->setError(AL_INVALID_VALUE, "Unsupported AL_SOURCE_SPATIALIZE_SOFT: 0x%04x\n",
            values[0]);
        return;

    case AL_STEREO_MODE_SOFT:
        CHECKSIZE(values, 1);
        {
            const ALenum state{GetSourceState(Source, GetSourceVoice(Source, Context))};
            if(state == AL_PLAYING || state == AL_PAUSED)
                SETERR_RETURN(Context, AL_INVALID_OPERATION,,
                    "Modifying stereo mode on playing or paused source %u", Source->id);
        }
        if(auto mode = StereoModeFromEnum(values[0]))
        {
            Source->mStereoMode = *mode;
            return;
        }
        Context->setError(AL_INVALID_VALUE, "Unsupported AL_STEREO_MODE_SOFT: 0x%04x\n",
            values[0]);
        return;

    case AL_AUXILIARY_SEND_FILTER:
        CHECKSIZE(values, 3);
        slotlock = std::unique_lock<std::mutex>{Context->mEffectSlotLock};
        if(values[0] && (slot=LookupEffectSlot(Context, static_cast<ALuint>(values[0]))) == nullptr)
            SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid effect ID %u", values[0]);
        if(static_cast<ALuint>(values[1]) >= device->NumAuxSends)
            SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid send %u", values[1]);

        if(values[2])
        {
            std::lock_guard<std::mutex> _{device->FilterLock};
            ALfilter *filter{LookupFilter(device, static_cast<ALuint>(values[2]))};
            if(!filter)
                SETERR_RETURN(Context, AL_INVALID_VALUE,, "Invalid filter ID %u", values[2]);

            auto &send = Source->Send[static_cast<ALuint>(values[1])];
            send.Gain = filter->Gain;
            send.GainHF = filter->GainHF;
            send.HFReference = filter->HFReference;
            send.GainLF = filter->GainLF;
            send.LFReference = filter->LFReference;
        }
        else
        {
            /* Disable filter */
            auto &send = Source->Send[static_cast<ALuint>(values[1])];
            send.Gain = 1.0f;
            send.GainHF = 1.0f;
            send.HFReference = LOWPASSFREQREF;
            send.GainLF = 1.0f;
            send.LFReference = HIGHPASSFREQREF;
        }

        if(slot != Source->Send[static_cast<ALuint>(values[1])].Slot && IsPlayingOrPaused(Source))
        {
            /* Add refcount on the new slot, and release the previous slot */
            if(slot) IncrementRef(slot->ref);
            if(auto *oldslot = Source->Send[static_cast<ALuint>(values[1])].Slot)
                DecrementRef(oldslot->ref);
            Source->Send[static_cast<ALuint>(values[1])].Slot = slot;

            /* We must force an update if the auxiliary slot changed on an
             * active source, in case the slot is about to be deleted.
             */
            Voice *voice{GetSourceVoice(Source, Context)};
            if(voice) UpdateSourceProps(Source, voice, Context);
            else Source->mPropsDirty = true;
        }
        else
        {
            if(slot) IncrementRef(slot->ref);
            if(auto *oldslot = Source->Send[static_cast<ALuint>(values[1])].Slot)
                DecrementRef(oldslot->ref);
            Source->Send[static_cast<ALuint>(values[1])].Slot = slot;
            UpdateSourceProps(Source, Context);
        }
        return;


    /* 1x float */
    case AL_CONE_INNER_ANGLE:
    case AL_CONE_OUTER_ANGLE:
    case AL_PITCH:
    case AL_GAIN:
    case AL_MIN_GAIN:
    case AL_MAX_GAIN:
    case AL_REFERENCE_DISTANCE:
    case AL_ROLLOFF_FACTOR:
    case AL_CONE_OUTER_GAIN:
    case AL_MAX_DISTANCE:
    case AL_DOPPLER_FACTOR:
    case AL_CONE_OUTER_GAINHF:
    case AL_AIR_ABSORPTION_FACTOR:
    case AL_ROOM_ROLLOFF_FACTOR:
    case AL_SOURCE_RADIUS:
    case AL_SEC_LENGTH_SOFT:
    case AL_SUPER_STEREO_WIDTH_SOFT:
        CHECKSIZE(values, 1);
        fvals[0] = static_cast<float>(values[0]);
        return SetSourcefv(Source, Context, prop, {fvals, 1u});

    /* 3x float */
    case AL_POSITION:
    case AL_VELOCITY:
    case AL_DIRECTION:
        CHECKSIZE(values, 3);
        fvals[0] = static_cast<float>(values[0]);
        fvals[1] = static_cast<float>(values[1]);
        fvals[2] = static_cast<float>(values[2]);
        return SetSourcefv(Source, Context, prop, {fvals, 3u});

    /* 6x float */
    case AL_ORIENTATION:
        CHECKSIZE(values, 6);
        fvals[0] = static_cast<float>(values[0]);
        fvals[1] = static_cast<float>(values[1]);
        fvals[2] = static_cast<float>(values[2]);
        fvals[3] = static_cast<float>(values[3]);
        fvals[4] = static_cast<float>(values[4]);
        fvals[5] = static_cast<float>(values[5]);
        return SetSourcefv(Source, Context, prop, {fvals, 6u});

    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
    case AL_STEREO_ANGLES:
        break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    Context->setError(AL_INVALID_ENUM, "Invalid source integer property 0x%04x", prop);
    return;
}

void SetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop,
    const al::span<const int64_t> values)
{
    float fvals[MaxValues];
    int   ivals[MaxValues];

    switch(prop)
    {
    case AL_SOURCE_TYPE:
    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
    case AL_SOURCE_STATE:
    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        /* Query only */
        SETERR_RETURN(Context, AL_INVALID_OPERATION,,
            "Setting read-only source property 0x%04x", prop);

    /* 1x int */
    case AL_SOURCE_RELATIVE:
    case AL_LOOPING:
    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
    case AL_DIRECT_FILTER_GAINHF_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
    case AL_DIRECT_CHANNELS_SOFT:
    case AL_DISTANCE_MODEL:
    case AL_SOURCE_RESAMPLER_SOFT:
    case AL_SOURCE_SPATIALIZE_SOFT:
    case AL_STEREO_MODE_SOFT:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] <= INT_MAX && values[0] >= INT_MIN);

        ivals[0] = static_cast<int>(values[0]);
        return SetSourceiv(Source, Context, prop, {ivals, 1u});

    /* 1x uint */
    case AL_BUFFER:
    case AL_DIRECT_FILTER:
        CHECKSIZE(values, 1);
        CHECKVAL(values[0] <= UINT_MAX && values[0] >= 0);

        ivals[0] = static_cast<int>(values[0]);
        return SetSourceiv(Source, Context, prop, {ivals, 1u});

    /* 3x uint */
    case AL_AUXILIARY_SEND_FILTER:
        CHECKSIZE(values, 3);
        CHECKVAL(values[0] <= UINT_MAX && values[0] >= 0 && values[1] <= UINT_MAX && values[1] >= 0
            && values[2] <= UINT_MAX && values[2] >= 0);

        ivals[0] = static_cast<int>(values[0]);
        ivals[1] = static_cast<int>(values[1]);
        ivals[2] = static_cast<int>(values[2]);
        return SetSourceiv(Source, Context, prop, {ivals, 3u});

    /* 1x float */
    case AL_CONE_INNER_ANGLE:
    case AL_CONE_OUTER_ANGLE:
    case AL_PITCH:
    case AL_GAIN:
    case AL_MIN_GAIN:
    case AL_MAX_GAIN:
    case AL_REFERENCE_DISTANCE:
    case AL_ROLLOFF_FACTOR:
    case AL_CONE_OUTER_GAIN:
    case AL_MAX_DISTANCE:
    case AL_DOPPLER_FACTOR:
    case AL_CONE_OUTER_GAINHF:
    case AL_AIR_ABSORPTION_FACTOR:
    case AL_ROOM_ROLLOFF_FACTOR:
    case AL_SOURCE_RADIUS:
    case AL_SEC_LENGTH_SOFT:
    case AL_SUPER_STEREO_WIDTH_SOFT:
        CHECKSIZE(values, 1);
        fvals[0] = static_cast<float>(values[0]);
        return SetSourcefv(Source, Context, prop, {fvals, 1u});

    /* 3x float */
    case AL_POSITION:
    case AL_VELOCITY:
    case AL_DIRECTION:
        CHECKSIZE(values, 3);
        fvals[0] = static_cast<float>(values[0]);
        fvals[1] = static_cast<float>(values[1]);
        fvals[2] = static_cast<float>(values[2]);
        return SetSourcefv(Source, Context, prop, {fvals, 3u});

    /* 6x float */
    case AL_ORIENTATION:
        CHECKSIZE(values, 6);
        fvals[0] = static_cast<float>(values[0]);
        fvals[1] = static_cast<float>(values[1]);
        fvals[2] = static_cast<float>(values[2]);
        fvals[3] = static_cast<float>(values[3]);
        fvals[4] = static_cast<float>(values[4]);
        fvals[5] = static_cast<float>(values[5]);
        return SetSourcefv(Source, Context, prop, {fvals, 6u});

    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
    case AL_STEREO_ANGLES:
        break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    Context->setError(AL_INVALID_ENUM, "Invalid source integer64 property 0x%04x", prop);
    return;
}

#undef CHECKVAL
#undef CHECKSIZE

#define CHECKSIZE(v, s) do { \
    if LIKELY((v).size() == (s) || (v).size() == MaxValues) break;            \
    Context->setError(AL_INVALID_ENUM,                                        \
        "Property 0x%04x expects %d value(s), got %zu", prop, (s),            \
        (v).size());                                                          \
    return false;                                                             \
} while(0)

bool GetSourcedv(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<double> values);
bool GetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<int> values);
bool GetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<int64_t> values);

bool GetSourcedv(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<double> values)
{
    ALCdevice *device{Context->mALDevice.get()};
    ClockLatency clocktime;
    nanoseconds srcclock;
    int ivals[MaxValues];
    bool err;

    switch(prop)
    {
    case AL_GAIN:
        CHECKSIZE(values, 1);
        values[0] = Source->Gain;
        return true;

    case AL_PITCH:
        CHECKSIZE(values, 1);
        values[0] = Source->Pitch;
        return true;

    case AL_MAX_DISTANCE:
        CHECKSIZE(values, 1);
        values[0] = Source->MaxDistance;
        return true;

    case AL_ROLLOFF_FACTOR:
        CHECKSIZE(values, 1);
        values[0] = Source->RolloffFactor;
        return true;

    case AL_REFERENCE_DISTANCE:
        CHECKSIZE(values, 1);
        values[0] = Source->RefDistance;
        return true;

    case AL_CONE_INNER_ANGLE:
        CHECKSIZE(values, 1);
        values[0] = Source->InnerAngle;
        return true;

    case AL_CONE_OUTER_ANGLE:
        CHECKSIZE(values, 1);
        values[0] = Source->OuterAngle;
        return true;

    case AL_MIN_GAIN:
        CHECKSIZE(values, 1);
        values[0] = Source->MinGain;
        return true;

    case AL_MAX_GAIN:
        CHECKSIZE(values, 1);
        values[0] = Source->MaxGain;
        return true;

    case AL_CONE_OUTER_GAIN:
        CHECKSIZE(values, 1);
        values[0] = Source->OuterGain;
        return true;

    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
        CHECKSIZE(values, 1);
        values[0] = GetSourceOffset(Source, prop, Context);
        return true;

    case AL_CONE_OUTER_GAINHF:
        CHECKSIZE(values, 1);
        values[0] = Source->OuterGainHF;
        return true;

    case AL_AIR_ABSORPTION_FACTOR:
        CHECKSIZE(values, 1);
        values[0] = Source->AirAbsorptionFactor;
        return true;

    case AL_ROOM_ROLLOFF_FACTOR:
        CHECKSIZE(values, 1);
        values[0] = Source->RoomRolloffFactor;
        return true;

    case AL_DOPPLER_FACTOR:
        CHECKSIZE(values, 1);
        values[0] = Source->DopplerFactor;
        return true;

    case AL_SOURCE_RADIUS:
        CHECKSIZE(values, 1);
        values[0] = Source->Radius;
        return true;

    case AL_SUPER_STEREO_WIDTH_SOFT:
        CHECKSIZE(values, 1);
        values[0] = Source->EnhWidth;
        return true;

    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_SEC_LENGTH_SOFT:
        CHECKSIZE(values, 1);
        values[0] = GetSourceLength(Source, prop);
        return true;

    case AL_STEREO_ANGLES:
        CHECKSIZE(values, 2);
        values[0] = Source->StereoPan[0];
        values[1] = Source->StereoPan[1];
        return true;

    case AL_SEC_OFFSET_LATENCY_SOFT:
        CHECKSIZE(values, 2);
        /* Get the source offset with the clock time first. Then get the clock
         * time with the device latency. Order is important.
         */
        values[0] = GetSourceSecOffset(Source, Context, &srcclock);
        {
            std::lock_guard<std::mutex> _{device->StateLock};
            clocktime = GetClockLatency(device, device->Backend.get());
        }
        if(srcclock == clocktime.ClockTime)
            values[1] = static_cast<double>(clocktime.Latency.count()) / 1000000000.0;
        else
        {
            /* If the clock time incremented, reduce the latency by that much
             * since it's that much closer to the source offset it got earlier.
             */
            const nanoseconds diff{clocktime.ClockTime - srcclock};
            const nanoseconds latency{clocktime.Latency - std::min(clocktime.Latency, diff)};
            values[1] = static_cast<double>(latency.count()) / 1000000000.0;
        }
        return true;

    case AL_SEC_OFFSET_CLOCK_SOFT:
        CHECKSIZE(values, 2);
        values[0] = GetSourceSecOffset(Source, Context, &srcclock);
        values[1] = static_cast<double>(srcclock.count()) / 1000000000.0;
        return true;

    case AL_POSITION:
        CHECKSIZE(values, 3);
        values[0] = Source->Position[0];
        values[1] = Source->Position[1];
        values[2] = Source->Position[2];
        return true;

    case AL_VELOCITY:
        CHECKSIZE(values, 3);
        values[0] = Source->Velocity[0];
        values[1] = Source->Velocity[1];
        values[2] = Source->Velocity[2];
        return true;

    case AL_DIRECTION:
        CHECKSIZE(values, 3);
        values[0] = Source->Direction[0];
        values[1] = Source->Direction[1];
        values[2] = Source->Direction[2];
        return true;

    case AL_ORIENTATION:
        CHECKSIZE(values, 6);
        values[0] = Source->OrientAt[0];
        values[1] = Source->OrientAt[1];
        values[2] = Source->OrientAt[2];
        values[3] = Source->OrientUp[0];
        values[4] = Source->OrientUp[1];
        values[5] = Source->OrientUp[2];
        return true;

    /* 1x int */
    case AL_SOURCE_RELATIVE:
    case AL_LOOPING:
    case AL_SOURCE_STATE:
    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
    case AL_SOURCE_TYPE:
    case AL_DIRECT_FILTER_GAINHF_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
    case AL_DIRECT_CHANNELS_SOFT:
    case AL_DISTANCE_MODEL:
    case AL_SOURCE_RESAMPLER_SOFT:
    case AL_SOURCE_SPATIALIZE_SOFT:
    case AL_STEREO_MODE_SOFT:
        CHECKSIZE(values, 1);
        if((err=GetSourceiv(Source, Context, prop, {ivals, 1u})) != false)
            values[0] = static_cast<double>(ivals[0]);
        return err;

    case AL_BUFFER:
    case AL_DIRECT_FILTER:
    case AL_AUXILIARY_SEND_FILTER:
    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        break;
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    Context->setError(AL_INVALID_ENUM, "Invalid source double property 0x%04x", prop);
    return false;
}

bool GetSourceiv(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<int> values)
{
    double dvals[MaxValues];
    bool err;

    switch(prop)
    {
    case AL_SOURCE_RELATIVE:
        CHECKSIZE(values, 1);
        values[0] = Source->HeadRelative;
        return true;

    case AL_LOOPING:
        CHECKSIZE(values, 1);
        values[0] = Source->Looping;
        return true;

    case AL_BUFFER:
        CHECKSIZE(values, 1);
        {
            ALbufferQueueItem *BufferList{(Source->SourceType == AL_STATIC)
                ? &Source->mQueue.front() : nullptr};
            ALbuffer *buffer{BufferList ? BufferList->mBuffer : nullptr};
            values[0] = buffer ? static_cast<int>(buffer->id) : 0;
        }
        return true;

    case AL_SOURCE_STATE:
        CHECKSIZE(values, 1);
        values[0] = GetSourceState(Source, GetSourceVoice(Source, Context));
        return true;

    case AL_BUFFERS_QUEUED:
        CHECKSIZE(values, 1);
        values[0] = static_cast<int>(Source->mQueue.size());
        return true;

    case AL_BUFFERS_PROCESSED:
        CHECKSIZE(values, 1);
        if(Source->Looping || Source->SourceType != AL_STREAMING)
        {
            /* Buffers on a looping source are in a perpetual state of PENDING,
             * so don't report any as PROCESSED
             */
            values[0] = 0;
        }
        else
        {
            int played{0};
            if(Source->state != AL_INITIAL)
            {
                const VoiceBufferItem *Current{nullptr};
                if(Voice *voice{GetSourceVoice(Source, Context)})
                    Current = voice->mCurrentBuffer.load(std::memory_order_relaxed);
                for(auto &item : Source->mQueue)
                {
                    if(&item == Current)
                        break;
                    ++played;
                }
            }
            values[0] = played;
        }
        return true;

    case AL_SOURCE_TYPE:
        CHECKSIZE(values, 1);
        values[0] = Source->SourceType;
        return true;

    case AL_DIRECT_FILTER_GAINHF_AUTO:
        CHECKSIZE(values, 1);
        values[0] = Source->DryGainHFAuto;
        return true;

    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
        CHECKSIZE(values, 1);
        values[0] = Source->WetGainAuto;
        return true;

    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
        CHECKSIZE(values, 1);
        values[0] = Source->WetGainHFAuto;
        return true;

    case AL_DIRECT_CHANNELS_SOFT:
        CHECKSIZE(values, 1);
        values[0] = EnumFromDirectMode(Source->DirectChannels);
        return true;

    case AL_DISTANCE_MODEL:
        CHECKSIZE(values, 1);
        values[0] = ALenumFromDistanceModel(Source->mDistanceModel);
        return true;

    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_SEC_LENGTH_SOFT:
        CHECKSIZE(values, 1);
        values[0] = static_cast<int>(mind(GetSourceLength(Source, prop),
            std::numeric_limits<int>::max()));
        return true;

    case AL_SOURCE_RESAMPLER_SOFT:
        CHECKSIZE(values, 1);
        values[0] = static_cast<int>(Source->mResampler);
        return true;

    case AL_SOURCE_SPATIALIZE_SOFT:
        CHECKSIZE(values, 1);
        values[0] = EnumFromSpatializeMode(Source->mSpatialize);
        return true;

    case AL_STEREO_MODE_SOFT:
        CHECKSIZE(values, 1);
        values[0] = EnumFromStereoMode(Source->mStereoMode);
        return true;

    /* 1x float/double */
    case AL_CONE_INNER_ANGLE:
    case AL_CONE_OUTER_ANGLE:
    case AL_PITCH:
    case AL_GAIN:
    case AL_MIN_GAIN:
    case AL_MAX_GAIN:
    case AL_REFERENCE_DISTANCE:
    case AL_ROLLOFF_FACTOR:
    case AL_CONE_OUTER_GAIN:
    case AL_MAX_DISTANCE:
    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
    case AL_DOPPLER_FACTOR:
    case AL_AIR_ABSORPTION_FACTOR:
    case AL_ROOM_ROLLOFF_FACTOR:
    case AL_CONE_OUTER_GAINHF:
    case AL_SOURCE_RADIUS:
    case AL_SUPER_STEREO_WIDTH_SOFT:
        CHECKSIZE(values, 1);
        if((err=GetSourcedv(Source, Context, prop, {dvals, 1u})) != false)
            values[0] = static_cast<int>(dvals[0]);
        return err;

    /* 3x float/double */
    case AL_POSITION:
    case AL_VELOCITY:
    case AL_DIRECTION:
        CHECKSIZE(values, 3);
        if((err=GetSourcedv(Source, Context, prop, {dvals, 3u})) != false)
        {
            values[0] = static_cast<int>(dvals[0]);
            values[1] = static_cast<int>(dvals[1]);
            values[2] = static_cast<int>(dvals[2]);
        }
        return err;

    /* 6x float/double */
    case AL_ORIENTATION:
        CHECKSIZE(values, 6);
        if((err=GetSourcedv(Source, Context, prop, {dvals, 6u})) != false)
        {
            values[0] = static_cast<int>(dvals[0]);
            values[1] = static_cast<int>(dvals[1]);
            values[2] = static_cast<int>(dvals[2]);
            values[3] = static_cast<int>(dvals[3]);
            values[4] = static_cast<int>(dvals[4]);
            values[5] = static_cast<int>(dvals[5]);
        }
        return err;

    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        break; /* i64 only */
    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
        break; /* Double only */
    case AL_STEREO_ANGLES:
        break; /* Float/double only */

    case AL_DIRECT_FILTER:
    case AL_AUXILIARY_SEND_FILTER:
        break; /* ??? */
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    Context->setError(AL_INVALID_ENUM, "Invalid source integer property 0x%04x", prop);
    return false;
}

bool GetSourcei64v(ALsource *Source, ALCcontext *Context, SourceProp prop, const al::span<int64_t> values)
{
    ALCdevice *device{Context->mALDevice.get()};
    ClockLatency clocktime;
    nanoseconds srcclock;
    double dvals[MaxValues];
    int ivals[MaxValues];
    bool err;

    switch(prop)
    {
    case AL_BYTE_LENGTH_SOFT:
    case AL_SAMPLE_LENGTH_SOFT:
    case AL_SEC_LENGTH_SOFT:
        CHECKSIZE(values, 1);
        values[0] = static_cast<int64_t>(GetSourceLength(Source, prop));
        return true;

    case AL_SAMPLE_OFFSET_LATENCY_SOFT:
        CHECKSIZE(values, 2);
        /* Get the source offset with the clock time first. Then get the clock
         * time with the device latency. Order is important.
         */
        values[0] = GetSourceSampleOffset(Source, Context, &srcclock);
        {
            std::lock_guard<std::mutex> _{device->StateLock};
            clocktime = GetClockLatency(device, device->Backend.get());
        }
        if(srcclock == clocktime.ClockTime)
            values[1] = clocktime.Latency.count();
        else
        {
            /* If the clock time incremented, reduce the latency by that much
             * since it's that much closer to the source offset it got earlier.
             */
            const nanoseconds diff{clocktime.ClockTime - srcclock};
            values[1] = nanoseconds{clocktime.Latency - std::min(clocktime.Latency, diff)}.count();
        }
        return true;

    case AL_SAMPLE_OFFSET_CLOCK_SOFT:
        CHECKSIZE(values, 2);
        values[0] = GetSourceSampleOffset(Source, Context, &srcclock);
        values[1] = srcclock.count();
        return true;

    /* 1x float/double */
    case AL_CONE_INNER_ANGLE:
    case AL_CONE_OUTER_ANGLE:
    case AL_PITCH:
    case AL_GAIN:
    case AL_MIN_GAIN:
    case AL_MAX_GAIN:
    case AL_REFERENCE_DISTANCE:
    case AL_ROLLOFF_FACTOR:
    case AL_CONE_OUTER_GAIN:
    case AL_MAX_DISTANCE:
    case AL_SEC_OFFSET:
    case AL_SAMPLE_OFFSET:
    case AL_BYTE_OFFSET:
    case AL_DOPPLER_FACTOR:
    case AL_AIR_ABSORPTION_FACTOR:
    case AL_ROOM_ROLLOFF_FACTOR:
    case AL_CONE_OUTER_GAINHF:
    case AL_SOURCE_RADIUS:
    case AL_SUPER_STEREO_WIDTH_SOFT:
        CHECKSIZE(values, 1);
        if((err=GetSourcedv(Source, Context, prop, {dvals, 1u})) != false)
            values[0] = static_cast<int64_t>(dvals[0]);
        return err;

    /* 3x float/double */
    case AL_POSITION:
    case AL_VELOCITY:
    case AL_DIRECTION:
        CHECKSIZE(values, 3);
        if((err=GetSourcedv(Source, Context, prop, {dvals, 3u})) != false)
        {
            values[0] = static_cast<int64_t>(dvals[0]);
            values[1] = static_cast<int64_t>(dvals[1]);
            values[2] = static_cast<int64_t>(dvals[2]);
        }
        return err;

    /* 6x float/double */
    case AL_ORIENTATION:
        CHECKSIZE(values, 6);
        if((err=GetSourcedv(Source, Context, prop, {dvals, 6u})) != false)
        {
            values[0] = static_cast<int64_t>(dvals[0]);
            values[1] = static_cast<int64_t>(dvals[1]);
            values[2] = static_cast<int64_t>(dvals[2]);
            values[3] = static_cast<int64_t>(dvals[3]);
            values[4] = static_cast<int64_t>(dvals[4]);
            values[5] = static_cast<int64_t>(dvals[5]);
        }
        return err;

    /* 1x int */
    case AL_SOURCE_RELATIVE:
    case AL_LOOPING:
    case AL_SOURCE_STATE:
    case AL_BUFFERS_QUEUED:
    case AL_BUFFERS_PROCESSED:
    case AL_SOURCE_TYPE:
    case AL_DIRECT_FILTER_GAINHF_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:
    case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:
    case AL_DIRECT_CHANNELS_SOFT:
    case AL_DISTANCE_MODEL:
    case AL_SOURCE_RESAMPLER_SOFT:
    case AL_SOURCE_SPATIALIZE_SOFT:
    case AL_STEREO_MODE_SOFT:
        CHECKSIZE(values, 1);
        if((err=GetSourceiv(Source, Context, prop, {ivals, 1u})) != false)
            values[0] = ivals[0];
        return err;

    /* 1x uint */
    case AL_BUFFER:
    case AL_DIRECT_FILTER:
        CHECKSIZE(values, 1);
        if((err=GetSourceiv(Source, Context, prop, {ivals, 1u})) != false)
            values[0] = static_cast<ALuint>(ivals[0]);
        return err;

    /* 3x uint */
    case AL_AUXILIARY_SEND_FILTER:
        CHECKSIZE(values, 3);
        if((err=GetSourceiv(Source, Context, prop, {ivals, 3u})) != false)
        {
            values[0] = static_cast<ALuint>(ivals[0]);
            values[1] = static_cast<ALuint>(ivals[1]);
            values[2] = static_cast<ALuint>(ivals[2]);
        }
        return err;

    case AL_SEC_OFFSET_LATENCY_SOFT:
    case AL_SEC_OFFSET_CLOCK_SOFT:
        break; /* Double only */
    case AL_STEREO_ANGLES:
        break; /* Float/double only */
    }

    ERR("Unexpected property: 0x%04x\n", prop);
    Context->setError(AL_INVALID_ENUM, "Invalid source integer64 property 0x%04x", prop);
    return false;
}

} // namespace

AL_API void AL_APIENTRY alGenSources(ALsizei n, ALuint *sources)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Generating %d sources", n);
    if UNLIKELY(n <= 0) return;

#ifdef ALSOFT_EAX
    const bool has_eax{context->has_eax()};
    std::unique_lock<std::mutex> proplock{};
    if(has_eax)
        proplock = std::unique_lock<std::mutex>{context->mPropLock};
#endif
    std::unique_lock<std::mutex> srclock{context->mSourceLock};
    ALCdevice *device{context->mALDevice.get()};
    if(static_cast<ALuint>(n) > device->SourcesMax-context->mNumSources)
    {
        context->setError(AL_OUT_OF_MEMORY, "Exceeding %u source limit (%u + %d)",
            device->SourcesMax, context->mNumSources, n);
        return;
    }
    if(!EnsureSources(context.get(), static_cast<ALuint>(n)))
    {
        context->setError(AL_OUT_OF_MEMORY, "Failed to allocate %d source%s", n, (n==1)?"":"s");
        return;
    }

    if(n == 1)
    {
        ALsource *source{AllocSource(context.get())};
        sources[0] = source->id;

#ifdef ALSOFT_EAX
        if(has_eax)
            source->eax_initialize(context.get());
#endif // ALSOFT_EAX
    }
    else
    {
#ifdef ALSOFT_EAX
        auto eax_sources = al::vector<ALsource*>{};
        if(has_eax)
            eax_sources.reserve(static_cast<ALuint>(n));
#endif // ALSOFT_EAX

        al::vector<ALuint> ids;
        ids.reserve(static_cast<ALuint>(n));
        do {
            ALsource *source{AllocSource(context.get())};
            ids.emplace_back(source->id);

#ifdef ALSOFT_EAX
            if(has_eax)
                eax_sources.emplace_back(source);
#endif // ALSOFT_EAX
        } while(--n);
        std::copy(ids.cbegin(), ids.cend(), sources);

#ifdef ALSOFT_EAX
        for(auto& eax_source : eax_sources)
            eax_source->eax_initialize(context.get());
#endif // ALSOFT_EAX
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alDeleteSources(ALsizei n, const ALuint *sources)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        SETERR_RETURN(context, AL_INVALID_VALUE,, "Deleting %d sources", n);

    std::lock_guard<std::mutex> _{context->mSourceLock};

    /* Check that all Sources are valid */
    auto validate_source = [&context](const ALuint sid) -> bool
    { return LookupSource(context.get(), sid) != nullptr; };

    const ALuint *sources_end = sources + n;
    auto invsrc = std::find_if_not(sources, sources_end, validate_source);
    if UNLIKELY(invsrc != sources_end)
    {
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", *invsrc);
        return;
    }

    /* All good. Delete source IDs. */
    auto delete_source = [&context](const ALuint sid) -> void
    {
        ALsource *src{LookupSource(context.get(), sid)};
        if(src) FreeSource(context.get(), src);
    };
    std::for_each(sources, sources_end, delete_source);
}
END_API_FUNC

AL_API ALboolean AL_APIENTRY alIsSource(ALuint source)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if LIKELY(context)
    {
        std::lock_guard<std::mutex> _{context->mSourceLock};
        if(LookupSource(context.get(), source) != nullptr)
            return AL_TRUE;
    }
    return AL_FALSE;
}
END_API_FUNC


AL_API void AL_APIENTRY alSourcef(ALuint source, ALenum param, ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
        SetSourcefv(Source, context.get(), static_cast<SourceProp>(param), {&value, 1u});
}
END_API_FUNC

AL_API void AL_APIENTRY alSource3f(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
    {
        const float fvals[3]{ value1, value2, value3 };
        SetSourcefv(Source, context.get(), static_cast<SourceProp>(param), fvals);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSourcefv(ALuint source, ALenum param, const ALfloat *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        SetSourcefv(Source, context.get(), static_cast<SourceProp>(param), {values, MaxValues});
}
END_API_FUNC


AL_API void AL_APIENTRY alSourcedSOFT(ALuint source, ALenum param, ALdouble value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
    {
        const float fval[1]{static_cast<float>(value)};
        SetSourcefv(Source, context.get(), static_cast<SourceProp>(param), fval);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSource3dSOFT(ALuint source, ALenum param, ALdouble value1, ALdouble value2, ALdouble value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
    {
        const float fvals[3]{static_cast<float>(value1), static_cast<float>(value2),
            static_cast<float>(value3)};
        SetSourcefv(Source, context.get(), static_cast<SourceProp>(param), fvals);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSourcedvSOFT(ALuint source, ALenum param, const ALdouble *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        const ALuint count{DoubleValsByProp(param)};
        float fvals[MaxValues];
        for(ALuint i{0};i < count;i++)
            fvals[i] = static_cast<float>(values[i]);
        SetSourcefv(Source, context.get(), static_cast<SourceProp>(param), {fvals, count});
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alSourcei(ALuint source, ALenum param, ALint value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
        SetSourceiv(Source, context.get(), static_cast<SourceProp>(param), {&value, 1u});
}
END_API_FUNC

AL_API void AL_APIENTRY alSource3i(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
    {
        const int ivals[3]{ value1, value2, value3 };
        SetSourceiv(Source, context.get(), static_cast<SourceProp>(param), ivals);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSourceiv(ALuint source, ALenum param, const ALint *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source = LookupSource(context.get(), source);
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        SetSourceiv(Source, context.get(), static_cast<SourceProp>(param), {values, MaxValues});
}
END_API_FUNC


AL_API void AL_APIENTRY alSourcei64SOFT(ALuint source, ALenum param, ALint64SOFT value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
        SetSourcei64v(Source, context.get(), static_cast<SourceProp>(param), {&value, 1u});
}
END_API_FUNC

AL_API void AL_APIENTRY alSource3i64SOFT(ALuint source, ALenum param, ALint64SOFT value1, ALint64SOFT value2, ALint64SOFT value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else
    {
        const int64_t i64vals[3]{ value1, value2, value3 };
        SetSourcei64v(Source, context.get(), static_cast<SourceProp>(param), i64vals);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSourcei64vSOFT(ALuint source, ALenum param, const ALint64SOFT *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        SetSourcei64v(Source, context.get(), static_cast<SourceProp>(param), {values, MaxValues});
}
END_API_FUNC


AL_API void AL_APIENTRY alGetSourcef(ALuint source, ALenum param, ALfloat *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!value)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        double dval[1];
        if(GetSourcedv(Source, context.get(), static_cast<SourceProp>(param), dval))
            *value = static_cast<float>(dval[0]);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSource3f(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!(value1 && value2 && value3))
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        double dvals[3];
        if(GetSourcedv(Source, context.get(), static_cast<SourceProp>(param), dvals))
        {
            *value1 = static_cast<float>(dvals[0]);
            *value2 = static_cast<float>(dvals[1]);
            *value3 = static_cast<float>(dvals[2]);
        }
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSourcefv(ALuint source, ALenum param, ALfloat *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        const ALuint count{FloatValsByProp(param)};
        double dvals[MaxValues];
        if(GetSourcedv(Source, context.get(), static_cast<SourceProp>(param), {dvals, count}))
        {
            for(ALuint i{0};i < count;i++)
                values[i] = static_cast<float>(dvals[i]);
        }
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alGetSourcedSOFT(ALuint source, ALenum param, ALdouble *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!value)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        GetSourcedv(Source, context.get(), static_cast<SourceProp>(param), {value, 1u});
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSource3dSOFT(ALuint source, ALenum param, ALdouble *value1, ALdouble *value2, ALdouble *value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!(value1 && value2 && value3))
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        double dvals[3];
        if(GetSourcedv(Source, context.get(), static_cast<SourceProp>(param), dvals))
        {
            *value1 = dvals[0];
            *value2 = dvals[1];
            *value3 = dvals[2];
        }
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSourcedvSOFT(ALuint source, ALenum param, ALdouble *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        GetSourcedv(Source, context.get(), static_cast<SourceProp>(param), {values, MaxValues});
}
END_API_FUNC


AL_API void AL_APIENTRY alGetSourcei(ALuint source, ALenum param, ALint *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!value)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        GetSourceiv(Source, context.get(), static_cast<SourceProp>(param), {value, 1u});
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSource3i(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!(value1 && value2 && value3))
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        int ivals[3];
        if(GetSourceiv(Source, context.get(), static_cast<SourceProp>(param), ivals))
        {
            *value1 = ivals[0];
            *value2 = ivals[1];
            *value3 = ivals[2];
        }
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSourceiv(ALuint source, ALenum param, ALint *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        GetSourceiv(Source, context.get(), static_cast<SourceProp>(param), {values, MaxValues});
}
END_API_FUNC


AL_API void AL_APIENTRY alGetSourcei64SOFT(ALuint source, ALenum param, ALint64SOFT *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!value)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        GetSourcei64v(Source, context.get(), static_cast<SourceProp>(param), {value, 1u});
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSource3i64SOFT(ALuint source, ALenum param, ALint64SOFT *value1, ALint64SOFT *value2, ALint64SOFT *value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!(value1 && value2 && value3))
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
    {
        int64_t i64vals[3];
        if(GetSourcei64v(Source, context.get(), static_cast<SourceProp>(param), i64vals))
        {
            *value1 = i64vals[0];
            *value2 = i64vals[1];
            *value3 = i64vals[2];
        }
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetSourcei64vSOFT(ALuint source, ALenum param, ALint64SOFT *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *Source{LookupSource(context.get(), source)};
    if UNLIKELY(!Source)
        context->setError(AL_INVALID_NAME, "Invalid source ID %u", source);
    else if UNLIKELY(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else
        GetSourcei64v(Source, context.get(), static_cast<SourceProp>(param), {values, MaxValues});
}
END_API_FUNC


AL_API void AL_APIENTRY alSourcePlay(ALuint source)
START_API_FUNC
{ alSourcePlayv(1, &source); }
END_API_FUNC

AL_API void AL_APIENTRY alSourcePlayv(ALsizei n, const ALuint *sources)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Playing %d sources", n);
    if UNLIKELY(n <= 0) return;

    al::vector<ALsource*> extra_sources;
    std::array<ALsource*,8> source_storage;
    al::span<ALsource*> srchandles;
    if LIKELY(static_cast<ALuint>(n) <= source_storage.size())
        srchandles = {source_storage.data(), static_cast<ALuint>(n)};
    else
    {
        extra_sources.resize(static_cast<ALuint>(n));
        srchandles = {extra_sources.data(), extra_sources.size()};
    }

    std::lock_guard<std::mutex> _{context->mSourceLock};
    for(auto &srchdl : srchandles)
    {
        srchdl = LookupSource(context.get(), *sources);
        if(!srchdl)
            SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid source ID %u", *sources);
        ++sources;
    }

    ALCdevice *device{context->mALDevice.get()};
    /* If the device is disconnected, and voices stop on disconnect, go right
     * to stopped.
     */
    if UNLIKELY(!device->Connected.load(std::memory_order_acquire))
    {
        if(context->mStopVoicesOnDisconnect.load(std::memory_order_acquire))
        {
            for(ALsource *source : srchandles)
            {
                /* TODO: Send state change event? */
                source->Offset = 0.0;
                source->OffsetType = AL_NONE;
                source->state = AL_STOPPED;
            }
            return;
        }
    }

    /* Count the number of reusable voices. */
    auto voicelist = context->getVoicesSpan();
    size_t free_voices{0};
    for(const Voice *voice : voicelist)
    {
        free_voices += (voice->mPlayState.load(std::memory_order_acquire) == Voice::Stopped
            && voice->mSourceID.load(std::memory_order_relaxed) == 0u
            && voice->mPendingChange.load(std::memory_order_relaxed) == false);
        if(free_voices == srchandles.size())
            break;
    }
    if UNLIKELY(srchandles.size() != free_voices)
    {
        const size_t inc_amount{srchandles.size() - free_voices};
        auto &allvoices = *context->mVoices.load(std::memory_order_relaxed);
        if(inc_amount > allvoices.size() - voicelist.size())
        {
            /* Increase the number of voices to handle the request. */
            context->allocVoices(inc_amount - (allvoices.size() - voicelist.size()));
        }
        context->mActiveVoiceCount.fetch_add(inc_amount, std::memory_order_release);
        voicelist = context->getVoicesSpan();
    }

    auto voiceiter = voicelist.begin();
    ALuint vidx{0};
    VoiceChange *tail{}, *cur{};
    for(ALsource *source : srchandles)
    {
        /* Check that there is a queue containing at least one valid, non zero
         * length buffer.
         */
        auto BufferList = source->mQueue.begin();
        for(;BufferList != source->mQueue.end();++BufferList)
        {
            if(BufferList->mSampleLen != 0 || BufferList->mCallback)
                break;
        }

        /* If there's nothing to play, go right to stopped. */
        if UNLIKELY(BufferList == source->mQueue.end())
        {
            /* NOTE: A source without any playable buffers should not have a
             * Voice since it shouldn't be in a playing or paused state. So
             * there's no need to look up its voice and clear the source.
             */
            source->Offset = 0.0;
            source->OffsetType = AL_NONE;
            source->state = AL_STOPPED;
            continue;
        }

        if(!cur)
            cur = tail = GetVoiceChanger(context.get());
        else
        {
            cur->mNext.store(GetVoiceChanger(context.get()), std::memory_order_relaxed);
            cur = cur->mNext.load(std::memory_order_relaxed);
        }

        Voice *voice{GetSourceVoice(source, context.get())};
        switch(GetSourceState(source, voice))
        {
        case AL_PAUSED:
            /* A source that's paused simply resumes. If there's no voice, it
             * was lost from a disconnect, so just start over with a new one.
             */
            cur->mOldVoice = nullptr;
            if(!voice) break;
            cur->mVoice = voice;
            cur->mSourceID = source->id;
            cur->mState = VChangeState::Play;
            source->state = AL_PLAYING;
#ifdef ALSOFT_EAX
            if(source->eax_is_initialized())
                source->eax_commit();
#endif // ALSOFT_EAX
            continue;

        case AL_PLAYING:
            /* A source that's already playing is restarted from the beginning.
             * Stop the current voice and start a new one so it properly cross-
             * fades back to the beginning.
             */
            if(voice)
                voice->mPendingChange.store(true, std::memory_order_relaxed);
            cur->mOldVoice = voice;
            voice = nullptr;
            break;

        default:
            assert(voice == nullptr);
            cur->mOldVoice = nullptr;
#ifdef ALSOFT_EAX
            if(source->eax_is_initialized())
                source->eax_commit();
#endif // ALSOFT_EAX
            break;
        }

        /* Find the next unused voice to play this source with. */
        for(;voiceiter != voicelist.end();++voiceiter,++vidx)
        {
            Voice *v{*voiceiter};
            if(v->mPlayState.load(std::memory_order_acquire) == Voice::Stopped
                && v->mSourceID.load(std::memory_order_relaxed) == 0u
                && v->mPendingChange.load(std::memory_order_relaxed) == false)
            {
                voice = v;
                break;
            }
        }
        ASSUME(voice != nullptr);

        voice->mPosition.store(0u, std::memory_order_relaxed);
        voice->mPositionFrac.store(0, std::memory_order_relaxed);
        voice->mCurrentBuffer.store(&source->mQueue.front(), std::memory_order_relaxed);
        voice->mFlags.reset();
        /* A source that's not playing or paused has any offset applied when it
         * starts playing.
         */
        if(const ALenum offsettype{source->OffsetType})
        {
            const double offset{source->Offset};
            source->OffsetType = AL_NONE;
            source->Offset = 0.0;
            if(auto vpos = GetSampleOffset(source->mQueue, offsettype, offset))
            {
                voice->mPosition.store(vpos->pos, std::memory_order_relaxed);
                voice->mPositionFrac.store(vpos->frac, std::memory_order_relaxed);
                voice->mCurrentBuffer.store(vpos->bufferitem, std::memory_order_relaxed);
                if(vpos->pos!=0 || vpos->frac!=0 || vpos->bufferitem!=&source->mQueue.front())
                    voice->mFlags.set(VoiceIsFading);
            }
        }
        InitVoice(voice, source, std::addressof(*BufferList), context.get(), device);

        source->VoiceIdx = vidx;
        source->state = AL_PLAYING;

        cur->mVoice = voice;
        cur->mSourceID = source->id;
        cur->mState = VChangeState::Play;
    }
    if LIKELY(tail)
        SendVoiceChanges(context.get(), tail);
}
END_API_FUNC


AL_API void AL_APIENTRY alSourcePause(ALuint source)
START_API_FUNC
{ alSourcePausev(1, &source); }
END_API_FUNC

AL_API void AL_APIENTRY alSourcePausev(ALsizei n, const ALuint *sources)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Pausing %d sources", n);
    if UNLIKELY(n <= 0) return;

    al::vector<ALsource*> extra_sources;
    std::array<ALsource*,8> source_storage;
    al::span<ALsource*> srchandles;
    if LIKELY(static_cast<ALuint>(n) <= source_storage.size())
        srchandles = {source_storage.data(), static_cast<ALuint>(n)};
    else
    {
        extra_sources.resize(static_cast<ALuint>(n));
        srchandles = {extra_sources.data(), extra_sources.size()};
    }

    std::lock_guard<std::mutex> _{context->mSourceLock};
    for(auto &srchdl : srchandles)
    {
        srchdl = LookupSource(context.get(), *sources);
        if(!srchdl)
            SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid source ID %u", *sources);
        ++sources;
    }

    /* Pausing has to be done in two steps. First, for each source that's
     * detected to be playing, chamge the voice (asynchronously) to
     * stopping/paused.
     */
    VoiceChange *tail{}, *cur{};
    for(ALsource *source : srchandles)
    {
        Voice *voice{GetSourceVoice(source, context.get())};
        if(GetSourceState(source, voice) == AL_PLAYING)
        {
            if(!cur)
                cur = tail = GetVoiceChanger(context.get());
            else
            {
                cur->mNext.store(GetVoiceChanger(context.get()), std::memory_order_relaxed);
                cur = cur->mNext.load(std::memory_order_relaxed);
            }
            cur->mVoice = voice;
            cur->mSourceID = source->id;
            cur->mState = VChangeState::Pause;
        }
    }
    if LIKELY(tail)
    {
        SendVoiceChanges(context.get(), tail);
        /* Second, now that the voice changes have been sent, because it's
         * possible that the voice stopped after it was detected playing and
         * before the voice got paused, recheck that the source is still
         * considered playing and set it to paused if so.
         */
        for(ALsource *source : srchandles)
        {
            Voice *voice{GetSourceVoice(source, context.get())};
            if(GetSourceState(source, voice) == AL_PLAYING)
                source->state = AL_PAUSED;
        }
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alSourceStop(ALuint source)
START_API_FUNC
{ alSourceStopv(1, &source); }
END_API_FUNC

AL_API void AL_APIENTRY alSourceStopv(ALsizei n, const ALuint *sources)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Stopping %d sources", n);
    if UNLIKELY(n <= 0) return;

    al::vector<ALsource*> extra_sources;
    std::array<ALsource*,8> source_storage;
    al::span<ALsource*> srchandles;
    if LIKELY(static_cast<ALuint>(n) <= source_storage.size())
        srchandles = {source_storage.data(), static_cast<ALuint>(n)};
    else
    {
        extra_sources.resize(static_cast<ALuint>(n));
        srchandles = {extra_sources.data(), extra_sources.size()};
    }

    std::lock_guard<std::mutex> _{context->mSourceLock};
    for(auto &srchdl : srchandles)
    {
        srchdl = LookupSource(context.get(), *sources);
        if(!srchdl)
            SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid source ID %u", *sources);
        ++sources;
    }

    VoiceChange *tail{}, *cur{};
    for(ALsource *source : srchandles)
    {
        if(Voice *voice{GetSourceVoice(source, context.get())})
        {
            if(!cur)
                cur = tail = GetVoiceChanger(context.get());
            else
            {
                cur->mNext.store(GetVoiceChanger(context.get()), std::memory_order_relaxed);
                cur = cur->mNext.load(std::memory_order_relaxed);
            }
            voice->mPendingChange.store(true, std::memory_order_relaxed);
            cur->mVoice = voice;
            cur->mSourceID = source->id;
            cur->mState = VChangeState::Stop;
            source->state = AL_STOPPED;
        }
        source->Offset = 0.0;
        source->OffsetType = AL_NONE;
        source->VoiceIdx = INVALID_VOICE_IDX;
    }
    if LIKELY(tail)
        SendVoiceChanges(context.get(), tail);
}
END_API_FUNC


AL_API void AL_APIENTRY alSourceRewind(ALuint source)
START_API_FUNC
{ alSourceRewindv(1, &source); }
END_API_FUNC

AL_API void AL_APIENTRY alSourceRewindv(ALsizei n, const ALuint *sources)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Rewinding %d sources", n);
    if UNLIKELY(n <= 0) return;

    al::vector<ALsource*> extra_sources;
    std::array<ALsource*,8> source_storage;
    al::span<ALsource*> srchandles;
    if LIKELY(static_cast<ALuint>(n) <= source_storage.size())
        srchandles = {source_storage.data(), static_cast<ALuint>(n)};
    else
    {
        extra_sources.resize(static_cast<ALuint>(n));
        srchandles = {extra_sources.data(), extra_sources.size()};
    }

    std::lock_guard<std::mutex> _{context->mSourceLock};
    for(auto &srchdl : srchandles)
    {
        srchdl = LookupSource(context.get(), *sources);
        if(!srchdl)
            SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid source ID %u", *sources);
        ++sources;
    }

    VoiceChange *tail{}, *cur{};
    for(ALsource *source : srchandles)
    {
        Voice *voice{GetSourceVoice(source, context.get())};
        if(source->state != AL_INITIAL)
        {
            if(!cur)
                cur = tail = GetVoiceChanger(context.get());
            else
            {
                cur->mNext.store(GetVoiceChanger(context.get()), std::memory_order_relaxed);
                cur = cur->mNext.load(std::memory_order_relaxed);
            }
            if(voice)
                voice->mPendingChange.store(true, std::memory_order_relaxed);
            cur->mVoice = voice;
            cur->mSourceID = source->id;
            cur->mState = VChangeState::Reset;
            source->state = AL_INITIAL;
        }
        source->Offset = 0.0;
        source->OffsetType = AL_NONE;
        source->VoiceIdx = INVALID_VOICE_IDX;
    }
    if LIKELY(tail)
        SendVoiceChanges(context.get(), tail);
}
END_API_FUNC


AL_API void AL_APIENTRY alSourceQueueBuffers(ALuint src, ALsizei nb, const ALuint *buffers)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(nb < 0)
        context->setError(AL_INVALID_VALUE, "Queueing %d buffers", nb);
    if UNLIKELY(nb <= 0) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *source{LookupSource(context.get(),src)};
    if UNLIKELY(!source)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid source ID %u", src);

    /* Can't queue on a Static Source */
    if UNLIKELY(source->SourceType == AL_STATIC)
        SETERR_RETURN(context, AL_INVALID_OPERATION,, "Queueing onto static source %u", src);

    /* Check for a valid Buffer, for its frequency and format */
    ALCdevice *device{context->mALDevice.get()};
    ALbuffer *BufferFmt{nullptr};
    for(auto &item : source->mQueue)
    {
        BufferFmt = item.mBuffer;
        if(BufferFmt) break;
    }

    std::unique_lock<std::mutex> buflock{device->BufferLock};
    const size_t NewListStart{source->mQueue.size()};
    ALbufferQueueItem *BufferList{nullptr};
    for(ALsizei i{0};i < nb;i++)
    {
        bool fmt_mismatch{false};
        ALbuffer *buffer{nullptr};
        if(buffers[i] && (buffer=LookupBuffer(device, buffers[i])) == nullptr)
        {
            context->setError(AL_INVALID_NAME, "Queueing invalid buffer ID %u", buffers[i]);
            goto buffer_error;
        }
        if(buffer && buffer->mCallback)
        {
            context->setError(AL_INVALID_OPERATION, "Queueing callback buffer %u", buffers[i]);
            goto buffer_error;
        }

        source->mQueue.emplace_back();
        if(!BufferList)
            BufferList = &source->mQueue.back();
        else
        {
            auto &item = source->mQueue.back();
            BufferList->mNext.store(&item, std::memory_order_relaxed);
            BufferList = &item;
        }
        if(!buffer) continue;
        BufferList->mSampleLen = buffer->mSampleLen;
        BufferList->mLoopEnd = buffer->mSampleLen;
        BufferList->mSamples = buffer->mData.data();
        BufferList->mBuffer = buffer;
        IncrementRef(buffer->ref);

        if(buffer->MappedAccess != 0 && !(buffer->MappedAccess&AL_MAP_PERSISTENT_BIT_SOFT))
        {
            context->setError(AL_INVALID_OPERATION, "Queueing non-persistently mapped buffer %u",
                buffer->id);
            goto buffer_error;
        }

        if(BufferFmt == nullptr)
            BufferFmt = buffer;
        else
        {
            fmt_mismatch |= BufferFmt->mSampleRate != buffer->mSampleRate;
            fmt_mismatch |= BufferFmt->mChannels != buffer->mChannels;
            if(BufferFmt->isBFormat())
            {
                fmt_mismatch |= BufferFmt->mAmbiLayout != buffer->mAmbiLayout;
                fmt_mismatch |= BufferFmt->mAmbiScaling != buffer->mAmbiScaling;
            }
            fmt_mismatch |= BufferFmt->mAmbiOrder != buffer->mAmbiOrder;
            fmt_mismatch |= BufferFmt->OriginalType != buffer->OriginalType;
        }
        if UNLIKELY(fmt_mismatch)
        {
            context->setError(AL_INVALID_OPERATION, "Queueing buffer with mismatched format");

        buffer_error:
            /* A buffer failed (invalid ID or format), so unlock and release
             * each buffer we had.
             */
            auto iter = source->mQueue.begin() + ptrdiff_t(NewListStart);
            for(;iter != source->mQueue.end();++iter)
            {
                if(ALbuffer *buf{iter->mBuffer})
                    DecrementRef(buf->ref);
            }
            source->mQueue.resize(NewListStart);
            return;
        }
    }
    /* All buffers good. */
    buflock.unlock();

    /* Source is now streaming */
    source->SourceType = AL_STREAMING;

    if(NewListStart != 0)
    {
        auto iter = source->mQueue.begin() + ptrdiff_t(NewListStart);
        (iter-1)->mNext.store(std::addressof(*iter), std::memory_order_release);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSourceUnqueueBuffers(ALuint src, ALsizei nb, ALuint *buffers)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(nb < 0)
        context->setError(AL_INVALID_VALUE, "Unqueueing %d buffers", nb);
    if UNLIKELY(nb <= 0) return;

    std::lock_guard<std::mutex> _{context->mSourceLock};
    ALsource *source{LookupSource(context.get(),src)};
    if UNLIKELY(!source)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid source ID %u", src);

    if UNLIKELY(source->SourceType != AL_STREAMING)
        SETERR_RETURN(context, AL_INVALID_VALUE,, "Unqueueing from a non-streaming source %u",
            src);
    if UNLIKELY(source->Looping)
        SETERR_RETURN(context, AL_INVALID_VALUE,, "Unqueueing from looping source %u", src);

    /* Make sure enough buffers have been processed to unqueue. */
    uint processed{0u};
    if LIKELY(source->state != AL_INITIAL)
    {
        VoiceBufferItem *Current{nullptr};
        if(Voice *voice{GetSourceVoice(source, context.get())})
            Current = voice->mCurrentBuffer.load(std::memory_order_relaxed);
        for(auto &item : source->mQueue)
        {
            if(&item == Current)
                break;
            ++processed;
        }
    }
    if UNLIKELY(processed < static_cast<ALuint>(nb))
        SETERR_RETURN(context, AL_INVALID_VALUE,, "Unqueueing %d buffer%s (only %u processed)",
            nb, (nb==1)?"":"s", processed);

    do {
        auto &head = source->mQueue.front();
        if(ALbuffer *buffer{head.mBuffer})
        {
            *(buffers++) = buffer->id;
            DecrementRef(buffer->ref);
        }
        else
            *(buffers++) = 0;
        source->mQueue.pop_front();
    } while(--nb);
}
END_API_FUNC


AL_API void AL_APIENTRY alSourceQueueBufferLayersSOFT(ALuint, ALsizei, const ALuint*)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    context->setError(AL_INVALID_OPERATION, "alSourceQueueBufferLayersSOFT not supported");
}
END_API_FUNC


ALsource::ALsource()
{
    Direct.Gain = 1.0f;
    Direct.GainHF = 1.0f;
    Direct.HFReference = LOWPASSFREQREF;
    Direct.GainLF = 1.0f;
    Direct.LFReference = HIGHPASSFREQREF;
    for(auto &send : Send)
    {
        send.Slot = nullptr;
        send.Gain = 1.0f;
        send.GainHF = 1.0f;
        send.HFReference = LOWPASSFREQREF;
        send.GainLF = 1.0f;
        send.LFReference = HIGHPASSFREQREF;
    }
}

ALsource::~ALsource()
{
    for(auto &item : mQueue)
    {
        if(ALbuffer *buffer{item.mBuffer})
            DecrementRef(buffer->ref);
    }

    auto clear_send = [](ALsource::SendData &send) -> void
    { if(send.Slot) DecrementRef(send.Slot->ref); };
    std::for_each(Send.begin(), Send.end(), clear_send);
}

void UpdateAllSourceProps(ALCcontext *context)
{
    std::lock_guard<std::mutex> _{context->mSourceLock};
#ifdef ALSOFT_EAX
    if(context->has_eax())
    {
        /* If EAX is enabled, we need to go through and commit all sources' EAX
         * changes, along with updating its voice, if any.
         */
        for(auto &sublist : context->mSourceList)
        {
            uint64_t usemask{~sublist.FreeMask};
            while(usemask)
            {
                const int idx{al::countr_zero(usemask)};
                usemask &= ~(1_u64 << idx);

                ALsource *source{sublist.Sources + idx};
                source->eax_commit();

                if(Voice *voice{GetSourceVoice(source, context)})
                {
                    if(std::exchange(source->mPropsDirty, false))
                        UpdateSourceProps(source, voice, context);
                }
            }
        }
    }
    else
#endif
    {
        auto voicelist = context->getVoicesSpan();
        ALuint vidx{0u};
        for(Voice *voice : voicelist)
        {
            ALuint sid{voice->mSourceID.load(std::memory_order_acquire)};
            ALsource *source = sid ? LookupSource(context, sid) : nullptr;
            if(source && source->VoiceIdx == vidx)
            {
                if(std::exchange(source->mPropsDirty, false))
                    UpdateSourceProps(source, voice, context);
            }
            ++vidx;
        }
    }
}

SourceSubList::~SourceSubList()
{
    uint64_t usemask{~FreeMask};
    while(usemask)
    {
        const int idx{al::countr_zero(usemask)};
        usemask &= ~(1_u64 << idx);
        al::destroy_at(Sources+idx);
    }
    FreeMask = ~usemask;
    al_free(Sources);
    Sources = nullptr;
}


#ifdef ALSOFT_EAX
class EaxSourceException :
    public EaxException
{
public:
    explicit EaxSourceException(
        const char* message)
        :
        EaxException{"EAX_SOURCE", message}
    {
    }
}; // EaxSourceException


class EaxSourceActiveFxSlotsException :
    public EaxException
{
public:
    explicit EaxSourceActiveFxSlotsException(
        const char* message)
        :
        EaxException{"EAX_SOURCE_ACTIVE_FX_SLOTS", message}
    {
    }
}; // EaxSourceActiveFxSlotsException


class EaxSourceSendException :
    public EaxException
{
public:
    explicit EaxSourceSendException(
        const char* message)
        :
        EaxException{"EAX_SOURCE_SEND", message}
    {
    }
}; // EaxSourceSendException


void EaxUpdateSourceVoice(ALsource *source, ALCcontext *context)
{
    if(Voice *voice{GetSourceVoice(source, context)})
    {
        if(std::exchange(source->mPropsDirty, false))
            UpdateSourceProps(source, voice, context);
    }
}


void ALsource::eax_initialize(ALCcontext *context) noexcept
{
    assert(context);
    eax_al_context_ = context;
    eax_set_defaults();
    eax_initialize_fx_slots();

    eax_d_ = eax_;
}

void ALsource::eax_update_filters()
{
    eax_update_filters_internal();
}

void ALsource::eax_update(EaxContextSharedDirtyFlags)
{
    /* NOTE: EaxContextSharedDirtyFlags only has one flag (primary_fx_slot_id),
     * which must be true for this to be called.
     */
    if(eax_uses_primary_id_)
        eax_update_primary_fx_slot_id();
}

void ALsource::eax_commit_and_update()
{
    eax_apply_deferred();
    EaxUpdateSourceVoice(this, eax_al_context_);
}

ALsource* ALsource::eax_lookup_source(
    ALCcontext& al_context,
    ALuint source_id) noexcept
{
    return LookupSource(&al_context, source_id);
}

[[noreturn]]
void ALsource::eax_fail(
    const char* message)
{
    throw EaxSourceException{message};
}

void ALsource::eax_set_source_defaults() noexcept
{
    eax1_.fMix = EAX_REVERBMIX_USEDISTANCE;

    eax_.source.lDirect = EAXSOURCE_DEFAULTDIRECT;
    eax_.source.lDirectHF = EAXSOURCE_DEFAULTDIRECTHF;
    eax_.source.lRoom = EAXSOURCE_DEFAULTROOM;
    eax_.source.lRoomHF = EAXSOURCE_DEFAULTROOMHF;
    eax_.source.lObstruction = EAXSOURCE_DEFAULTOBSTRUCTION;
    eax_.source.flObstructionLFRatio = EAXSOURCE_DEFAULTOBSTRUCTIONLFRATIO;
    eax_.source.lOcclusion = EAXSOURCE_DEFAULTOCCLUSION;
    eax_.source.flOcclusionLFRatio = EAXSOURCE_DEFAULTOCCLUSIONLFRATIO;
    eax_.source.flOcclusionRoomRatio = EAXSOURCE_DEFAULTOCCLUSIONROOMRATIO;
    eax_.source.flOcclusionDirectRatio = EAXSOURCE_DEFAULTOCCLUSIONDIRECTRATIO;
    eax_.source.lExclusion = EAXSOURCE_DEFAULTEXCLUSION;
    eax_.source.flExclusionLFRatio = EAXSOURCE_DEFAULTEXCLUSIONLFRATIO;
    eax_.source.lOutsideVolumeHF = EAXSOURCE_DEFAULTOUTSIDEVOLUMEHF;
    eax_.source.flDopplerFactor = EAXSOURCE_DEFAULTDOPPLERFACTOR;
    eax_.source.flRolloffFactor = EAXSOURCE_DEFAULTROLLOFFFACTOR;
    eax_.source.flRoomRolloffFactor = EAXSOURCE_DEFAULTROOMROLLOFFFACTOR;
    eax_.source.flAirAbsorptionFactor = EAXSOURCE_DEFAULTAIRABSORPTIONFACTOR;
    eax_.source.ulFlags = EAXSOURCE_DEFAULTFLAGS;
    eax_.source.flMacroFXFactor = EAXSOURCE_DEFAULTMACROFXFACTOR;
}

void ALsource::eax_set_active_fx_slots_defaults() noexcept
{
    eax_.active_fx_slots = EAX50SOURCE_3DDEFAULTACTIVEFXSLOTID;
}

void ALsource::eax_set_send_defaults(EAXSOURCEALLSENDPROPERTIES& eax_send) noexcept
{
    eax_send.guidReceivingFXSlotID = EAX_NULL_GUID;
    eax_send.lSend = EAXSOURCE_DEFAULTSEND;
    eax_send.lSendHF = EAXSOURCE_DEFAULTSENDHF;
    eax_send.lOcclusion = EAXSOURCE_DEFAULTOCCLUSION;
    eax_send.flOcclusionLFRatio = EAXSOURCE_DEFAULTOCCLUSIONLFRATIO;
    eax_send.flOcclusionRoomRatio = EAXSOURCE_DEFAULTOCCLUSIONROOMRATIO;
    eax_send.flOcclusionDirectRatio = EAXSOURCE_DEFAULTOCCLUSIONDIRECTRATIO;
    eax_send.lExclusion = EAXSOURCE_DEFAULTEXCLUSION;
    eax_send.flExclusionLFRatio = EAXSOURCE_DEFAULTEXCLUSIONLFRATIO;
}

void ALsource::eax_set_sends_defaults() noexcept
{
    for (auto& eax_send : eax_.sends)
    {
        eax_set_send_defaults(eax_send);
    }
}

void ALsource::eax_set_speaker_levels_defaults() noexcept
{
    std::fill(eax_.speaker_levels.begin(), eax_.speaker_levels.end(), EAXSOURCE_DEFAULTSPEAKERLEVEL);
}

void ALsource::eax_set_defaults() noexcept
{
    eax_set_source_defaults();
    eax_set_active_fx_slots_defaults();
    eax_set_sends_defaults();
    eax_set_speaker_levels_defaults();
}

float ALsource::eax_calculate_dst_occlusion_mb(
    long src_occlusion_mb,
    float path_ratio,
    float lf_ratio) noexcept
{
    const auto ratio_1 = path_ratio + lf_ratio - 1.0F;
    const auto ratio_2 = path_ratio * lf_ratio;
    const auto ratio = (ratio_2 > ratio_1) ? ratio_2 : ratio_1;
    const auto dst_occlustion_mb = static_cast<float>(src_occlusion_mb) * ratio;

    return dst_occlustion_mb;
}

EaxAlLowPassParam ALsource::eax_create_direct_filter_param() const noexcept
{
    auto gain_mb =
        static_cast<float>(eax_.source.lDirect) +

        (static_cast<float>(eax_.source.lObstruction) * eax_.source.flObstructionLFRatio) +

        eax_calculate_dst_occlusion_mb(
            eax_.source.lOcclusion,
            eax_.source.flOcclusionDirectRatio,
            eax_.source.flOcclusionLFRatio);

    auto gain_hf_mb =
        static_cast<float>(eax_.source.lDirectHF) +

        static_cast<float>(eax_.source.lObstruction) +

        (static_cast<float>(eax_.source.lOcclusion) * eax_.source.flOcclusionDirectRatio);

    for (auto i = std::size_t{}; i < EAX_MAX_FXSLOTS; ++i)
    {
        if (eax_active_fx_slots_[i])
        {
            const auto& send = eax_.sends[i];

            gain_mb += eax_calculate_dst_occlusion_mb(
                send.lOcclusion,
                send.flOcclusionDirectRatio,
                send.flOcclusionLFRatio);

            gain_hf_mb += static_cast<float>(send.lOcclusion) * send.flOcclusionDirectRatio;
        }
    }

    const auto al_low_pass_param = EaxAlLowPassParam
    {
        level_mb_to_gain(gain_mb),
        minf(level_mb_to_gain(gain_hf_mb), 1.0f)
    };

    return al_low_pass_param;
}

EaxAlLowPassParam ALsource::eax_create_room_filter_param(
    const ALeffectslot& fx_slot,
    const EAXSOURCEALLSENDPROPERTIES& send) const noexcept
{
    const auto& fx_slot_eax = fx_slot.eax_get_eax_fx_slot();

    const auto gain_mb =
        static_cast<float>(
            eax_.source.lRoom +
            send.lSend) +

        eax_calculate_dst_occlusion_mb(
            eax_.source.lOcclusion,
            eax_.source.flOcclusionRoomRatio,
            eax_.source.flOcclusionLFRatio
        ) +

        eax_calculate_dst_occlusion_mb(
            send.lOcclusion,
            send.flOcclusionRoomRatio,
            send.flOcclusionLFRatio
        ) +

        (static_cast<float>(eax_.source.lExclusion) * eax_.source.flExclusionLFRatio) +
        (static_cast<float>(send.lExclusion) * send.flExclusionLFRatio) +

        0.0F;

    const auto gain_hf_mb =
        static_cast<float>(
            eax_.source.lRoomHF +
            send.lSendHF) +

        (static_cast<float>(fx_slot_eax.lOcclusion + eax_.source.lOcclusion) * eax_.source.flOcclusionRoomRatio) +
        (static_cast<float>(send.lOcclusion) * send.flOcclusionRoomRatio) +

        static_cast<float>(
            eax_.source.lExclusion +
            send.lExclusion) +

        0.0F;

    const auto al_low_pass_param = EaxAlLowPassParam
    {
        level_mb_to_gain(gain_mb),
        minf(level_mb_to_gain(gain_hf_mb), 1.0f)
    };

    return al_low_pass_param;
}

void ALsource::eax_set_fx_slots()
{
    eax_uses_primary_id_ = false;
    eax_has_active_fx_slots_ = false;
    eax_active_fx_slots_.fill(false);

    for(const auto& eax_active_fx_slot_id : eax_.active_fx_slots.guidActiveFXSlots)
    {
        auto fx_slot_index = EaxFxSlotIndex{};

        if(eax_active_fx_slot_id == EAX_PrimaryFXSlotID)
        {
            eax_uses_primary_id_ = true;
            fx_slot_index = eax_al_context_->eax_get_primary_fx_slot_index();
        }
        else
        {
            fx_slot_index = eax_active_fx_slot_id;
        }

        if(fx_slot_index.has_value())
        {
            eax_has_active_fx_slots_ = true;
            eax_active_fx_slots_[*fx_slot_index] = true;
        }
    }

    for(auto i = 0u;i < eax_active_fx_slots_.size();++i)
    {
        if(!eax_active_fx_slots_[i])
            eax_set_al_source_send(nullptr, i, EaxAlLowPassParam{1.0f, 1.0f});
    }
}

void ALsource::eax_initialize_fx_slots()
{
    eax_set_fx_slots();
    eax_update_filters_internal();
}

void ALsource::eax_update_direct_filter_internal()
{
    const auto& direct_param = eax_create_direct_filter_param();

    Direct.Gain = direct_param.gain;
    Direct.GainHF = direct_param.gain_hf;
    Direct.HFReference = LOWPASSFREQREF;
    Direct.GainLF = 1.0f;
    Direct.LFReference = HIGHPASSFREQREF;
    mPropsDirty = true;
}

void ALsource::eax_update_room_filters_internal()
{
    if (!eax_has_active_fx_slots_)
    {
        return;
    }

    for (auto i = 0u; i < EAX_MAX_FXSLOTS; ++i)
    {
        if (eax_active_fx_slots_[i])
        {
            auto& fx_slot = eax_al_context_->eax_get_fx_slot(static_cast<std::size_t>(i));
            const auto& send = eax_.sends[i];
            const auto& room_param = eax_create_room_filter_param(fx_slot, send);

            eax_set_al_source_send(&fx_slot, i, room_param);
        }
    }
}

void ALsource::eax_update_filters_internal()
{
    eax_update_direct_filter_internal();
    eax_update_room_filters_internal();
}

void ALsource::eax_update_primary_fx_slot_id()
{
    const auto& previous_primary_fx_slot_index = eax_al_context_->eax_get_previous_primary_fx_slot_index();
    const auto& primary_fx_slot_index = eax_al_context_->eax_get_primary_fx_slot_index();

    if (previous_primary_fx_slot_index == primary_fx_slot_index)
    {
        return;
    }

    if (previous_primary_fx_slot_index.has_value())
    {
        const auto fx_slot_index = previous_primary_fx_slot_index.value();
        eax_active_fx_slots_[fx_slot_index] = false;

        eax_set_al_source_send(nullptr, fx_slot_index, EaxAlLowPassParam{1.0f, 1.0f});
    }

    if (primary_fx_slot_index.has_value())
    {
        const auto fx_slot_index = primary_fx_slot_index.value();
        eax_active_fx_slots_[fx_slot_index] = true;

        auto& fx_slot = eax_al_context_->eax_get_fx_slot(fx_slot_index);
        const auto& send = eax_.sends[fx_slot_index];
        const auto& room_param = eax_create_room_filter_param(fx_slot, send);

        eax_set_al_source_send(&fx_slot, fx_slot_index, room_param);
    }

    eax_has_active_fx_slots_ = std::any_of(
        eax_active_fx_slots_.cbegin(),
        eax_active_fx_slots_.cend(),
        [](const auto& item)
        {
            return item;
        }
    );
}

void ALsource::eax_defer_active_fx_slots(
    const EaxEaxCall& eax_call)
{
    const auto active_fx_slots_span =
        eax_call.get_values<EaxSourceActiveFxSlotsException, const GUID>();

    const auto fx_slot_count = active_fx_slots_span.size();

    if (fx_slot_count <= 0 || fx_slot_count > EAX_MAX_FXSLOTS)
    {
        throw EaxSourceActiveFxSlotsException{"Count out of range."};
    }

    for (auto i = std::size_t{}; i < fx_slot_count; ++i)
    {
        const auto& fx_slot_guid = active_fx_slots_span[i];

        if (fx_slot_guid != EAX_NULL_GUID &&
            fx_slot_guid != EAX_PrimaryFXSlotID &&
            fx_slot_guid != EAXPROPERTYID_EAX40_FXSlot0 &&
            fx_slot_guid != EAXPROPERTYID_EAX50_FXSlot0 &&
            fx_slot_guid != EAXPROPERTYID_EAX40_FXSlot1 &&
            fx_slot_guid != EAXPROPERTYID_EAX50_FXSlot1 &&
            fx_slot_guid != EAXPROPERTYID_EAX40_FXSlot2 &&
            fx_slot_guid != EAXPROPERTYID_EAX50_FXSlot2 &&
            fx_slot_guid != EAXPROPERTYID_EAX40_FXSlot3 &&
            fx_slot_guid != EAXPROPERTYID_EAX50_FXSlot3)
        {
            throw EaxSourceActiveFxSlotsException{"Unsupported GUID."};
        }
    }

    for (auto i = std::size_t{}; i < fx_slot_count; ++i)
    {
        eax_d_.active_fx_slots.guidActiveFXSlots[i] = active_fx_slots_span[i];
    }

    for (auto i = fx_slot_count; i < EAX_MAX_FXSLOTS; ++i)
    {
        eax_d_.active_fx_slots.guidActiveFXSlots[i] = EAX_NULL_GUID;
    }

    eax_are_active_fx_slots_dirty_ = (eax_d_.active_fx_slots != eax_.active_fx_slots);
}


const char* ALsource::eax_get_exclusion_name() noexcept
{
    return "Exclusion";
}

const char* ALsource::eax_get_exclusion_lf_ratio_name() noexcept
{
    return "Exclusion LF Ratio";
}

const char* ALsource::eax_get_occlusion_name() noexcept
{
    return "Occlusion";
}

const char* ALsource::eax_get_occlusion_lf_ratio_name() noexcept
{
    return "Occlusion LF Ratio";
}

const char* ALsource::eax_get_occlusion_direct_ratio_name() noexcept
{
    return "Occlusion Direct Ratio";
}

const char* ALsource::eax_get_occlusion_room_ratio_name() noexcept
{
    return "Occlusion Room Ratio";
}

void ALsource::eax1_validate_reverb_mix(float reverb_mix)
{
    if (reverb_mix == EAX_REVERBMIX_USEDISTANCE)
        return;

    eax_validate_range<EaxSourceSendException>("Reverb Mix", reverb_mix, EAX_BUFFER_MINREVERBMIX, EAX_BUFFER_MAXREVERBMIX);
}

void ALsource::eax_validate_send_receiving_fx_slot_guid(
    const GUID& guidReceivingFXSlotID)
{
    if (guidReceivingFXSlotID != EAXPROPERTYID_EAX40_FXSlot0 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX50_FXSlot0 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX40_FXSlot1 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX50_FXSlot1 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX40_FXSlot2 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX50_FXSlot2 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX40_FXSlot3 &&
        guidReceivingFXSlotID != EAXPROPERTYID_EAX50_FXSlot3)
    {
        throw EaxSourceSendException{"Unsupported receiving FX slot GUID."};
    }
}

void ALsource::eax_validate_send_send(
    long lSend)
{
    eax_validate_range<EaxSourceSendException>(
        "Send",
        lSend,
        EAXSOURCE_MINSEND,
        EAXSOURCE_MAXSEND);
}

void ALsource::eax_validate_send_send_hf(
    long lSendHF)
{
    eax_validate_range<EaxSourceSendException>(
        "Send HF",
        lSendHF,
        EAXSOURCE_MINSENDHF,
        EAXSOURCE_MAXSENDHF);
}

void ALsource::eax_validate_send_occlusion(
    long lOcclusion)
{
    eax_validate_range<EaxSourceSendException>(
        eax_get_occlusion_name(),
        lOcclusion,
        EAXSOURCE_MINOCCLUSION,
        EAXSOURCE_MAXOCCLUSION);
}

void ALsource::eax_validate_send_occlusion_lf_ratio(
    float flOcclusionLFRatio)
{
    eax_validate_range<EaxSourceSendException>(
        eax_get_occlusion_lf_ratio_name(),
        flOcclusionLFRatio,
        EAXSOURCE_MINOCCLUSIONLFRATIO,
        EAXSOURCE_MAXOCCLUSIONLFRATIO);
}

void ALsource::eax_validate_send_occlusion_room_ratio(
    float flOcclusionRoomRatio)
{
    eax_validate_range<EaxSourceSendException>(
        eax_get_occlusion_room_ratio_name(),
        flOcclusionRoomRatio,
        EAXSOURCE_MINOCCLUSIONROOMRATIO,
        EAXSOURCE_MAXOCCLUSIONROOMRATIO);
}

void ALsource::eax_validate_send_occlusion_direct_ratio(
    float flOcclusionDirectRatio)
{
    eax_validate_range<EaxSourceSendException>(
        eax_get_occlusion_direct_ratio_name(),
        flOcclusionDirectRatio,
        EAXSOURCE_MINOCCLUSIONDIRECTRATIO,
        EAXSOURCE_MAXOCCLUSIONDIRECTRATIO);
}

void ALsource::eax_validate_send_exclusion(
    long lExclusion)
{
    eax_validate_range<EaxSourceSendException>(
        eax_get_exclusion_name(),
        lExclusion,
        EAXSOURCE_MINEXCLUSION,
        EAXSOURCE_MAXEXCLUSION);
}

void ALsource::eax_validate_send_exclusion_lf_ratio(
    float flExclusionLFRatio)
{
    eax_validate_range<EaxSourceSendException>(
        eax_get_exclusion_lf_ratio_name(),
        flExclusionLFRatio,
        EAXSOURCE_MINEXCLUSIONLFRATIO,
        EAXSOURCE_MAXEXCLUSIONLFRATIO);
}

void ALsource::eax_validate_send(
    const EAXSOURCESENDPROPERTIES& all)
{
    eax_validate_send_receiving_fx_slot_guid(all.guidReceivingFXSlotID);
    eax_validate_send_send(all.lSend);
    eax_validate_send_send_hf(all.lSendHF);
}

void ALsource::eax_validate_send_exclusion_all(
    const EAXSOURCEEXCLUSIONSENDPROPERTIES& all)
{
    eax_validate_send_receiving_fx_slot_guid(all.guidReceivingFXSlotID);
    eax_validate_send_exclusion(all.lExclusion);
    eax_validate_send_exclusion_lf_ratio(all.flExclusionLFRatio);
}

void ALsource::eax_validate_send_occlusion_all(
    const EAXSOURCEOCCLUSIONSENDPROPERTIES& all)
{
    eax_validate_send_receiving_fx_slot_guid(all.guidReceivingFXSlotID);
    eax_validate_send_occlusion(all.lOcclusion);
    eax_validate_send_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_validate_send_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_validate_send_occlusion_direct_ratio(all.flOcclusionDirectRatio);
}

void ALsource::eax_validate_send_all(
    const EAXSOURCEALLSENDPROPERTIES& all)
{
    eax_validate_send_receiving_fx_slot_guid(all.guidReceivingFXSlotID);
    eax_validate_send_send(all.lSend);
    eax_validate_send_send_hf(all.lSendHF);
    eax_validate_send_occlusion(all.lOcclusion);
    eax_validate_send_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_validate_send_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_validate_send_occlusion_direct_ratio(all.flOcclusionDirectRatio);
    eax_validate_send_exclusion(all.lExclusion);
    eax_validate_send_exclusion_lf_ratio(all.flExclusionLFRatio);
}

EaxFxSlotIndexValue ALsource::eax_get_send_index(
    const GUID& send_guid)
{
    if (false)
    {
    }
    else if (send_guid == EAXPROPERTYID_EAX40_FXSlot0 || send_guid == EAXPROPERTYID_EAX50_FXSlot0)
    {
        return 0;
    }
    else if (send_guid == EAXPROPERTYID_EAX40_FXSlot1 || send_guid == EAXPROPERTYID_EAX50_FXSlot1)
    {
        return 1;
    }
    else if (send_guid == EAXPROPERTYID_EAX40_FXSlot2 || send_guid == EAXPROPERTYID_EAX50_FXSlot2)
    {
        return 2;
    }
    else if (send_guid == EAXPROPERTYID_EAX40_FXSlot3 || send_guid == EAXPROPERTYID_EAX50_FXSlot3)
    {
        return 3;
    }
    else
    {
        throw EaxSourceSendException{"Unsupported receiving FX slot GUID."};
    }
}

void ALsource::eax_defer_send_send(
    long lSend,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].lSend = lSend;

    eax_sends_dirty_flags_.sends[index].lSend =
        (eax_.sends[index].lSend != eax_d_.sends[index].lSend);
}

void ALsource::eax_defer_send_send_hf(
    long lSendHF,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].lSendHF = lSendHF;

    eax_sends_dirty_flags_.sends[index].lSendHF =
        (eax_.sends[index].lSendHF != eax_d_.sends[index].lSendHF);
}

void ALsource::eax_defer_send_occlusion(
    long lOcclusion,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].lOcclusion = lOcclusion;

    eax_sends_dirty_flags_.sends[index].lOcclusion =
        (eax_.sends[index].lOcclusion != eax_d_.sends[index].lOcclusion);
}

void ALsource::eax_defer_send_occlusion_lf_ratio(
    float flOcclusionLFRatio,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].flOcclusionLFRatio = flOcclusionLFRatio;

    eax_sends_dirty_flags_.sends[index].flOcclusionLFRatio =
        (eax_.sends[index].flOcclusionLFRatio != eax_d_.sends[index].flOcclusionLFRatio);
}

void ALsource::eax_defer_send_occlusion_room_ratio(
    float flOcclusionRoomRatio,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].flOcclusionRoomRatio = flOcclusionRoomRatio;

    eax_sends_dirty_flags_.sends[index].flOcclusionRoomRatio =
        (eax_.sends[index].flOcclusionRoomRatio != eax_d_.sends[index].flOcclusionRoomRatio);
}

void ALsource::eax_defer_send_occlusion_direct_ratio(
    float flOcclusionDirectRatio,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].flOcclusionDirectRatio = flOcclusionDirectRatio;

    eax_sends_dirty_flags_.sends[index].flOcclusionDirectRatio =
        (eax_.sends[index].flOcclusionDirectRatio != eax_d_.sends[index].flOcclusionDirectRatio);
}

void ALsource::eax_defer_send_exclusion(
    long lExclusion,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].lExclusion = lExclusion;

    eax_sends_dirty_flags_.sends[index].lExclusion =
        (eax_.sends[index].lExclusion != eax_d_.sends[index].lExclusion);
}

void ALsource::eax_defer_send_exclusion_lf_ratio(
    float flExclusionLFRatio,
    EaxFxSlotIndexValue index)
{
    eax_d_.sends[index].flExclusionLFRatio = flExclusionLFRatio;

    eax_sends_dirty_flags_.sends[index].flExclusionLFRatio =
        (eax_.sends[index].flExclusionLFRatio != eax_d_.sends[index].flExclusionLFRatio);
}

void ALsource::eax_defer_send(
    const EAXSOURCESENDPROPERTIES& all,
    EaxFxSlotIndexValue index)
{
    eax_defer_send_send(all.lSend, index);
    eax_defer_send_send_hf(all.lSendHF, index);
}

void ALsource::eax_defer_send_exclusion_all(
    const EAXSOURCEEXCLUSIONSENDPROPERTIES& all,
    EaxFxSlotIndexValue index)
{
    eax_defer_send_exclusion(all.lExclusion, index);
    eax_defer_send_exclusion_lf_ratio(all.flExclusionLFRatio, index);
}

void ALsource::eax_defer_send_occlusion_all(
    const EAXSOURCEOCCLUSIONSENDPROPERTIES& all,
    EaxFxSlotIndexValue index)
{
    eax_defer_send_occlusion(all.lOcclusion, index);
    eax_defer_send_occlusion_lf_ratio(all.flOcclusionLFRatio, index);
    eax_defer_send_occlusion_room_ratio(all.flOcclusionRoomRatio, index);
    eax_defer_send_occlusion_direct_ratio(all.flOcclusionDirectRatio, index);
}

void ALsource::eax_defer_send_all(
    const EAXSOURCEALLSENDPROPERTIES& all,
    EaxFxSlotIndexValue index)
{
    eax_defer_send_send(all.lSend, index);
    eax_defer_send_send_hf(all.lSendHF, index);
    eax_defer_send_occlusion(all.lOcclusion, index);
    eax_defer_send_occlusion_lf_ratio(all.flOcclusionLFRatio, index);
    eax_defer_send_occlusion_room_ratio(all.flOcclusionRoomRatio, index);
    eax_defer_send_occlusion_direct_ratio(all.flOcclusionDirectRatio, index);
    eax_defer_send_exclusion(all.lExclusion, index);
    eax_defer_send_exclusion_lf_ratio(all.flExclusionLFRatio, index);
}

void ALsource::eax_defer_send(
    const EaxEaxCall& eax_call)
{
    const auto eax_all_span =
        eax_call.get_values<EaxSourceException, const EAXSOURCESENDPROPERTIES>();

    const auto count = eax_all_span.size();

    if (count <= 0 || count > EAX_MAX_FXSLOTS)
    {
        throw EaxSourceSendException{"Send count out of range."};
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        eax_validate_send(all);
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        const auto send_index = eax_get_send_index(all.guidReceivingFXSlotID);
        eax_defer_send(all, send_index);
    }
}

void ALsource::eax_defer_send_exclusion_all(
    const EaxEaxCall& eax_call)
{
    const auto eax_all_span =
        eax_call.get_values<EaxSourceException, const EAXSOURCEEXCLUSIONSENDPROPERTIES>();

    const auto count = eax_all_span.size();

    if (count <= 0 || count > EAX_MAX_FXSLOTS)
    {
        throw EaxSourceSendException{"Send exclusion all count out of range."};
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        eax_validate_send_exclusion_all(all);
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        const auto send_index = eax_get_send_index(all.guidReceivingFXSlotID);
        eax_defer_send_exclusion_all(all, send_index);
    }
}

void ALsource::eax_defer_send_occlusion_all(
    const EaxEaxCall& eax_call)
{
    const auto eax_all_span =
        eax_call.get_values<EaxSourceException, const EAXSOURCEOCCLUSIONSENDPROPERTIES>();

    const auto count = eax_all_span.size();

    if (count <= 0 || count > EAX_MAX_FXSLOTS)
    {
        throw EaxSourceSendException{"Send occlusion all count out of range."};
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        eax_validate_send_occlusion_all(all);
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        const auto send_index = eax_get_send_index(all.guidReceivingFXSlotID);
        eax_defer_send_occlusion_all(all, send_index);
    }
}

void ALsource::eax_defer_send_all(
    const EaxEaxCall& eax_call)
{
    const auto eax_all_span =
        eax_call.get_values<EaxSourceException, const EAXSOURCEALLSENDPROPERTIES>();

    const auto count = eax_all_span.size();

    if (count <= 0 || count > EAX_MAX_FXSLOTS)
    {
        throw EaxSourceSendException{"Send all count out of range."};
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        eax_validate_send_all(all);
    }

    for (auto i = std::size_t{}; i < count; ++i)
    {
        const auto& all = eax_all_span[i];
        const auto send_index = eax_get_send_index(all.guidReceivingFXSlotID);
        eax_defer_send_all(all, send_index);
    }
}


void ALsource::eax_validate_source_direct(
    long direct)
{
    eax_validate_range<EaxSourceException>(
        "Direct",
        direct,
        EAXSOURCE_MINDIRECT,
        EAXSOURCE_MAXDIRECT);
}

void ALsource::eax_validate_source_direct_hf(
    long direct_hf)
{
    eax_validate_range<EaxSourceException>(
        "Direct HF",
        direct_hf,
        EAXSOURCE_MINDIRECTHF,
        EAXSOURCE_MAXDIRECTHF);
}

void ALsource::eax_validate_source_room(
    long room)
{
    eax_validate_range<EaxSourceException>(
        "Room",
        room,
        EAXSOURCE_MINROOM,
        EAXSOURCE_MAXROOM);
}

void ALsource::eax_validate_source_room_hf(
    long room_hf)
{
    eax_validate_range<EaxSourceException>(
        "Room HF",
        room_hf,
        EAXSOURCE_MINROOMHF,
        EAXSOURCE_MAXROOMHF);
}

void ALsource::eax_validate_source_obstruction(
    long obstruction)
{
    eax_validate_range<EaxSourceException>(
        "Obstruction",
        obstruction,
        EAXSOURCE_MINOBSTRUCTION,
        EAXSOURCE_MAXOBSTRUCTION);
}

void ALsource::eax_validate_source_obstruction_lf_ratio(
    float obstruction_lf_ratio)
{
    eax_validate_range<EaxSourceException>(
        "Obstruction LF Ratio",
        obstruction_lf_ratio,
        EAXSOURCE_MINOBSTRUCTIONLFRATIO,
        EAXSOURCE_MAXOBSTRUCTIONLFRATIO);
}

void ALsource::eax_validate_source_occlusion(
    long occlusion)
{
    eax_validate_range<EaxSourceException>(
        eax_get_occlusion_name(),
        occlusion,
        EAXSOURCE_MINOCCLUSION,
        EAXSOURCE_MAXOCCLUSION);
}

void ALsource::eax_validate_source_occlusion_lf_ratio(
    float occlusion_lf_ratio)
{
    eax_validate_range<EaxSourceException>(
        eax_get_occlusion_lf_ratio_name(),
        occlusion_lf_ratio,
        EAXSOURCE_MINOCCLUSIONLFRATIO,
        EAXSOURCE_MAXOCCLUSIONLFRATIO);
}

void ALsource::eax_validate_source_occlusion_room_ratio(
    float occlusion_room_ratio)
{
    eax_validate_range<EaxSourceException>(
        eax_get_occlusion_room_ratio_name(),
        occlusion_room_ratio,
        EAXSOURCE_MINOCCLUSIONROOMRATIO,
        EAXSOURCE_MAXOCCLUSIONROOMRATIO);
}

void ALsource::eax_validate_source_occlusion_direct_ratio(
    float occlusion_direct_ratio)
{
    eax_validate_range<EaxSourceException>(
        eax_get_occlusion_direct_ratio_name(),
        occlusion_direct_ratio,
        EAXSOURCE_MINOCCLUSIONDIRECTRATIO,
        EAXSOURCE_MAXOCCLUSIONDIRECTRATIO);
}

void ALsource::eax_validate_source_exclusion(
    long exclusion)
{
    eax_validate_range<EaxSourceException>(
        eax_get_exclusion_name(),
        exclusion,
        EAXSOURCE_MINEXCLUSION,
        EAXSOURCE_MAXEXCLUSION);
}

void ALsource::eax_validate_source_exclusion_lf_ratio(
    float exclusion_lf_ratio)
{
    eax_validate_range<EaxSourceException>(
        eax_get_exclusion_lf_ratio_name(),
        exclusion_lf_ratio,
        EAXSOURCE_MINEXCLUSIONLFRATIO,
        EAXSOURCE_MAXEXCLUSIONLFRATIO);
}

void ALsource::eax_validate_source_outside_volume_hf(
    long outside_volume_hf)
{
    eax_validate_range<EaxSourceException>(
        "Outside Volume HF",
        outside_volume_hf,
        EAXSOURCE_MINOUTSIDEVOLUMEHF,
        EAXSOURCE_MAXOUTSIDEVOLUMEHF);
}

void ALsource::eax_validate_source_doppler_factor(
    float doppler_factor)
{
    eax_validate_range<EaxSourceException>(
        "Doppler Factor",
        doppler_factor,
        EAXSOURCE_MINDOPPLERFACTOR,
        EAXSOURCE_MAXDOPPLERFACTOR);
}

void ALsource::eax_validate_source_rolloff_factor(
    float rolloff_factor)
{
    eax_validate_range<EaxSourceException>(
        "Rolloff Factor",
        rolloff_factor,
        EAXSOURCE_MINROLLOFFFACTOR,
        EAXSOURCE_MAXROLLOFFFACTOR);
}

void ALsource::eax_validate_source_room_rolloff_factor(
    float room_rolloff_factor)
{
    eax_validate_range<EaxSourceException>(
        "Room Rolloff Factor",
        room_rolloff_factor,
        EAXSOURCE_MINROOMROLLOFFFACTOR,
        EAXSOURCE_MAXROOMROLLOFFFACTOR);
}

void ALsource::eax_validate_source_air_absorption_factor(
    float air_absorption_factor)
{
    eax_validate_range<EaxSourceException>(
        "Air Absorption Factor",
        air_absorption_factor,
        EAXSOURCE_MINAIRABSORPTIONFACTOR,
        EAXSOURCE_MAXAIRABSORPTIONFACTOR);
}

void ALsource::eax_validate_source_flags(
    unsigned long flags,
    int eax_version)
{
    eax_validate_range<EaxSourceException>(
        "Flags",
        flags,
        0UL,
        ~((eax_version == 5) ? EAX50SOURCEFLAGS_RESERVED : EAX20SOURCEFLAGS_RESERVED));
}

void ALsource::eax_validate_source_macro_fx_factor(
    float macro_fx_factor)
{
    eax_validate_range<EaxSourceException>(
        "Macro FX Factor",
        macro_fx_factor,
        EAXSOURCE_MINMACROFXFACTOR,
        EAXSOURCE_MAXMACROFXFACTOR);
}

void ALsource::eax_validate_source_2d_all(
    const EAXSOURCE2DPROPERTIES& all,
    int eax_version)
{
    eax_validate_source_direct(all.lDirect);
    eax_validate_source_direct_hf(all.lDirectHF);
    eax_validate_source_room(all.lRoom);
    eax_validate_source_room_hf(all.lRoomHF);
    eax_validate_source_flags(all.ulFlags, eax_version);
}

void ALsource::eax_validate_source_obstruction_all(
    const EAXOBSTRUCTIONPROPERTIES& all)
{
    eax_validate_source_obstruction(all.lObstruction);
    eax_validate_source_obstruction_lf_ratio(all.flObstructionLFRatio);
}

void ALsource::eax_validate_source_exclusion_all(
    const EAXEXCLUSIONPROPERTIES& all)
{
    eax_validate_source_exclusion(all.lExclusion);
    eax_validate_source_exclusion_lf_ratio(all.flExclusionLFRatio);
}

void ALsource::eax_validate_source_occlusion_all(
    const EAXOCCLUSIONPROPERTIES& all)
{
    eax_validate_source_occlusion(all.lOcclusion);
    eax_validate_source_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_validate_source_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_validate_source_occlusion_direct_ratio(all.flOcclusionDirectRatio);
}

void ALsource::eax_validate_source_all(
    const EAX20BUFFERPROPERTIES& all,
    int eax_version)
{
    eax_validate_source_direct(all.lDirect);
    eax_validate_source_direct_hf(all.lDirectHF);
    eax_validate_source_room(all.lRoom);
    eax_validate_source_room_hf(all.lRoomHF);
    eax_validate_source_obstruction(all.lObstruction);
    eax_validate_source_obstruction_lf_ratio(all.flObstructionLFRatio);
    eax_validate_source_occlusion(all.lOcclusion);
    eax_validate_source_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_validate_source_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_validate_source_outside_volume_hf(all.lOutsideVolumeHF);
    eax_validate_source_room_rolloff_factor(all.flRoomRolloffFactor);
    eax_validate_source_air_absorption_factor(all.flAirAbsorptionFactor);
    eax_validate_source_flags(all.dwFlags, eax_version);
}

void ALsource::eax_validate_source_all(
    const EAX30SOURCEPROPERTIES& all,
    int eax_version)
{
    eax_validate_source_direct(all.lDirect);
    eax_validate_source_direct_hf(all.lDirectHF);
    eax_validate_source_room(all.lRoom);
    eax_validate_source_room_hf(all.lRoomHF);
    eax_validate_source_obstruction(all.lObstruction);
    eax_validate_source_obstruction_lf_ratio(all.flObstructionLFRatio);
    eax_validate_source_occlusion(all.lOcclusion);
    eax_validate_source_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_validate_source_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_validate_source_occlusion_direct_ratio(all.flOcclusionDirectRatio);
    eax_validate_source_exclusion(all.lExclusion);
    eax_validate_source_exclusion_lf_ratio(all.flExclusionLFRatio);
    eax_validate_source_outside_volume_hf(all.lOutsideVolumeHF);
    eax_validate_source_doppler_factor(all.flDopplerFactor);
    eax_validate_source_rolloff_factor(all.flRolloffFactor);
    eax_validate_source_room_rolloff_factor(all.flRoomRolloffFactor);
    eax_validate_source_air_absorption_factor(all.flAirAbsorptionFactor);
    eax_validate_source_flags(all.ulFlags, eax_version);
}

void ALsource::eax_validate_source_all(
    const EAX50SOURCEPROPERTIES& all,
    int eax_version)
{
    eax_validate_source_all(static_cast<EAX30SOURCEPROPERTIES>(all), eax_version);
    eax_validate_source_macro_fx_factor(all.flMacroFXFactor);
}

void ALsource::eax_validate_source_speaker_id(
    long speaker_id)
{
    eax_validate_range<EaxSourceException>(
        "Speaker Id",
        speaker_id,
        static_cast<long>(EAXSPEAKER_FRONT_LEFT),
        static_cast<long>(EAXSPEAKER_LOW_FREQUENCY));
}

void ALsource::eax_validate_source_speaker_level(
    long speaker_level)
{
    eax_validate_range<EaxSourceException>(
        "Speaker Level",
        speaker_level,
        EAXSOURCE_MINSPEAKERLEVEL,
        EAXSOURCE_MAXSPEAKERLEVEL);
}

void ALsource::eax_validate_source_speaker_level_all(
    const EAXSPEAKERLEVELPROPERTIES& all)
{
    eax_validate_source_speaker_id(all.lSpeakerID);
    eax_validate_source_speaker_level(all.lLevel);
}

void ALsource::eax_defer_source_direct(
    long lDirect)
{
    eax_d_.source.lDirect = lDirect;
    eax_source_dirty_filter_flags_.lDirect = (eax_.source.lDirect != eax_d_.source.lDirect);
}

void ALsource::eax_defer_source_direct_hf(
    long lDirectHF)
{
    eax_d_.source.lDirectHF = lDirectHF;
    eax_source_dirty_filter_flags_.lDirectHF = (eax_.source.lDirectHF != eax_d_.source.lDirectHF);
}

void ALsource::eax_defer_source_room(
    long lRoom)
{
    eax_d_.source.lRoom = lRoom;
    eax_source_dirty_filter_flags_.lRoom = (eax_.source.lRoom != eax_d_.source.lRoom);
}

void ALsource::eax_defer_source_room_hf(
    long lRoomHF)
{
    eax_d_.source.lRoomHF = lRoomHF;
    eax_source_dirty_filter_flags_.lRoomHF = (eax_.source.lRoomHF != eax_d_.source.lRoomHF);
}

void ALsource::eax_defer_source_obstruction(
    long lObstruction)
{
    eax_d_.source.lObstruction = lObstruction;
    eax_source_dirty_filter_flags_.lObstruction = (eax_.source.lObstruction != eax_d_.source.lObstruction);
}

void ALsource::eax_defer_source_obstruction_lf_ratio(
    float flObstructionLFRatio)
{
    eax_d_.source.flObstructionLFRatio = flObstructionLFRatio;
    eax_source_dirty_filter_flags_.flObstructionLFRatio = (eax_.source.flObstructionLFRatio != eax_d_.source.flObstructionLFRatio);
}

void ALsource::eax_defer_source_occlusion(
    long lOcclusion)
{
    eax_d_.source.lOcclusion = lOcclusion;
    eax_source_dirty_filter_flags_.lOcclusion = (eax_.source.lOcclusion != eax_d_.source.lOcclusion);
}

void ALsource::eax_defer_source_occlusion_lf_ratio(
    float flOcclusionLFRatio)
{
    eax_d_.source.flOcclusionLFRatio = flOcclusionLFRatio;
    eax_source_dirty_filter_flags_.flOcclusionLFRatio = (eax_.source.flOcclusionLFRatio != eax_d_.source.flOcclusionLFRatio);
}

void ALsource::eax_defer_source_occlusion_room_ratio(
    float flOcclusionRoomRatio)
{
    eax_d_.source.flOcclusionRoomRatio = flOcclusionRoomRatio;
    eax_source_dirty_filter_flags_.flOcclusionRoomRatio = (eax_.source.flOcclusionRoomRatio != eax_d_.source.flOcclusionRoomRatio);
}

void ALsource::eax_defer_source_occlusion_direct_ratio(
    float flOcclusionDirectRatio)
{
    eax_d_.source.flOcclusionDirectRatio = flOcclusionDirectRatio;
    eax_source_dirty_filter_flags_.flOcclusionDirectRatio = (eax_.source.flOcclusionDirectRatio != eax_d_.source.flOcclusionDirectRatio);
}

void ALsource::eax_defer_source_exclusion(
    long lExclusion)
{
    eax_d_.source.lExclusion = lExclusion;
    eax_source_dirty_filter_flags_.lExclusion = (eax_.source.lExclusion != eax_d_.source.lExclusion);
}

void ALsource::eax_defer_source_exclusion_lf_ratio(
    float flExclusionLFRatio)
{
    eax_d_.source.flExclusionLFRatio = flExclusionLFRatio;
    eax_source_dirty_filter_flags_.flExclusionLFRatio = (eax_.source.flExclusionLFRatio != eax_d_.source.flExclusionLFRatio);
}

void ALsource::eax_defer_source_outside_volume_hf(
    long lOutsideVolumeHF)
{
    eax_d_.source.lOutsideVolumeHF = lOutsideVolumeHF;
    eax_source_dirty_misc_flags_.lOutsideVolumeHF = (eax_.source.lOutsideVolumeHF != eax_d_.source.lOutsideVolumeHF);
}

void ALsource::eax_defer_source_doppler_factor(
    float flDopplerFactor)
{
    eax_d_.source.flDopplerFactor = flDopplerFactor;
    eax_source_dirty_misc_flags_.flDopplerFactor = (eax_.source.flDopplerFactor != eax_d_.source.flDopplerFactor);
}

void ALsource::eax_defer_source_rolloff_factor(
    float flRolloffFactor)
{
    eax_d_.source.flRolloffFactor = flRolloffFactor;
    eax_source_dirty_misc_flags_.flRolloffFactor = (eax_.source.flRolloffFactor != eax_d_.source.flRolloffFactor);
}

void ALsource::eax_defer_source_room_rolloff_factor(
    float flRoomRolloffFactor)
{
    eax_d_.source.flRoomRolloffFactor = flRoomRolloffFactor;
    eax_source_dirty_misc_flags_.flRoomRolloffFactor = (eax_.source.flRoomRolloffFactor != eax_d_.source.flRoomRolloffFactor);
}

void ALsource::eax_defer_source_air_absorption_factor(
    float flAirAbsorptionFactor)
{
    eax_d_.source.flAirAbsorptionFactor = flAirAbsorptionFactor;
    eax_source_dirty_misc_flags_.flAirAbsorptionFactor = (eax_.source.flAirAbsorptionFactor != eax_d_.source.flAirAbsorptionFactor);
}

void ALsource::eax_defer_source_flags(
    unsigned long ulFlags)
{
    eax_d_.source.ulFlags = ulFlags;
    eax_source_dirty_misc_flags_.ulFlags = (eax_.source.ulFlags != eax_d_.source.ulFlags);
}

void ALsource::eax_defer_source_macro_fx_factor(
    float flMacroFXFactor)
{
    eax_d_.source.flMacroFXFactor = flMacroFXFactor;
    eax_source_dirty_misc_flags_.flMacroFXFactor = (eax_.source.flMacroFXFactor != eax_d_.source.flMacroFXFactor);
}

void ALsource::eax_defer_source_2d_all(
    const EAXSOURCE2DPROPERTIES& all)
{
    eax_defer_source_direct(all.lDirect);
    eax_defer_source_direct_hf(all.lDirectHF);
    eax_defer_source_room(all.lRoom);
    eax_defer_source_room_hf(all.lRoomHF);
    eax_defer_source_flags(all.ulFlags);
}

void ALsource::eax_defer_source_obstruction_all(
    const EAXOBSTRUCTIONPROPERTIES& all)
{
    eax_defer_source_obstruction(all.lObstruction);
    eax_defer_source_obstruction_lf_ratio(all.flObstructionLFRatio);
}

void ALsource::eax_defer_source_exclusion_all(
    const EAXEXCLUSIONPROPERTIES& all)
{
    eax_defer_source_exclusion(all.lExclusion);
    eax_defer_source_exclusion_lf_ratio(all.flExclusionLFRatio);
}

void ALsource::eax_defer_source_occlusion_all(
    const EAXOCCLUSIONPROPERTIES& all)
{
    eax_defer_source_occlusion(all.lOcclusion);
    eax_defer_source_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_defer_source_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_defer_source_occlusion_direct_ratio(all.flOcclusionDirectRatio);
}

void ALsource::eax_defer_source_all(
    const EAX20BUFFERPROPERTIES& all)
{
    eax_defer_source_direct(all.lDirect);
    eax_defer_source_direct_hf(all.lDirectHF);
    eax_defer_source_room(all.lRoom);
    eax_defer_source_room_hf(all.lRoomHF);
    eax_defer_source_obstruction(all.lObstruction);
    eax_defer_source_obstruction_lf_ratio(all.flObstructionLFRatio);
    eax_defer_source_occlusion(all.lOcclusion);
    eax_defer_source_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_defer_source_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_defer_source_outside_volume_hf(all.lOutsideVolumeHF);
    eax_defer_source_room_rolloff_factor(all.flRoomRolloffFactor);
    eax_defer_source_air_absorption_factor(all.flAirAbsorptionFactor);
    eax_defer_source_flags(all.dwFlags);
}

void ALsource::eax_defer_source_all(
    const EAX30SOURCEPROPERTIES& all)
{
    eax_defer_source_direct(all.lDirect);
    eax_defer_source_direct_hf(all.lDirectHF);
    eax_defer_source_room(all.lRoom);
    eax_defer_source_room_hf(all.lRoomHF);
    eax_defer_source_obstruction(all.lObstruction);
    eax_defer_source_obstruction_lf_ratio(all.flObstructionLFRatio);
    eax_defer_source_occlusion(all.lOcclusion);
    eax_defer_source_occlusion_lf_ratio(all.flOcclusionLFRatio);
    eax_defer_source_occlusion_room_ratio(all.flOcclusionRoomRatio);
    eax_defer_source_occlusion_direct_ratio(all.flOcclusionDirectRatio);
    eax_defer_source_exclusion(all.lExclusion);
    eax_defer_source_exclusion_lf_ratio(all.flExclusionLFRatio);
    eax_defer_source_outside_volume_hf(all.lOutsideVolumeHF);
    eax_defer_source_doppler_factor(all.flDopplerFactor);
    eax_defer_source_rolloff_factor(all.flRolloffFactor);
    eax_defer_source_room_rolloff_factor(all.flRoomRolloffFactor);
    eax_defer_source_air_absorption_factor(all.flAirAbsorptionFactor);
    eax_defer_source_flags(all.ulFlags);
}

void ALsource::eax_defer_source_all(
    const EAX50SOURCEPROPERTIES& all)
{
    eax_defer_source_all(static_cast<const EAX30SOURCEPROPERTIES&>(all));
    eax_defer_source_macro_fx_factor(all.flMacroFXFactor);
}

void ALsource::eax_defer_source_speaker_level_all(
    const EAXSPEAKERLEVELPROPERTIES& all)
{
    const auto speaker_index = static_cast<std::size_t>(all.lSpeakerID - 1);
    auto& speaker_level_d = eax_d_.speaker_levels[speaker_index];
    const auto& speaker_level = eax_.speaker_levels[speaker_index];

    if (speaker_level != speaker_level_d)
    {
        eax_source_dirty_misc_flags_.speaker_levels = true;
    }
}

void ALsource::eax1_set_efx()
{
    const auto primary_fx_slot_index = eax_al_context_->eax_get_primary_fx_slot_index();

    if (!primary_fx_slot_index.has_value())
        return;

    WetGainAuto = (eax1_.fMix == EAX_REVERBMIX_USEDISTANCE);
    const auto filter_gain = (WetGainAuto ? 1.0F : eax1_.fMix);
    auto& fx_slot = eax_al_context_->eax_get_fx_slot(*primary_fx_slot_index);
    eax_set_al_source_send(&fx_slot, *primary_fx_slot_index, EaxAlLowPassParam{filter_gain, 1.0F});
    mPropsDirty = true;
}

void ALsource::eax1_set_reverb_mix(const EaxEaxCall& eax_call)
{
    const auto reverb_mix = eax_call.get_value<EaxSourceException, const decltype(EAXBUFFER_REVERBPROPERTIES::fMix)>();
    eax1_validate_reverb_mix(reverb_mix);

    if (eax1_.fMix == reverb_mix)
        return;

    eax1_.fMix = reverb_mix;
    eax1_set_efx();
}

void ALsource::eax_defer_source_direct(
    const EaxEaxCall& eax_call)
{
    const auto direct =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lDirect)>();

    eax_validate_source_direct(direct);
    eax_defer_source_direct(direct);
}

void ALsource::eax_defer_source_direct_hf(
    const EaxEaxCall& eax_call)
{
    const auto direct_hf =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lDirectHF)>();

    eax_validate_source_direct_hf(direct_hf);
    eax_defer_source_direct_hf(direct_hf);
}

void ALsource::eax_defer_source_room(
    const EaxEaxCall& eax_call)
{
    const auto room =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lRoom)>();

    eax_validate_source_room(room);
    eax_defer_source_room(room);
}

void ALsource::eax_defer_source_room_hf(
    const EaxEaxCall& eax_call)
{
    const auto room_hf =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lRoomHF)>();

    eax_validate_source_room_hf(room_hf);
    eax_defer_source_room_hf(room_hf);
}

void ALsource::eax_defer_source_obstruction(
    const EaxEaxCall& eax_call)
{
    const auto obstruction =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lObstruction)>();

    eax_validate_source_obstruction(obstruction);
    eax_defer_source_obstruction(obstruction);
}

void ALsource::eax_defer_source_obstruction_lf_ratio(
    const EaxEaxCall& eax_call)
{
    const auto obstruction_lf_ratio =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flObstructionLFRatio)>();

    eax_validate_source_obstruction_lf_ratio(obstruction_lf_ratio);
    eax_defer_source_obstruction_lf_ratio(obstruction_lf_ratio);
}

void ALsource::eax_defer_source_occlusion(
    const EaxEaxCall& eax_call)
{
    const auto occlusion =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lOcclusion)>();

    eax_validate_source_occlusion(occlusion);
    eax_defer_source_occlusion(occlusion);
}

void ALsource::eax_defer_source_occlusion_lf_ratio(
    const EaxEaxCall& eax_call)
{
    const auto occlusion_lf_ratio =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flOcclusionLFRatio)>();

    eax_validate_source_occlusion_lf_ratio(occlusion_lf_ratio);
    eax_defer_source_occlusion_lf_ratio(occlusion_lf_ratio);
}

void ALsource::eax_defer_source_occlusion_room_ratio(
    const EaxEaxCall& eax_call)
{
    const auto occlusion_room_ratio =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flOcclusionRoomRatio)>();

    eax_validate_source_occlusion_room_ratio(occlusion_room_ratio);
    eax_defer_source_occlusion_room_ratio(occlusion_room_ratio);
}

void ALsource::eax_defer_source_occlusion_direct_ratio(
    const EaxEaxCall& eax_call)
{
    const auto occlusion_direct_ratio =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flOcclusionDirectRatio)>();

    eax_validate_source_occlusion_direct_ratio(occlusion_direct_ratio);
    eax_defer_source_occlusion_direct_ratio(occlusion_direct_ratio);
}

void ALsource::eax_defer_source_exclusion(
    const EaxEaxCall& eax_call)
{
    const auto exclusion =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lExclusion)>();

    eax_validate_source_exclusion(exclusion);
    eax_defer_source_exclusion(exclusion);
}

void ALsource::eax_defer_source_exclusion_lf_ratio(
    const EaxEaxCall& eax_call)
{
    const auto exclusion_lf_ratio =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flExclusionLFRatio)>();

    eax_validate_source_exclusion_lf_ratio(exclusion_lf_ratio);
    eax_defer_source_exclusion_lf_ratio(exclusion_lf_ratio);
}

void ALsource::eax_defer_source_outside_volume_hf(
    const EaxEaxCall& eax_call)
{
    const auto outside_volume_hf =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::lOutsideVolumeHF)>();

    eax_validate_source_outside_volume_hf(outside_volume_hf);
    eax_defer_source_outside_volume_hf(outside_volume_hf);
}

void ALsource::eax_defer_source_doppler_factor(
    const EaxEaxCall& eax_call)
{
    const auto doppler_factor =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flDopplerFactor)>();

    eax_validate_source_doppler_factor(doppler_factor);
    eax_defer_source_doppler_factor(doppler_factor);
}

void ALsource::eax_defer_source_rolloff_factor(
    const EaxEaxCall& eax_call)
{
    const auto rolloff_factor =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flRolloffFactor)>();

    eax_validate_source_rolloff_factor(rolloff_factor);
    eax_defer_source_rolloff_factor(rolloff_factor);
}

void ALsource::eax_defer_source_room_rolloff_factor(
    const EaxEaxCall& eax_call)
{
    const auto room_rolloff_factor =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flRoomRolloffFactor)>();

    eax_validate_source_room_rolloff_factor(room_rolloff_factor);
    eax_defer_source_room_rolloff_factor(room_rolloff_factor);
}

void ALsource::eax_defer_source_air_absorption_factor(
    const EaxEaxCall& eax_call)
{
    const auto air_absorption_factor =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::flAirAbsorptionFactor)>();

    eax_validate_source_air_absorption_factor(air_absorption_factor);
    eax_defer_source_air_absorption_factor(air_absorption_factor);
}

void ALsource::eax_defer_source_flags(
    const EaxEaxCall& eax_call)
{
    const auto flags =
        eax_call.get_value<EaxSourceException, const decltype(EAX30SOURCEPROPERTIES::ulFlags)>();

    eax_validate_source_flags(flags, eax_call.get_version());
    eax_defer_source_flags(flags);
}

void ALsource::eax_defer_source_macro_fx_factor(
    const EaxEaxCall& eax_call)
{
    const auto macro_fx_factor =
        eax_call.get_value<EaxSourceException, const decltype(EAX50SOURCEPROPERTIES::flMacroFXFactor)>();

    eax_validate_source_macro_fx_factor(macro_fx_factor);
    eax_defer_source_macro_fx_factor(macro_fx_factor);
}

void ALsource::eax_defer_source_2d_all(
    const EaxEaxCall& eax_call)
{
    const auto all = eax_call.get_value<EaxSourceException, const EAXSOURCE2DPROPERTIES>();

    eax_validate_source_2d_all(all, eax_call.get_version());
    eax_defer_source_2d_all(all);
}

void ALsource::eax_defer_source_obstruction_all(
    const EaxEaxCall& eax_call)
{
    const auto all = eax_call.get_value<EaxSourceException, const EAXOBSTRUCTIONPROPERTIES>();

    eax_validate_source_obstruction_all(all);
    eax_defer_source_obstruction_all(all);
}

void ALsource::eax_defer_source_exclusion_all(
    const EaxEaxCall& eax_call)
{
    const auto all = eax_call.get_value<EaxSourceException, const EAXEXCLUSIONPROPERTIES>();

    eax_validate_source_exclusion_all(all);
    eax_defer_source_exclusion_all(all);
}

void ALsource::eax_defer_source_occlusion_all(
    const EaxEaxCall& eax_call)
{
    const auto all = eax_call.get_value<EaxSourceException, const EAXOCCLUSIONPROPERTIES>();

    eax_validate_source_occlusion_all(all);
    eax_defer_source_occlusion_all(all);
}

void ALsource::eax_defer_source_all(
    const EaxEaxCall& eax_call)
{
    const auto eax_version = eax_call.get_version();

    if (eax_version == 2)
    {
        const auto all = eax_call.get_value<EaxSourceException, const EAX20BUFFERPROPERTIES>();

        eax_validate_source_all(all, eax_version);
        eax_defer_source_all(all);
    }
    else if (eax_version < 5)
    {
        const auto all = eax_call.get_value<EaxSourceException, const EAX30SOURCEPROPERTIES>();

        eax_validate_source_all(all, eax_version);
        eax_defer_source_all(all);
    }
    else
    {
        const auto all = eax_call.get_value<EaxSourceException, const EAX50SOURCEPROPERTIES>();

        eax_validate_source_all(all, eax_version);
        eax_defer_source_all(all);
    }
}

void ALsource::eax_defer_source_speaker_level_all(
    const EaxEaxCall& eax_call)
{
    const auto speaker_level_properties = eax_call.get_value<EaxSourceException, const EAXSPEAKERLEVELPROPERTIES>();

    eax_validate_source_speaker_level_all(speaker_level_properties);
    eax_defer_source_speaker_level_all(speaker_level_properties);
}

void ALsource::eax_set_outside_volume_hf()
{
    const auto efx_gain_hf = clamp(
        level_mb_to_gain(static_cast<float>(eax_.source.lOutsideVolumeHF)),
        AL_MIN_CONE_OUTER_GAINHF,
        AL_MAX_CONE_OUTER_GAINHF
    );

    OuterGainHF = efx_gain_hf;
}

void ALsource::eax_set_doppler_factor()
{
    DopplerFactor = eax_.source.flDopplerFactor;
}

void ALsource::eax_set_rolloff_factor()
{
    RolloffFactor2 = eax_.source.flRolloffFactor;
}

void ALsource::eax_set_room_rolloff_factor()
{
    RoomRolloffFactor = eax_.source.flRoomRolloffFactor;
}

void ALsource::eax_set_air_absorption_factor()
{
    AirAbsorptionFactor = eax_.source.flAirAbsorptionFactor;
}

void ALsource::eax_set_direct_hf_auto_flag()
{
    const auto is_enable = (eax_.source.ulFlags & EAXSOURCEFLAGS_DIRECTHFAUTO) != 0;

    DryGainHFAuto = is_enable;
}

void ALsource::eax_set_room_auto_flag()
{
    const auto is_enable = (eax_.source.ulFlags & EAXSOURCEFLAGS_ROOMAUTO) != 0;

    WetGainAuto = is_enable;
}

void ALsource::eax_set_room_hf_auto_flag()
{
    const auto is_enable = (eax_.source.ulFlags & EAXSOURCEFLAGS_ROOMHFAUTO) != 0;

    WetGainHFAuto = is_enable;
}

void ALsource::eax_set_flags()
{
    eax_set_direct_hf_auto_flag();
    eax_set_room_auto_flag();
    eax_set_room_hf_auto_flag();
    eax_set_speaker_levels();
}

void ALsource::eax_set_macro_fx_factor()
{
    // TODO
}

void ALsource::eax_set_speaker_levels()
{
    // TODO
}

void ALsource::eax1_set(const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case DSPROPERTY_EAXBUFFER_ALL:
        case DSPROPERTY_EAXBUFFER_REVERBMIX:
            eax1_set_reverb_mix(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void ALsource::eax_apply_deferred()
{
    if (!eax_are_active_fx_slots_dirty_ &&
        eax_sends_dirty_flags_ == EaxSourceSendsDirtyFlags{} &&
        eax_source_dirty_filter_flags_ == EaxSourceSourceFilterDirtyFlags{} &&
        eax_source_dirty_misc_flags_ == EaxSourceSourceMiscDirtyFlags{})
    {
        return;
    }

    eax_ = eax_d_;

    if (eax_are_active_fx_slots_dirty_)
    {
        eax_are_active_fx_slots_dirty_ = false;
        eax_set_fx_slots();
        eax_update_filters_internal();
    }
    else if (eax_has_active_fx_slots_)
    {
        if (eax_source_dirty_filter_flags_ != EaxSourceSourceFilterDirtyFlags{})
        {
            eax_update_filters_internal();
        }
        else if (eax_sends_dirty_flags_ != EaxSourceSendsDirtyFlags{})
        {
            for (auto i = std::size_t{}; i < EAX_MAX_FXSLOTS; ++i)
            {
                if (eax_active_fx_slots_[i])
                {
                    if (eax_sends_dirty_flags_.sends[i] != EaxSourceSendDirtyFlags{})
                    {
                        eax_update_filters_internal();
                        break;
                    }
                }
            }
        }
    }

    if (eax_source_dirty_misc_flags_ != EaxSourceSourceMiscDirtyFlags{})
    {
        if (eax_source_dirty_misc_flags_.lOutsideVolumeHF)
        {
            eax_set_outside_volume_hf();
        }

        if (eax_source_dirty_misc_flags_.flDopplerFactor)
        {
            eax_set_doppler_factor();
        }

        if (eax_source_dirty_misc_flags_.flRolloffFactor)
        {
            eax_set_rolloff_factor();
        }

        if (eax_source_dirty_misc_flags_.flRoomRolloffFactor)
        {
            eax_set_room_rolloff_factor();
        }

        if (eax_source_dirty_misc_flags_.flAirAbsorptionFactor)
        {
            eax_set_air_absorption_factor();
        }

        if (eax_source_dirty_misc_flags_.ulFlags)
        {
            eax_set_flags();
        }

        if (eax_source_dirty_misc_flags_.flMacroFXFactor)
        {
            eax_set_macro_fx_factor();
        }

        mPropsDirty = true;

        eax_source_dirty_misc_flags_ = EaxSourceSourceMiscDirtyFlags{};
    }

    eax_sends_dirty_flags_ = EaxSourceSendsDirtyFlags{};
    eax_source_dirty_filter_flags_ = EaxSourceSourceFilterDirtyFlags{};
}

void ALsource::eax_set(
    const EaxEaxCall& eax_call)
{
    if (eax_call.get_version() == 1)
    {
        eax1_set(eax_call);
        return;
    }

    switch (eax_call.get_property_id())
    {
        case EAXSOURCE_NONE:
            break;

        case EAXSOURCE_ALLPARAMETERS:
            eax_defer_source_all(eax_call);
            break;

        case EAXSOURCE_OBSTRUCTIONPARAMETERS:
            eax_defer_source_obstruction_all(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONPARAMETERS:
            eax_defer_source_occlusion_all(eax_call);
            break;

        case EAXSOURCE_EXCLUSIONPARAMETERS:
            eax_defer_source_exclusion_all(eax_call);
            break;

        case EAXSOURCE_DIRECT:
            eax_defer_source_direct(eax_call);
            break;

        case EAXSOURCE_DIRECTHF:
            eax_defer_source_direct_hf(eax_call);
            break;

        case EAXSOURCE_ROOM:
            eax_defer_source_room(eax_call);
            break;

        case EAXSOURCE_ROOMHF:
            eax_defer_source_room_hf(eax_call);
            break;

        case EAXSOURCE_OBSTRUCTION:
            eax_defer_source_obstruction(eax_call);
            break;

        case EAXSOURCE_OBSTRUCTIONLFRATIO:
            eax_defer_source_obstruction_lf_ratio(eax_call);
            break;

        case EAXSOURCE_OCCLUSION:
            eax_defer_source_occlusion(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONLFRATIO:
            eax_defer_source_occlusion_lf_ratio(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONROOMRATIO:
            eax_defer_source_occlusion_room_ratio(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONDIRECTRATIO:
            eax_defer_source_occlusion_direct_ratio(eax_call);
            break;

        case EAXSOURCE_EXCLUSION:
            eax_defer_source_exclusion(eax_call);
            break;

        case EAXSOURCE_EXCLUSIONLFRATIO:
            eax_defer_source_exclusion_lf_ratio(eax_call);
            break;

        case EAXSOURCE_OUTSIDEVOLUMEHF:
            eax_defer_source_outside_volume_hf(eax_call);
            break;

        case EAXSOURCE_DOPPLERFACTOR:
            eax_defer_source_doppler_factor(eax_call);
            break;

        case EAXSOURCE_ROLLOFFFACTOR:
            eax_defer_source_rolloff_factor(eax_call);
            break;

        case EAXSOURCE_ROOMROLLOFFFACTOR:
            eax_defer_source_room_rolloff_factor(eax_call);
            break;

        case EAXSOURCE_AIRABSORPTIONFACTOR:
            eax_defer_source_air_absorption_factor(eax_call);
            break;

        case EAXSOURCE_FLAGS:
            eax_defer_source_flags(eax_call);
            break;

        case EAXSOURCE_SENDPARAMETERS:
            eax_defer_send(eax_call);
            break;

        case EAXSOURCE_ALLSENDPARAMETERS:
            eax_defer_send_all(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONSENDPARAMETERS:
            eax_defer_send_occlusion_all(eax_call);
            break;

        case EAXSOURCE_EXCLUSIONSENDPARAMETERS:
            eax_defer_send_exclusion_all(eax_call);
            break;

        case EAXSOURCE_ACTIVEFXSLOTID:
            eax_defer_active_fx_slots(eax_call);
            break;

        case EAXSOURCE_MACROFXFACTOR:
            eax_defer_source_macro_fx_factor(eax_call);
            break;

        case EAXSOURCE_SPEAKERLEVELS:
            eax_defer_source_speaker_level_all(eax_call);
            break;

        case EAXSOURCE_ALL2DPARAMETERS:
            eax_defer_source_2d_all(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

const GUID& ALsource::eax_get_send_fx_slot_guid(
    int eax_version,
    EaxFxSlotIndexValue fx_slot_index)
{
    switch (eax_version)
    {
        case 4:
            switch (fx_slot_index)
            {
                case 0:
                    return EAXPROPERTYID_EAX40_FXSlot0;

                case 1:
                    return EAXPROPERTYID_EAX40_FXSlot1;

                case 2:
                    return EAXPROPERTYID_EAX40_FXSlot2;

                case 3:
                    return EAXPROPERTYID_EAX40_FXSlot3;

                default:
                    eax_fail("FX slot index out of range.");
            }

        case 5:
            switch (fx_slot_index)
            {
                case 0:
                    return EAXPROPERTYID_EAX50_FXSlot0;

                case 1:
                    return EAXPROPERTYID_EAX50_FXSlot1;

                case 2:
                    return EAXPROPERTYID_EAX50_FXSlot2;

                case 3:
                    return EAXPROPERTYID_EAX50_FXSlot3;

                default:
                    eax_fail("FX slot index out of range.");
            }

        default:
            eax_fail("Unsupported EAX version.");
    }
}

void ALsource::eax_copy_send(
    const EAXSOURCEALLSENDPROPERTIES& src_send,
    EAXSOURCESENDPROPERTIES& dst_send)
{
    dst_send.lSend = src_send.lSend;
    dst_send.lSendHF = src_send.lSendHF;
}

void ALsource::eax_copy_send(
    const EAXSOURCEALLSENDPROPERTIES& src_send,
    EAXSOURCEALLSENDPROPERTIES& dst_send)
{
    dst_send = src_send;
}

void ALsource::eax_copy_send(
    const EAXSOURCEALLSENDPROPERTIES& src_send,
    EAXSOURCEOCCLUSIONSENDPROPERTIES& dst_send)
{
    dst_send.lOcclusion = src_send.lOcclusion;
    dst_send.flOcclusionLFRatio = src_send.flOcclusionLFRatio;
    dst_send.flOcclusionRoomRatio = src_send.flOcclusionRoomRatio;
    dst_send.flOcclusionDirectRatio = src_send.flOcclusionDirectRatio;
}

void ALsource::eax_copy_send(
    const EAXSOURCEALLSENDPROPERTIES& src_send,
    EAXSOURCEEXCLUSIONSENDPROPERTIES& dst_send)
{
    dst_send.lExclusion = src_send.lExclusion;
    dst_send.flExclusionLFRatio = src_send.flExclusionLFRatio;
}

void ALsource::eax1_get(const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case DSPROPERTY_EAXBUFFER_ALL:
        case DSPROPERTY_EAXBUFFER_REVERBMIX:
            eax_call.set_value<EaxSourceException>(eax1_);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void ALsource::eax_api_get_source_all_v2(
    const EaxEaxCall& eax_call)
{
    auto eax_2_all = EAX20BUFFERPROPERTIES{};
    eax_2_all.lDirect = eax_.source.lDirect;
    eax_2_all.lDirectHF = eax_.source.lDirectHF;
    eax_2_all.lRoom = eax_.source.lRoom;
    eax_2_all.lRoomHF = eax_.source.lRoomHF;
    eax_2_all.flRoomRolloffFactor = eax_.source.flRoomRolloffFactor;
    eax_2_all.lObstruction = eax_.source.lObstruction;
    eax_2_all.flObstructionLFRatio = eax_.source.flObstructionLFRatio;
    eax_2_all.lOcclusion = eax_.source.lOcclusion;
    eax_2_all.flOcclusionLFRatio = eax_.source.flOcclusionLFRatio;
    eax_2_all.flOcclusionRoomRatio = eax_.source.flOcclusionRoomRatio;
    eax_2_all.lOutsideVolumeHF = eax_.source.lOutsideVolumeHF;
    eax_2_all.flAirAbsorptionFactor = eax_.source.flAirAbsorptionFactor;
    eax_2_all.dwFlags = eax_.source.ulFlags;

    eax_call.set_value<EaxSourceException>(eax_2_all);
}

void ALsource::eax_api_get_source_all_v3(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<EaxSourceException>(static_cast<const EAX30SOURCEPROPERTIES&>(eax_.source));
}

void ALsource::eax_api_get_source_all_v5(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<EaxSourceException>(eax_.source);
}

void ALsource::eax_api_get_source_all(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_version())
    {
        case 2:
            eax_api_get_source_all_v2(eax_call);
            break;

        case 3:
        case 4:
            eax_api_get_source_all_v3(eax_call);
            break;

        case 5:
            eax_api_get_source_all_v5(eax_call);
            break;

        default:
            eax_fail("Unsupported EAX version.");
    }
}

void ALsource::eax_api_get_source_all_obstruction(
    const EaxEaxCall& eax_call)
{
    auto eax_obstruction_all = EAXOBSTRUCTIONPROPERTIES{};
    eax_obstruction_all.lObstruction = eax_.source.lObstruction;
    eax_obstruction_all.flObstructionLFRatio = eax_.source.flObstructionLFRatio;

    eax_call.set_value<EaxSourceException>(eax_obstruction_all);
}

void ALsource::eax_api_get_source_all_occlusion(
    const EaxEaxCall& eax_call)
{
    auto eax_occlusion_all = EAXOCCLUSIONPROPERTIES{};
    eax_occlusion_all.lOcclusion = eax_.source.lOcclusion;
    eax_occlusion_all.flOcclusionLFRatio = eax_.source.flOcclusionLFRatio;
    eax_occlusion_all.flOcclusionRoomRatio = eax_.source.flOcclusionRoomRatio;
    eax_occlusion_all.flOcclusionDirectRatio = eax_.source.flOcclusionDirectRatio;

    eax_call.set_value<EaxSourceException>(eax_occlusion_all);
}

void ALsource::eax_api_get_source_all_exclusion(
    const EaxEaxCall& eax_call)
{
    auto eax_exclusion_all = EAXEXCLUSIONPROPERTIES{};
    eax_exclusion_all.lExclusion = eax_.source.lExclusion;
    eax_exclusion_all.flExclusionLFRatio = eax_.source.flExclusionLFRatio;

    eax_call.set_value<EaxSourceException>(eax_exclusion_all);
}

void ALsource::eax_api_get_source_active_fx_slot_id(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_version())
    {
        case 4:
            {
                const auto& active_fx_slots = reinterpret_cast<const EAX40ACTIVEFXSLOTS&>(eax_.active_fx_slots);
                eax_call.set_value<EaxSourceException>(active_fx_slots);
            }
            break;

        case 5:
            {
                const auto& active_fx_slots = reinterpret_cast<const EAX50ACTIVEFXSLOTS&>(eax_.active_fx_slots);
                eax_call.set_value<EaxSourceException>(active_fx_slots);
            }
            break;

        default:
            eax_fail("Unsupported EAX version.");
    }
}

void ALsource::eax_api_get_source_all_2d(
    const EaxEaxCall& eax_call)
{
    auto eax_2d_all = EAXSOURCE2DPROPERTIES{};
    eax_2d_all.lDirect = eax_.source.lDirect;
    eax_2d_all.lDirectHF = eax_.source.lDirectHF;
    eax_2d_all.lRoom = eax_.source.lRoom;
    eax_2d_all.lRoomHF = eax_.source.lRoomHF;
    eax_2d_all.ulFlags = eax_.source.ulFlags;

    eax_call.set_value<EaxSourceException>(eax_2d_all);
}

void ALsource::eax_api_get_source_speaker_level_all(
    const EaxEaxCall& eax_call)
{
    auto& all = eax_call.get_value<EaxSourceException, EAXSPEAKERLEVELPROPERTIES>();

    eax_validate_source_speaker_id(all.lSpeakerID);
    const auto speaker_index = static_cast<std::size_t>(all.lSpeakerID - 1);
    all.lLevel = eax_.speaker_levels[speaker_index];
}

void ALsource::eax_get(
    const EaxEaxCall& eax_call)
{
    if (eax_call.get_version() == 1)
    {
        eax1_get(eax_call);
        return;
    }

    switch (eax_call.get_property_id())
    {
        case EAXSOURCE_NONE:
            break;

        case EAXSOURCE_ALLPARAMETERS:
            eax_api_get_source_all(eax_call);
            break;

        case EAXSOURCE_OBSTRUCTIONPARAMETERS:
            eax_api_get_source_all_obstruction(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONPARAMETERS:
            eax_api_get_source_all_occlusion(eax_call);
            break;

        case EAXSOURCE_EXCLUSIONPARAMETERS:
            eax_api_get_source_all_exclusion(eax_call);
            break;

        case EAXSOURCE_DIRECT:
            eax_call.set_value<EaxSourceException>(eax_.source.lDirect);
            break;

        case EAXSOURCE_DIRECTHF:
            eax_call.set_value<EaxSourceException>(eax_.source.lDirectHF);
            break;

        case EAXSOURCE_ROOM:
            eax_call.set_value<EaxSourceException>(eax_.source.lRoom);
            break;

        case EAXSOURCE_ROOMHF:
            eax_call.set_value<EaxSourceException>(eax_.source.lRoomHF);
            break;

        case EAXSOURCE_OBSTRUCTION:
            eax_call.set_value<EaxSourceException>(eax_.source.lObstruction);
            break;

        case EAXSOURCE_OBSTRUCTIONLFRATIO:
            eax_call.set_value<EaxSourceException>(eax_.source.flObstructionLFRatio);
            break;

        case EAXSOURCE_OCCLUSION:
            eax_call.set_value<EaxSourceException>(eax_.source.lOcclusion);
            break;

        case EAXSOURCE_OCCLUSIONLFRATIO:
            eax_call.set_value<EaxSourceException>(eax_.source.flOcclusionLFRatio);
            break;

        case EAXSOURCE_OCCLUSIONROOMRATIO:
            eax_call.set_value<EaxSourceException>(eax_.source.flOcclusionRoomRatio);
            break;

        case EAXSOURCE_OCCLUSIONDIRECTRATIO:
            eax_call.set_value<EaxSourceException>(eax_.source.flOcclusionDirectRatio);
            break;

        case EAXSOURCE_EXCLUSION:
            eax_call.set_value<EaxSourceException>(eax_.source.lExclusion);
            break;

        case EAXSOURCE_EXCLUSIONLFRATIO:
            eax_call.set_value<EaxSourceException>(eax_.source.flExclusionLFRatio);
            break;

        case EAXSOURCE_OUTSIDEVOLUMEHF:
            eax_call.set_value<EaxSourceException>(eax_.source.lOutsideVolumeHF);
            break;

        case EAXSOURCE_DOPPLERFACTOR:
            eax_call.set_value<EaxSourceException>(eax_.source.flDopplerFactor);
            break;

        case EAXSOURCE_ROLLOFFFACTOR:
            eax_call.set_value<EaxSourceException>(eax_.source.flRolloffFactor);
            break;

        case EAXSOURCE_ROOMROLLOFFFACTOR:
            eax_call.set_value<EaxSourceException>(eax_.source.flRoomRolloffFactor);
            break;

        case EAXSOURCE_AIRABSORPTIONFACTOR:
            eax_call.set_value<EaxSourceException>(eax_.source.flAirAbsorptionFactor);
            break;

        case EAXSOURCE_FLAGS:
            eax_call.set_value<EaxSourceException>(eax_.source.ulFlags);
            break;

        case EAXSOURCE_SENDPARAMETERS:
            eax_api_get_send_properties<EaxSourceException, EAXSOURCESENDPROPERTIES>(eax_call);
            break;

        case EAXSOURCE_ALLSENDPARAMETERS:
            eax_api_get_send_properties<EaxSourceException, EAXSOURCEALLSENDPROPERTIES>(eax_call);
            break;

        case EAXSOURCE_OCCLUSIONSENDPARAMETERS:
            eax_api_get_send_properties<EaxSourceException, EAXSOURCEOCCLUSIONSENDPROPERTIES>(eax_call);
            break;

        case EAXSOURCE_EXCLUSIONSENDPARAMETERS:
            eax_api_get_send_properties<EaxSourceException, EAXSOURCEEXCLUSIONSENDPROPERTIES>(eax_call);
            break;

        case EAXSOURCE_ACTIVEFXSLOTID:
            eax_api_get_source_active_fx_slot_id(eax_call);
            break;

        case EAXSOURCE_MACROFXFACTOR:
            eax_call.set_value<EaxSourceException>(eax_.source.flMacroFXFactor);
            break;

        case EAXSOURCE_SPEAKERLEVELS:
            eax_api_get_source_speaker_level_all(eax_call);
            break;

        case EAXSOURCE_ALL2DPARAMETERS:
            eax_api_get_source_all_2d(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void ALsource::eax_set_al_source_send(
    ALeffectslot *slot,
    size_t sendidx,
    const EaxAlLowPassParam &filter)
{
    if(sendidx >= EAX_MAX_FXSLOTS)
        return;

    auto &send = Send[sendidx];
    send.Gain = filter.gain;
    send.GainHF = filter.gain_hf;
    send.HFReference = LOWPASSFREQREF;
    send.GainLF = 1.0f;
    send.LFReference = HIGHPASSFREQREF;

    if(slot) IncrementRef(slot->ref);
    if(auto *oldslot = send.Slot)
        DecrementRef(oldslot->ref);
    send.Slot = slot;

    mPropsDirty = true;
}

#endif // ALSOFT_EAX
