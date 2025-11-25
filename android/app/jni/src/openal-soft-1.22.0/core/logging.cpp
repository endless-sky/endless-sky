
#include "config.h"

#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "strutils.h"
#include "vector.h"


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void al_print(LogLevel level, FILE *logfile, const char *fmt, ...)
{
    al::vector<char> dynmsg;
    char stcmsg[256];
    char *str{stcmsg};

    std::va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    const int msglen{std::vsnprintf(str, sizeof(stcmsg), fmt, args)};
    if(unlikely(msglen >= 0 && static_cast<size_t>(msglen) >= sizeof(stcmsg)))
    {
        dynmsg.resize(static_cast<size_t>(msglen) + 1u);
        str = dynmsg.data();
        std::vsnprintf(str, dynmsg.size(), fmt, args2);
    }
    va_end(args2);
    va_end(args);

    if(gLogLevel >= level)
    {
        fputs(str, logfile);
        fflush(logfile);
    }
    /* OutputDebugStringW has no 'level' property to distinguish between
     * informational, warning, or error debug messages. So only print them for
     * non-Release builds.
     */
#ifndef NDEBUG
    std::wstring wstr{utf8_to_wstr(str)};
    OutputDebugStringW(wstr.c_str());
#endif
}

#else

#ifdef __ANDROID__
#include <android/log.h>
#endif

void al_print(LogLevel level, FILE *logfile, const char *fmt, ...)
{
    al::vector<char> dynmsg;
    char stcmsg[256];
    char *str{stcmsg};

    std::va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    const int msglen{std::vsnprintf(str, sizeof(stcmsg), fmt, args)};
    if(unlikely(msglen >= 0 && static_cast<size_t>(msglen) >= sizeof(stcmsg)))
    {
        dynmsg.resize(static_cast<size_t>(msglen) + 1u);
        str = dynmsg.data();
        std::vsnprintf(str, dynmsg.size(), fmt, args2);
    }
    va_end(args2);
    va_end(args);

    if(gLogLevel >= level)
    {
        std::fputs(str, logfile);
        std::fflush(logfile);
    }
#ifdef __ANDROID__
    auto android_severity = [](LogLevel l) noexcept
    {
        switch(l)
        {
        case LogLevel::Trace: return ANDROID_LOG_DEBUG;
        case LogLevel::Warning: return ANDROID_LOG_WARN;
        case LogLevel::Error: return ANDROID_LOG_ERROR;
        /* Should not happen. */
        case LogLevel::Disable:
            break;
        }
        return ANDROID_LOG_ERROR;
    };
    __android_log_print(android_severity(level), "openal", "%s", str);
#endif
}

#endif
