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

#include "filter.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/efx.h"

#include "albit.h"
#include "alc/context.h"
#include "alc/device.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "core/except.h"
#include "opthelpers.h"
#include "vector.h"


namespace {

class filter_exception final : public al::base_exception {
    ALenum mErrorCode;

public:
#ifdef __USE_MINGW_ANSI_STDIO
    [[gnu::format(gnu_printf, 3, 4)]]
#else
    [[gnu::format(printf, 3, 4)]]
#endif
    filter_exception(ALenum code, const char *msg, ...) : mErrorCode{code}
    {
        std::va_list args;
        va_start(args, msg);
        setMessage(msg, args);
        va_end(args);
    }
    ALenum errorCode() const noexcept { return mErrorCode; }
};


#define DEFINE_ALFILTER_VTABLE(T)                                  \
const ALfilter::Vtable T##_vtable = {                              \
    T##_setParami, T##_setParamiv, T##_setParamf, T##_setParamfv,  \
    T##_getParami, T##_getParamiv, T##_getParamf, T##_getParamfv,  \
}

void ALlowpass_setParami(ALfilter*, ALenum param, int)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid low-pass integer property 0x%04x", param}; }
void ALlowpass_setParamiv(ALfilter*, ALenum param, const int*)
{
    throw filter_exception{AL_INVALID_ENUM, "Invalid low-pass integer-vector property 0x%04x",
        param};
}
void ALlowpass_setParamf(ALfilter *filter, ALenum param, float val)
{
    switch(param)
    {
    case AL_LOWPASS_GAIN:
        if(!(val >= AL_LOWPASS_MIN_GAIN && val <= AL_LOWPASS_MAX_GAIN))
            throw filter_exception{AL_INVALID_VALUE, "Low-pass gain %f out of range", val};
        filter->Gain = val;
        break;

    case AL_LOWPASS_GAINHF:
        if(!(val >= AL_LOWPASS_MIN_GAINHF && val <= AL_LOWPASS_MAX_GAINHF))
            throw filter_exception{AL_INVALID_VALUE, "Low-pass gainhf %f out of range", val};
        filter->GainHF = val;
        break;

    default:
        throw filter_exception{AL_INVALID_ENUM, "Invalid low-pass float property 0x%04x", param};
    }
}
void ALlowpass_setParamfv(ALfilter *filter, ALenum param, const float *vals)
{ ALlowpass_setParamf(filter, param, vals[0]); }

