/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2000 by authors.
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

#include "version.h"

#include <atomic>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <string>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "alc/alu.h"
#include "alc/context.h"
#include "alc/inprogext.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "atomic.h"
#include "core/context.h"
#include "core/except.h"
#include "core/mixer/defs.h"
#include "core/voice.h"
#include "intrusive_ptr.h"
#include "opthelpers.h"
#include "strutils.h"

#ifdef ALSOFT_EAX
#include "alc/device.h"

#include "eax_globals.h"
#include "eax_x_ram.h"
#endif // ALSOFT_EAX


namespace {

constexpr ALchar alVendor[] = "OpenAL Community";
constexpr ALchar alVersion[] = "1.1 ALSOFT " ALSOFT_VERSION;
constexpr ALchar alRenderer[] = "OpenAL Soft";

// Error Messages
constexpr ALchar alNoError[] = "No Error";
constexpr ALchar alErrInvalidName[] = "Invalid Name";
constexpr ALchar alErrInvalidEnum[] = "Invalid Enum";
constexpr ALchar alErrInvalidValue[] = "Invalid Value";
constexpr ALchar alErrInvalidOp[] = "Invalid Operation";
constexpr ALchar alErrOutOfMemory[] = "Out of Memory";

/* Resampler strings */
template<Resampler rtype> struct ResamplerName { };
template<> struct ResamplerName<Resampler::Point>
{ static constexpr const ALchar *Get() noexcept { return "Nearest"; } };
template<> struct ResamplerName<Resampler::Linear>
{ static constexpr const ALchar *Get() noexcept { return "Linear"; } };
template<> struct ResamplerName<Resampler::Cubic>
{ static constexpr const ALchar *Get() noexcept { return "Cubic"; } };
template<> struct ResamplerName<Resampler::FastBSinc12>
{ static constexpr const ALchar *Get() noexcept { return "11th order Sinc (fast)"; } };
template<> struct ResamplerName<Resampler::BSinc12>
{ static constexpr const ALchar *Get() noexcept { return "11th order Sinc"; } };
template<> struct ResamplerName<Resampler::FastBSinc24>
{ static constexpr const ALchar *Get() noexcept { return "23rd order Sinc (fast)"; } };
template<> struct ResamplerName<Resampler::BSinc24>
{ static constexpr const ALchar *Get() noexcept { return "23rd order Sinc"; } };

const ALchar *GetResamplerName(const Resampler rtype)
{
#define HANDLE_RESAMPLER(r) case r: return ResamplerName<r>::Get()
    switch(rtype)
    {
    HANDLE_RESAMPLER(Resampler::Point);
    HANDLE_RESAMPLER(Resampler::Linear);
    HANDLE_RESAMPLER(Resampler::Cubic);
    HANDLE_RESAMPLER(Resampler::FastBSinc12);
    HANDLE_RESAMPLER(Resampler::BSinc12);
    HANDLE_RESAMPLER(Resampler::FastBSinc24);
    HANDLE_RESAMPLER(Resampler::BSinc24);
    }
#undef HANDLE_RESAMPLER
    /* Should never get here. */
    throw std::runtime_error{"Unexpected resampler index"};
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

} // namespace

/* WARNING: Non-standard export! Not part of any extension, or exposed in the
 * alcFunctions list.
 */
AL_API const ALchar* AL_APIENTRY alsoft_get_version(void)
START_API_FUNC
{
    static const auto spoof = al::getenv("ALSOFT_SPOOF_VERSION");
    if(spoof) return spoof->c_str();
    return ALSOFT_VERSION;
}
END_API_FUNC

#define DO_UPDATEPROPS() do {                                                 \
    if(!context->mDeferUpdates)                                               \
        UpdateContextProps(context.get());                                    \
    else                                                                      \
        context->mPropsDirty = true;                                          \
} while(0)


AL_API void AL_APIENTRY alEnable(ALenum capability)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    switch(capability)
    {
    case AL_SOURCE_DISTANCE_MODEL:
        {
            std::lock_guard<std::mutex> _{context->mPropLock};
            context->mSourceDistanceModel = true;
            DO_UPDATEPROPS();
        }
        break;

    case AL_STOP_SOURCES_ON_DISCONNECT_SOFT:
        context->setError(AL_INVALID_OPERATION, "Re-enabling AL_STOP_SOURCES_ON_DISCONNECT_SOFT not yet supported");
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid enable property 0x%04x", capability);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alDisable(ALenum capability)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    switch(capability)
    {
    case AL_SOURCE_DISTANCE_MODEL:
        {
            std::lock_guard<std::mutex> _{context->mPropLock};
            context->mSourceDistanceModel = false;
            DO_UPDATEPROPS();
        }
        break;

    case AL_STOP_SOURCES_ON_DISCONNECT_SOFT:
        context->mStopVoicesOnDisconnect = false;
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid disable property 0x%04x", capability);
    }
}
END_API_FUNC

