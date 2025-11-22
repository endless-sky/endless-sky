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

#include "listener.h"

#include <cmath>
#include <mutex>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/efx.h"

#include "alc/context.h"
#include "almalloc.h"
#include "atomic.h"
#include "core/except.h"
#include "opthelpers.h"


namespace {

inline void UpdateProps(ALCcontext *context)
{
    if(!context->mDeferUpdates)
    {
        UpdateContextProps(context);
        return;
    }
    context->mPropsDirty = true;
}

#ifdef ALSOFT_EAX
inline void CommitAndUpdateProps(ALCcontext *context)
{
    if(!context->mDeferUpdates)
    {
        if(context->has_eax())
        {
            context->mHoldUpdates.store(true, std::memory_order_release);
            while((context->mUpdateCount.load(std::memory_order_acquire)&1) != 0) {
                /* busy-wait */
            }

            context->eax_commit_and_update_sources();
        }
        UpdateContextProps(context);
        context->mHoldUpdates.store(false, std::memory_order_release);
        return;
    }
    context->mPropsDirty = true;
}

#else

inline void CommitAndUpdateProps(ALCcontext *context)
{ UpdateProps(context); }
#endif

} // namespace

AL_API void AL_APIENTRY alListenerf(ALenum param, ALfloat value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    switch(param)
    {
    case AL_GAIN:
        if(!(value >= 0.0f && std::isfinite(value)))
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Listener gain out of range");
        listener.Gain = value;
        UpdateProps(context.get());
        break;

    case AL_METERS_PER_UNIT:
        if(!(value >= AL_MIN_METERS_PER_UNIT && value <= AL_MAX_METERS_PER_UNIT))
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Listener meters per unit out of range");
        listener.mMetersPerUnit = value;
        UpdateProps(context.get());
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener float property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    switch(param)
    {
    case AL_POSITION:
        if(!(std::isfinite(value1) && std::isfinite(value2) && std::isfinite(value3)))
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Listener position out of range");
        listener.Position[0] = value1;
        listener.Position[1] = value2;
        listener.Position[2] = value3;
        CommitAndUpdateProps(context.get());
        break;

    case AL_VELOCITY:
        if(!(std::isfinite(value1) && std::isfinite(value2) && std::isfinite(value3)))
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Listener velocity out of range");
        listener.Velocity[0] = value1;
        listener.Velocity[1] = value2;
        listener.Velocity[2] = value3;
        CommitAndUpdateProps(context.get());
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener 3-float property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alListenerfv(ALenum param, const ALfloat *values)
START_API_FUNC
{
    if(values)
    {
        switch(param)
        {
        case AL_GAIN:
        case AL_METERS_PER_UNIT:
            alListenerf(param, values[0]);
            return;

        case AL_POSITION:
        case AL_VELOCITY:
            alListener3f(param, values[0], values[1], values[2]);
            return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!values) SETERR_RETURN(context, AL_INVALID_VALUE,, "NULL pointer");
    switch(param)
    {
    case AL_ORIENTATION:
        if(!(std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]) &&
             std::isfinite(values[3]) && std::isfinite(values[4]) && std::isfinite(values[5])))
            SETERR_RETURN(context, AL_INVALID_VALUE,, "Listener orientation out of range");
        /* AT then UP */
        listener.OrientAt[0] = values[0];
        listener.OrientAt[1] = values[1];
        listener.OrientAt[2] = values[2];
        listener.OrientUp[0] = values[3];
        listener.OrientUp[1] = values[4];
        listener.OrientUp[2] = values[5];
        CommitAndUpdateProps(context.get());
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener float-vector property");
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alListeneri(ALenum param, ALint /*value*/)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    switch(param)
    {
    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener integer property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alListener3i(ALenum param, ALint value1, ALint value2, ALint value3)
START_API_FUNC
{
    switch(param)
    {
    case AL_POSITION:
    case AL_VELOCITY:
        alListener3f(param, static_cast<ALfloat>(value1), static_cast<ALfloat>(value2), static_cast<ALfloat>(value3));
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    switch(param)
    {
    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener 3-integer property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alListeneriv(ALenum param, const ALint *values)
START_API_FUNC
{
    if(values)
    {
        ALfloat fvals[6];
        switch(param)
        {
        case AL_POSITION:
        case AL_VELOCITY:
            alListener3f(param, static_cast<ALfloat>(values[0]), static_cast<ALfloat>(values[1]), static_cast<ALfloat>(values[2]));
            return;

        case AL_ORIENTATION:
            fvals[0] = static_cast<ALfloat>(values[0]);
            fvals[1] = static_cast<ALfloat>(values[1]);
            fvals[2] = static_cast<ALfloat>(values[2]);
            fvals[3] = static_cast<ALfloat>(values[3]);
            fvals[4] = static_cast<ALfloat>(values[4]);
            fvals[5] = static_cast<ALfloat>(values[5]);
            alListenerfv(param, fvals);
            return;
        }
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener integer-vector property");
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alGetListenerf(ALenum param, ALfloat *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!value)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    case AL_GAIN:
        *value = listener.Gain;
        break;

    case AL_METERS_PER_UNIT:
        *value = listener.mMetersPerUnit;
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener float property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetListener3f(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!value1 || !value2 || !value3)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    case AL_POSITION:
        *value1 = listener.Position[0];
        *value2 = listener.Position[1];
        *value3 = listener.Position[2];
        break;

    case AL_VELOCITY:
        *value1 = listener.Velocity[0];
        *value2 = listener.Velocity[1];
        *value3 = listener.Velocity[2];
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener 3-float property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetListenerfv(ALenum param, ALfloat *values)
START_API_FUNC
{
    switch(param)
    {
    case AL_GAIN:
    case AL_METERS_PER_UNIT:
        alGetListenerf(param, values);
        return;

    case AL_POSITION:
    case AL_VELOCITY:
        alGetListener3f(param, values+0, values+1, values+2);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    case AL_ORIENTATION:
        // AT then UP
        values[0] = listener.OrientAt[0];
        values[1] = listener.OrientAt[1];
        values[2] = listener.OrientAt[2];
        values[3] = listener.OrientUp[0];
        values[4] = listener.OrientUp[1];
        values[5] = listener.OrientUp[2];
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener float-vector property");
    }
}
END_API_FUNC


AL_API void AL_APIENTRY alGetListeneri(ALenum param, ALint *value)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!value)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener integer property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetListener3i(ALenum param, ALint *value1, ALint *value2, ALint *value3)
START_API_FUNC
{
    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!value1 || !value2 || !value3)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    case AL_POSITION:
        *value1 = static_cast<ALint>(listener.Position[0]);
        *value2 = static_cast<ALint>(listener.Position[1]);
        *value3 = static_cast<ALint>(listener.Position[2]);
        break;

    case AL_VELOCITY:
        *value1 = static_cast<ALint>(listener.Velocity[0]);
        *value2 = static_cast<ALint>(listener.Velocity[1]);
        *value3 = static_cast<ALint>(listener.Velocity[2]);
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener 3-integer property");
    }
}
END_API_FUNC

AL_API void AL_APIENTRY alGetListeneriv(ALenum param, ALint* values)
START_API_FUNC
{
    switch(param)
    {
    case AL_POSITION:
    case AL_VELOCITY:
        alGetListener3i(param, values+0, values+1, values+2);
        return;
    }

    ContextRef context{GetContextRef()};
    if UNLIKELY(!context) return;

    ALlistener &listener = context->mListener;
    std::lock_guard<std::mutex> _{context->mPropLock};
    if(!values)
        context->setError(AL_INVALID_VALUE, "NULL pointer");
    else switch(param)
    {
    case AL_ORIENTATION:
        // AT then UP
        values[0] = static_cast<ALint>(listener.OrientAt[0]);
        values[1] = static_cast<ALint>(listener.OrientAt[1]);
        values[2] = static_cast<ALint>(listener.OrientAt[2]);
        values[3] = static_cast<ALint>(listener.OrientUp[0]);
        values[4] = static_cast<ALint>(listener.OrientUp[1]);
        values[5] = static_cast<ALint>(listener.OrientUp[2]);
        break;

    default:
        context->setError(AL_INVALID_ENUM, "Invalid listener integer-vector property");
    }
}
END_API_FUNC
