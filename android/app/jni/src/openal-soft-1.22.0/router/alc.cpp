
#include "config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mutex>
#include <algorithm>
#include <array>

#include "AL/alc.h"
#include "alstring.h"
#include "router.h"


#define DECL(x) { #x, reinterpret_cast<void*>(x) }
struct FuncExportEntry {
    const char *funcName;
    void *address;
};
static const std::array<FuncExportEntry,128> alcFunctions{{
    DECL(alcCreateContext),
    DECL(alcMakeContextCurrent),
    DECL(alcProcessContext),
    DECL(alcSuspendContext),
    DECL(alcDestroyContext),
    DECL(alcGetCurrentContext),
    DECL(alcGetContextsDevice),
    DECL(alcOpenDevice),
    DECL(alcCloseDevice),
    DECL(alcGetError),
    DECL(alcIsExtensionPresent),
    DECL(alcGetProcAddress),
    DECL(alcGetEnumValue),
    DECL(alcGetString),
    DECL(alcGetIntegerv),
    DECL(alcCaptureOpenDevice),
    DECL(alcCaptureCloseDevice),
    DECL(alcCaptureStart),
    DECL(alcCaptureStop),
    DECL(alcCaptureSamples),

    DECL(alcSetThreadContext),
    DECL(alcGetThreadContext),

    DECL(alEnable),
    DECL(alDisable),
    DECL(alIsEnabled),
    DECL(alGetString),
    DECL(alGetBooleanv),
    DECL(alGetIntegerv),
    DECL(alGetFloatv),
    DECL(alGetDoublev),
    DECL(alGetBoolean),
    DECL(alGetInteger),
    DECL(alGetFloat),
    DECL(alGetDouble),
    DECL(alGetError),
    DECL(alIsExtensionPresent),
    DECL(alGetProcAddress),
    DECL(alGetEnumValue),
    DECL(alListenerf),
    DECL(alListener3f),
    DECL(alListenerfv),
    DECL(alListeneri),
    DECL(alListener3i),
    DECL(alListeneriv),
    DECL(alGetListenerf),
    DECL(alGetListener3f),
    DECL(alGetListenerfv),
    DECL(alGetListeneri),
    DECL(alGetListener3i),
    DECL(alGetListeneriv),
    DECL(alGenSources),
    DECL(alDeleteSources),
    DECL(alIsSource),
    DECL(alSourcef),
    DECL(alSource3f),
    DECL(alSourcefv),
    DECL(alSourcei),
    DECL(alSource3i),
    DECL(alSourceiv),
    DECL(alGetSourcef),
    DECL(alGetSource3f),
    DECL(alGetSourcefv),
    DECL(alGetSourcei),
    DECL(alGetSource3i),
    DECL(alGetSourceiv),
    DECL(alSourcePlayv),
    DECL(alSourceStopv),
    DECL(alSourceRewindv),
    DECL(alSourcePausev),
    DECL(alSourcePlay),
    DECL(alSourceStop),
    DECL(alSourceRewind),
    DECL(alSourcePause),
    DECL(alSourceQueueBuffers),
    DECL(alSourceUnqueueBuffers),
    DECL(alGenBuffers),
    DECL(alDeleteBuffers),
    DECL(alIsBuffer),
    DECL(alBufferData),
    DECL(alBufferf),
    DECL(alBuffer3f),
    DECL(alBufferfv),
    DECL(alBufferi),
    DECL(alBuffer3i),
    DECL(alBufferiv),
    DECL(alGetBufferf),
    DECL(alGetBuffer3f),
    DECL(alGetBufferfv),
    DECL(alGetBufferi),
    DECL(alGetBuffer3i),
    DECL(alGetBufferiv),
    DECL(alDopplerFactor),
    DECL(alDopplerVelocity),
    DECL(alSpeedOfSound),
    DECL(alDistanceModel),
}};
#undef DECL

