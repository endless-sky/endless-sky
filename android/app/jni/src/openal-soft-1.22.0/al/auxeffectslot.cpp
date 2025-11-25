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

#include "auxeffectslot.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <thread>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/efx.h"

#include "albit.h"
#include "alc/alu.h"
#include "alc/context.h"
#include "alc/device.h"
#include "alc/inprogext.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "alspan.h"
#include "buffer.h"
#include "core/except.h"
#include "core/fpu_ctrl.h"
#include "core/logging.h"
#include "effect.h"
#include "opthelpers.h"

#ifdef ALSOFT_EAX
#include "eax_exception.h"
#include "eax_utils.h"
#endif // ALSOFT_EAX

namespace {

struct FactoryItem {
    EffectSlotType Type;
    EffectStateFactory* (&GetFactory)(void);
};
constexpr FactoryItem FactoryList[] = {
    { EffectSlotType::None, NullStateFactory_getFactory },
    { EffectSlotType::EAXReverb, ReverbStateFactory_getFactory },
    { EffectSlotType::Reverb, StdReverbStateFactory_getFactory },
    { EffectSlotType::Autowah, AutowahStateFactory_getFactory },
    { EffectSlotType::Chorus, ChorusStateFactory_getFactory },
    { EffectSlotType::Compressor, CompressorStateFactory_getFactory },
    { EffectSlotType::Distortion, DistortionStateFactory_getFactory },
    { EffectSlotType::Echo, EchoStateFactory_getFactory },
    { EffectSlotType::Equalizer, EqualizerStateFactory_getFactory },
    { EffectSlotType::Flanger, FlangerStateFactory_getFactory },
    { EffectSlotType::FrequencyShifter, FshifterStateFactory_getFactory },
    { EffectSlotType::RingModulator, ModulatorStateFactory_getFactory },
    { EffectSlotType::PitchShifter, PshifterStateFactory_getFactory },
    { EffectSlotType::VocalMorpher, VmorpherStateFactory_getFactory },
    { EffectSlotType::DedicatedDialog, DedicatedStateFactory_getFactory },
    { EffectSlotType::DedicatedLFE, DedicatedStateFactory_getFactory },
    { EffectSlotType::Convolution, ConvolutionStateFactory_getFactory },
};

EffectStateFactory *getFactoryByType(EffectSlotType type)
{
    auto iter = std::find_if(std::begin(FactoryList), std::end(FactoryList),
        [type](const FactoryItem &item) noexcept -> bool
        { return item.Type == type; });
    return (iter != std::end(FactoryList)) ? iter->GetFactory() : nullptr;
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

inline ALeffect *LookupEffect(ALCdevice *device, ALuint id) noexcept
{
    const size_t lidx{(id-1) >> 6};
    const ALuint slidx{(id-1) & 0x3f};

    if UNLIKELY(lidx >= device->EffectList.size())
        return nullptr;
    EffectSubList &sublist = device->EffectList[lidx];
    if UNLIKELY(sublist.FreeMask & (1_u64 << slidx))
        return nullptr;
    return sublist.Effects + slidx;
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


inline auto GetEffectBuffer(ALbuffer *buffer) noexcept -> EffectState::Buffer
{
    if(!buffer) return EffectState::Buffer{};
    return EffectState::Buffer{buffer, buffer->mData};
}


void AddActiveEffectSlots(const al::span<ALeffectslot*> auxslots, ALCcontext *context)
{
    if(auxslots.empty()) return;
    EffectSlotArray *curarray{context->mActiveAuxSlots.load(std::memory_order_acquire)};
    size_t newcount{curarray->size() + auxslots.size()};

    /* Insert the new effect slots into the head of the array, followed by the
     * existing ones.
     */
    EffectSlotArray *newarray = EffectSlot::CreatePtrArray(newcount);
    auto slotiter = std::transform(auxslots.begin(), auxslots.end(), newarray->begin(),
        [](ALeffectslot *auxslot) noexcept { return &auxslot->mSlot; });
    std::copy(curarray->begin(), curarray->end(), slotiter);

    /* Remove any duplicates (first instance of each will be kept). */
    auto last = newarray->end();
    for(auto start=newarray->begin()+1;;)
    {
        last = std::remove(start, last, *(start-1));
        if(start == last) break;
        ++start;
    }
    newcount = static_cast<size_t>(std::distance(newarray->begin(), last));

    /* Reallocate newarray if the new size ended up smaller from duplicate
     * removal.
     */
    if UNLIKELY(newcount < newarray->size())
    {
        curarray = newarray;
        newarray = EffectSlot::CreatePtrArray(newcount);
        std::copy_n(curarray->begin(), newcount, newarray->begin());
        delete curarray;
        curarray = nullptr;
    }
    std::uninitialized_fill_n(newarray->end(), newcount, nullptr);

    curarray = context->mActiveAuxSlots.exchange(newarray, std::memory_order_acq_rel);
    context->mDevice->waitForMix();

    al::destroy_n(curarray->end(), curarray->size());
    delete curarray;
}

void RemoveActiveEffectSlots(const al::span<ALeffectslot*> auxslots, ALCcontext *context)
{
    if(auxslots.empty()) return;
    EffectSlotArray *curarray{context->mActiveAuxSlots.load(std::memory_order_acquire)};

    /* Don't shrink the allocated array size since we don't know how many (if
     * any) of the effect slots to remove are in the array.
     */
    EffectSlotArray *newarray = EffectSlot::CreatePtrArray(curarray->size());

    auto new_end = std::copy(curarray->begin(), curarray->end(), newarray->begin());
    /* Remove elements from newarray that match any ID in slotids. */
    for(const ALeffectslot *auxslot : auxslots)
    {
        auto slot_match = [auxslot](EffectSlot *slot) noexcept -> bool
        { return (slot == &auxslot->mSlot); };
        new_end = std::remove_if(newarray->begin(), new_end, slot_match);
    }

    /* Reallocate with the new size. */
    auto newsize = static_cast<size_t>(std::distance(newarray->begin(), new_end));
    if LIKELY(newsize != newarray->size())
    {
        curarray = newarray;
        newarray = EffectSlot::CreatePtrArray(newsize);
        std::copy_n(curarray->begin(), newsize, newarray->begin());

        delete curarray;
        curarray = nullptr;
    }
    std::uninitialized_fill_n(newarray->end(), newsize, nullptr);

    curarray = context->mActiveAuxSlots.exchange(newarray, std::memory_order_acq_rel);
    context->mDevice->waitForMix();

    al::destroy_n(curarray->end(), curarray->size());
    delete curarray;
}


EffectSlotType EffectSlotTypeFromEnum(ALenum type)
{
    switch(type)
    {
    case AL_EFFECT_NULL: return EffectSlotType::None;
    case AL_EFFECT_REVERB: return EffectSlotType::Reverb;
    case AL_EFFECT_CHORUS: return EffectSlotType::Chorus;
    case AL_EFFECT_DISTORTION: return EffectSlotType::Distortion;
    case AL_EFFECT_ECHO: return EffectSlotType::Echo;
    case AL_EFFECT_FLANGER: return EffectSlotType::Flanger;
    case AL_EFFECT_FREQUENCY_SHIFTER: return EffectSlotType::FrequencyShifter;
    case AL_EFFECT_VOCAL_MORPHER: return EffectSlotType::VocalMorpher;
    case AL_EFFECT_PITCH_SHIFTER: return EffectSlotType::PitchShifter;
    case AL_EFFECT_RING_MODULATOR: return EffectSlotType::RingModulator;
    case AL_EFFECT_AUTOWAH: return EffectSlotType::Autowah;
    case AL_EFFECT_COMPRESSOR: return EffectSlotType::Compressor;
    case AL_EFFECT_EQUALIZER: return EffectSlotType::Equalizer;
    case AL_EFFECT_EAXREVERB: return EffectSlotType::EAXReverb;
    case AL_EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT: return EffectSlotType::DedicatedLFE;
    case AL_EFFECT_DEDICATED_DIALOGUE: return EffectSlotType::DedicatedDialog;
    case AL_EFFECT_CONVOLUTION_REVERB_SOFT: return EffectSlotType::Convolution;
    }
    ERR("Unhandled effect enum: 0x%04x\n", type);
    return EffectSlotType::None;
}

bool EnsureEffectSlots(ALCcontext *context, size_t needed)
{
    size_t count{std::accumulate(context->mEffectSlotList.cbegin(),
        context->mEffectSlotList.cend(), size_t{0},
        [](size_t cur, const EffectSlotSubList &sublist) noexcept -> size_t
        { return cur + static_cast<ALuint>(al::popcount(sublist.FreeMask)); })};

    while(needed > count)
    {
        if UNLIKELY(context->mEffectSlotList.size() >= 1<<25)
            return false;

        context->mEffectSlotList.emplace_back();
        auto sublist = context->mEffectSlotList.end() - 1;
        sublist->FreeMask = ~0_u64;
        sublist->EffectSlots = static_cast<ALeffectslot*>(
            al_calloc(alignof(ALeffectslot), sizeof(ALeffectslot)*64));
        if UNLIKELY(!sublist->EffectSlots)
        {
            context->mEffectSlotList.pop_back();
            return false;
        }
        count += 64;
    }
    return true;
}

ALeffectslot *AllocEffectSlot(ALCcontext *context)
{
    auto sublist = std::find_if(context->mEffectSlotList.begin(), context->mEffectSlotList.end(),
        [](const EffectSlotSubList &entry) noexcept -> bool
        { return entry.FreeMask != 0; });
    auto lidx = static_cast<ALuint>(std::distance(context->mEffectSlotList.begin(), sublist));
    auto slidx = static_cast<ALuint>(al::countr_zero(sublist->FreeMask));
    ASSUME(slidx < 64);

    ALeffectslot *slot{al::construct_at(sublist->EffectSlots + slidx)};
    aluInitEffectPanning(&slot->mSlot, context);

    /* Add 1 to avoid source ID 0. */
    slot->id = ((lidx<<6) | slidx) + 1;

    context->mNumEffectSlots += 1;
    sublist->FreeMask &= ~(1_u64 << slidx);

    return slot;
}

void FreeEffectSlot(ALCcontext *context, ALeffectslot *slot)
{
    const ALuint id{slot->id - 1};
    const size_t lidx{id >> 6};
    const ALuint slidx{id & 0x3f};

    al::destroy_at(slot);

    context->mEffectSlotList[lidx].FreeMask |= 1_u64 << slidx;
    context->mNumEffectSlots--;
}


inline void UpdateProps(ALeffectslot *slot, ALCcontext *context)
{
    if(!context->mDeferUpdates && slot->mState == SlotState::Playing)
    {
        slot->updateProps(context);
        return;
    }
    slot->mPropsDirty = true;
}

} // namespace


AL_API void AL_APIENTRY alGenAuxiliaryEffectSlots(ALsizei n, ALuint *effectslots)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Generating %d effect slots", n);
    if UNLIKELY(n <= 0) return;

    std::unique_lock<std::mutex> slotlock{context->mEffectSlotLock};
    ALCdevice *device{context->mALDevice.get()};
    if(static_cast<ALuint>(n) > device->AuxiliaryEffectSlotMax-context->mNumEffectSlots)
    {
        context->setError(AL_OUT_OF_MEMORY, "Exceeding %u effect slot limit (%u + %d)",
            device->AuxiliaryEffectSlotMax, context->mNumEffectSlots, n);
        return;
    }
    if(!EnsureEffectSlots(context.get(), static_cast<ALuint>(n)))
    {
        context->setError(AL_OUT_OF_MEMORY, "Failed to allocate %d effectslot%s", n,
            (n==1) ? "" : "s");
        return;
    }

    if(n == 1)
    {
        ALeffectslot *slot{AllocEffectSlot(context.get())};
        if(!slot) return;
        effectslots[0] = slot->id;
    }
    else
    {
        al::vector<ALuint> ids;
        ALsizei count{n};
        ids.reserve(static_cast<ALuint>(count));
        do {
            ALeffectslot *slot{AllocEffectSlot(context.get())};
            if(!slot)
            {
                slotlock.unlock();
                alDeleteAuxiliaryEffectSlots(static_cast<ALsizei>(ids.size()), ids.data());
                return;
            }
            ids.emplace_back(slot->id);
        } while(--count);
        std::copy(ids.cbegin(), ids.cend(), effectslots);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alDeleteAuxiliaryEffectSlots(ALsizei n, const ALuint *effectslots)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Deleting %d effect slots", n);
    if UNLIKELY(n <= 0) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    if(n == 1)
    {
        ALeffectslot *slot{LookupEffectSlot(context.get(), effectslots[0])};
        if UNLIKELY(!slot)
        {
            context->setError(AL_INVALID_NAME, "Invalid effect slot ID %u", effectslots[0]);
            return;
        }
        if UNLIKELY(ReadRef(slot->ref) != 0)
        {
            context->setError(AL_INVALID_OPERATION, "Deleting in-use effect slot %u",
                effectslots[0]);
            return;
        }
        RemoveActiveEffectSlots({&slot, 1u}, context.get());
        FreeEffectSlot(context.get(), slot);
    }
    else
    {
        auto slots = al::vector<ALeffectslot*>(static_cast<ALuint>(n));
        for(size_t i{0};i < slots.size();++i)
        {
            ALeffectslot *slot{LookupEffectSlot(context.get(), effectslots[i])};
            if UNLIKELY(!slot)
            {
                context->setError(AL_INVALID_NAME, "Invalid effect slot ID %u", effectslots[i]);
                return;
            }
            if UNLIKELY(ReadRef(slot->ref) != 0)
            {
                context->setError(AL_INVALID_OPERATION, "Deleting in-use effect slot %u",
                    effectslots[i]);
                return;
            }
            slots[i] = slot;
        }
        /* Remove any duplicates. */
        auto slots_end = slots.end();
        for(auto start=slots.begin()+1;start != slots_end;++start)
        {
            slots_end = std::remove(start, slots_end, *(start-1));
            if(start == slots_end) break;
        }
        slots.erase(slots_end, slots.end());

        /* All effectslots are valid, remove and delete them */
        RemoveActiveEffectSlots(slots, context.get());
        for(ALeffectslot *slot : slots)
            FreeEffectSlot(context.get(), slot);
    }
}
END_API_FUNC

AL_API ALboolean AL_APIENTRY alIsAuxiliaryEffectSlot(ALuint effectslot)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if LIKELY(context)
    {
        std::lock_guard<std::mutex> _{context->mEffectSlotLock};
        if(LookupEffectSlot(context.get(), effectslot) != nullptr)
            return AL_TRUE;
    }
    return AL_FALSE;
}
END_API_FUNC


AL_API void AL_APIENTRY alAuxiliaryEffectSlotPlaySOFT(ALuint slotid)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot{LookupEffectSlot(context.get(), slotid)};
    if UNLIKELY(!slot)
    {
        context->setError(AL_INVALID_NAME, "Invalid effect slot ID %u", slotid);
        return;
    }
    if(slot->mState == SlotState::Playing)
        return;

