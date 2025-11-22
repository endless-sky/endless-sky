#ifndef CORE_LOGGING_H
#define CORE_LOGGING_H

#include <stdio.h>

#include "opthelpers.h"


enum class LogLevel {
    Disable,
    Error,
    Warning,
    Trace
};
extern LogLevel gLogLevel;

extern FILE *gLogFile;


#if !defined(_WIN32) && !defined(__ANDROID__)
#define TRACE(...) do {                                                       \
    if UNLIKELY(gLogLevel >= LogLevel::Trace)                                 \
        fprintf(gLogFile, "[ALSOFT] (II) " __VA_ARGS__);                      \
} while(0)

#define WARN(...) do {                                                        \
    if UNLIKELY(gLogLevel >= LogLevel::Warning)                               \
        fprintf(gLogFile, "[ALSOFT] (WW) " __VA_ARGS__);                      \
} while(0)

#define ERR(...) do {                                                         \
    if UNLIKELY(gLogLevel >= LogLevel::Error)                                 \
        fprintf(gLogFile, "[ALSOFT] (EE) " __VA_ARGS__);                      \
} while(0)

#else

#ifdef __USE_MINGW_ANSI_STDIO
[[gnu::format(gnu_printf,3,4)]]
#else
[[gnu::format(printf,3,4)]]
#endif
void al_print(LogLevel level, FILE *logfile, const char *fmt, ...);

#define TRACE(...) al_print(LogLevel::Trace, gLogFile, "[ALSOFT] (II) " __VA_ARGS__)

#define WARN(...) al_print(LogLevel::Warning, gLogFile, "[ALSOFT] (WW) " __VA_ARGS__)

#define ERR(...) al_print(LogLevel::Error, gLogFile, "[ALSOFT] (EE) " __VA_ARGS__)
#endif

#endif /* CORE_LOGGING_H */
