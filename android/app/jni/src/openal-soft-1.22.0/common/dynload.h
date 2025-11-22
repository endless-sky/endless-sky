#ifndef AL_DYNLOAD_H
#define AL_DYNLOAD_H

#if defined(_WIN32) || defined(HAVE_DLFCN_H)

#define HAVE_DYNLOAD

void *LoadLib(const char *name);
void CloseLib(void *handle);
void *GetSymbol(void *handle, const char *name);

#endif

#endif /* AL_DYNLOAD_H */