    slot->mPropsDirty = false;
    slot->updateProps(context.get());

    AddActiveEffectSlots({&slot, 1}, context.get());
    slot->mState = SlotState::Playing;
}
END_API_FUNC

AL_API void AL_APIENTRY alAuxiliaryEffectSlotPlayvSOFT(ALsizei n, const ALuint *slotids)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Playing %d effect slots", n);
    if UNLIKELY(n <= 0) return;

    auto slots = al::vector<ALeffectslot*>(static_cast<ALuint>(n));
    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    for(size_t i{0};i < slots.size();++i)
    {
        ALeffectslot *slot{LookupEffectSlot(context.get(), slotids[i])};
        if UNLIKELY(!slot)
        {
            context->setError(AL_INVALID_NAME, "Invalid effect slot ID %u", slotids[i]);
            return;
        }

        if(slot->mState != SlotState::Playing)
        {
            slot->mPropsDirty = false;
            slot->updateProps(context.get());
        }
        slots[i] = slot;
    };

    AddActiveEffectSlots(slots, context.get());
    for(auto slot : slots)
        slot->mState = SlotState::Playing;
}
END_API_FUNC

AL_API void AL_APIENTRY alAuxiliaryEffectSlotStopSOFT(ALuint slotid)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot{LookupEffectSlot(context.get(), slotid)};
    if UNLIKELY(!slot)
    {
        context->setError(AL_INVALID_NAME, "Invalid effect slot ID %u", slotid);
        return;
    }

    RemoveActiveEffectSlots({&slot, 1}, context.get());
    slot->mState = SlotState::Stopped;
}
END_API_FUNC

