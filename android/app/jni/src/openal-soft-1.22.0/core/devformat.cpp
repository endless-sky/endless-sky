
#include "config.h"

#include "devformat.h"


uint BytesFromDevFmt(DevFmtType type) noexcept
{
    switch(type)
    {
    case DevFmtByte: return sizeof(int8_t);
    case DevFmtUByte: return sizeof(uint8_t);
    case DevFmtShort: return sizeof(int16_t);
    case DevFmtUShort: return sizeof(uint16_t);
    case DevFmtInt: return sizeof(int32_t);
    case DevFmtUInt: return sizeof(uint32_t);
    case DevFmtFloat: return sizeof(float);
    }
    return 0;
}
uint ChannelsFromDevFmt(DevFmtChannels chans, uint ambiorder) noexcept
{
    switch(chans)
    {
    case DevFmtMono: return 1;
    case DevFmtStereo: return 2;
    case DevFmtQuad: return 4;
    case DevFmtX51: return 6;
    case DevFmtX61: return 7;
    case DevFmtX71: return 8;
    case DevFmtAmbi3D: return (ambiorder+1) * (ambiorder+1);
    }
    return 0;
}

const char *DevFmtTypeString(DevFmtType type) noexcept
{
    switch(type)
    {
    case DevFmtByte: return "Int8";
    case DevFmtUByte: return "UInt8";
    case DevFmtShort: return "Int16";
    case DevFmtUShort: return "UInt16";
    case DevFmtInt: return "Int32";
    case DevFmtUInt: return "UInt32";
    case DevFmtFloat: return "Float32";
    }
    return "(unknown type)";
}
const char *DevFmtChannelsString(DevFmtChannels chans) noexcept
{
    switch(chans)
    {
    case DevFmtMono: return "Mono";
    case DevFmtStereo: return "Stereo";
    case DevFmtQuad: return "Quadraphonic";
    case DevFmtX51: return "5.1 Surround";
    case DevFmtX61: return "6.1 Surround";
    case DevFmtX71: return "7.1 Surround";
    case DevFmtAmbi3D: return "Ambisonic 3D";
    }
    return "(unknown channels)";
}
