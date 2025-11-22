
#include "config.h"

#include "dynload.h"

#include "strutils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void *LoadLib(const char *name)
{
    std::wstring wname{utf8_to_wstr(name)};
    return LoadLibraryW(wname.c_str());
}
void CloseLib(void *handle)
{ FreeLibrary(static_cast<HMODULE>(handle)); }
void *GetSymbol(void *handle, const char *name)
{ return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name)); }

#elif defined(HAVE_DLFCN_H)

#include <dlfcn.h>

void *LoadLib(const char *name)
{
    dlerror();
    void *handle{dlopen(name, RTLD_NOW)};
    const char *err{dlerror()};
    if(err) handle = nullptr;
    return handle;
}
void CloseLib(void *handle)
{ dlclose(handle); }
void *GetSymbol(void *handle, const char *name)
{
    dlerror();
    void *sym{dlsym(handle, name)};
    const char *err{dlerror()};
    if(err) sym = nullptr;
    return sym;
}
#endif