AL_API ALboolean AL_APIENTRY alIsEnabled(ALenum capability)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return AL_FALSE;

    std::lock_guard<std::mutex> _{context->mPropLock};
    ALboolean value{AL_FALSE};
    switch(capability)
    {
    case AL_SOURCE_DISTANCE_MODEL:
        value = context->mSourceDistanceModel ? AL_TRUE : AL_FALSE;
        break;

    case AL_STOP_SOURCES_ON_DISCONNECT_SOFT:
        value = context->mStopVoicesOnDisconnect ? AL_TRUE : AL_FALSE;
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid is enabled property 0x%04x", capability);
    }

    return value;
}
END_API_FUNC

AL_API ALboolean AL_APIENTRY alGetBoolean(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return AL_FALSE;

    std::lock_guard<std::mutex> _{context->mPropLock};
    ALboolean value{AL_FALSE};
    switch(pname)
    {
    case AL_DOPPLER_FACTOR:
        if(context->mDopplerFactor != 0.0f)
            value = AL_TRUE;
        break;

    case AL_DOPPLER_VELOCITY:
        if(context->mDopplerVelocity != 0.0f)
            value = AL_TRUE;
        break;

    case AL_DISTANCE_MODEL:
        if(context->mDistanceModel == DistanceModel::Default)
            value = AL_TRUE;
        break;

    case AL_SPEED_OF_SOUND:
        if(context->mSpeedOfSound != 0.0f)
            value = AL_TRUE;
        break;

    case AL_DEFERRED_UPDATES_SOFT:
        if(context->mDeferUpdates)
            value = AL_TRUE;
        break;

    case AL_GAIN_LIMIT_SOFT:
        if(GainMixMax/context->mGainBoost != 0.0f)
            value = AL_TRUE;
        break;

    case AL_NUM_RESAMPLERS_SOFT:
        /* Always non-0. */
        value = AL_TRUE;
        break;

    case AL_DEFAULT_RESAMPLER_SOFT:
        value = static_cast<int>(ResamplerDefault) ? AL_TRUE : AL_FALSE;
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid boolean property 0x%04x", pname);
    }

    return value;
}
END_API_FUNC

AL_API ALdouble AL_APIENTRY alGetDouble(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return 0.0;

    std::lock_guard<std::mutex> _{context->mPropLock};
    ALdouble value{0.0};
    switch(pname)
    {
    case AL_DOPPLER_FACTOR:
        value = context->mDopplerFactor;
        break;

    case AL_DOPPLER_VELOCITY:
        value = context->mDopplerVelocity;
        break;

    case AL_DISTANCE_MODEL:
        value = static_cast<ALdouble>(ALenumFromDistanceModel(context->mDistanceModel));
        break;

    case AL_SPEED_OF_SOUND:
        value = context->mSpeedOfSound;
        break;

    case AL_DEFERRED_UPDATES_SOFT:
        if(context->mDeferUpdates)
            value = static_cast<ALdouble>(AL_TRUE);
        break;

    case AL_GAIN_LIMIT_SOFT:
        value = ALdouble{GainMixMax}/context->mGainBoost;
        break;

    case AL_NUM_RESAMPLERS_SOFT:
        value = static_cast<ALdouble>(Resampler::Max) + 1.0;
        break;

    case AL_DEFAULT_RESAMPLER_SOFT:
        value = static_cast<ALdouble>(ResamplerDefault);
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid double property 0x%04x", pname);
    }

    return value;
}
END_API_FUNC

AL_API ALfloat AL_APIENTRY alGetFloat(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return 0.0f;

    std::lock_guard<std::mutex> _{context->mPropLock};
    ALfloat value{0.0f};
    switch(pname)
    {
    case AL_DOPPLER_FACTOR:
        value = context->mDopplerFactor;
        break;

    case AL_DOPPLER_VELOCITY:
        value = context->mDopplerVelocity;
        break;

    case AL_DISTANCE_MODEL:
        value = static_cast<ALfloat>(ALenumFromDistanceModel(context->mDistanceModel));
        break;

    case AL_SPEED_OF_SOUND:
        value = context->mSpeedOfSound;
        break;

    case AL_DEFERRED_UPDATES_SOFT:
        if(context->mDeferUpdates)
            value = static_cast<ALfloat>(AL_TRUE);
        break;

    case AL_GAIN_LIMIT_SOFT:
        value = GainMixMax/context->mGainBoost;
        break;

    case AL_NUM_RESAMPLERS_SOFT:
        value = static_cast<ALfloat>(Resampler::Max) + 1.0f;
        break;

    case AL_DEFAULT_RESAMPLER_SOFT:
        value = static_cast<ALfloat>(ResamplerDefault);
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid float property 0x%04x", pname);
    }

    return value;
}
END_API_FUNC

