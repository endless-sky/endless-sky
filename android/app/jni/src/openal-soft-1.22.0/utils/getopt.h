#ifndef GETOPT_H
#define GETOPT_H

#ifndef _WIN32

#include <unistd.h>

#else /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *optarg;
extern int optind, opterr, optopt, optreset;

int getopt(int nargc, char * const nargv[], const char *ostr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_WIN32 */

#endif /* !GETOPT_H */

