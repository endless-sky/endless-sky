
#include "config.h"

#include "router.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AL/alc.h"
#include "AL/al.h"

#include "almalloc.h"
#include "strutils.h"

#include "version.h"


std::vector<DriverIface> DriverList;

thread_local DriverIface *ThreadCtxDriver;

enum LogLevel LogLevel = LogLevel_Error;
FILE *LogFile;

#ifdef __MINGW32__
DriverIface *GetThreadDriver() noexcept { return ThreadCtxDriver; }
void SetThreadDriver(DriverIface *driver) noexcept { ThreadCtxDriver = driver; }
#endif

static void LoadDriverList(void);


BOOL APIENTRY DllMain(HINSTANCE, DWORD reason, void*)
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        LogFile = stderr;
        if(auto logfname = al::getenv("ALROUTER_LOGFILE"))
        {
            FILE *f = fopen(logfname->c_str(), "w");
            if(f == nullptr)
                ERR("Could not open log file: %s\n", logfname->c_str());
            else
                LogFile = f;
        }
        if(auto loglev = al::getenv("ALROUTER_LOGLEVEL"))
        {
            char *end = nullptr;
            long l = strtol(loglev->c_str(), &end, 0);
            if(!end || *end != '\0')
                ERR("Invalid log level value: %s\n", loglev->c_str());
            else if(l < LogLevel_None || l > LogLevel_Trace)
                ERR("Log level out of range: %s\n", loglev->c_str());
            else
                LogLevel = static_cast<enum LogLevel>(l);
        }
        TRACE("Initializing router v0.1-%s %s\n", ALSOFT_GIT_COMMIT_HASH, ALSOFT_GIT_BRANCH);
        LoadDriverList();

        break;

    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        DriverList.clear();

        if(LogFile && LogFile != stderr)
            fclose(LogFile);
        LogFile = nullptr;

        break;
    }
    return TRUE;
}


