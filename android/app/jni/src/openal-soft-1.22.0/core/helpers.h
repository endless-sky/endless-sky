#ifndef CORE_HELPERS_H
#define CORE_HELPERS_H

#include <string>

#include "vector.h"


struct PathNamePair { std::string path, fname; };
const PathNamePair &GetProcBinary(void);

extern int RTPrioLevel;
extern bool AllowRTTimeLimit;
void SetRTPriority(void);

al::vector<std::string> SearchDataFiles(const char *match, const char *subdir);

#endif /* CORE_HELPERS_H */