AL_API void AL_APIENTRY alAuxiliaryEffectSlotStopvSOFT(ALsizei n, const ALuint *slotids)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Stopping %d effect slots", n);
    if UNLIKELY(n <= 0) return;

    auto slots = al::vector<ALeffectslot*>(static_cast<ALuint>(n));
    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    for(size_t i{0};i < slots.size();++i)
    {
        ALeffectslot *slot{LookupEffectSlot(context.get(), slotids[i])};
        if UNLIKELY(!slot)
        {
            context->setError(AL_INVALID_NAME, "Invalid effect slot ID %u", slotids[i]);
            return;
        }

        slots[i] = slot;
    };

    RemoveActiveEffectSlots(slots, context.get());
    for(auto slot : slots)
        slot->mState = SlotState::Stopped;
}
END_API_FUNC


AL_API void AL_APIENTRY alAuxiliaryEffectSloti(ALuint effectslot, ALenum param, ALint value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    ALeffectslot *target{};
    ALCdevice *device{};
    ALenum err{};
    switch(param)
    {
    case AL_EFFECTSLOT_EFFECT:
        device = context->mALDevice.get();

        {
            std::lock_guard<std::mutex> ___{device->EffectLock};
            ALeffect *effect{value ? LookupEffect(device, static_cast<ALuint>(value)) : nullptr};
            if(effect)
                err = slot->initEffect(effect->type, effect->Props, context.get());
            else
            {
                if(value != 0)
                    SETERR_RETURN(context, AL_INVALID_VALUE,, "Invalid effect ID %u", value);
                err = slot->initEffect(AL_EFFECT_NULL, EffectProps{}, context.get());
            }
        }
        if UNLIKELY(err != AL_NO_ERROR)
        {
            context->setError(err, "Effect initialization failed");
            return;
        }
        if UNLIKELY(slot->mState == SlotState::Initial)
        {
            slot->mPropsDirty = false;
            slot->updateProps(context.get());

            AddActiveEffectSlots({&slot, 1}, context.get());
            slot->mState = SlotState::Playing;
            return;
        }
        break;

    case AL_EFFECTSLOT_AUXILIARY_SEND_AUTO:
        if(!(value == AL_TRUE || value == AL_FALSE))
            SETERR_RETURN(context, AL_INVALID_VALUE,,
                "Effect slot auxiliary send auto out of range");
        if UNLIKELY(slot->AuxSendAuto == !!value)
            return;
        slot->AuxSendAuto = !!value;
        break;

    case AL_EFFECTSLOT_TARGET_SOFT:
        target = LookupEffectSlot(context.get(), static_cast<ALuint>(value));
        if(value && !target)
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Invalid effect slot target ID");
        if UNLIKELY(slot->Target == target)
            return;
        if(target)
        {
            ALeffectslot *checker{target};
            while(checker && checker != slot)
                checker = checker->Target;
            if(checker)
                SETERR_RETURN(context, AL_INVALID_OPERATION,,
                    "Setting target of effect slot ID %u to %u creates circular chain", slot->id,
                    target->id);
        }

        if(ALeffectslot *oldtarget{slot->Target})
        {
            /* We must force an update if there was an existing effect slot
             * target, in case it's about to be deleted.
             */
            if(target) IncrementRef(target->ref);
            DecrementRef(oldtarget->ref);
            slot->Target = target;
            slot->updateProps(context.get());
            return;
        }

        if(target) IncrementRef(target->ref);
        slot->Target = target;
        break;

    case AL_BUFFER:
        device = context->mALDevice.get();

        if(slot->mState == SlotState::Playing)
            SETERR_RETURN(context, AL_INVALID_OPERATION,,
                "Setting buffer on playing effect slot %u", slot->id);

        if(ALbuffer *buffer{slot->Buffer})
        {
            if UNLIKELY(buffer->id == static_cast<ALuint>(value))
                return;
        }
        else if UNLIKELY(value == 0)
            return;

        {
            std::lock_guard<std::mutex> ___{device->BufferLock};
            ALbuffer *buffer{};
            if(value)
            {
                buffer = LookupBuffer(device, static_cast<ALuint>(value));
                if(!buffer) SETERR_RETURN(context, AL_INVALID_VALUE,, "Invalid buffer ID");
                if(buffer->mCallback)
                    SETERR_RETURN(context, AL_INVALID_OPERATION,,
                        "Callback buffer not valid for effects");

                IncrementRef(buffer->ref);
            }

            if(ALbuffer *oldbuffer{slot->Buffer})
                DecrementRef(oldbuffer->ref);
            slot->Buffer = buffer;

            FPUCtl mixer_mode{};
            auto *state = slot->Effect.State.get();
            state->deviceUpdate(device, GetEffectBuffer(buffer));
        }
        break;

    case AL_EFFECTSLOT_STATE_SOFT:
        SETERR_RETURN(context, AL_INVALID_OPERATION,, "AL_EFFECTSLOT_STATE_SOFT is read-only");

    default:
        SETERR_RETURN(context, AL_INVALID_ENUM,, "Invalid effect slot integer property 0x%04x",
            param);
    }
    UpdateProps(slot, context.get());
}
END_API_FUNC