#define DECL(x) { #x, (x) }
struct EnumExportEntry {
    const ALCchar *enumName;
    ALCenum value;
};
static const std::array<EnumExportEntry,92> alcEnumerations{{
    DECL(ALC_INVALID),
    DECL(ALC_FALSE),
    DECL(ALC_TRUE),

    DECL(ALC_MAJOR_VERSION),
    DECL(ALC_MINOR_VERSION),
    DECL(ALC_ATTRIBUTES_SIZE),
    DECL(ALC_ALL_ATTRIBUTES),
    DECL(ALC_DEFAULT_DEVICE_SPECIFIER),
    DECL(ALC_DEVICE_SPECIFIER),
    DECL(ALC_ALL_DEVICES_SPECIFIER),
    DECL(ALC_DEFAULT_ALL_DEVICES_SPECIFIER),
    DECL(ALC_EXTENSIONS),
    DECL(ALC_FREQUENCY),
    DECL(ALC_REFRESH),
    DECL(ALC_SYNC),
    DECL(ALC_MONO_SOURCES),
    DECL(ALC_STEREO_SOURCES),
    DECL(ALC_CAPTURE_DEVICE_SPECIFIER),
    DECL(ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER),
    DECL(ALC_CAPTURE_SAMPLES),

    DECL(ALC_NO_ERROR),
    DECL(ALC_INVALID_DEVICE),
    DECL(ALC_INVALID_CONTEXT),
    DECL(ALC_INVALID_ENUM),
    DECL(ALC_INVALID_VALUE),
    DECL(ALC_OUT_OF_MEMORY),

    DECL(AL_INVALID),
    DECL(AL_NONE),
    DECL(AL_FALSE),
    DECL(AL_TRUE),

    DECL(AL_SOURCE_RELATIVE),
    DECL(AL_CONE_INNER_ANGLE),
    DECL(AL_CONE_OUTER_ANGLE),
    DECL(AL_PITCH),
    DECL(AL_POSITION),
    DECL(AL_DIRECTION),
    DECL(AL_VELOCITY),
    DECL(AL_LOOPING),
    DECL(AL_BUFFER),
    DECL(AL_GAIN),
    DECL(AL_MIN_GAIN),
    DECL(AL_MAX_GAIN),
    DECL(AL_ORIENTATION),
    DECL(AL_REFERENCE_DISTANCE),
    DECL(AL_ROLLOFF_FACTOR),
    DECL(AL_CONE_OUTER_GAIN),
    DECL(AL_MAX_DISTANCE),
    DECL(AL_SEC_OFFSET),
    DECL(AL_SAMPLE_OFFSET),
    DECL(AL_BYTE_OFFSET),
    DECL(AL_SOURCE_TYPE),
    DECL(AL_STATIC),
    DECL(AL_STREAMING),
    DECL(AL_UNDETERMINED),

    DECL(AL_SOURCE_STATE),
    DECL(AL_INITIAL),
    DECL(AL_PLAYING),
    DECL(AL_PAUSED),
    DECL(AL_STOPPED),

    DECL(AL_BUFFERS_QUEUED),
    DECL(AL_BUFFERS_PROCESSED),

    DECL(AL_FORMAT_MONO8),
    DECL(AL_FORMAT_MONO16),
    DECL(AL_FORMAT_STEREO8),
    DECL(AL_FORMAT_STEREO16),

    DECL(AL_FREQUENCY),
    DECL(AL_BITS),
    DECL(AL_CHANNELS),
    DECL(AL_SIZE),

    DECL(AL_UNUSED),
    DECL(AL_PENDING),
    DECL(AL_PROCESSED),

    DECL(AL_NO_ERROR),
    DECL(AL_INVALID_NAME),
    DECL(AL_INVALID_ENUM),
    DECL(AL_INVALID_VALUE),
    DECL(AL_INVALID_OPERATION),
    DECL(AL_OUT_OF_MEMORY),

    DECL(AL_VENDOR),
    DECL(AL_VERSION),
    DECL(AL_RENDERER),
    DECL(AL_EXTENSIONS),

    DECL(AL_DOPPLER_FACTOR),
    DECL(AL_DOPPLER_VELOCITY),
    DECL(AL_DISTANCE_MODEL),
    DECL(AL_SPEED_OF_SOUND),

    DECL(AL_INVERSE_DISTANCE),
    DECL(AL_INVERSE_DISTANCE_CLAMPED),
    DECL(AL_LINEAR_DISTANCE),
    DECL(AL_LINEAR_DISTANCE_CLAMPED),
    DECL(AL_EXPONENT_DISTANCE),
    DECL(AL_EXPONENT_DISTANCE_CLAMPED),
}};
#undef DECL

