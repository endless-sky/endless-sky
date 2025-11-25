#ifndef CORE_DEVFORMAT_H
#define CORE_DEVFORMAT_H

#include <cstdint>


using uint = unsigned int;

enum Channel : unsigned char {
    FrontLeft = 0,
    FrontRight,
    FrontCenter,
    LFE,
    BackLeft,
    BackRight,
    BackCenter,
    SideLeft,
    SideRight,

    TopCenter,
    TopFrontLeft,
    TopFrontCenter,
    TopFrontRight,
    TopBackLeft,
    TopBackCenter,
    TopBackRight,

    MaxChannels
};


/* Device formats */
enum DevFmtType : unsigned char {
    DevFmtByte,
    DevFmtUByte,
    DevFmtShort,
    DevFmtUShort,
    DevFmtInt,
    DevFmtUInt,
    DevFmtFloat,

    DevFmtTypeDefault = DevFmtFloat
};
enum DevFmtChannels : unsigned char {
    DevFmtMono,
    DevFmtStereo,
    DevFmtQuad,
    DevFmtX51,
    DevFmtX61,
    DevFmtX71,
    DevFmtAmbi3D,

    DevFmtChannelsDefault = DevFmtStereo
};
#define MAX_OUTPUT_CHANNELS  16

/* DevFmtType traits, providing the type, etc given a DevFmtType. */
template<DevFmtType T>
struct DevFmtTypeTraits { };

template<>
struct DevFmtTypeTraits<DevFmtByte> { using Type = int8_t; };
template<>
struct DevFmtTypeTraits<DevFmtUByte> { using Type = uint8_t; };
template<>
struct DevFmtTypeTraits<DevFmtShort> { using Type = int16_t; };
template<>
struct DevFmtTypeTraits<DevFmtUShort> { using Type = uint16_t; };
template<>
struct DevFmtTypeTraits<DevFmtInt> { using Type = int32_t; };
template<>
struct DevFmtTypeTraits<DevFmtUInt> { using Type = uint32_t; };
template<>
struct DevFmtTypeTraits<DevFmtFloat> { using Type = float; };

template<DevFmtType T>
using DevFmtType_t = typename DevFmtTypeTraits<T>::Type;


uint BytesFromDevFmt(DevFmtType type) noexcept;
uint ChannelsFromDevFmt(DevFmtChannels chans, uint ambiorder) noexcept;
inline uint FrameSizeFromDevFmt(DevFmtChannels chans, DevFmtType type, uint ambiorder) noexcept
{ return ChannelsFromDevFmt(chans, ambiorder) * BytesFromDevFmt(type); }

const char *DevFmtTypeString(DevFmtType type) noexcept;
const char *DevFmtChannelsString(DevFmtChannels chans) noexcept;

enum class DevAmbiLayout : bool {
    FuMa,
    ACN,

    Default = ACN
};

enum class DevAmbiScaling : unsigned char {
    FuMa,
    SN3D,
    N3D,

    Default = SN3D
};

#endif /* CORE_DEVFORMAT_H */