AL_API ALint AL_APIENTRY alGetInteger(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return 0;

    std::lock_guard<std::mutex> _{context->mPropLock};
    ALint value{0};
    switch(pname)
    {
    case AL_DOPPLER_FACTOR:
        value = static_cast<ALint>(context->mDopplerFactor);
        break;

    case AL_DOPPLER_VELOCITY:
        value = static_cast<ALint>(context->mDopplerVelocity);
        break;

    case AL_DISTANCE_MODEL:
        value = ALenumFromDistanceModel(context->mDistanceModel);
        break;

    case AL_SPEED_OF_SOUND:
        value = static_cast<ALint>(context->mSpeedOfSound);
        break;

    case AL_DEFERRED_UPDATES_SOFT:
        if(context->mDeferUpdates)
            value = AL_TRUE;
        break;

    case AL_GAIN_LIMIT_SOFT:
        value = static_cast<ALint>(GainMixMax/context->mGainBoost);
        break;

    case AL_NUM_RESAMPLERS_SOFT:
        value = static_cast<int>(Resampler::Max) + 1;
        break;

    case AL_DEFAULT_RESAMPLER_SOFT:
        value = static_cast<int>(ResamplerDefault);
        break;

#ifdef ALSOFT_EAX

#define EAX_ERROR "[alGetInteger] EAX not enabled."

    case AL_EAX_RAM_SIZE:
        if (eax_g_is_enabled)
        {
            value = eax_x_ram_max_size;
        }
        else
        {
            context->setError(AL_INVALID_VALUE, EAX_ERROR);
        }

        break;

    case AL_EAX_RAM_FREE:
        if (eax_g_is_enabled)
        {
            auto device = context->mALDevice.get();
            std::lock_guard<std::mutex> device_lock{device->BufferLock};

            value = static_cast<ALint>(device->eax_x_ram_free_size);
        }
        else
        {
            context->setError(AL_INVALID_VALUE, EAX_ERROR);
        }

        break;

#undef EAX_ERROR

#endif // ALSOFT_EAX

    default:
        context->setError(AL_INVALID_VALUE, "Invalid integer property 0x%04x", pname);
    }

    return value;
}
END_API_FUNC

AL_API ALint64SOFT AL_APIENTRY alGetInteger64SOFT(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return 0_i64;

    std::lock_guard<std::mutex> _{context->mPropLock};
    ALint64SOFT value{0};
    switch(pname)
    {
    case AL_DOPPLER_FACTOR:
        value = static_cast<ALint64SOFT>(context->mDopplerFactor);
        break;

    case AL_DOPPLER_VELOCITY:
        value = static_cast<ALint64SOFT>(context->mDopplerVelocity);
        break;

    case AL_DISTANCE_MODEL:
        value = ALenumFromDistanceModel(context->mDistanceModel);
        break;

    case AL_SPEED_OF_SOUND:
        value = static_cast<ALint64SOFT>(context->mSpeedOfSound);
        break;

    case AL_DEFERRED_UPDATES_SOFT:
        if(context->mDeferUpdates)
            value = AL_TRUE;
        break;

    case AL_GAIN_LIMIT_SOFT:
        value = static_cast<ALint64SOFT>(GainMixMax/context->mGainBoost);
        break;

    case AL_NUM_RESAMPLERS_SOFT:
        value = static_cast<ALint64SOFT>(Resampler::Max) + 1;
        break;

    case AL_DEFAULT_RESAMPLER_SOFT:
        value = static_cast<ALint64SOFT>(ResamplerDefault);
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid integer64 property 0x%04x", pname);
    }

    return value;
}
END_API_FUNC

AL_API ALvoid* AL_APIENTRY alGetPointerSOFT(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return nullptr;

    std::lock_guard<std::mutex> _{context->mPropLock};
    void *value{nullptr};
    switch(pname)
    {
    case AL_EVENT_CALLBACK_FUNCTION_SOFT:
        value = reinterpret_cast<void*>(context->mEventCb);
        break;

    case AL_EVENT_CALLBACK_USER_PARAM_SOFT:
        value = context->mEventParam;
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid pointer property 0x%04x", pname);
    }

    return value;
}
END_API_FUNC