void ALlowpass_getParami(const ALfilter*, ALenum param, int*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid low-pass integer property 0x%04x", param}; }
void ALlowpass_getParamiv(const ALfilter*, ALenum param, int*)
{
    throw filter_exception{AL_INVALID_ENUM, "Invalid low-pass integer-vector property 0x%04x",
        param};
}
void ALlowpass_getParamf(const ALfilter *filter, ALenum param, float *val)
{
    switch(param)
    {
    case AL_LOWPASS_GAIN:
        *val = filter->Gain;
        break;

    case AL_LOWPASS_GAINHF:
        *val = filter->GainHF;
        break;

    default:
        throw filter_exception{AL_INVALID_ENUM, "Invalid low-pass float property 0x%04x", param};
    }
}
void ALlowpass_getParamfv(const ALfilter *filter, ALenum param, float *vals)
{ ALlowpass_getParamf(filter, param, vals); }

DEFINE_ALFILTER_VTABLE(ALlowpass);


void ALhighpass_setParami(ALfilter*, ALenum param, int)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid high-pass integer property 0x%04x", param}; }
void ALhighpass_setParamiv(ALfilter*, ALenum param, const int*)
{
    throw filter_exception{AL_INVALID_ENUM, "Invalid high-pass integer-vector property 0x%04x",
        param};
}
void ALhighpass_setParamf(ALfilter *filter, ALenum param, float val)
{
    switch(param)
    {
    case AL_HIGHPASS_GAIN:
        if(!(val >= AL_HIGHPASS_MIN_GAIN && val <= AL_HIGHPASS_MAX_GAIN))
            throw filter_exception{AL_INVALID_VALUE, "High-pass gain %f out of range", val};
        filter->Gain = val;
        break;

    case AL_HIGHPASS_GAINLF:
        if(!(val >= AL_HIGHPASS_MIN_GAINLF && val <= AL_HIGHPASS_MAX_GAINLF))
            throw filter_exception{AL_INVALID_VALUE, "High-pass gainlf %f out of range", val};
        filter->GainLF = val;
        break;

    default:
        throw filter_exception{AL_INVALID_ENUM, "Invalid high-pass float property 0x%04x", param};
    }
}
void ALhighpass_setParamfv(ALfilter *filter, ALenum param, const float *vals)
{ ALhighpass_setParamf(filter, param, vals[0]); }

void ALhighpass_getParami(const ALfilter*, ALenum param, int*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid high-pass integer property 0x%04x", param}; }
void ALhighpass_getParamiv(const ALfilter*, ALenum param, int*)
{
    throw filter_exception{AL_INVALID_ENUM, "Invalid high-pass integer-vector property 0x%04x",
        param};
}
void ALhighpass_getParamf(const ALfilter *filter, ALenum param, float *val)
{
    switch(param)
    {
    case AL_HIGHPASS_GAIN:
        *val = filter->Gain;
        break;

    case AL_HIGHPASS_GAINLF:
        *val = filter->GainLF;
        break;

    default:
        throw filter_exception{AL_INVALID_ENUM, "Invalid high-pass float property 0x%04x", param};
    }
}
void ALhighpass_getParamfv(const ALfilter *filter, ALenum param, float *vals)
{ ALhighpass_getParamf(filter, param, vals); }

DEFINE_ALFILTER_VTABLE(ALhighpass);


void ALbandpass_setParami(ALfilter*, ALenum param, int)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid band-pass integer property 0x%04x", param}; }
void ALbandpass_setParamiv(ALfilter*, ALenum param, const int*)
{
    throw filter_exception{AL_INVALID_ENUM, "Invalid band-pass integer-vector property 0x%04x",
        param};
}
void ALbandpass_setParamf(ALfilter *filter, ALenum param, float val)
{
    switch(param)
    {
    case AL_BANDPASS_GAIN:
        if(!(val >= AL_BANDPASS_MIN_GAIN && val <= AL_BANDPASS_MAX_GAIN))
            throw filter_exception{AL_INVALID_VALUE, "Band-pass gain %f out of range", val};
        filter->Gain = val;
        break;

    case AL_BANDPASS_GAINHF:
        if(!(val >= AL_BANDPASS_MIN_GAINHF && val <= AL_BANDPASS_MAX_GAINHF))
            throw filter_exception{AL_INVALID_VALUE, "Band-pass gainhf %f out of range", val};
        filter->GainHF = val;
        break;

    case AL_BANDPASS_GAINLF:
        if(!(val >= AL_BANDPASS_MIN_GAINLF && val <= AL_BANDPASS_MAX_GAINLF))
            throw filter_exception{AL_INVALID_VALUE, "Band-pass gainlf %f out of range", val};
        filter->GainLF = val;
        break;

    default:
        throw filter_exception{AL_INVALID_ENUM, "Invalid band-pass float property 0x%04x", param};
    }
}
void ALbandpass_setParamfv(ALfilter *filter, ALenum param, const float *vals)
{ ALbandpass_setParamf(filter, param, vals[0]); }

void ALbandpass_getParami(const ALfilter*, ALenum param, int*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid band-pass integer property 0x%04x", param}; }
void ALbandpass_getParamiv(const ALfilter*, ALenum param, int*)
{
    throw filter_exception{AL_INVALID_ENUM, "Invalid band-pass integer-vector property 0x%04x",
        param};
}
void ALbandpass_getParamf(const ALfilter *filter, ALenum param, float *val)
{
    switch(param)
    {
    case AL_BANDPASS_GAIN:
        *val = filter->Gain;
        break;

    case AL_BANDPASS_GAINHF:
        *val = filter->GainHF;
        break;

    case AL_BANDPASS_GAINLF:
        *val = filter->GainLF;
        break;

    default:
        throw filter_exception{AL_INVALID_ENUM, "Invalid band-pass float property 0x%04x", param};
    }
}
void ALbandpass_getParamfv(const ALfilter *filter, ALenum param, float *vals)
{ ALbandpass_getParamf(filter, param, vals); }

DEFINE_ALFILTER_VTABLE(ALbandpass);


void ALnullfilter_setParami(ALfilter*, ALenum param, int)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }
void ALnullfilter_setParamiv(ALfilter*, ALenum param, const int*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }
void ALnullfilter_setParamf(ALfilter*, ALenum param, float)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }
void ALnullfilter_setParamfv(ALfilter*, ALenum param, const float*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }

void ALnullfilter_getParami(const ALfilter*, ALenum param, int*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }
void ALnullfilter_getParamiv(const ALfilter*, ALenum param, int*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }
void ALnullfilter_getParamf(const ALfilter*, ALenum param, float*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }
void ALnullfilter_getParamfv(const ALfilter*, ALenum param, float*)
{ throw filter_exception{AL_INVALID_ENUM, "Invalid null filter property 0x%04x", param}; }

DEFINE_ALFILTER_VTABLE(ALnullfilter);


void InitFilterParams(ALfilter *filter, ALenum type)
{
    if(type == AL_FILTER_LOWPASS)
    {
        filter->Gain = AL_LOWPASS_DEFAULT_GAIN;
        filter->GainHF = AL_LOWPASS_DEFAULT_GAINHF;
        filter->HFReference = LOWPASSFREQREF;
        filter->GainLF = 1.0f;
        filter->LFReference = HIGHPASSFREQREF;
        filter->vtab = &ALlowpass_vtable;
    }
    else if(type == AL_FILTER_HIGHPASS)
    {
        filter->Gain = AL_HIGHPASS_DEFAULT_GAIN;
        filter->GainHF = 1.0f;
        filter->HFReference = LOWPASSFREQREF;
        filter->GainLF = AL_HIGHPASS_DEFAULT_GAINLF;
        filter->LFReference = HIGHPASSFREQREF;
        filter->vtab = &ALhighpass_vtable;
    }
    else if(type == AL_FILTER_BANDPASS)
    {
        filter->Gain = AL_BANDPASS_DEFAULT_GAIN;
        filter->GainHF = AL_BANDPASS_DEFAULT_GAINHF;
        filter->HFReference = LOWPASSFREQREF;
        filter->GainLF = AL_BANDPASS_DEFAULT_GAINLF;
        filter->LFReference = HIGHPASSFREQREF;
        filter->vtab = &ALbandpass_vtable;
    }
    else
    {
        filter->Gain = 1.0f;
        filter->GainHF = 1.0f;
        filter->HFReference = LOWPASSFREQREF;
        filter->GainLF = 1.0f;
        filter->LFReference = HIGHPASSFREQREF;
        filter->vtab = &ALnullfilter_vtable;
    }
    filter->type = type;
}

bool EnsureFilters(ALCdevice *device, size_t needed)
{
    size_t count{std::accumulate(device->FilterList.cbegin(), device->FilterList.cend(), size_t{0},
        [](size_t cur, const FilterSubList &sublist) noexcept -> size_t
        { return cur + static_cast<ALuint>(al::popcount(sublist.FreeMask)); })};

    while(needed > count)
    {
        if UNLIKELY(device->FilterList.size() >= 1<<25)
            return false;

        device->FilterList.emplace_back();
        auto sublist = device->FilterList.end() - 1;
        sublist->FreeMask = ~0_u64;
        sublist->Filters = static_cast<ALfilter*>(al_calloc(alignof(ALfilter), sizeof(ALfilter)*64));
        if UNLIKELY(!sublist->Filters)
        {
            device->FilterList.pop_back();
            return false;
        }
        count += 64;
    }
    return true;
}


ALfilter *AllocFilter(ALCdevice *device)
{
    auto sublist = std::find_if(device->FilterList.begin(), device->FilterList.end(),
        [](const FilterSubList &entry) noexcept -> bool
        { return entry.FreeMask != 0; });
    auto lidx = static_cast<ALuint>(std::distance(device->FilterList.begin(), sublist));
    auto slidx = static_cast<ALuint>(al::countr_zero(sublist->FreeMask));
    ASSUME(slidx < 64);

    ALfilter *filter{al::construct_at(sublist->Filters + slidx)};
    InitFilterParams(filter, AL_FILTER_NULL);

    /* Add 1 to avoid filter ID 0. */
    filter->id = ((lidx<<6) | slidx) + 1;

    sublist->FreeMask &= ~(1_u64 << slidx);

    return filter;
}

void FreeFilter(ALCdevice *device, ALfilter *filter)
{
    const ALuint id{filter->id - 1};
    const size_t lidx{id >> 6};
    const ALuint slidx{id & 0x3f};

    al::destroy_at(filter);

    device->FilterList[lidx].FreeMask |= 1_u64 << slidx;
}


inline ALfilter *LookupFilter(ALCdevice *device, ALuint id)
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

} // namespace