AL_API void AL_APIENTRY alAuxiliaryEffectSlotiv(ALuint effectslot, ALenum param, const ALint *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_EFFECTSLOT_EFFECT:
    case AL_EFFECTSLOT_AUXILIARY_SEND_AUTO:
    case AL_EFFECTSLOT_TARGET_SOFT:
    case AL_EFFECTSLOT_STATE_SOFT:
    case AL_BUFFER:
        alAuxiliaryEffectSloti(effectslot, param, values[0]);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    default:
        SETERR_RETURN(context, AL_INVALID_ENUM,,
            "Invalid effect slot integer-vector property 0x%04x", param);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alAuxiliaryEffectSlotf(ALuint effectslot, ALenum param, ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    std::lock_guard<std::mutex> __{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    case AL_EFFECTSLOT_GAIN:
        if(!(value >= 0.0f && value <= 1.0f))
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Effect slot gain out of range");
        if UNLIKELY(slot->Gain == value)
            return;
        slot->Gain = value;
        break;

    default:
        SETERR_RETURN(context, AL_INVALID_ENUM,, "Invalid effect slot float property 0x%04x",
            param);
    }
    UpdateProps(slot, context.get());
}
END_API_FUNC

AL_API void AL_APIENTRY alAuxiliaryEffectSlotfv(ALuint effectslot, ALenum param, const ALfloat *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_EFFECTSLOT_GAIN:
        alAuxiliaryEffectSlotf(effectslot, param, values[0]);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    default:
        SETERR_RETURN(context, AL_INVALID_ENUM,,
            "Invalid effect slot float-vector property 0x%04x", param);
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alGetAuxiliaryEffectSloti(ALuint effectslot, ALenum param, ALint *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    case AL_EFFECTSLOT_AUXILIARY_SEND_AUTO:
        *value = slot->AuxSendAuto ? AL_TRUE : AL_FALSE;
        break;

    case AL_EFFECTSLOT_TARGET_SOFT:
        if(auto *target = slot->Target)
            *value = static_cast<ALint>(target->id);
        else
            *value = 0;
        break;

    case AL_EFFECTSLOT_STATE_SOFT:
        *value = static_cast<int>(slot->mState);
        break;

    case AL_BUFFER:
        if(auto *buffer = slot->Buffer)
            *value = static_cast<ALint>(buffer->id);
        else
            *value = 0;
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid effect slot integer property 0x%04x", param);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetAuxiliaryEffectSlotiv(ALuint effectslot, ALenum param, ALint *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_EFFECTSLOT_EFFECT:
    case AL_EFFECTSLOT_AUXILIARY_SEND_AUTO:
    case AL_EFFECTSLOT_TARGET_SOFT:
    case AL_EFFECTSLOT_STATE_SOFT:
    case AL_BUFFER:
        alGetAuxiliaryEffectSloti(effectslot, param, values);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    default:
        context->setError(AL_INVALID_ENUM, "Invalid effect slot integer-vector property 0x%04x",
            param);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetAuxiliaryEffectSlotf(ALuint effectslot, ALenum param, ALfloat *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    case AL_EFFECTSLOT_GAIN:
        *value = slot->Gain;
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid effect slot float property 0x%04x", param);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetAuxiliaryEffectSlotfv(ALuint effectslot, ALenum param, ALfloat *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_EFFECTSLOT_GAIN:
        alGetAuxiliaryEffectSlotf(effectslot, param, values);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
    ALeffectslot *slot = LookupEffectSlot(context.get(), effectslot);
    if UNLIKELY(!slot)
        SETERR_RETURN(context, AL_INVALID_NAME,, "Invalid effect slot ID %u", effectslot);

    switch(param)
    {
    default:
        context->setError(AL_INVALID_ENUM, "Invalid effect slot float-vector property 0x%04x",
            param);
    }
}
END_API_FUNC


ALeffectslot::ALeffectslot()
{
    EffectStateFactory *factory{getFactoryByType(EffectSlotType::None)};
    if(!factory) throw std::runtime_error{"Failed to get null effect factory"};

    al::intrusive_ptr<EffectState> state{factory->create()};
    Effect.State = state;
    mSlot.mEffectState = state.release();
}

ALeffectslot::~ALeffectslot()
{
    if(Target)
        DecrementRef(Target->ref);
    Target = nullptr;
    if(Buffer)
        DecrementRef(Buffer->ref);
    Buffer = nullptr;

    EffectSlotProps *props{mSlot.Update.exchange(nullptr)};
    if(props)
    {
        TRACE("Freed unapplied AuxiliaryEffectSlot update %p\n",
            decltype(std::declval<void*>()){props});
        delete props;
    }

    if(mSlot.mEffectState)
        mSlot.mEffectState->release();
}

ALenum ALeffectslot::initEffect(ALenum effectType, const EffectProps &effectProps,
    ALCcontext *context)
{
    EffectSlotType newtype{EffectSlotTypeFromEnum(effectType)};
    if(newtype != Effect.Type)
    {
        EffectStateFactory *factory{getFactoryByType(newtype)};
        if(!factory)
        {
            ERR("Failed to find factory for effect slot type %d\n", static_cast<int>(newtype));
            return AL_INVALID_ENUM;
        }
        al::intrusive_ptr<EffectState> state{factory->create()};

        ALCdevice *device{context->mALDevice.get()};
        std::unique_lock<std::mutex> statelock{device->StateLock};
        state->mOutTarget = device->Dry.Buffer;
        {
            FPUCtl mixer_mode{};
            state->deviceUpdate(device, GetEffectBuffer(Buffer));
        }

        Effect.Type = newtype;
        Effect.Props = effectProps;

        Effect.State = std::move(state);
    }
    else if(newtype != EffectSlotType::None)
        Effect.Props = effectProps;

    /* Remove state references from old effect slot property updates. */
    EffectSlotProps *props{context->mFreeEffectslotProps.load()};
    while(props)
    {
        props->State = nullptr;
        props = props->next.load(std::memory_order_relaxed);
    }

    return AL_NO_ERROR;
}

void ALeffectslot::updateProps(ALCcontext *context)
{
    /* Get an unused property container, or allocate a new one as needed. */
    EffectSlotProps *props{context->mFreeEffectslotProps.load(std::memory_order_relaxed)};
    if(!props)
        props = new EffectSlotProps{};
    else
    {
        EffectSlotProps *next;
        do {
            next = props->next.load(std::memory_order_relaxed);
        } while(context->mFreeEffectslotProps.compare_exchange_weak(props, next,
                std::memory_order_seq_cst, std::memory_order_acquire) == 0);
    }

    /* Copy in current property values. */
    props->Gain = Gain;
    props->AuxSendAuto = AuxSendAuto;
    props->Target = Target ? &Target->mSlot : nullptr;

    props->Type = Effect.Type;
    props->Props = Effect.Props;
    props->State = Effect.State;

    /* Set the new container for updating internal parameters. */
    props = mSlot.Update.exchange(props, std::memory_order_acq_rel);
    if(props)
    {
        /* If there was an unused update container, put it back in the
         * freelist.
         */
        props->State = nullptr;
        AtomicReplaceHead(context->mFreeEffectslotProps, props);
    }
}

void UpdateAllEffectSlotProps(ALCcontext *context)
{
    std::lock_guard<std::mutex> _{context->mEffectSlotLock};
#ifdef ALSOFT_EAX
    if(context->has_eax())
        context->eax_commit_fx_slots();
#endif
    for(auto &sublist : context->mEffectSlotList)
    {
        uint64_t usemask{~sublist.FreeMask};
        while(usemask)
        {
            const int idx{al::countr_zero(usemask)};
            usemask &= ~(1_u64 << idx);
            ALeffectslot *slot{sublist.EffectSlots + idx};

            if(slot->mState != SlotState::Stopped && std::exchange(slot->mPropsDirty, false))
                slot->updateProps(context);
        }
    }
}

EffectSlotSubList::~EffectSlotSubList()
{
    uint64_t usemask{~FreeMask};
    while(usemask)
    {
        const int idx{al::countr_zero(usemask)};
        al::destroy_at(EffectSlots+idx);
        usemask &= ~(1_u64 << idx);
    }
    FreeMask = ~usemask;
    al_free(EffectSlots);
    EffectSlots = nullptr;
}

#ifdef ALSOFT_EAX
namespace {

class EaxFxSlotException :
    public EaxException
{
public:
	explicit EaxFxSlotException(
		const char* message)
		:
		EaxException{"EAX_FX_SLOT", message}
	{
	}
}; // EaxFxSlotException


} // namespace


void ALeffectslot::eax_initialize(
    ALCcontext& al_context,
    EaxFxSlotIndexValue index)
{
    eax_al_context_ = &al_context;

    if (index >= EAX_MAX_FXSLOTS)
    {
        eax_fail("Index out of range.");
    }

    eax_fx_slot_index_ = index;

    eax_initialize_eax();
    eax_initialize_lock();
    eax_initialize_effects();
}

const EAX50FXSLOTPROPERTIES& ALeffectslot::eax_get_eax_fx_slot() const noexcept
{
    return eax_eax_fx_slot_;
}

void ALeffectslot::eax_ensure_is_unlocked() const
{
    if (eax_is_locked_)
        eax_fail("Locked.");
}

void ALeffectslot::eax_validate_fx_slot_effect(
    const GUID& eax_effect_id)
{
    eax_ensure_is_unlocked();

    if (eax_effect_id != EAX_NULL_GUID &&
        eax_effect_id != EAX_REVERB_EFFECT &&
        eax_effect_id != EAX_AGCCOMPRESSOR_EFFECT &&
        eax_effect_id != EAX_AUTOWAH_EFFECT &&
        eax_effect_id != EAX_CHORUS_EFFECT &&
        eax_effect_id != EAX_DISTORTION_EFFECT &&
        eax_effect_id != EAX_ECHO_EFFECT &&
        eax_effect_id != EAX_EQUALIZER_EFFECT &&
        eax_effect_id != EAX_FLANGER_EFFECT &&
        eax_effect_id != EAX_FREQUENCYSHIFTER_EFFECT &&
        eax_effect_id != EAX_VOCALMORPHER_EFFECT &&
        eax_effect_id != EAX_PITCHSHIFTER_EFFECT &&
        eax_effect_id != EAX_RINGMODULATOR_EFFECT)
    {
        eax_fail("Unsupported EAX effect GUID.");
    }
}

void ALeffectslot::eax_validate_fx_slot_volume(
    long eax_volume)
{
    eax_validate_range<EaxFxSlotException>(
        "Volume",
        eax_volume,
        EAXFXSLOT_MINVOLUME,
        EAXFXSLOT_MAXVOLUME);
}

void ALeffectslot::eax_validate_fx_slot_lock(
    long eax_lock)
{
    eax_ensure_is_unlocked();

    eax_validate_range<EaxFxSlotException>(
        "Lock",
        eax_lock,
        EAXFXSLOT_MINLOCK,
        EAXFXSLOT_MAXLOCK);
}

void ALeffectslot::eax_validate_fx_slot_flags(
    unsigned long eax_flags,
    int eax_version)
{
    eax_validate_range<EaxFxSlotException>(
        "Flags",
        eax_flags,
        0UL,
        ~(eax_version == 4 ? EAX40FXSLOTFLAGS_RESERVED : EAX50FXSLOTFLAGS_RESERVED));
}

void ALeffectslot::eax_validate_fx_slot_occlusion(
    long eax_occlusion)
{
    eax_validate_range<EaxFxSlotException>(
        "Occlusion",
        eax_occlusion,
        EAXFXSLOT_MINOCCLUSION,
        EAXFXSLOT_MAXOCCLUSION);
}

void ALeffectslot::eax_validate_fx_slot_occlusion_lf_ratio(
    float eax_occlusion_lf_ratio)
{
    eax_validate_range<EaxFxSlotException>(
        "Occlusion LF Ratio",
        eax_occlusion_lf_ratio,
        EAXFXSLOT_MINOCCLUSIONLFRATIO,
        EAXFXSLOT_MAXOCCLUSIONLFRATIO);
}

void ALeffectslot::eax_validate_fx_slot_all(
    const EAX40FXSLOTPROPERTIES& fx_slot,
    int eax_version)
{
    eax_validate_fx_slot_effect(fx_slot.guidLoadEffect);
    eax_validate_fx_slot_volume(fx_slot.lVolume);
    eax_validate_fx_slot_lock(fx_slot.lLock);
    eax_validate_fx_slot_flags(fx_slot.ulFlags, eax_version);
}

void ALeffectslot::eax_validate_fx_slot_all(
    const EAX50FXSLOTPROPERTIES& fx_slot,
    int eax_version)
{
    eax_validate_fx_slot_all(static_cast<const EAX40FXSLOTPROPERTIES&>(fx_slot), eax_version);

    eax_validate_fx_slot_occlusion(fx_slot.lOcclusion);
    eax_validate_fx_slot_occlusion_lf_ratio(fx_slot.flOcclusionLFRatio);
}

void ALeffectslot::eax_set_fx_slot_effect(
    const GUID& eax_effect_id)
{
    if (eax_eax_fx_slot_.guidLoadEffect == eax_effect_id)
    {
        return;
    }

    eax_eax_fx_slot_.guidLoadEffect = eax_effect_id;

    eax_set_fx_slot_effect();
}

void ALeffectslot::eax_set_fx_slot_volume(
    long eax_volume)
{
    if (eax_eax_fx_slot_.lVolume == eax_volume)
    {
        return;
    }

    eax_eax_fx_slot_.lVolume = eax_volume;

    eax_set_fx_slot_volume();
}

void ALeffectslot::eax_set_fx_slot_lock(
    long eax_lock)
{
    if (eax_eax_fx_slot_.lLock == eax_lock)
    {
        return;
    }

    eax_eax_fx_slot_.lLock = eax_lock;
}

void ALeffectslot::eax_set_fx_slot_flags(
    unsigned long eax_flags)
{
    if (eax_eax_fx_slot_.ulFlags == eax_flags)
    {
        return;
    }

    eax_eax_fx_slot_.ulFlags = eax_flags;

    eax_set_fx_slot_flags();
}

// [[nodiscard]]
bool ALeffectslot::eax_set_fx_slot_occlusion(
    long eax_occlusion)
{
    if (eax_eax_fx_slot_.lOcclusion == eax_occlusion)
    {
        return false;
    }

    eax_eax_fx_slot_.lOcclusion = eax_occlusion;

    return true;
}

// [[nodiscard]]
bool ALeffectslot::eax_set_fx_slot_occlusion_lf_ratio(
    float eax_occlusion_lf_ratio)
{
    if (eax_eax_fx_slot_.flOcclusionLFRatio == eax_occlusion_lf_ratio)
    {
        return false;
    }

    eax_eax_fx_slot_.flOcclusionLFRatio = eax_occlusion_lf_ratio;

    return true;
}

void ALeffectslot::eax_set_fx_slot_all(
    const EAX40FXSLOTPROPERTIES& eax_fx_slot)
{
    eax_set_fx_slot_effect(eax_fx_slot.guidLoadEffect);
    eax_set_fx_slot_volume(eax_fx_slot.lVolume);
    eax_set_fx_slot_lock(eax_fx_slot.lLock);
    eax_set_fx_slot_flags(eax_fx_slot.ulFlags);
}

// [[nodiscard]]
bool ALeffectslot::eax_set_fx_slot_all(
    const EAX50FXSLOTPROPERTIES& eax_fx_slot)
{
    eax_set_fx_slot_all(static_cast<const EAX40FXSLOTPROPERTIES&>(eax_fx_slot));

    const auto is_occlusion_modified = eax_set_fx_slot_occlusion(eax_fx_slot.lOcclusion);
    const auto is_occlusion_lf_ratio_modified = eax_set_fx_slot_occlusion_lf_ratio(eax_fx_slot.flOcclusionLFRatio);

    return is_occlusion_modified || is_occlusion_lf_ratio_modified;
}

void ALeffectslot::eax_unlock_legacy() noexcept
{
    assert(eax_fx_slot_index_ < 2);
    eax_is_locked_ = false;
    eax_eax_fx_slot_.lLock = EAXFXSLOT_UNLOCKED;
}

[[noreturn]]
void ALeffectslot::eax_fail(
    const char* message)
{
    throw EaxFxSlotException{message};
}

GUID ALeffectslot::eax_get_eax_default_effect_guid() const noexcept
{
    switch (eax_fx_slot_index_)
    {
        case 0: return EAX_REVERB_EFFECT;
        case 1: return EAX_CHORUS_EFFECT;
        default: return EAX_NULL_GUID;
    }
}

long ALeffectslot::eax_get_eax_default_lock() const noexcept
{
    return eax_fx_slot_index_ < 2 ? EAXFXSLOT_LOCKED : EAXFXSLOT_UNLOCKED;
}

void ALeffectslot::eax_set_eax_fx_slot_defaults()
{
    eax_eax_fx_slot_.guidLoadEffect = eax_get_eax_default_effect_guid();
    eax_eax_fx_slot_.lVolume = EAXFXSLOT_DEFAULTVOLUME;
    eax_eax_fx_slot_.lLock = eax_get_eax_default_lock();
    eax_eax_fx_slot_.ulFlags = EAX40FXSLOT_DEFAULTFLAGS;
    eax_eax_fx_slot_.lOcclusion = EAXFXSLOT_DEFAULTOCCLUSION;
    eax_eax_fx_slot_.flOcclusionLFRatio = EAXFXSLOT_DEFAULTOCCLUSIONLFRATIO;
}

void ALeffectslot::eax_initialize_eax()
{
    eax_set_eax_fx_slot_defaults();
}

void ALeffectslot::eax_initialize_lock()
{
    eax_is_locked_ = (eax_fx_slot_index_ < 2);
}

void ALeffectslot::eax_initialize_effects()
{
    eax_set_fx_slot_effect();
}

void ALeffectslot::eax_get_fx_slot_all(
    const EaxEaxCall& eax_call) const
{
    switch (eax_call.get_version())
    {
        case 4:
            eax_call.set_value<EaxFxSlotException, EAX40FXSLOTPROPERTIES>(eax_eax_fx_slot_);
            break;

        case 5:
            eax_call.set_value<EaxFxSlotException, EAX50FXSLOTPROPERTIES>(eax_eax_fx_slot_);
            break;

        default:
            eax_fail("Unsupported EAX version.");
    }
}

void ALeffectslot::eax_get_fx_slot(
    const EaxEaxCall& eax_call) const
{
    switch (eax_call.get_property_id())
    {
        case EAXFXSLOT_ALLPARAMETERS:
            eax_get_fx_slot_all(eax_call);
            break;

        case EAXFXSLOT_LOADEFFECT:
            eax_call.set_value<EaxFxSlotException>(eax_eax_fx_slot_.guidLoadEffect);
            break;

        case EAXFXSLOT_VOLUME:
            eax_call.set_value<EaxFxSlotException>(eax_eax_fx_slot_.lVolume);
            break;

        case EAXFXSLOT_LOCK:
            eax_call.set_value<EaxFxSlotException>(eax_eax_fx_slot_.lLock);
            break;

        case EAXFXSLOT_FLAGS:
            eax_call.set_value<EaxFxSlotException>(eax_eax_fx_slot_.ulFlags);
            break;

        case EAXFXSLOT_OCCLUSION:
            eax_call.set_value<EaxFxSlotException>(eax_eax_fx_slot_.lOcclusion);
            break;

        case EAXFXSLOT_OCCLUSIONLFRATIO:
            eax_call.set_value<EaxFxSlotException>(eax_eax_fx_slot_.flOcclusionLFRatio);
            break;

        default:
            eax_fail("Unsupported FX slot property id.");
    }
}

// [[nodiscard]]
bool ALeffectslot::eax_get(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_set_id())
    {
        case EaxEaxCallPropertySetId::fx_slot:
            eax_get_fx_slot(eax_call);
            break;

        case EaxEaxCallPropertySetId::fx_slot_effect:
            eax_dispatch_effect(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }

    return false;
}

void ALeffectslot::eax_set_fx_slot_effect(
    ALenum al_effect_type)
{
    if(!IsValidEffectType(al_effect_type))
        eax_fail("Unsupported effect.");

    eax_effect_ = nullptr;
    eax_effect_ = eax_create_eax_effect(al_effect_type);

    eax_set_effect_slot_effect(*eax_effect_);
}

void ALeffectslot::eax_set_fx_slot_effect()
{
    auto al_effect_type = ALenum{};

    if (false)
    {
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_NULL_GUID)
    {
        al_effect_type = AL_EFFECT_NULL;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_AUTOWAH_EFFECT)
    {
        al_effect_type = AL_EFFECT_AUTOWAH;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_CHORUS_EFFECT)
    {
        al_effect_type = AL_EFFECT_CHORUS;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_AGCCOMPRESSOR_EFFECT)
    {
        al_effect_type = AL_EFFECT_COMPRESSOR;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_DISTORTION_EFFECT)
    {
        al_effect_type = AL_EFFECT_DISTORTION;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_REVERB_EFFECT)
    {
        al_effect_type = AL_EFFECT_EAXREVERB;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_ECHO_EFFECT)
    {
        al_effect_type = AL_EFFECT_ECHO;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_EQUALIZER_EFFECT)
    {
        al_effect_type = AL_EFFECT_EQUALIZER;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_FLANGER_EFFECT)
    {
        al_effect_type = AL_EFFECT_FLANGER;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_FREQUENCYSHIFTER_EFFECT)
    {
        al_effect_type = AL_EFFECT_FREQUENCY_SHIFTER;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_PITCHSHIFTER_EFFECT)
    {
        al_effect_type = AL_EFFECT_PITCH_SHIFTER;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_RINGMODULATOR_EFFECT)
    {
        al_effect_type = AL_EFFECT_RING_MODULATOR;
    }
    else if (eax_eax_fx_slot_.guidLoadEffect == EAX_VOCALMORPHER_EFFECT)
    {
        al_effect_type = AL_EFFECT_VOCAL_MORPHER;
    }
    else
    {
        eax_fail("Unsupported effect.");
    }

    eax_set_fx_slot_effect(al_effect_type);
}

void ALeffectslot::eax_set_efx_effect_slot_gain()
{
    const auto gain = level_mb_to_gain(
        static_cast<float>(clamp(
            eax_eax_fx_slot_.lVolume,
            EAXFXSLOT_MINVOLUME,
            EAXFXSLOT_MAXVOLUME)));

    eax_set_effect_slot_gain(gain);
}

void ALeffectslot::eax_set_fx_slot_volume()
{
    eax_set_efx_effect_slot_gain();
}

void ALeffectslot::eax_set_effect_slot_send_auto()
{
    eax_set_effect_slot_send_auto((eax_eax_fx_slot_.ulFlags & EAXFXSLOTFLAGS_ENVIRONMENT) != 0);
}

void ALeffectslot::eax_set_fx_slot_flags()
{
    eax_set_effect_slot_send_auto();
}

void ALeffectslot::eax_set_fx_slot_effect(
    const EaxEaxCall& eax_call)
{
    const auto& eax_effect_id =
        eax_call.get_value<EaxFxSlotException, const decltype(EAX40FXSLOTPROPERTIES::guidLoadEffect)>();

    eax_validate_fx_slot_effect(eax_effect_id);
    eax_set_fx_slot_effect(eax_effect_id);
}

void ALeffectslot::eax_set_fx_slot_volume(
    const EaxEaxCall& eax_call)
{
    const auto& eax_volume =
        eax_call.get_value<EaxFxSlotException, const decltype(EAX40FXSLOTPROPERTIES::lVolume)>();

    eax_validate_fx_slot_volume(eax_volume);
    eax_set_fx_slot_volume(eax_volume);
}

void ALeffectslot::eax_set_fx_slot_lock(
    const EaxEaxCall& eax_call)
{
    const auto& eax_lock =
        eax_call.get_value<EaxFxSlotException, const decltype(EAX40FXSLOTPROPERTIES::lLock)>();

    eax_validate_fx_slot_lock(eax_lock);
    eax_set_fx_slot_lock(eax_lock);
}

void ALeffectslot::eax_set_fx_slot_flags(
    const EaxEaxCall& eax_call)
{
    const auto& eax_flags =
        eax_call.get_value<EaxFxSlotException, const decltype(EAX40FXSLOTPROPERTIES::ulFlags)>();

    eax_validate_fx_slot_flags(eax_flags, eax_call.get_version());
    eax_set_fx_slot_flags(eax_flags);
}

// [[nodiscard]]
bool ALeffectslot::eax_set_fx_slot_occlusion(
    const EaxEaxCall& eax_call)
{
    const auto& eax_occlusion =
        eax_call.get_value<EaxFxSlotException, const decltype(EAX50FXSLOTPROPERTIES::lOcclusion)>();

    eax_validate_fx_slot_occlusion(eax_occlusion);

    return eax_set_fx_slot_occlusion(eax_occlusion);
}

// [[nodiscard]]
bool ALeffectslot::eax_set_fx_slot_occlusion_lf_ratio(
    const EaxEaxCall& eax_call)
{
    const auto& eax_occlusion_lf_ratio =
        eax_call.get_value<EaxFxSlotException, const decltype(EAX50FXSLOTPROPERTIES::flOcclusionLFRatio)>();

    eax_validate_fx_slot_occlusion_lf_ratio(eax_occlusion_lf_ratio);

    return eax_set_fx_slot_occlusion_lf_ratio(eax_occlusion_lf_ratio);
}

// [[nodiscard]]
bool ALeffectslot::eax_set_fx_slot_all(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_version())
    {
        case 4:
            {
                const auto& eax_all =
                    eax_call.get_value<EaxFxSlotException, const EAX40FXSLOTPROPERTIES>();

                eax_validate_fx_slot_all(eax_all, eax_call.get_version());
                eax_set_fx_slot_all(eax_all);

                return false;
            }

        case 5:
            {
                const auto& eax_all =
                    eax_call.get_value<EaxFxSlotException, const EAX50FXSLOTPROPERTIES>();

                eax_validate_fx_slot_all(eax_all, eax_call.get_version());
                return eax_set_fx_slot_all(eax_all);
            }

        default:
            eax_fail("Unsupported EAX version.");
    }
}

bool ALeffectslot::eax_set_fx_slot(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case EAXFXSLOT_NONE:
            return false;

        case EAXFXSLOT_ALLPARAMETERS:
            return eax_set_fx_slot_all(eax_call);

        case EAXFXSLOT_LOADEFFECT:
            eax_set_fx_slot_effect(eax_call);
            return false;

        case EAXFXSLOT_VOLUME:
            eax_set_fx_slot_volume(eax_call);
            return false;

        case EAXFXSLOT_LOCK:
            eax_set_fx_slot_lock(eax_call);
            return false;

        case EAXFXSLOT_FLAGS:
            eax_set_fx_slot_flags(eax_call);
            return false;

        case EAXFXSLOT_OCCLUSION:
            return eax_set_fx_slot_occlusion(eax_call);

        case EAXFXSLOT_OCCLUSIONLFRATIO:
            return eax_set_fx_slot_occlusion_lf_ratio(eax_call);


        default:
            eax_fail("Unsupported FX slot property id.");
    }
}

// [[nodiscard]]
bool ALeffectslot::eax_set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_set_id())
    {
        case EaxEaxCallPropertySetId::fx_slot:
            return eax_set_fx_slot(eax_call);

        case EaxEaxCallPropertySetId::fx_slot_effect:
            eax_dispatch_effect(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }

    return false;
}

