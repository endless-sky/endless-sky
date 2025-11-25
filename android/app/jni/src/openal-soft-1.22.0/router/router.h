#ifndef ROUTER_ROUTER_H
#define ROUTER_ROUTER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>

#include <stdio.h>

#include <atomic>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "AL/alc.h"
#include "AL/al.h"
#include "AL/alext.h"


#define MAKE_ALC_VER(major, minor) (((major)<<8) | (minor))

struct DriverIface {
    std::wstring Name;
    HMODULE Module{nullptr};
    int ALCVer{0};

    LPALCCREATECONTEXT alcCreateContext{nullptr};
    LPALCMAKECONTEXTCURRENT alcMakeContextCurrent{nullptr};
    LPALCPROCESSCONTEXT alcProcessContext{nullptr};
    LPALCSUSPENDCONTEXT alcSuspendContext{nullptr};
    LPALCDESTROYCONTEXT alcDestroyContext{nullptr};
    LPALCGETCURRENTCONTEXT alcGetCurrentContext{nullptr};
    LPALCGETCONTEXTSDEVICE alcGetContextsDevice{nullptr};
    LPALCOPENDEVICE alcOpenDevice{nullptr};
    LPALCCLOSEDEVICE alcCloseDevice{nullptr};
    LPALCGETERROR alcGetError{nullptr};
    LPALCISEXTENSIONPRESENT alcIsExtensionPresent{nullptr};
    LPALCGETPROCADDRESS alcGetProcAddress{nullptr};
    LPALCGETENUMVALUE alcGetEnumValue{nullptr};
    LPALCGETSTRING alcGetString{nullptr};
    LPALCGETINTEGERV alcGetIntegerv{nullptr};
    LPALCCAPTUREOPENDEVICE alcCaptureOpenDevice{nullptr};
    LPALCCAPTURECLOSEDEVICE alcCaptureCloseDevice{nullptr};
    LPALCCAPTURESTART alcCaptureStart{nullptr};
    LPALCCAPTURESTOP alcCaptureStop{nullptr};
    LPALCCAPTURESAMPLES alcCaptureSamples{nullptr};

    PFNALCSETTHREADCONTEXTPROC alcSetThreadContext{nullptr};
    PFNALCGETTHREADCONTEXTPROC alcGetThreadContext{nullptr};

    LPALENABLE alEnable{nullptr};
    LPALDISABLE alDisable{nullptr};
    LPALISENABLED alIsEnabled{nullptr};
    LPALGETSTRING alGetString{nullptr};
    LPALGETBOOLEANV alGetBooleanv{nullptr};
    LPALGETINTEGERV alGetIntegerv{nullptr};
    LPALGETFLOATV alGetFloatv{nullptr};
    LPALGETDOUBLEV alGetDoublev{nullptr};
    LPALGETBOOLEAN alGetBoolean{nullptr};
    LPALGETINTEGER alGetInteger{nullptr};
    LPALGETFLOAT alGetFloat{nullptr};
    LPALGETDOUBLE alGetDouble{nullptr};
    LPALGETERROR alGetError{nullptr};
    LPALISEXTENSIONPRESENT alIsExtensionPresent{nullptr};
    LPALGETPROCADDRESS alGetProcAddress{nullptr};
    LPALGETENUMVALUE alGetEnumValue{nullptr};
    LPALLISTENERF alListenerf{nullptr};
    LPALLISTENER3F alListener3f{nullptr};
    LPALLISTENERFV alListenerfv{nullptr};
    LPALLISTENERI alListeneri{nullptr};
    LPALLISTENER3I alListener3i{nullptr};
    LPALLISTENERIV alListeneriv{nullptr};
    LPALGETLISTENERF alGetListenerf{nullptr};
    LPALGETLISTENER3F alGetListener3f{nullptr};
    LPALGETLISTENERFV alGetListenerfv{nullptr};
    LPALGETLISTENERI alGetListeneri{nullptr};
    LPALGETLISTENER3I alGetListener3i{nullptr};
    LPALGETLISTENERIV alGetListeneriv{nullptr};
    LPALGENSOURCES alGenSources{nullptr};
    LPALDELETESOURCES alDeleteSources{nullptr};
    LPALISSOURCE alIsSource{nullptr};
    LPALSOURCEF alSourcef{nullptr};
    LPALSOURCE3F alSource3f{nullptr};
    LPALSOURCEFV alSourcefv{nullptr};
    LPALSOURCEI alSourcei{nullptr};
    LPALSOURCE3I alSource3i{nullptr};
    LPALSOURCEIV alSourceiv{nullptr};
    LPALGETSOURCEF alGetSourcef{nullptr};
    LPALGETSOURCE3F alGetSource3f{nullptr};
    LPALGETSOURCEFV alGetSourcefv{nullptr};
    LPALGETSOURCEI alGetSourcei{nullptr};
    LPALGETSOURCE3I alGetSource3i{nullptr};
    LPALGETSOURCEIV alGetSourceiv{nullptr};
    LPALSOURCEPLAYV alSourcePlayv{nullptr};
    LPALSOURCESTOPV alSourceStopv{nullptr};
    LPALSOURCEREWINDV alSourceRewindv{nullptr};
    LPALSOURCEPAUSEV alSourcePausev{nullptr};
    LPALSOURCEPLAY alSourcePlay{nullptr};
    LPALSOURCESTOP alSourceStop{nullptr};
    LPALSOURCEREWIND alSourceRewind{nullptr};
    LPALSOURCEPAUSE alSourcePause{nullptr};
    LPALSOURCEQUEUEBUFFERS alSourceQueueBuffers{nullptr};
    LPALSOURCEUNQUEUEBUFFERS alSourceUnqueueBuffers{nullptr};
    LPALGENBUFFERS alGenBuffers{nullptr};
    LPALDELETEBUFFERS alDeleteBuffers{nullptr};
    LPALISBUFFER alIsBuffer{nullptr};
    LPALBUFFERF alBufferf{nullptr};
    LPALBUFFER3F alBuffer3f{nullptr};
    LPALBUFFERFV alBufferfv{nullptr};
    LPALBUFFERI alBufferi{nullptr};
    LPALBUFFER3I alBuffer3i{nullptr};
    LPALBUFFERIV alBufferiv{nullptr};
    LPALGETBUFFERF alGetBufferf{nullptr};
    LPALGETBUFFER3F alGetBuffer3f{nullptr};
    LPALGETBUFFERFV alGetBufferfv{nullptr};
    LPALGETBUFFERI alGetBufferi{nullptr};
    LPALGETBUFFER3I alGetBuffer3i{nullptr};
    LPALGETBUFFERIV alGetBufferiv{nullptr};
    LPALBUFFERDATA alBufferData{nullptr};
    LPALDOPPLERFACTOR alDopplerFactor{nullptr};
    LPALDOPPLERVELOCITY alDopplerVelocity{nullptr};
    LPALSPEEDOFSOUND alSpeedOfSound{nullptr};
    LPALDISTANCEMODEL alDistanceModel{nullptr};