AL_API void AL_APIENTRY alGenFilters(ALsizei n, ALuint *filters)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Generating %d filters", n);
    if UNLIKELY(n <= 0) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};
    if(!EnsureFilters(device, static_cast<ALuint>(n)))
    {
        context->setError(AL_OUT_OF_MEMORY, "Failed to allocate %d filter%s", n, (n==1)?"":"s");
        return;
    }

    if LIKELY(n == 1)
    {
        /* Special handling for the easy and normal case. */
        ALfilter *filter{AllocFilter(device)};
        if(filter) filters[0] = filter->id;
    }
    else
    {
        /* Store the allocated buffer IDs in a separate local list, to avoid
         * modifying the user storage in case of failure.
         */
        al::vector<ALuint> ids;
        ids.reserve(static_cast<ALuint>(n));
        do {
            ALfilter *filter{AllocFilter(device)};
            ids.emplace_back(filter->id);
        } while(--n);
        std::copy(ids.begin(), ids.end(), filters);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alDeleteFilters(ALsizei n, const ALuint *filters)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if UNLIKELY(n < 0)
        context->setError(AL_INVALID_VALUE, "Deleting %d filters", n);
    if UNLIKELY(n <= 0) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    /* First try to find any filters that are invalid. */
    auto validate_filter = [device](const ALuint fid) -> bool
    { return !fid || LookupFilter(device, fid) != nullptr; };

    const ALuint *filters_end = filters + n;
    auto invflt = std::find_if_not(filters, filters_end, validate_filter);
    if UNLIKELY(invflt != filters_end)
    {
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", *invflt);
        return;
    }

    /* All good. Delete non-0 filter IDs. */
    auto delete_filter = [device](const ALuint fid) -> void
    {
        ALfilter *filter{fid ? LookupFilter(device, fid) : nullptr};
        if(filter) FreeFilter(device, filter);
    };
    std::for_each(filters, filters_end, delete_filter);
}
END_API_FUNC