void ALeffectslot::eax_dispatch_effect(const EaxEaxCall& eax_call)
{ if(eax_effect_) eax_effect_->dispatch(eax_call); }

void ALeffectslot::eax_apply_deferred()
{
    /* The other FXSlot properties (volume, effect, etc) aren't deferred? */

    auto is_changed = false;
    if(eax_effect_)
        is_changed = eax_effect_->apply_deferred();
    if(is_changed)
        eax_set_effect_slot_effect(*eax_effect_);
}


void ALeffectslot::eax_set_effect_slot_effect(EaxEffect &effect)
{
#define EAX_PREFIX "[EAX_SET_EFFECT_SLOT_EFFECT] "

    const auto error = initEffect(effect.al_effect_type_, effect.al_effect_props_, eax_al_context_);
    if (error != AL_NO_ERROR)
    {
        ERR(EAX_PREFIX "%s\n", "Failed to initialize an effect.");
        return;
    }

    if (mState == SlotState::Initial)
    {
        mPropsDirty = false;
        updateProps(eax_al_context_);

        auto effect_slot_ptr = this;

        AddActiveEffectSlots({&effect_slot_ptr, 1}, eax_al_context_);
        mState = SlotState::Playing;

        return;
    }

    UpdateProps(this, eax_al_context_);

#undef EAX_PREFIX
}

