#ifndef AL_BUFFER_H
#define AL_BUFFER_H

#include <atomic>

#include "AL/al.h"

#include "albyte.h"
#include "alc/inprogext.h"
#include "almalloc.h"
#include "atomic.h"
#include "core/buffer_storage.h"
#include "vector.h"

#ifdef ALSOFT_EAX
#include "eax_x_ram.h"
#endif // ALSOFT_EAX

/* User formats */
enum UserFmtType : unsigned char {
    UserFmtUByte = FmtUByte,
    UserFmtShort = FmtShort,
    UserFmtFloat = FmtFloat,
    UserFmtMulaw = FmtMulaw,
    UserFmtAlaw = FmtAlaw,
    UserFmtDouble = FmtDouble,

    UserFmtIMA4 = 128,
    UserFmtMSADPCM,
};
enum UserFmtChannels : unsigned char {
    UserFmtMono = FmtMono,
    UserFmtStereo = FmtStereo,
    UserFmtRear = FmtRear,
    UserFmtQuad = FmtQuad,
    UserFmtX51 = FmtX51,
    UserFmtX61 = FmtX61,
    UserFmtX71 = FmtX71,
    UserFmtBFormat2D = FmtBFormat2D,
    UserFmtBFormat3D = FmtBFormat3D,
    UserFmtUHJ2 = FmtUHJ2,
    UserFmtUHJ3 = FmtUHJ3,
    UserFmtUHJ4 = FmtUHJ4,
};


struct ALbuffer : public BufferStorage {
    ALbitfieldSOFT Access{0u};

    al::vector<al::byte,16> mData;

    UserFmtType OriginalType{UserFmtShort};
    ALuint OriginalSize{0};
    ALuint OriginalAlign{0};

    ALuint UnpackAlign{0};
    ALuint PackAlign{0};
    ALuint UnpackAmbiOrder{1};

    ALbitfieldSOFT MappedAccess{0u};
    ALsizei MappedOffset{0};
    ALsizei MappedSize{0};

    ALuint mLoopStart{0u};
    ALuint mLoopEnd{0u};

    /* Number of times buffer was attached to a source (deletion can only occur when 0) */
    RefCount ref{0u};

    /* Self ID */
    ALuint id{0};

    DISABLE_ALLOC()

#ifdef ALSOFT_EAX
    ALenum eax_x_ram_mode{AL_STORAGE_AUTOMATIC};
    bool eax_x_ram_is_hardware{};
#endif // ALSOFT_EAX
};

#endif