AL_API void AL_APIENTRY alGetBooleanv(ALenum pname, ALboolean *values)
START_API_FUNC
{
    if(values)
    {
        switch(pname)
        {
            case AL_DOPPLER_FACTOR:
            case AL_DOPPLER_VELOCITY:
            case AL_DISTANCE_MODEL:
            case AL_SPEED_OF_SOUND:
            case AL_DEFERRED_UPDATES_SOFT:
            case AL_GAIN_LIMIT_SOFT:
            case AL_NUM_RESAMPLERS_SOFT:
            case AL_DEFAULT_RESAMPLER_SOFT:
                values[0] = alGetBoolean(pname);
                return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(pname)
    {
    default:
        context->setError(AL_INVALID_VALUE, "Invalid boolean-vector property 0x%04x", pname);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetDoublev(ALenum pname, ALdouble *values)
START_API_FUNC
{
    if(values)
    {
        switch(pname)
        {
            case AL_DOPPLER_FACTOR:
            case AL_DOPPLER_VELOCITY:
            case AL_DISTANCE_MODEL:
            case AL_SPEED_OF_SOUND:
            case AL_DEFERRED_UPDATES_SOFT:
            case AL_GAIN_LIMIT_SOFT:
            case AL_NUM_RESAMPLERS_SOFT:
            case AL_DEFAULT_RESAMPLER_SOFT:
                values[0] = alGetDouble(pname);
                return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(pname)
    {
    default:
        context->setError(AL_INVALID_VALUE, "Invalid double-vector property 0x%04x", pname);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetFloatv(ALenum pname, ALfloat *values)
START_API_FUNC
{
    if(values)
    {
        switch(pname)
        {
            case AL_DOPPLER_FACTOR:
            case AL_DOPPLER_VELOCITY:
            case AL_DISTANCE_MODEL:
            case AL_SPEED_OF_SOUND:
            case AL_DEFERRED_UPDATES_SOFT:
            case AL_GAIN_LIMIT_SOFT:
            case AL_NUM_RESAMPLERS_SOFT:
            case AL_DEFAULT_RESAMPLER_SOFT:
                values[0] = alGetFloat(pname);
                return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(pname)
    {
    default:
        context->setError(AL_INVALID_VALUE, "Invalid float-vector property 0x%04x", pname);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetIntegerv(ALenum pname, ALint *values)
START_API_FUNC
{
    if(values)
    {
        switch(pname)
        {
            case AL_DOPPLER_FACTOR:
            case AL_DOPPLER_VELOCITY:
            case AL_DISTANCE_MODEL:
            case AL_SPEED_OF_SOUND:
            case AL_DEFERRED_UPDATES_SOFT:
            case AL_GAIN_LIMIT_SOFT:
            case AL_NUM_RESAMPLERS_SOFT:
            case AL_DEFAULT_RESAMPLER_SOFT:
                values[0] = alGetInteger(pname);
                return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(pname)
    {
    default:
        context->setError(AL_INVALID_VALUE, "Invalid integer-vector property 0x%04x", pname);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetInteger64vSOFT(ALenum pname, ALint64SOFT *values)
START_API_FUNC
{
    if(values)
    {
        switch(pname)
        {
            case AL_DOPPLER_FACTOR:
            case AL_DOPPLER_VELOCITY:
            case AL_DISTANCE_MODEL:
            case AL_SPEED_OF_SOUND:
            case AL_DEFERRED_UPDATES_SOFT:
            case AL_GAIN_LIMIT_SOFT:
            case AL_NUM_RESAMPLERS_SOFT:
            case AL_DEFAULT_RESAMPLER_SOFT:
                values[0] = alGetInteger64SOFT(pname);
                return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(pname)
    {
    default:
        context->setError(AL_INVALID_VALUE, "Invalid integer64-vector property 0x%04x", pname);
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetPointervSOFT(ALenum pname, ALvoid **values)
START_API_FUNC
{
    if(values)
    {
        switch(pname)
        {
            case AL_EVENT_CALLBACK_FUNCTION_SOFT:
            case AL_EVENT_CALLBACK_USER_PARAM_SOFT:
                values[0] = alGetPointerSOFT(pname);
                return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(pname)
    {
    default:
        context->setError(AL_INVALID_VALUE, "Invalid pointer-vector property 0x%04x", pname);
    }
}
END_API_FUNC

AL_API const ALchar* AL_APIENTRY alGetString(ALenum pname)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return nullptr;

    const ALchar *value{nullptr};
    switch(pname)
    {
    case AL_VENDOR:
        value = alVendor;
        break;

    case AL_VERSION:
        value = alVersion;
        break;

    case AL_RENDERER:
        value = alRenderer;
        break;

    case AL_EXTENSIONS:
        value = context->mExtensionList;
        break;

    case AL_NO_ERROR:
        value = alNoError;
        break;

    case AL_INVALID_NAME:
        value = alErrInvalidName;
        break;

    case AL_INVALID_ENUM:
        value = alErrInvalidEnum;
        break;

    case AL_INVALID_VALUE:
        value = alErrInvalidValue;
        break;

    case AL_INVALID_OPERATION:
        value = alErrInvalidOp;
        break;

    case AL_OUT_OF_MEMORY:
        value = alErrOutOfMemory;
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid string property 0x%04x", pname);
    }
    return value;
}
END_API_FUNC

AL_API void AL_APIENTRY alDopplerFactor(ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!(value >= 0.0f && std::isfinite(value)))
        context->setError(AL_INVALID_VALUE, "Doppler factor %f out of range", value);
    else
    {
        std::lock_guard<std::mutex> _{context->mPropLock};
        context->mDopplerFactor = value;
        DO_UPDATEPROPS();
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alDopplerVelocity(ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!(value >= 0.0f && std::isfinite(value)))
        context->setError(AL_INVALID_VALUE, "Doppler velocity %f out of range", value);
    else
    {
        std::lock_guard<std::mutex> _{context->mPropLock};
        context->mDopplerVelocity = value;
        DO_UPDATEPROPS();
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alSpeedOfSound(ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(!(value > 0.0f && std::isfinite(value)))
        context->setError(AL_INVALID_VALUE, "Speed of sound %f out of range", value);
    else
    {
        std::lock_guard<std::mutex> _{context->mPropLock};
        context->mSpeedOfSound = value;
        DO_UPDATEPROPS();
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alDistanceModel(ALenum value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    if(auto model = DistanceModelFromALenum(value))
    {
        std::lock_guard<std::mutex> _{context->mPropLock};
        context->mDistanceModel = *model;
        if(!context->mSourceDistanceModel)
            DO_UPDATEPROPS();
    }
    else
        context->setError(AL_INVALID_VALUE, "Distance model 0x%04x out of range", value);
}
END_API_FUNC


AL_API void AL_APIENTRY alDeferUpdatesSOFT(void)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    context->deferUpdates();
}
END_API_FUNC

AL_API void AL_APIENTRY alProcessUpdatesSOFT(void)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    context->processUpdates();
}
END_API_FUNC


AL_API const ALchar* AL_APIENTRY alGetStringiSOFT(ALenum pname, ALsizei index)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return nullptr;

    const ALchar *value{nullptr};
    switch(pname)
    {
    case AL_RESAMPLER_NAME_SOFT:
        if(index < 0 || index > static_cast<ALint>(Resampler::Max))
            context->setError(AL_INVALID_VALUE, "Resampler name index %d out of range", index);
        else
            value = GetResamplerName(static_cast<Resampler>(index));
        break;

    default:
        context->setError(AL_INVALID_VALUE, "Invalid string indexed property");
    }
    return value;
}
END_API_FUNC


void UpdateContextProps(ALCcontext *context)
{
    /* Get an unused proprty container, or allocate a new one as needed. */
    ContextProps *props{context->mFreeContextProps.load(std::memory_order_acquire)};
    if(!props)
        props = new ContextProps{};
    else
    {
        ContextProps *next;
        do {
            next = props->next.load(std::memory_order_relaxed);
        } while(context->mFreeContextProps.compare_exchange_weak(props, next,
                std::memory_order_seq_cst, std::memory_order_acquire) == 0);
    }

    /* Copy in current property values. */
    ALlistener &listener = context->mListener;
    props->Position = listener.Position;
    props->Velocity = listener.Velocity;
    props->OrientAt = listener.OrientAt;
    props->OrientUp = listener.OrientUp;
    props->Gain = listener.Gain;
    props->MetersPerUnit = listener.mMetersPerUnit;

    props->DopplerFactor = context->mDopplerFactor;
    props->DopplerVelocity = context->mDopplerVelocity;
    props->SpeedOfSound = context->mSpeedOfSound;

    props->SourceDistanceModel = context->mSourceDistanceModel;
    props->mDistanceModel = context->mDistanceModel;

    /* Set the new container for updating internal parameters. */
    props = context->mParams.ContextUpdate.exchange(props, std::memory_order_acq_rel);
    if(props)
    {
        /* If there was an unused update container, put it back in the
         * freelist.
         */
        AtomicReplaceHead(context->mFreeContextProps, props);
    }
}