static const ALCchar alcNoError[] = "No Error";
static const ALCchar alcErrInvalidDevice[] = "Invalid Device";
static const ALCchar alcErrInvalidContext[] = "Invalid Context";
static const ALCchar alcErrInvalidEnum[] = "Invalid Enum";
static const ALCchar alcErrInvalidValue[] = "Invalid Value";
static const ALCchar alcErrOutOfMemory[] = "Out of Memory";
static const ALCchar alcExtensionList[] =
    "ALC_ENUMERATE_ALL_EXT ALC_ENUMERATION_EXT ALC_EXT_CAPTURE "
    "ALC_EXT_thread_local_context";

static const ALCint alcMajorVersion = 1;
static const ALCint alcMinorVersion = 1;


static std::recursive_mutex EnumerationLock;
static std::mutex ContextSwitchLock;

static std::atomic<ALCenum> LastError{ALC_NO_ERROR};
static PtrIntMap DeviceIfaceMap;
static PtrIntMap ContextIfaceMap;


typedef struct EnumeratedList {
    std::vector<ALCchar> Names;
    std::vector<ALCint> Indicies;

    void clear()
    {
        Names.clear();
        Indicies.clear();
    }
} EnumeratedList;
static EnumeratedList DevicesList;
static EnumeratedList AllDevicesList;
static EnumeratedList CaptureDevicesList;

static void AppendDeviceList(EnumeratedList *list, const ALCchar *names, ALint idx)
{
    const ALCchar *name_end = names;
    if(!name_end) return;

    ALCsizei count = 0;
    while(*name_end)
    {
        TRACE("Enumerated \"%s\", driver %d\n", name_end, idx);
        ++count;
        name_end += strlen(name_end)+1;
    }
    if(names == name_end)
        return;

    list->Names.reserve(list->Names.size() + (name_end - names) + 1);
    list->Names.insert(list->Names.cend(), names, name_end);

    list->Indicies.reserve(list->Indicies.size() + count);
    list->Indicies.insert(list->Indicies.cend(), count, idx);
}

static ALint GetDriverIndexForName(const EnumeratedList *list, const ALCchar *name)
{
    const ALCchar *devnames = list->Names.data();
    const ALCint *index = list->Indicies.data();

    while(devnames && *devnames)
    {
        if(strcmp(name, devnames) == 0)
            return *index;
        devnames += strlen(devnames)+1;
        index++;
    }
    return -1;
}