AL_API ALboolean AL_APIENTRY alIsFilter(ALuint filter)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if LIKELY(context)
    {
        ALCdevice *device{context->mALDevice.get()};
        std::lock_guard<std::mutex> _{device->FilterLock};
        if(!filter || LookupFilter(device, filter))
            return AL_TRUE;
    }
    return AL_FALSE;
}
END_API_FUNC


AL_API void AL_APIENTRY alFilteri(ALuint filter, ALenum param, ALint value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else
    {
        if(param == AL_FILTER_TYPE)
        {
            if(value == AL_FILTER_NULL || value == AL_FILTER_LOWPASS
                || value == AL_FILTER_HIGHPASS || value == AL_FILTER_BANDPASS)
                InitFilterParams(alfilt, value);
            else
                context->setError(AL_INVALID_VALUE, "Invalid filter type 0x%04x", value);
        }
        else try
        {
            /* Call the appropriate handler */
            alfilt->setParami(param, value);
        }
        catch(filter_exception &e) {
            context->setError(e.errorCode(), "%s", e.what());
        }
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alFilteriv(ALuint filter, ALenum param, const ALint *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_FILTER_TYPE:
        alFilteri(filter, param, values[0]);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else try
    {
        /* Call the appropriate handler */
        alfilt->setParamiv(param, values);
    }
    catch(filter_exception &e) {
        context->setError(e.errorCode(), "%s", e.what());
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alFilterf(ALuint filter, ALenum param, ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else try
    {
        /* Call the appropriate handler */
        alfilt->setParamf(param, value);
    }
    catch(filter_exception &e) {
        context->setError(e.errorCode(), "%s", e.what());
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alFilterfv(ALuint filter, ALenum param, const ALfloat *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else try
    {
        /* Call the appropriate handler */
        alfilt->setParamfv(param, values);
    }
    catch(filter_exception &e) {
        context->setError(e.errorCode(), "%s", e.what());
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetFilteri(ALuint filter, ALenum param, ALint *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    const ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else
    {
        if(param == AL_FILTER_TYPE)
            *value = alfilt->type;
        else try
        {
            /* Call the appropriate handler */
            alfilt->getParami(param, value);
        }
        catch(filter_exception &e) {
            context->setError(e.errorCode(), "%s", e.what());
        }
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetFilteriv(ALuint filter, ALenum param, ALint *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_FILTER_TYPE:
        alGetFilteri(filter, param, values);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    const ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else try
    {
        /* Call the appropriate handler */
        alfilt->getParamiv(param, values);
    }
    catch(filter_exception &e) {
        context->setError(e.errorCode(), "%s", e.what());
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetFilterf(ALuint filter, ALenum param, ALfloat *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    const ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else try
    {
        /* Call the appropriate handler */
        alfilt->getParamf(param, value);
    }
    catch(filter_exception &e) {
        context->setError(e.errorCode(), "%s", e.what());
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetFilterfv(ALuint filter, ALenum param, ALfloat *values)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALCdevice *device{context->mALDevice.get()};
    std::lock_guard<std::mutex> _{device->FilterLock};

    const ALfilter *alfilt{LookupFilter(device, filter)};
    if UNLIKELY(!alfilt)
        context->setError(AL_INVALID_NAME, "Invalid filter ID %u", filter);
    else try
    {
        /* Call the appropriate handler */
        alfilt->getParamfv(param, values);
    }
    catch(filter_exception &e) {
        context->setError(e.errorCode(), "%s", e.what());
    }
}
END_API_FUNC


FilterSubList::~FilterSubList()
{
    uint64_t usemask{~FreeMask};
    while(usemask)
    {
        const int idx{al::countr_zero(usemask)};
        al::destroy_at(Filters+idx);
        usemask &= ~(1_u64 << idx);
    }
    FreeMask = ~usemask;
    al_free(Filters);
    Filters = nullptr;
}
