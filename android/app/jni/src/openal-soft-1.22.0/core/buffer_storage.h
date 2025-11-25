#ifndef CORE_BUFFER_STORAGE_H
#define CORE_BUFFER_STORAGE_H

#include <atomic>

#include "albyte.h"
#include "alnumeric.h"
#include "ambidefs.h"


using uint = unsigned int;

/* Storable formats */
enum FmtType : unsigned char {
    FmtUByte,
    FmtShort,
    FmtFloat,
    FmtDouble,
    FmtMulaw,
    FmtAlaw,
};
enum FmtChannels : unsigned char {
    FmtMono,
    FmtStereo,
    FmtRear,
    FmtQuad,
    FmtX51, /* (WFX order) */
    FmtX61, /* (WFX order) */
    FmtX71, /* (WFX order) */
    FmtBFormat2D,
    FmtBFormat3D,
    FmtUHJ2, /* 2-channel UHJ, aka "BHJ", stereo-compatible */
    FmtUHJ3, /* 3-channel UHJ, aka "THJ" */
    FmtUHJ4, /* 4-channel UHJ, aka "PHJ" */
    FmtSuperStereo, /* Stereo processed with Super Stereo. */
};

enum class AmbiLayout : unsigned char {
    FuMa,
    ACN,
};
enum class AmbiScaling : unsigned char {
    FuMa,
    SN3D,
    N3D,
    UHJ,
};

uint BytesFromFmt(FmtType type) noexcept;
uint ChannelsFromFmt(FmtChannels chans, uint ambiorder) noexcept;
inline uint FrameSizeFromFmt(FmtChannels chans, FmtType type, uint ambiorder) noexcept
{ return ChannelsFromFmt(chans, ambiorder) * BytesFromFmt(type); }

constexpr bool IsBFormat(FmtChannels chans) noexcept
{ return chans == FmtBFormat2D || chans == FmtBFormat3D; }

/* Super Stereo is considered part of the UHJ family here, since it goes
 * through similar processing as UHJ, both result in a B-Format signal, and
 * needs the same consideration as BHJ (three channel result with only two
 * channel input).
 */
constexpr bool IsUHJ(FmtChannels chans) noexcept
{ return chans == FmtUHJ2 || chans == FmtUHJ3 || chans == FmtUHJ4 || chans == FmtSuperStereo; }

/** Ambisonic formats are either B-Format or UHJ formats. */
constexpr bool IsAmbisonic(FmtChannels chans) noexcept
{ return IsBFormat(chans) || IsUHJ(chans); }

constexpr bool Is2DAmbisonic(FmtChannels chans) noexcept
{
    return chans == FmtBFormat2D || chans == FmtUHJ2 || chans == FmtUHJ3
        || chans == FmtSuperStereo;
}


using CallbackType = int(*)(void*, void*, int);

struct BufferStorage {
    CallbackType mCallback{nullptr};
    void *mUserData{nullptr};

    uint mSampleRate{0u};
    FmtChannels mChannels{FmtMono};
    FmtType mType{FmtShort};
    uint mSampleLen{0u};

    AmbiLayout mAmbiLayout{AmbiLayout::FuMa};
    AmbiScaling mAmbiScaling{AmbiScaling::FuMa};
    uint mAmbiOrder{0u};

    inline uint bytesFromFmt() const noexcept { return BytesFromFmt(mType); }
    inline uint channelsFromFmt() const noexcept
    { return ChannelsFromFmt(mChannels, mAmbiOrder); }
    inline uint frameSizeFromFmt() const noexcept { return channelsFromFmt() * bytesFromFmt(); }

    inline bool isBFormat() const noexcept { return IsBFormat(mChannels); }
};

#endif /* CORE_BUFFER_STORAGE_H */