ALC_API ALCdevice* ALC_APIENTRY alcOpenDevice(const ALCchar *devicename)
{
    ALCdevice *device = nullptr;
    ALint idx = 0;

    /* Prior to the enumeration extension, apps would hardcode these names as a
     * quality hint for the wrapper driver. Ignore them since there's no sane
     * way to map them.
     */
    if(devicename && (devicename[0] == '\0' ||
                      strcmp(devicename, "DirectSound3D") == 0 ||
                      strcmp(devicename, "DirectSound") == 0 ||
                      strcmp(devicename, "MMSYSTEM") == 0))
        devicename = nullptr;
    if(devicename)
    {
        {
            std::lock_guard<std::recursive_mutex> _{EnumerationLock};
            if(DevicesList.Names.empty())
                (void)alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
            idx = GetDriverIndexForName(&DevicesList, devicename);
            if(idx < 0)
            {
                if(AllDevicesList.Names.empty())
                    (void)alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
                idx = GetDriverIndexForName(&AllDevicesList, devicename);
            }
        }

        if(idx < 0)
        {
            LastError.store(ALC_INVALID_VALUE);
            TRACE("Failed to find driver for name \"%s\"\n", devicename);
            return nullptr;
        }
        TRACE("Found driver %d for name \"%s\"\n", idx, devicename);
        device = DriverList[idx].alcOpenDevice(devicename);
    }
    else
    {
        for(const auto &drv : DriverList)
        {
            if(drv.ALCVer >= MAKE_ALC_VER(1, 1) ||
               drv.alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
            {
                TRACE("Using default device from driver %d\n", idx);
                device = drv.alcOpenDevice(nullptr);
                break;
            }
            ++idx;
        }
    }

    if(device)
    {
        if(DeviceIfaceMap.insert(device, idx) != ALC_NO_ERROR)
        {
            DriverList[idx].alcCloseDevice(device);
            device = nullptr;
        }
    }

    return device;
}

ALC_API ALCboolean ALC_APIENTRY alcCloseDevice(ALCdevice *device)
{
    ALint idx;

    if(!device || (idx=DeviceIfaceMap.lookupByKey(device)) < 0)
    {
        LastError.store(ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }
    if(!DriverList[idx].alcCloseDevice(device))
        return ALC_FALSE;
    DeviceIfaceMap.removeByKey(device);
    return ALC_TRUE;
}


ALC_API ALCcontext* ALC_APIENTRY alcCreateContext(ALCdevice *device, const ALCint *attrlist)
{
    ALCcontext *context;
    ALint idx;

    if(!device || (idx=DeviceIfaceMap.lookupByKey(device)) < 0)
    {
        LastError.store(ALC_INVALID_DEVICE);
        return nullptr;
    }
    context = DriverList[idx].alcCreateContext(device, attrlist);
    if(context)
    {
        if(ContextIfaceMap.insert(context, idx) != ALC_NO_ERROR)
        {
            DriverList[idx].alcDestroyContext(context);
            context = nullptr;
        }
    }

    return context;
}

ALC_API ALCboolean ALC_APIENTRY alcMakeContextCurrent(ALCcontext *context)
{
    ALint idx = -1;

    std::lock_guard<std::mutex> _{ContextSwitchLock};
    if(context)
    {
        idx = ContextIfaceMap.lookupByKey(context);
        if(idx < 0)
        {
            LastError.store(ALC_INVALID_CONTEXT);
            return ALC_FALSE;
        }
        if(!DriverList[idx].alcMakeContextCurrent(context))
            return ALC_FALSE;
    }

    /* Unset the context from the old driver if it's different from the new
     * current one.
     */
    if(idx < 0)
    {
        DriverIface *oldiface = GetThreadDriver();
        if(oldiface) oldiface->alcSetThreadContext(nullptr);
        oldiface = CurrentCtxDriver.exchange(nullptr);
        if(oldiface) oldiface->alcMakeContextCurrent(nullptr);
    }
    else
    {
        DriverIface *oldiface = GetThreadDriver();
        if(oldiface && oldiface != &DriverList[idx])
            oldiface->alcSetThreadContext(nullptr);
        oldiface = CurrentCtxDriver.exchange(&DriverList[idx]);
        if(oldiface && oldiface != &DriverList[idx])
            oldiface->alcMakeContextCurrent(nullptr);
    }
    SetThreadDriver(nullptr);

    return ALC_TRUE;
}

ALC_API void ALC_APIENTRY alcProcessContext(ALCcontext *context)
{
    if(context)
    {
        ALint idx = ContextIfaceMap.lookupByKey(context);
        if(idx >= 0)
            return DriverList[idx].alcProcessContext(context);
    }
    LastError.store(ALC_INVALID_CONTEXT);
}

ALC_API void ALC_APIENTRY alcSuspendContext(ALCcontext *context)
{
    if(context)
    {
        ALint idx = ContextIfaceMap.lookupByKey(context);
        if(idx >= 0)
            return DriverList[idx].alcSuspendContext(context);
    }
    LastError.store(ALC_INVALID_CONTEXT);
}

ALC_API void ALC_APIENTRY alcDestroyContext(ALCcontext *context)
{
    ALint idx;

    if(!context || (idx=ContextIfaceMap.lookupByKey(context)) < 0)
    {
        LastError.store(ALC_INVALID_CONTEXT);
        return;
    }

    DriverList[idx].alcDestroyContext(context);
    ContextIfaceMap.removeByKey(context);
}

ALC_API ALCcontext* ALC_APIENTRY alcGetCurrentContext(void)
{
    DriverIface *iface = GetThreadDriver();
    if(!iface) iface = CurrentCtxDriver.load();
    return iface ? iface->alcGetCurrentContext() : nullptr;
}

ALC_API ALCdevice* ALC_APIENTRY alcGetContextsDevice(ALCcontext *context)
{
    if(context)
    {
        ALint idx = ContextIfaceMap.lookupByKey(context);
        if(idx >= 0)
            return DriverList[idx].alcGetContextsDevice(context);
    }
    LastError.store(ALC_INVALID_CONTEXT);
    return nullptr;
}


ALC_API ALCenum ALC_APIENTRY alcGetError(ALCdevice *device)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx < 0) return ALC_INVALID_DEVICE;
        return DriverList[idx].alcGetError(device);
    }
    return LastError.exchange(ALC_NO_ERROR);
}