    template<typename T>
    DriverIface(T&& name, HMODULE mod)
      : Name(std::forward<T>(name)), Module(mod)
    { }
    ~DriverIface()
    {
        if(Module)
            FreeLibrary(Module);
        Module = nullptr;
    }
};

extern std::vector<DriverIface> DriverList;

extern thread_local DriverIface *ThreadCtxDriver;
extern std::atomic<DriverIface*> CurrentCtxDriver;

/* HACK: MinGW generates bad code when accessing an extern thread_local object.
 * Add a wrapper function for it that only accesses it where it's defined.
 */
#ifdef __MINGW32__
DriverIface *GetThreadDriver() noexcept;
void SetThreadDriver(DriverIface *driver) noexcept;
#else
inline DriverIface *GetThreadDriver() noexcept { return ThreadCtxDriver; }
inline void SetThreadDriver(DriverIface *driver) noexcept { ThreadCtxDriver = driver; }
#endif


class PtrIntMap {
    void **mKeys{nullptr};
    /* Shares memory with keys. */
    int *mValues{nullptr};

    ALsizei mSize{0};
    ALsizei mCapacity{0};
    std::mutex mLock;

public:
    PtrIntMap() = default;
    ~PtrIntMap();

    ALenum insert(void *key, int value);
    int removeByKey(void *key);
    int lookupByKey(void *key);
};


enum LogLevel {
    LogLevel_None  = 0,
    LogLevel_Error = 1,
    LogLevel_Warn  = 2,
    LogLevel_Trace = 3,
};
extern enum LogLevel LogLevel;
extern FILE *LogFile;

#define TRACE(...) do {                                   \
    if(LogLevel >= LogLevel_Trace)                        \
    {                                                     \
        fprintf(LogFile, "AL Router (II): " __VA_ARGS__); \
        fflush(LogFile);                                  \
    }                                                     \
} while(0)
#define WARN(...) do {                                    \
    if(LogLevel >= LogLevel_Warn)                         \
    {                                                     \
        fprintf(LogFile, "AL Router (WW): " __VA_ARGS__); \
        fflush(LogFile);                                  \
    }                                                     \
} while(0)
#define ERR(...) do {                                     \
    if(LogLevel >= LogLevel_Error)                        \
    {                                                     \
        fprintf(LogFile, "AL Router (EE): " __VA_ARGS__); \
        fflush(LogFile);                                  \
    }                                                     \
} while(0)

#endif /* ROUTER_ROUTER_H */
