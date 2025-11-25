#ifndef WIN_MAIN_UTF8_H
#define WIN_MAIN_UTF8_H

/* For Windows systems this provides a way to get UTF-8 encoded argv strings,
 * and also overrides fopen to accept UTF-8 filenames. Working with wmain
 * directly complicates cross-platform compatibility, while normal main() in
 * Windows uses the current codepage (which has limited availability of
 * characters).
 *
 * For MinGW, you must link with -municode
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>

#ifdef __cplusplus
#include <memory>

#define STATIC_CAST(...) static_cast<__VA_ARGS__>
#define REINTERPRET_CAST(...) reinterpret_cast<__VA_ARGS__>

#else

#define STATIC_CAST(...) (__VA_ARGS__)
#define REINTERPRET_CAST(...) (__VA_ARGS__)
#endif

static FILE *my_fopen(const char *fname, const char *mode)
{
    wchar_t *wname=NULL, *wmode=NULL;
    int namelen, modelen;
    FILE *file = NULL;
    errno_t err;

    namelen = MultiByteToWideChar(CP_UTF8, 0, fname, -1, NULL, 0);
    modelen = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);

    if(namelen <= 0 || modelen <= 0)
    {
        fprintf(stderr, "Failed to convert UTF-8 fname \"%s\", mode \"%s\"\n", fname, mode);
        return NULL;
    }

#ifdef __cplusplus
    auto strbuf = std::make_unique<wchar_t[]>(static_cast<size_t>(namelen+modelen));
    wname = strbuf.get();
#else
    wname = (wchar_t*)calloc(sizeof(wchar_t), (size_t)(namelen+modelen));
#endif
    wmode = wname + namelen;
    MultiByteToWideChar(CP_UTF8, 0, fname, -1, wname, namelen);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, modelen);

    err = _wfopen_s(&file, wname, wmode);
    if(err)
    {
        errno = err;
        file = NULL;
    }

#ifndef __cplusplus
    free(wname);
#endif
    return file;
}
#define fopen my_fopen


/* SDL overrides main and provides UTF-8 args for us. */
#if !defined(SDL_MAIN_NEEDED) && !defined(SDL_MAIN_AVAILABLE)
int my_main(int, char**);
#define main my_main

#ifdef __cplusplus
extern "C"
#endif
int wmain(int argc, wchar_t **wargv)
{
    char **argv;
    size_t total;
    int i;

    total = sizeof(*argv) * STATIC_CAST(size_t)(argc);
    for(i = 0;i < argc;i++)
        total += STATIC_CAST(size_t)(WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL,
            NULL));

#ifdef __cplusplus
    auto argbuf = std::make_unique<char[]>(total);
    argv = reinterpret_cast<char**>(argbuf.get());
#else
    argv = (char**)calloc(1, total);
#endif
    argv[0] = REINTERPRET_CAST(char*)(argv + argc);
    for(i = 0;i < argc-1;i++)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], 65535, NULL, NULL);
        argv[i+1] = argv[i] + len;
    }
    WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], 65535, NULL, NULL);

#ifdef __cplusplus
    return main(argc, argv);
#else
    i = main(argc, argv);

    free(argv);
    return i;
#endif
}
#endif /* !defined(SDL_MAIN_NEEDED) && !defined(SDL_MAIN_AVAILABLE) */

#endif /* _WIN32 */

#endif /* WIN_MAIN_UTF8_H */
