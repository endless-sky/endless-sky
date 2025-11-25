#ifndef PRAGMADEFS_H
#define PRAGMADEFS_H

#if defined(_MSC_VER)
#define DIAGNOSTIC_PUSH __pragma(warning(push))
#define DIAGNOSTIC_POP __pragma(warning(pop))
#define std_pragma(...)
#define msc_pragma __pragma
#else
#if defined(__GNUC__) || defined(__clang__)
#define DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
#define DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#else
#define DIAGNOSTIC_PUSH
#define DIAGNOSTIC_POP
#endif
#define std_pragma _Pragma
#define msc_pragma(...)
#endif

#endif /* PRAGMADEFS_H */