ALC_API ALCboolean ALC_APIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCchar *extname)
{
    const char *ptr;
    size_t len;

    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx < 0)
        {
            LastError.store(ALC_INVALID_DEVICE);
            return ALC_FALSE;
        }
        return DriverList[idx].alcIsExtensionPresent(device, extname);
    }

    len = strlen(extname);
    ptr = alcExtensionList;
    while(ptr && *ptr)
    {
        if(al::strncasecmp(ptr, extname, len) == 0 && (ptr[len] == '\0' || isspace(ptr[len])))
            return ALC_TRUE;
        if((ptr=strchr(ptr, ' ')) != nullptr)
        {
            do {
                ++ptr;
            } while(isspace(*ptr));
        }
    }
    return ALC_FALSE;
}

ALC_API void* ALC_APIENTRY alcGetProcAddress(ALCdevice *device, const ALCchar *funcname)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx < 0)
        {
            LastError.store(ALC_INVALID_DEVICE);
            return nullptr;
        }
        return DriverList[idx].alcGetProcAddress(device, funcname);
    }

    auto iter = std::find_if(alcFunctions.cbegin(), alcFunctions.cend(),
        [funcname](const FuncExportEntry &entry) -> bool
        { return strcmp(funcname, entry.funcName) == 0; }
    );
    return (iter != alcFunctions.cend()) ? iter->address : nullptr;
}

ALC_API ALCenum ALC_APIENTRY alcGetEnumValue(ALCdevice *device, const ALCchar *enumname)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx < 0)
        {
            LastError.store(ALC_INVALID_DEVICE);
            return 0;
        }
        return DriverList[idx].alcGetEnumValue(device, enumname);
    }

    auto iter = std::find_if(alcEnumerations.cbegin(), alcEnumerations.cend(),
        [enumname](const EnumExportEntry &entry) -> bool
        { return strcmp(enumname, entry.enumName) == 0; }
    );
    return (iter != alcEnumerations.cend()) ? iter->value : 0;
}

