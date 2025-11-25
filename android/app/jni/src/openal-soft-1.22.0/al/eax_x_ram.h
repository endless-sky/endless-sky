#ifndef EAX_X_RAM_INCLUDED
#define EAX_X_RAM_INCLUDED


#include "AL/al.h"


constexpr auto eax_x_ram_min_size = ALsizei{};
constexpr auto eax_x_ram_max_size = ALsizei{64 * 1'024 * 1'024};


constexpr auto AL_EAX_RAM_SIZE = ALenum{0x202201};
constexpr auto AL_EAX_RAM_FREE = ALenum{0x202202};

constexpr auto AL_STORAGE_AUTOMATIC = ALenum{0x202203};
constexpr auto AL_STORAGE_HARDWARE = ALenum{0x202204};
constexpr auto AL_STORAGE_ACCESSIBLE = ALenum{0x202205};


constexpr auto AL_EAX_RAM_SIZE_NAME = "AL_EAX_RAM_SIZE";
constexpr auto AL_EAX_RAM_FREE_NAME = "AL_EAX_RAM_FREE";

constexpr auto AL_STORAGE_AUTOMATIC_NAME = "AL_STORAGE_AUTOMATIC";
constexpr auto AL_STORAGE_HARDWARE_NAME = "AL_STORAGE_HARDWARE";
constexpr auto AL_STORAGE_ACCESSIBLE_NAME = "AL_STORAGE_ACCESSIBLE";


ALboolean AL_APIENTRY EAXSetBufferMode(
    ALsizei n,
    const ALuint* buffers,
    ALint value);

ALenum AL_APIENTRY EAXGetBufferMode(
    ALuint buffer,
    ALint* pReserved);


#endif // !EAX_X_RAM_INCLUDED