static void AddModule(HMODULE module, const WCHAR *name)
{
    for(auto &drv : DriverList)
    {
        if(drv.Module == module)
        {
            TRACE("Skipping already-loaded module %p\n", decltype(std::declval<void*>()){module});
            FreeLibrary(module);
            return;
        }
        if(drv.Name == name)
        {
            TRACE("Skipping similarly-named module %ls\n", name);
            FreeLibrary(module);
            return;
        }
    }

    DriverList.emplace_back(name, module);
    DriverIface &newdrv = DriverList.back();

    /* Load required functions. */
    int err = 0;
#define LOAD_PROC(x) do {                                                     \
    newdrv.x = reinterpret_cast<decltype(newdrv.x)>(reinterpret_cast<void*>(  \
        GetProcAddress(module, #x)));                                         \
    if(!newdrv.x)                                                             \
    {                                                                         \
        ERR("Failed to find entry point for %s in %ls\n", #x, name);          \
        err = 1;                                                              \
    }                                                                         \
} while(0)
    LOAD_PROC(alcCreateContext);
    LOAD_PROC(alcMakeContextCurrent);
    LOAD_PROC(alcProcessContext);
    LOAD_PROC(alcSuspendContext);
    LOAD_PROC(alcDestroyContext);
    LOAD_PROC(alcGetCurrentContext);
    LOAD_PROC(alcGetContextsDevice);
    LOAD_PROC(alcOpenDevice);
    LOAD_PROC(alcCloseDevice);
    LOAD_PROC(alcGetError);
    LOAD_PROC(alcIsExtensionPresent);
    LOAD_PROC(alcGetProcAddress);
    LOAD_PROC(alcGetEnumValue);
    LOAD_PROC(alcGetString);
    LOAD_PROC(alcGetIntegerv);
    LOAD_PROC(alcCaptureOpenDevice);
    LOAD_PROC(alcCaptureCloseDevice);
    LOAD_PROC(alcCaptureStart);
    LOAD_PROC(alcCaptureStop);
    LOAD_PROC(alcCaptureSamples);

    LOAD_PROC(alEnable);
    LOAD_PROC(alDisable);
    LOAD_PROC(alIsEnabled);
    LOAD_PROC(alGetString);
    LOAD_PROC(alGetBooleanv);
    LOAD_PROC(alGetIntegerv);
    LOAD_PROC(alGetFloatv);
    LOAD_PROC(alGetDoublev);
    LOAD_PROC(alGetBoolean);
    LOAD_PROC(alGetInteger);
    LOAD_PROC(alGetFloat);
    LOAD_PROC(alGetDouble);
    LOAD_PROC(alGetError);
    LOAD_PROC(alIsExtensionPresent);
    LOAD_PROC(alGetProcAddress);
    LOAD_PROC(alGetEnumValue);
    LOAD_PROC(alListenerf);
    LOAD_PROC(alListener3f);
    LOAD_PROC(alListenerfv);
    LOAD_PROC(alListeneri);
    LOAD_PROC(alListener3i);
    LOAD_PROC(alListeneriv);
    LOAD_PROC(alGetListenerf);
    LOAD_PROC(alGetListener3f);
    LOAD_PROC(alGetListenerfv);
    LOAD_PROC(alGetListeneri);
    LOAD_PROC(alGetListener3i);
    LOAD_PROC(alGetListeneriv);
    LOAD_PROC(alGenSources);
    LOAD_PROC(alDeleteSources);
    LOAD_PROC(alIsSource);
    LOAD_PROC(alSourcef);
    LOAD_PROC(alSource3f);
    LOAD_PROC(alSourcefv);
    LOAD_PROC(alSourcei);
    LOAD_PROC(alSource3i);
    LOAD_PROC(alSourceiv);
    LOAD_PROC(alGetSourcef);
    LOAD_PROC(alGetSource3f);
    LOAD_PROC(alGetSourcefv);
    LOAD_PROC(alGetSourcei);
    LOAD_PROC(alGetSource3i);
    LOAD_PROC(alGetSourceiv);
    LOAD_PROC(alSourcePlayv);
    LOAD_PROC(alSourceStopv);
    LOAD_PROC(alSourceRewindv);
    LOAD_PROC(alSourcePausev);
    LOAD_PROC(alSourcePlay);
    LOAD_PROC(alSourceStop);
    LOAD_PROC(alSourceRewind);
    LOAD_PROC(alSourcePause);
    LOAD_PROC(alSourceQueueBuffers);
    LOAD_PROC(alSourceUnqueueBuffers);
    LOAD_PROC(alGenBuffers);
    LOAD_PROC(alDeleteBuffers);
    LOAD_PROC(alIsBuffer);
    LOAD_PROC(alBufferf);
    LOAD_PROC(alBuffer3f);
    LOAD_PROC(alBufferfv);
    LOAD_PROC(alBufferi);
    LOAD_PROC(alBuffer3i);
    LOAD_PROC(alBufferiv);
    LOAD_PROC(alGetBufferf);
    LOAD_PROC(alGetBuffer3f);
    LOAD_PROC(alGetBufferfv);
    LOAD_PROC(alGetBufferi);
    LOAD_PROC(alGetBuffer3i);
    LOAD_PROC(alGetBufferiv);
    LOAD_PROC(alBufferData);
    LOAD_PROC(alDopplerFactor);
    LOAD_PROC(alDopplerVelocity);
    LOAD_PROC(alSpeedOfSound);
    LOAD_PROC(alDistanceModel);
    if(!err)
    {
        ALCint alc_ver[2] = { 0, 0 };
        newdrv.alcGetIntegerv(nullptr, ALC_MAJOR_VERSION, 1, &alc_ver[0]);
        newdrv.alcGetIntegerv(nullptr, ALC_MINOR_VERSION, 1, &alc_ver[1]);
        if(newdrv.alcGetError(nullptr) == ALC_NO_ERROR)
            newdrv.ALCVer = MAKE_ALC_VER(alc_ver[0], alc_ver[1]);
        else
        {
            WARN("Failed to query ALC version for %ls, assuming 1.0\n", name);
            newdrv.ALCVer = MAKE_ALC_VER(1, 0);
        }

#undef LOAD_PROC
#define LOAD_PROC(x) do {                                                     \
    newdrv.x = reinterpret_cast<decltype(newdrv.x)>(                          \
        newdrv.alcGetProcAddress(nullptr, #x));                               \
    if(!newdrv.x)                                                             \
    {                                                                         \
        ERR("Failed to find entry point for %s in %ls\n", #x, name);          \
        err = 1;                                                              \
    }                                                                         \
} while(0)
        if(newdrv.alcIsExtensionPresent(nullptr, "ALC_EXT_thread_local_context"))
        {
            LOAD_PROC(alcSetThreadContext);
            LOAD_PROC(alcGetThreadContext);
        }
    }

    if(err)
    {
        DriverList.pop_back();
        return;
    }
    TRACE("Loaded module %p, %ls, ALC %d.%d\n", decltype(std::declval<void*>()){module}, name,
          newdrv.ALCVer>>8, newdrv.ALCVer&255);
#undef LOAD_PROC
}

static void SearchDrivers(WCHAR *path)
{
    WIN32_FIND_DATAW fdata;

    TRACE("Searching for drivers in %ls...\n", path);
    std::wstring srchPath = path;
    srchPath += L"\\*oal.dll";

    HANDLE srchHdl = FindFirstFileW(srchPath.c_str(), &fdata);
    if(srchHdl != INVALID_HANDLE_VALUE)
    {
        do {
            HMODULE mod;

            srchPath = path;
            srchPath += L"\\";
            srchPath += fdata.cFileName;
            TRACE("Found %ls\n", srchPath.c_str());

            mod = LoadLibraryW(srchPath.c_str());
            if(!mod)
                WARN("Could not load %ls\n", srchPath.c_str());
            else
                AddModule(mod, fdata.cFileName);
        } while(FindNextFileW(srchHdl, &fdata));
        FindClose(srchHdl);
    }
}

static WCHAR *strrchrW(WCHAR *str, WCHAR ch)
{
    WCHAR *res = nullptr;
    while(str && *str != '\0')
    {
        if(*str == ch)
            res = str;
        ++str;
    }
    return res;
}

static int GetLoadedModuleDirectory(const WCHAR *name, WCHAR *moddir, DWORD length)
{
    HMODULE module = nullptr;
    WCHAR *sep0, *sep1;

    if(name)
    {
        module = GetModuleHandleW(name);
        if(!module) return 0;
    }

    if(GetModuleFileNameW(module, moddir, length) == 0)
        return 0;

    sep0 = strrchrW(moddir, '/');
    if(sep0) sep1 = strrchrW(sep0+1, '\\');
    else sep1 = strrchrW(moddir, '\\');

    if(sep1) *sep1 = '\0';
    else if(sep0) *sep0 = '\0';
    else *moddir = '\0';

    return 1;
}

void LoadDriverList(void)
{
    WCHAR dll_path[MAX_PATH+1] = L"";
    WCHAR cwd_path[MAX_PATH+1] = L"";
    WCHAR proc_path[MAX_PATH+1] = L"";
    WCHAR sys_path[MAX_PATH+1] = L"";
    int len;

    if(GetLoadedModuleDirectory(L"OpenAL32.dll", dll_path, MAX_PATH))
        TRACE("Got DLL path %ls\n", dll_path);

    GetCurrentDirectoryW(MAX_PATH, cwd_path);
    len = lstrlenW(cwd_path);
    if(len > 0 && (cwd_path[len-1] == '\\' || cwd_path[len-1] == '/'))
        cwd_path[len-1] = '\0';
    TRACE("Got current working directory %ls\n", cwd_path);

    if(GetLoadedModuleDirectory(nullptr, proc_path, MAX_PATH))
        TRACE("Got proc path %ls\n", proc_path);

    GetSystemDirectoryW(sys_path, MAX_PATH);
    len = lstrlenW(sys_path);
    if(len > 0 && (sys_path[len-1] == '\\' || sys_path[len-1] == '/'))
        sys_path[len-1] = '\0';
    TRACE("Got system path %ls\n", sys_path);

    /* Don't search the DLL's path if it is the same as the current working
     * directory, app's path, or system path (don't want to do duplicate
     * searches, or increase the priority of the app or system path).
     */
    if(dll_path[0] &&
       (!cwd_path[0] || wcscmp(dll_path, cwd_path) != 0) &&
       (!proc_path[0] || wcscmp(dll_path, proc_path) != 0) &&
       (!sys_path[0] || wcscmp(dll_path, sys_path) != 0))
        SearchDrivers(dll_path);
    if(cwd_path[0] &&
       (!proc_path[0] || wcscmp(cwd_path, proc_path) != 0) &&
       (!sys_path[0] || wcscmp(cwd_path, sys_path) != 0))
        SearchDrivers(cwd_path);
    if(proc_path[0] && (!sys_path[0] || wcscmp(proc_path, sys_path) != 0))
        SearchDrivers(proc_path);
    if(sys_path[0])
        SearchDrivers(sys_path);
}


PtrIntMap::~PtrIntMap()
{
    std::lock_guard<std::mutex> maplock{mLock};
    al_free(mKeys);
    mKeys = nullptr;
    mValues = nullptr;
    mSize = 0;
    mCapacity = 0;
}

ALenum PtrIntMap::insert(void *key, int value)
{
    std::lock_guard<std::mutex> maplock{mLock};
    auto iter = std::lower_bound(mKeys, mKeys+mSize, key);
    auto pos = static_cast<ALsizei>(std::distance(mKeys, iter));

    if(pos == mSize || mKeys[pos] != key)
    {
        if(mSize == mCapacity)
        {
            void **newkeys{nullptr};
            ALsizei newcap{mCapacity ? (mCapacity<<1) : 4};
            if(newcap > mCapacity)
                newkeys = static_cast<void**>(
                    al_calloc(16, (sizeof(mKeys[0])+sizeof(mValues[0]))*newcap)
                );
            if(!newkeys)
                return AL_OUT_OF_MEMORY;
            auto newvalues = reinterpret_cast<int*>(&newkeys[newcap]);

            if(mKeys)
            {
                std::copy_n(mKeys, mSize, newkeys);
                std::copy_n(mValues, mSize, newvalues);
            }
            al_free(mKeys);
            mKeys = newkeys;
            mValues = newvalues;
            mCapacity = newcap;
        }

        if(pos < mSize)
        {
            std::copy_backward(mKeys+pos, mKeys+mSize, mKeys+mSize+1);
            std::copy_backward(mValues+pos, mValues+mSize, mValues+mSize+1);
        }
        mSize++;
    }
    mKeys[pos] = key;
    mValues[pos] = value;

    return AL_NO_ERROR;
}

int PtrIntMap::removeByKey(void *key)
{
    int ret = -1;

    std::lock_guard<std::mutex> maplock{mLock};
    auto iter = std::lower_bound(mKeys, mKeys+mSize, key);
    auto pos = static_cast<ALsizei>(std::distance(mKeys, iter));
    if(pos < mSize && mKeys[pos] == key)
    {
        ret = mValues[pos];
        if(pos+1 < mSize)
        {
            std::copy(mKeys+pos+1, mKeys+mSize, mKeys+pos);
            std::copy(mValues+pos+1, mValues+mSize, mValues+pos);
        }
        mSize--;
    }

    return ret;
}

int PtrIntMap::lookupByKey(void *key)
{
    int ret = -1;

    std::lock_guard<std::mutex> maplock{mLock};
    auto iter = std::lower_bound(mKeys, mKeys+mSize, key);
    auto pos = static_cast<ALsizei>(std::distance(mKeys, iter));
    if(pos < mSize && mKeys[pos] == key)
        ret = mValues[pos];

    return ret;
}