ALC_API const ALCchar* ALC_APIENTRY alcGetString(ALCdevice *device, ALCenum param)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx < 0)
        {
            LastError.store(ALC_INVALID_DEVICE);
            return nullptr;
        }
        return DriverList[idx].alcGetString(device, param);
    }

    switch(param)
    {
    case ALC_NO_ERROR:
        return alcNoError;
    case ALC_INVALID_ENUM:
        return alcErrInvalidEnum;
    case ALC_INVALID_VALUE:
        return alcErrInvalidValue;
    case ALC_INVALID_DEVICE:
        return alcErrInvalidDevice;
    case ALC_INVALID_CONTEXT:
        return alcErrInvalidContext;
    case ALC_OUT_OF_MEMORY:
        return alcErrOutOfMemory;
    case ALC_EXTENSIONS:
        return alcExtensionList;

    case ALC_DEVICE_SPECIFIER:
    {
        std::lock_guard<std::recursive_mutex> _{EnumerationLock};
        DevicesList.clear();
        ALint idx{0};
        for(const auto &drv : DriverList)
        {
            /* Only enumerate names from drivers that support it. */
            if(drv.ALCVer >= MAKE_ALC_VER(1, 1)
                || drv.alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
                AppendDeviceList(&DevicesList,
                    drv.alcGetString(nullptr, ALC_DEVICE_SPECIFIER), idx);
            ++idx;
        }
        /* Ensure the list is double-null termianted. */
        if(DevicesList.Names.empty())
            DevicesList.Names.emplace_back('\0');
        DevicesList.Names.emplace_back('\0');
        return DevicesList.Names.data();
    }

    case ALC_ALL_DEVICES_SPECIFIER:
    {
        std::lock_guard<std::recursive_mutex> _{EnumerationLock};
        AllDevicesList.clear();
        ALint idx{0};
        for(const auto &drv : DriverList)
        {
            /* If the driver doesn't support ALC_ENUMERATE_ALL_EXT, substitute
             * standard enumeration.
             */
            if(drv.alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT"))
                AppendDeviceList(&AllDevicesList,
                    drv.alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER), idx);
            else if(drv.ALCVer >= MAKE_ALC_VER(1, 1)
                || drv.alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
                AppendDeviceList(&AllDevicesList,
                    drv.alcGetString(nullptr, ALC_DEVICE_SPECIFIER), idx);
            ++idx;
        }
        /* Ensure the list is double-null termianted. */
        if(AllDevicesList.Names.empty())
            AllDevicesList.Names.emplace_back('\0');
        AllDevicesList.Names.emplace_back('\0');
        return AllDevicesList.Names.data();
    }

    case ALC_CAPTURE_DEVICE_SPECIFIER:
    {
        std::lock_guard<std::recursive_mutex> _{EnumerationLock};
        CaptureDevicesList.clear();
        ALint idx{0};
        for(const auto &drv : DriverList)
        {
            if(drv.ALCVer >= MAKE_ALC_VER(1, 1)
                || drv.alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE"))
                AppendDeviceList(&CaptureDevicesList,
                    drv.alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER), idx);
            ++idx;
        }
        /* Ensure the list is double-null termianted. */
        if(CaptureDevicesList.Names.empty())
            CaptureDevicesList.Names.emplace_back('\0');
        CaptureDevicesList.Names.emplace_back('\0');
        return CaptureDevicesList.Names.data();
    }

    case ALC_DEFAULT_DEVICE_SPECIFIER:
    {
        auto iter = std::find_if(DriverList.cbegin(), DriverList.cend(),
            [](const DriverIface &drv) -> bool
            {
                return drv.ALCVer >= MAKE_ALC_VER(1, 1)
                    || drv.alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");
            }
        );
        if(iter != DriverList.cend())
            return iter->alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
        return "";
    }

    case ALC_DEFAULT_ALL_DEVICES_SPECIFIER:
    {
        auto iter = std::find_if(DriverList.cbegin(), DriverList.cend(),
            [](const DriverIface &drv) -> bool
            { return drv.alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") != ALC_FALSE; });
        if(iter != DriverList.cend())
            return iter->alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
        return "";
    }

    case ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER:
    {
        auto iter = std::find_if(DriverList.cbegin(), DriverList.cend(),
            [](const DriverIface &drv) -> bool
            {
                return drv.ALCVer >= MAKE_ALC_VER(1, 1)
                    || drv.alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE");
            }
        );
        if(iter != DriverList.cend())
            return iter->alcGetString(nullptr, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
        return "";
    }

    default:
        LastError.store(ALC_INVALID_ENUM);
        break;
    }
    return nullptr;
}

ALC_API void ALC_APIENTRY alcGetIntegerv(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx < 0)
        {
            LastError.store(ALC_INVALID_DEVICE);
            return;
        }
        return DriverList[idx].alcGetIntegerv(device, param, size, values);
    }

    if(size <= 0 || values == nullptr)
    {
        LastError.store(ALC_INVALID_VALUE);
        return;
    }

    switch(param)
    {
        case ALC_MAJOR_VERSION:
            if(size >= 1)
            {
                values[0] = alcMajorVersion;
                return;
            }
            /*fall-through*/
        case ALC_MINOR_VERSION:
            if(size >= 1)
            {
                values[0] = alcMinorVersion;
                return;
            }
            LastError.store(ALC_INVALID_VALUE);
            return;

        case ALC_ATTRIBUTES_SIZE:
        case ALC_ALL_ATTRIBUTES:
        case ALC_FREQUENCY:
        case ALC_REFRESH:
        case ALC_SYNC:
        case ALC_MONO_SOURCES:
        case ALC_STEREO_SOURCES:
        case ALC_CAPTURE_SAMPLES:
            LastError.store(ALC_INVALID_DEVICE);
            return;

        default:
            LastError.store(ALC_INVALID_ENUM);
            return;
    }
}


ALC_API ALCdevice* ALC_APIENTRY alcCaptureOpenDevice(const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize)
{
    ALCdevice *device = nullptr;
    ALint idx = 0;

    if(devicename && devicename[0] == '\0')
        devicename = nullptr;
    if(devicename)
    {
        {
            std::lock_guard<std::recursive_mutex> _{EnumerationLock};
            if(CaptureDevicesList.Names.empty())
                (void)alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);
            idx = GetDriverIndexForName(&CaptureDevicesList, devicename);
        }

        if(idx < 0)
        {
            LastError.store(ALC_INVALID_VALUE);
            TRACE("Failed to find driver for name \"%s\"\n", devicename);
            return nullptr;
        }
        TRACE("Found driver %d for name \"%s\"\n", idx, devicename);
        device = DriverList[idx].alcCaptureOpenDevice(devicename, frequency, format, buffersize);
    }
    else
    {
        for(const auto &drv : DriverList)
        {
            if(drv.ALCVer >= MAKE_ALC_VER(1, 1)
                || drv.alcIsExtensionPresent(nullptr, "ALC_EXT_CAPTURE"))
            {
                TRACE("Using default capture device from driver %d\n", idx);
                device = drv.alcCaptureOpenDevice(nullptr, frequency, format, buffersize);
                break;
            }
            ++idx;
        }
    }

    if(device)
    {
        if(DeviceIfaceMap.insert(device, idx) != ALC_NO_ERROR)
        {
            DriverList[idx].alcCaptureCloseDevice(device);
            device = nullptr;
        }
    }

    return device;
}

ALC_API ALCboolean ALC_APIENTRY alcCaptureCloseDevice(ALCdevice *device)
{
    ALint idx;

    if(!device || (idx=DeviceIfaceMap.lookupByKey(device)) < 0)
    {
        LastError.store(ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }
    if(!DriverList[idx].alcCaptureCloseDevice(device))
        return ALC_FALSE;
    DeviceIfaceMap.removeByKey(device);
    return ALC_TRUE;
}

ALC_API void ALC_APIENTRY alcCaptureStart(ALCdevice *device)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx >= 0)
            return DriverList[idx].alcCaptureStart(device);
    }
    LastError.store(ALC_INVALID_DEVICE);
}

