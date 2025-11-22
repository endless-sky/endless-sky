#ifndef LOADSOFA_H
#define LOADSOFA_H

#include "makemhr.h"


bool LoadSofaFile(const char *filename, const uint numThreads, const uint fftSize,
    const uint truncSize, const ChannelModeT chanMode, HrirDataT *hData);

#endif /* LOADSOFA_H */
