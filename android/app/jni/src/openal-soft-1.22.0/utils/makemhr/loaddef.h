#ifndef LOADDEF_H
#define LOADDEF_H

#include <istream>

#include "makemhr.h"


bool LoadDefInput(std::istream &istream, const char *startbytes, std::streamsize startbytecount,
    const char *filename, const uint fftSize, const uint truncSize, const ChannelModeT chanMode,
    HrirDataT *hData);

#endif /* LOADDEF_H */