void ALeffectslot::eax_set_effect_slot_send_auto(
    bool is_send_auto)
{
    if(AuxSendAuto == is_send_auto)
        return;

    AuxSendAuto = is_send_auto;
    UpdateProps(this, eax_al_context_);
}

void ALeffectslot::eax_set_effect_slot_gain(
    ALfloat gain)
{
#define EAX_PREFIX "[EAX_SET_EFFECT_SLOT_GAIN] "

    if(gain == Gain)
        return;
    if(gain < 0.0f || gain > 1.0f)
        ERR(EAX_PREFIX "Gain out of range (%f)\n", gain);

    Gain = clampf(gain, 0.0f, 1.0f);
    UpdateProps(this, eax_al_context_);

#undef EAX_PREFIX
}


void ALeffectslot::EaxDeleter::operator()(ALeffectslot* effect_slot)
{
    assert(effect_slot);
    eax_delete_al_effect_slot(*effect_slot->eax_al_context_, *effect_slot);
}


EaxAlEffectSlotUPtr eax_create_al_effect_slot(
    ALCcontext& context)
{
#define EAX_PREFIX "[EAX_MAKE_EFFECT_SLOT] "

    std::unique_lock<std::mutex> effect_slot_lock{context.mEffectSlotLock};

    auto& device = *context.mALDevice;

    if (context.mNumEffectSlots == device.AuxiliaryEffectSlotMax)
    {
        ERR(EAX_PREFIX "%s\n", "Out of memory.");
        return nullptr;
    }

    if (!EnsureEffectSlots(&context, 1))
    {
        ERR(EAX_PREFIX "%s\n", "Failed to ensure.");
        return nullptr;
    }

    auto effect_slot = EaxAlEffectSlotUPtr{AllocEffectSlot(&context)};
    if (!effect_slot)
    {
        ERR(EAX_PREFIX "%s\n", "Failed to allocate.");
        return nullptr;
    }

    return effect_slot;

#undef EAX_PREFIX
}

void eax_delete_al_effect_slot(
    ALCcontext& context,
    ALeffectslot& effect_slot)
{
#define EAX_PREFIX "[EAX_DELETE_EFFECT_SLOT] "

    std::lock_guard<std::mutex> effect_slot_lock{context.mEffectSlotLock};

    if (ReadRef(effect_slot.ref) != 0)
    {
        ERR(EAX_PREFIX "Deleting in-use effect slot %u.\n", effect_slot.id);
        return;
    }

    auto effect_slot_ptr = &effect_slot;

    RemoveActiveEffectSlots({&effect_slot_ptr, 1}, &context);
    FreeEffectSlot(&context, &effect_slot);

#undef EAX_PREFIX
}
#endif // ALSOFT_EAX