ALC_API void ALC_APIENTRY alcCaptureStop(ALCdevice *device)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx >= 0)
            return DriverList[idx].alcCaptureStop(device);
    }
    LastError.store(ALC_INVALID_DEVICE);
}

ALC_API void ALC_APIENTRY alcCaptureSamples(ALCdevice *device, ALCvoid *buffer, ALCsizei samples)
{
    if(device)
    {
        ALint idx = DeviceIfaceMap.lookupByKey(device);
        if(idx >= 0)
            return DriverList[idx].alcCaptureSamples(device, buffer, samples);
    }
    LastError.store(ALC_INVALID_DEVICE);
}


ALC_API ALCboolean ALC_APIENTRY alcSetThreadContext(ALCcontext *context)
{
    ALCenum err = ALC_INVALID_CONTEXT;
    ALint idx;

    if(!context)
    {
        DriverIface *oldiface = GetThreadDriver();
        if(oldiface && !oldiface->alcSetThreadContext(nullptr))
            return ALC_FALSE;
        SetThreadDriver(nullptr);
        return ALC_TRUE;
    }

    idx = ContextIfaceMap.lookupByKey(context);
    if(idx >= 0)
    {
        if(DriverList[idx].alcSetThreadContext(context))
        {
            DriverIface *oldiface = GetThreadDriver();
            if(oldiface != &DriverList[idx])
            {
                SetThreadDriver(&DriverList[idx]);
                if(oldiface) oldiface->alcSetThreadContext(nullptr);
            }
            return ALC_TRUE;
        }
        err = DriverList[idx].alcGetError(nullptr);
    }
    LastError.store(err);
    return ALC_FALSE;
}

ALC_API ALCcontext* ALC_APIENTRY alcGetThreadContext(void)
{
    DriverIface *iface = GetThreadDriver();
    if(iface) return iface->alcGetThreadContext();
    return nullptr;
}
