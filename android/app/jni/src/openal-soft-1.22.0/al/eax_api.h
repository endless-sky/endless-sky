#ifndef EAX_API_INCLUDED
#define EAX_API_INCLUDED


//
// EAX API.
//
// Based on headers `eax[2-5].h` included in Doom 3 source code:
// https://github.com/id-Software/DOOM-3/tree/master/neo/openal/include
//


#include <cfloat>
#include <cstdint>
#include <cstring>

#include <array>

#include "AL/al.h"


#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    std::uint32_t Data1;
    std::uint16_t Data2;
    std::uint16_t Data3;
    std::uint8_t Data4[8];
} GUID;

inline bool operator==(const GUID& lhs, const GUID& rhs) noexcept
{
    return std::memcmp(&lhs, &rhs, sizeof(GUID)) == 0;
}

inline bool operator!=(const GUID& lhs, const GUID& rhs) noexcept
{
    return !(lhs == rhs);
}
#endif // GUID_DEFINED


extern const GUID DSPROPSETID_EAX_ReverbProperties;

enum DSPROPERTY_EAX_REVERBPROPERTY : unsigned int
{
    DSPROPERTY_EAX_ALL,
    DSPROPERTY_EAX_ENVIRONMENT,
    DSPROPERTY_EAX_VOLUME,
    DSPROPERTY_EAX_DECAYTIME,
    DSPROPERTY_EAX_DAMPING,
}; // DSPROPERTY_EAX_REVERBPROPERTY

struct EAX_REVERBPROPERTIES
{
    unsigned long environment;
    float fVolume;
    float fDecayTime_sec;
    float fDamping;
}; // EAX_REVERBPROPERTIES

inline bool operator==(const EAX_REVERBPROPERTIES& lhs, const EAX_REVERBPROPERTIES& rhs) noexcept
{
    return std::memcmp(&lhs, &rhs, sizeof(EAX_REVERBPROPERTIES)) == 0;
}

extern const GUID DSPROPSETID_EAXBUFFER_ReverbProperties;

enum DSPROPERTY_EAXBUFFER_REVERBPROPERTY : unsigned int
{
    DSPROPERTY_EAXBUFFER_ALL,
    DSPROPERTY_EAXBUFFER_REVERBMIX,
}; // DSPROPERTY_EAXBUFFER_REVERBPROPERTY

struct EAXBUFFER_REVERBPROPERTIES
{
    float fMix;
};

inline bool operator==(const EAXBUFFER_REVERBPROPERTIES& lhs, const EAXBUFFER_REVERBPROPERTIES& rhs) noexcept
{
    return lhs.fMix == rhs.fMix;
}

constexpr auto EAX_BUFFER_MINREVERBMIX = 0.0F;
constexpr auto EAX_BUFFER_MAXREVERBMIX = 1.0F;
constexpr auto EAX_REVERBMIX_USEDISTANCE = -1.0F;


extern const GUID DSPROPSETID_EAX20_ListenerProperties;

enum DSPROPERTY_EAX20_LISTENERPROPERTY :
    unsigned int
{
    DSPROPERTY_EAX20LISTENER_NONE,
    DSPROPERTY_EAX20LISTENER_ALLPARAMETERS,
    DSPROPERTY_EAX20LISTENER_ROOM,
    DSPROPERTY_EAX20LISTENER_ROOMHF,
    DSPROPERTY_EAX20LISTENER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX20LISTENER_DECAYTIME,
    DSPROPERTY_EAX20LISTENER_DECAYHFRATIO,
    DSPROPERTY_EAX20LISTENER_REFLECTIONS,
    DSPROPERTY_EAX20LISTENER_REFLECTIONSDELAY,
    DSPROPERTY_EAX20LISTENER_REVERB,
    DSPROPERTY_EAX20LISTENER_REVERBDELAY,
    DSPROPERTY_EAX20LISTENER_ENVIRONMENT,
    DSPROPERTY_EAX20LISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAX20LISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAX20LISTENER_AIRABSORPTIONHF,
    DSPROPERTY_EAX20LISTENER_FLAGS
}; // DSPROPERTY_EAX20_LISTENERPROPERTY

struct EAX20LISTENERPROPERTIES
{
    long lRoom; // room effect level at low frequencies
    long lRoomHF; // room effect high-frequency level re. low frequency level
    float flRoomRolloffFactor; // like DS3D flRolloffFactor but for room effect
    float flDecayTime; // reverberation decay time at low frequencies
    float flDecayHFRatio; // high-frequency to low-frequency decay time ratio
    long lReflections; // early reflections level relative to room effect
    float flReflectionsDelay; // initial reflection delay time
    long lReverb; // late reverberation level relative to room effect
    float flReverbDelay; // late reverberation delay time relative to initial reflection
    unsigned long dwEnvironment; // sets all listener properties
    float flEnvironmentSize; // environment size in meters
    float flEnvironmentDiffusion; // environment diffusion
    float flAirAbsorptionHF; // change in level per meter at 5 kHz
    unsigned long dwFlags; // modifies the behavior of properties
}; // EAX20LISTENERPROPERTIES


extern const GUID DSPROPSETID_EAX20_BufferProperties;


enum DSPROPERTY_EAX20_BUFFERPROPERTY :
    unsigned int
{
    DSPROPERTY_EAX20BUFFER_NONE,
    DSPROPERTY_EAX20BUFFER_ALLPARAMETERS,
    DSPROPERTY_EAX20BUFFER_DIRECT,
    DSPROPERTY_EAX20BUFFER_DIRECTHF,
    DSPROPERTY_EAX20BUFFER_ROOM,
    DSPROPERTY_EAX20BUFFER_ROOMHF,
    DSPROPERTY_EAX20BUFFER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX20BUFFER_OBSTRUCTION,
    DSPROPERTY_EAX20BUFFER_OBSTRUCTIONLFRATIO,
    DSPROPERTY_EAX20BUFFER_OCCLUSION,
    DSPROPERTY_EAX20BUFFER_OCCLUSIONLFRATIO,
    DSPROPERTY_EAX20BUFFER_OCCLUSIONROOMRATIO,
    DSPROPERTY_EAX20BUFFER_OUTSIDEVOLUMEHF,
    DSPROPERTY_EAX20BUFFER_AIRABSORPTIONFACTOR,
    DSPROPERTY_EAX20BUFFER_FLAGS
}; // DSPROPERTY_EAX20_BUFFERPROPERTY


struct EAX20BUFFERPROPERTIES
{
    long lDirect; // direct path level
    long lDirectHF; // direct path level at high frequencies
    long lRoom; // room effect level
    long lRoomHF; // room effect level at high frequencies
    float flRoomRolloffFactor; // like DS3D flRolloffFactor but for room effect
    long lObstruction; // main obstruction control (attenuation at high frequencies) 
    float flObstructionLFRatio; // obstruction low-frequency level re. main control
    long lOcclusion; // main occlusion control (attenuation at high frequencies)
    float flOcclusionLFRatio; // occlusion low-frequency level re. main control
    float flOcclusionRoomRatio; // occlusion room effect level re. main control
    long lOutsideVolumeHF; // outside sound cone level at high frequencies
    float flAirAbsorptionFactor; // multiplies DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF
    unsigned long dwFlags; // modifies the behavior of properties
}; // EAX20BUFFERPROPERTIES


extern const GUID DSPROPSETID_EAX30_ListenerProperties;

extern const GUID DSPROPSETID_EAX30_BufferProperties;


constexpr auto EAX_MAX_FXSLOTS = 4;

constexpr auto EAX40_MAX_ACTIVE_FXSLOTS = 2;
constexpr auto EAX50_MAX_ACTIVE_FXSLOTS = 4;


constexpr auto EAX_OK = 0L;
constexpr auto EAXERR_INVALID_OPERATION = -1L;
constexpr auto EAXERR_INVALID_VALUE = -2L;
constexpr auto EAXERR_NO_EFFECT_LOADED = -3L;
constexpr auto EAXERR_UNKNOWN_EFFECT = -4L;
constexpr auto EAXERR_INCOMPATIBLE_SOURCE_TYPE = -5L;
constexpr auto EAXERR_INCOMPATIBLE_EAX_VERSION = -6L;


extern const GUID EAX_NULL_GUID;

extern const GUID EAX_PrimaryFXSlotID;


struct EAXVECTOR
{
    float x;
    float y;
    float z;
}; // EAXVECTOR

inline bool operator==(const EAXVECTOR& lhs, const EAXVECTOR& rhs) noexcept
{ return std::memcmp(&lhs, &rhs, sizeof(EAXVECTOR)) == 0; }

inline bool operator!=(const EAXVECTOR& lhs, const EAXVECTOR& rhs) noexcept
{ return !(lhs == rhs); }


extern const GUID EAXPROPERTYID_EAX40_Context;

extern const GUID EAXPROPERTYID_EAX50_Context;

// EAX50
enum :
    unsigned long
{
    HEADPHONES = 0,
    SPEAKERS_2,
    SPEAKERS_4,
    SPEAKERS_5,	// 5.1 speakers
    SPEAKERS_6, // 6.1 speakers
    SPEAKERS_7, // 7.1 speakers
};

// EAX50
enum :
    unsigned long
{
    EAX_40 = 5, // EAX 4.0
    EAX_50 = 6, // EAX 5.0
};

constexpr auto EAXCONTEXT_MINEAXSESSION = EAX_40;
constexpr auto EAXCONTEXT_MAXEAXSESSION = EAX_50;
constexpr auto EAXCONTEXT_DEFAULTEAXSESSION = EAX_40;

constexpr auto EAXCONTEXT_MINMAXACTIVESENDS = 2UL;
constexpr auto EAXCONTEXT_MAXMAXACTIVESENDS = 4UL;
constexpr auto EAXCONTEXT_DEFAULTMAXACTIVESENDS = 2UL;

// EAX50
struct EAXSESSIONPROPERTIES
{
    unsigned long ulEAXVersion;
    unsigned long ulMaxActiveSends;
}; // EAXSESSIONPROPERTIES

enum EAXCONTEXT_PROPERTY :
    unsigned int
{
    EAXCONTEXT_NONE = 0,
    EAXCONTEXT_ALLPARAMETERS,
    EAXCONTEXT_PRIMARYFXSLOTID,
    EAXCONTEXT_DISTANCEFACTOR,
    EAXCONTEXT_AIRABSORPTIONHF,
    EAXCONTEXT_HFREFERENCE,
    EAXCONTEXT_LASTERROR,

    // EAX50
    EAXCONTEXT_SPEAKERCONFIG,
    EAXCONTEXT_EAXSESSION,
    EAXCONTEXT_MACROFXFACTOR,
}; // EAXCONTEXT_PROPERTY

struct EAX40CONTEXTPROPERTIES
{
    GUID guidPrimaryFXSlotID;
    float flDistanceFactor;
    float flAirAbsorptionHF;
    float flHFReference;
}; // EAX40CONTEXTPROPERTIES

struct EAX50CONTEXTPROPERTIES :
    public EAX40CONTEXTPROPERTIES
{
    float flMacroFXFactor;
}; // EAX40CONTEXTPROPERTIES


bool operator==(
    const EAX40CONTEXTPROPERTIES& lhs,
    const EAX40CONTEXTPROPERTIES& rhs) noexcept;

bool operator==(
    const EAX50CONTEXTPROPERTIES& lhs,
    const EAX50CONTEXTPROPERTIES& rhs) noexcept;


constexpr auto EAXCONTEXT_MINDISTANCEFACTOR = FLT_MIN;
constexpr auto EAXCONTEXT_MAXDISTANCEFACTOR = FLT_MAX;
constexpr auto EAXCONTEXT_DEFAULTDISTANCEFACTOR = 1.0F;

constexpr auto EAXCONTEXT_MINAIRABSORPTIONHF = -100.0F;
constexpr auto EAXCONTEXT_MAXAIRABSORPTIONHF = 0.0F;
constexpr auto EAXCONTEXT_DEFAULTAIRABSORPTIONHF = -5.0F;

constexpr auto EAXCONTEXT_MINHFREFERENCE = 1000.0F;
constexpr auto EAXCONTEXT_MAXHFREFERENCE = 20000.0F;
constexpr auto EAXCONTEXT_DEFAULTHFREFERENCE = 5000.0F;

constexpr auto EAXCONTEXT_MINMACROFXFACTOR = 0.0F;
constexpr auto EAXCONTEXT_MAXMACROFXFACTOR = 1.0F;
constexpr auto EAXCONTEXT_DEFAULTMACROFXFACTOR = 0.0F;


extern const GUID EAXPROPERTYID_EAX40_FXSlot0;

extern const GUID EAXPROPERTYID_EAX50_FXSlot0;

extern const GUID EAXPROPERTYID_EAX40_FXSlot1;

extern const GUID EAXPROPERTYID_EAX50_FXSlot1;

extern const GUID EAXPROPERTYID_EAX40_FXSlot2;

extern const GUID EAXPROPERTYID_EAX50_FXSlot2;

extern const GUID EAXPROPERTYID_EAX40_FXSlot3;

extern const GUID EAXPROPERTYID_EAX50_FXSlot3;

extern const GUID EAXCONTEXT_DEFAULTPRIMARYFXSLOTID;

enum EAXFXSLOT_PROPERTY :
    unsigned int
{
    EAXFXSLOT_PARAMETER = 0,

    EAXFXSLOT_NONE = 0x10000,
    EAXFXSLOT_ALLPARAMETERS,
    EAXFXSLOT_LOADEFFECT,
    EAXFXSLOT_VOLUME,
    EAXFXSLOT_LOCK,
    EAXFXSLOT_FLAGS,

    // EAX50
    EAXFXSLOT_OCCLUSION,
    EAXFXSLOT_OCCLUSIONLFRATIO,
}; // EAXFXSLOT_PROPERTY

constexpr auto EAXFXSLOTFLAGS_ENVIRONMENT = 0x00000001UL;
// EAX50
constexpr auto EAXFXSLOTFLAGS_UPMIX = 0x00000002UL;

constexpr auto EAX40FXSLOTFLAGS_RESERVED = 0xFFFFFFFEUL; // reserved future use
constexpr auto EAX50FXSLOTFLAGS_RESERVED = 0xFFFFFFFCUL; // reserved future use


constexpr auto EAXFXSLOT_MINVOLUME = -10'000L;
constexpr auto EAXFXSLOT_MAXVOLUME = 0L;
constexpr auto EAXFXSLOT_DEFAULTVOLUME = 0L;

constexpr auto EAXFXSLOT_MINLOCK = 0L;
constexpr auto EAXFXSLOT_MAXLOCK = 1L;

enum :
    long
{
    EAXFXSLOT_UNLOCKED = 0,
    EAXFXSLOT_LOCKED = 1
};

constexpr auto EAXFXSLOT_MINOCCLUSION = -10'000L;
constexpr auto EAXFXSLOT_MAXOCCLUSION = 0L;
constexpr auto EAXFXSLOT_DEFAULTOCCLUSION = 0L;

constexpr auto EAXFXSLOT_MINOCCLUSIONLFRATIO = 0.0F;
constexpr auto EAXFXSLOT_MAXOCCLUSIONLFRATIO = 1.0F;
constexpr auto EAXFXSLOT_DEFAULTOCCLUSIONLFRATIO = 0.25F;

constexpr auto EAX40FXSLOT_DEFAULTFLAGS = EAXFXSLOTFLAGS_ENVIRONMENT;

constexpr auto EAX50FXSLOT_DEFAULTFLAGS =
    EAXFXSLOTFLAGS_ENVIRONMENT |
    EAXFXSLOTFLAGS_UPMIX; // ignored for reverb;

struct EAX40FXSLOTPROPERTIES
{
    GUID guidLoadEffect;
    long lVolume;
    long lLock;
    unsigned long ulFlags;
}; // EAX40FXSLOTPROPERTIES

struct EAX50FXSLOTPROPERTIES :
    public EAX40FXSLOTPROPERTIES
{
    long lOcclusion;
    float flOcclusionLFRatio;
}; // EAX50FXSLOTPROPERTIES

bool operator==(
    const EAX40FXSLOTPROPERTIES& lhs,
    const EAX40FXSLOTPROPERTIES& rhs) noexcept;

bool operator==(
    const EAX50FXSLOTPROPERTIES& lhs,
    const EAX50FXSLOTPROPERTIES& rhs) noexcept;

extern const GUID EAXPROPERTYID_EAX40_Source;

extern const GUID EAXPROPERTYID_EAX50_Source;

// Source object properties
enum EAXSOURCE_PROPERTY :
    unsigned int
{
    // EAX30

    EAXSOURCE_NONE,
    EAXSOURCE_ALLPARAMETERS,
    EAXSOURCE_OBSTRUCTIONPARAMETERS,
    EAXSOURCE_OCCLUSIONPARAMETERS,
    EAXSOURCE_EXCLUSIONPARAMETERS,
    EAXSOURCE_DIRECT,
    EAXSOURCE_DIRECTHF,
    EAXSOURCE_ROOM,
    EAXSOURCE_ROOMHF,
    EAXSOURCE_OBSTRUCTION,
    EAXSOURCE_OBSTRUCTIONLFRATIO,
    EAXSOURCE_OCCLUSION,
    EAXSOURCE_OCCLUSIONLFRATIO,
    EAXSOURCE_OCCLUSIONROOMRATIO,
    EAXSOURCE_OCCLUSIONDIRECTRATIO,
    EAXSOURCE_EXCLUSION,
    EAXSOURCE_EXCLUSIONLFRATIO,
    EAXSOURCE_OUTSIDEVOLUMEHF,
    EAXSOURCE_DOPPLERFACTOR,
    EAXSOURCE_ROLLOFFFACTOR,
    EAXSOURCE_ROOMROLLOFFFACTOR,
    EAXSOURCE_AIRABSORPTIONFACTOR,
    EAXSOURCE_FLAGS,

    // EAX40

    EAXSOURCE_SENDPARAMETERS,
    EAXSOURCE_ALLSENDPARAMETERS,
    EAXSOURCE_OCCLUSIONSENDPARAMETERS,
    EAXSOURCE_EXCLUSIONSENDPARAMETERS,
    EAXSOURCE_ACTIVEFXSLOTID,

    // EAX50

    EAXSOURCE_MACROFXFACTOR,
    EAXSOURCE_SPEAKERLEVELS,
    EAXSOURCE_ALL2DPARAMETERS,
}; // EAXSOURCE_PROPERTY


constexpr auto EAXSOURCEFLAGS_DIRECTHFAUTO = 0x00000001UL; // relates to EAXSOURCE_DIRECTHF
constexpr auto EAXSOURCEFLAGS_ROOMAUTO = 0x00000002UL; // relates to EAXSOURCE_ROOM
constexpr auto EAXSOURCEFLAGS_ROOMHFAUTO = 0x00000004UL; // relates to EAXSOURCE_ROOMHF
// EAX50
constexpr auto EAXSOURCEFLAGS_3DELEVATIONFILTER = 0x00000008UL;
// EAX50
constexpr auto EAXSOURCEFLAGS_UPMIX = 0x00000010UL;
// EAX50
constexpr auto EAXSOURCEFLAGS_APPLYSPEAKERLEVELS = 0x00000020UL;

constexpr auto EAX20SOURCEFLAGS_RESERVED = 0xFFFFFFF8UL; // reserved future use
constexpr auto EAX50SOURCEFLAGS_RESERVED = 0xFFFFFFC0UL; // reserved future use


constexpr auto EAXSOURCE_MINSEND = -10'000L;
constexpr auto EAXSOURCE_MAXSEND = 0L;
constexpr auto EAXSOURCE_DEFAULTSEND = 0L;

constexpr auto EAXSOURCE_MINSENDHF = -10'000L;
constexpr auto EAXSOURCE_MAXSENDHF = 0L;
constexpr auto EAXSOURCE_DEFAULTSENDHF = 0L;

constexpr auto EAXSOURCE_MINDIRECT = -10'000L;
constexpr auto EAXSOURCE_MAXDIRECT = 1'000L;
constexpr auto EAXSOURCE_DEFAULTDIRECT = 0L;

constexpr auto EAXSOURCE_MINDIRECTHF = -10'000L;
constexpr auto EAXSOURCE_MAXDIRECTHF = 0L;
constexpr auto EAXSOURCE_DEFAULTDIRECTHF = 0L;

constexpr auto EAXSOURCE_MINROOM = -10'000L;
constexpr auto EAXSOURCE_MAXROOM = 1'000L;
constexpr auto EAXSOURCE_DEFAULTROOM = 0L;

constexpr auto EAXSOURCE_MINROOMHF = -10'000L;
constexpr auto EAXSOURCE_MAXROOMHF = 0L;
constexpr auto EAXSOURCE_DEFAULTROOMHF = 0L;

constexpr auto EAXSOURCE_MINOBSTRUCTION = -10'000L;
constexpr auto EAXSOURCE_MAXOBSTRUCTION = 0L;
constexpr auto EAXSOURCE_DEFAULTOBSTRUCTION = 0L;

constexpr auto EAXSOURCE_MINOBSTRUCTIONLFRATIO = 0.0F;
constexpr auto EAXSOURCE_MAXOBSTRUCTIONLFRATIO = 1.0F;
constexpr auto EAXSOURCE_DEFAULTOBSTRUCTIONLFRATIO = 0.0F;

constexpr auto EAXSOURCE_MINOCCLUSION = -10'000L;
constexpr auto EAXSOURCE_MAXOCCLUSION = 0L;
constexpr auto EAXSOURCE_DEFAULTOCCLUSION = 0L;

constexpr auto EAXSOURCE_MINOCCLUSIONLFRATIO = 0.0F;
constexpr auto EAXSOURCE_MAXOCCLUSIONLFRATIO = 1.0F;
constexpr auto EAXSOURCE_DEFAULTOCCLUSIONLFRATIO = 0.25F;

constexpr auto EAXSOURCE_MINOCCLUSIONROOMRATIO = 0.0F;
constexpr auto EAXSOURCE_MAXOCCLUSIONROOMRATIO = 10.0F;
constexpr auto EAXSOURCE_DEFAULTOCCLUSIONROOMRATIO = 1.5F;

constexpr auto EAXSOURCE_MINOCCLUSIONDIRECTRATIO = 0.0F;
constexpr auto EAXSOURCE_MAXOCCLUSIONDIRECTRATIO = 10.0F;
constexpr auto EAXSOURCE_DEFAULTOCCLUSIONDIRECTRATIO = 1.0F;

constexpr auto EAXSOURCE_MINEXCLUSION = -10'000L;
constexpr auto EAXSOURCE_MAXEXCLUSION = 0L;
constexpr auto EAXSOURCE_DEFAULTEXCLUSION = 0L;

constexpr auto EAXSOURCE_MINEXCLUSIONLFRATIO = 0.0F;
constexpr auto EAXSOURCE_MAXEXCLUSIONLFRATIO = 1.0F;
constexpr auto EAXSOURCE_DEFAULTEXCLUSIONLFRATIO = 1.0F;

constexpr auto EAXSOURCE_MINOUTSIDEVOLUMEHF = -10'000L;
constexpr auto EAXSOURCE_MAXOUTSIDEVOLUMEHF = 0L;
constexpr auto EAXSOURCE_DEFAULTOUTSIDEVOLUMEHF = 0L;

constexpr auto EAXSOURCE_MINDOPPLERFACTOR = 0.0F;
constexpr auto EAXSOURCE_MAXDOPPLERFACTOR = 10.0F;
constexpr auto EAXSOURCE_DEFAULTDOPPLERFACTOR = 1.0F;

constexpr auto EAXSOURCE_MINROLLOFFFACTOR = 0.0F;
constexpr auto EAXSOURCE_MAXROLLOFFFACTOR = 10.0F;
constexpr auto EAXSOURCE_DEFAULTROLLOFFFACTOR = 0.0F;

constexpr auto EAXSOURCE_MINROOMROLLOFFFACTOR = 0.0F;
constexpr auto EAXSOURCE_MAXROOMROLLOFFFACTOR = 10.0F;
constexpr auto EAXSOURCE_DEFAULTROOMROLLOFFFACTOR = 0.0F;

constexpr auto EAXSOURCE_MINAIRABSORPTIONFACTOR = 0.0F;
constexpr auto EAXSOURCE_MAXAIRABSORPTIONFACTOR = 10.0F;
constexpr auto EAXSOURCE_DEFAULTAIRABSORPTIONFACTOR = 0.0F;

// EAX50

constexpr auto EAXSOURCE_MINMACROFXFACTOR = 0.0F;
constexpr auto EAXSOURCE_MAXMACROFXFACTOR = 1.0F;
constexpr auto EAXSOURCE_DEFAULTMACROFXFACTOR = 1.0F;

// EAX50

constexpr auto EAXSOURCE_MINSPEAKERLEVEL = -10'000L;
constexpr auto EAXSOURCE_MAXSPEAKERLEVEL = 0L;
constexpr auto EAXSOURCE_DEFAULTSPEAKERLEVEL = -10'000L;

constexpr auto EAXSOURCE_DEFAULTFLAGS =
    EAXSOURCEFLAGS_DIRECTHFAUTO |
    EAXSOURCEFLAGS_ROOMAUTO |
    EAXSOURCEFLAGS_ROOMHFAUTO;

enum :
    long
{
    EAXSPEAKER_FRONT_LEFT = 1,
    EAXSPEAKER_FRONT_CENTER = 2,
    EAXSPEAKER_FRONT_RIGHT = 3,
    EAXSPEAKER_SIDE_RIGHT = 4,
    EAXSPEAKER_REAR_RIGHT = 5,
    EAXSPEAKER_REAR_CENTER = 6,
    EAXSPEAKER_REAR_LEFT = 7,
    EAXSPEAKER_SIDE_LEFT = 8,
    EAXSPEAKER_LOW_FREQUENCY = 9
};

// EAXSOURCEFLAGS_DIRECTHFAUTO, EAXSOURCEFLAGS_ROOMAUTO and EAXSOURCEFLAGS_ROOMHFAUTO are ignored for 2D sources
// EAXSOURCEFLAGS_UPMIX is ignored for 3D sources
constexpr auto EAX50SOURCE_DEFAULTFLAGS =
    EAXSOURCEFLAGS_DIRECTHFAUTO |
    EAXSOURCEFLAGS_ROOMAUTO |
    EAXSOURCEFLAGS_ROOMHFAUTO |
    EAXSOURCEFLAGS_UPMIX;

struct EAX30SOURCEPROPERTIES
{
    long lDirect; // direct path level (at low and mid frequencies)
    long lDirectHF; // relative direct path level at high frequencies
    long lRoom; // room effect level (at low and mid frequencies)
    long lRoomHF; // relative room effect level at high frequencies
    long lObstruction; // main obstruction control (attenuation at high frequencies) 
    float flObstructionLFRatio; // obstruction low-frequency level re. main control
    long lOcclusion; // main occlusion control (attenuation at high frequencies)
    float flOcclusionLFRatio; // occlusion low-frequency level re. main control
    float flOcclusionRoomRatio; // relative occlusion control for room effect
    float flOcclusionDirectRatio; // relative occlusion control for direct path
    long lExclusion; // main exlusion control (attenuation at high frequencies)
    float flExclusionLFRatio; // exclusion low-frequency level re. main control
    long lOutsideVolumeHF; // outside sound cone level at high frequencies
    float flDopplerFactor; // like DS3D flDopplerFactor but per source
    float flRolloffFactor; // like DS3D flRolloffFactor but per source
    float flRoomRolloffFactor; // like DS3D flRolloffFactor but for room effect
    float flAirAbsorptionFactor; // multiplies EAXREVERB_AIRABSORPTIONHF
    unsigned long ulFlags; // modifies the behavior of properties
}; // EAX30SOURCEPROPERTIES

struct EAX50SOURCEPROPERTIES :
    public EAX30SOURCEPROPERTIES
{
    float flMacroFXFactor;
}; // EAX50SOURCEPROPERTIES

struct EAXSOURCEALLSENDPROPERTIES
{
    GUID guidReceivingFXSlotID;
    long lSend; // send level (at low and mid frequencies)
    long lSendHF; // relative send level at high frequencies
    long lOcclusion;
    float flOcclusionLFRatio;
    float flOcclusionRoomRatio;
    float flOcclusionDirectRatio;
    long lExclusion;
    float flExclusionLFRatio;
}; // EAXSOURCEALLSENDPROPERTIES

struct EAXSOURCE2DPROPERTIES
{
    long lDirect; // direct path level (at low and mid frequencies)
    long lDirectHF; // relative direct path level at high frequencies
    long lRoom; // room effect level (at low and mid frequencies)
    long lRoomHF; // relative room effect level at high frequencies
    unsigned long ulFlags; // modifies the behavior of properties
}; // EAXSOURCE2DPROPERTIES

struct EAXSPEAKERLEVELPROPERTIES
{
    long lSpeakerID;
    long lLevel;
}; // EAXSPEAKERLEVELPROPERTIES

struct EAX40ACTIVEFXSLOTS
{
    GUID guidActiveFXSlots[EAX40_MAX_ACTIVE_FXSLOTS];
}; // EAX40ACTIVEFXSLOTS

struct EAX50ACTIVEFXSLOTS
{
    GUID guidActiveFXSlots[EAX50_MAX_ACTIVE_FXSLOTS];
}; // EAX50ACTIVEFXSLOTS

bool operator==(
    const EAX50ACTIVEFXSLOTS& lhs,
    const EAX50ACTIVEFXSLOTS& rhs) noexcept;

bool operator!=(
    const EAX50ACTIVEFXSLOTS& lhs,
    const EAX50ACTIVEFXSLOTS& rhs) noexcept;

// Use this structure for EAXSOURCE_OBSTRUCTIONPARAMETERS property.
struct EAXOBSTRUCTIONPROPERTIES
{
    long lObstruction;
    float flObstructionLFRatio;
}; // EAXOBSTRUCTIONPROPERTIES

// Use this structure for EAXSOURCE_OCCLUSIONPARAMETERS property.
struct EAXOCCLUSIONPROPERTIES
{
    long lOcclusion;
    float flOcclusionLFRatio;
    float flOcclusionRoomRatio;
    float flOcclusionDirectRatio;
}; // EAXOCCLUSIONPROPERTIES

// Use this structure for EAXSOURCE_EXCLUSIONPARAMETERS property.
struct EAXEXCLUSIONPROPERTIES
{
    long lExclusion;
    float flExclusionLFRatio;
}; // EAXEXCLUSIONPROPERTIES

// Use this structure for EAXSOURCE_SENDPARAMETERS properties.
struct EAXSOURCESENDPROPERTIES
{
    GUID guidReceivingFXSlotID;
    long lSend;
    long lSendHF;
}; // EAXSOURCESENDPROPERTIES

// Use this structure for EAXSOURCE_OCCLUSIONSENDPARAMETERS 
struct EAXSOURCEOCCLUSIONSENDPROPERTIES
{
    GUID guidReceivingFXSlotID;
    long lOcclusion;
    float flOcclusionLFRatio;
    float flOcclusionRoomRatio;
    float flOcclusionDirectRatio;
}; // EAXSOURCEOCCLUSIONSENDPROPERTIES

// Use this structure for EAXSOURCE_EXCLUSIONSENDPARAMETERS
struct EAXSOURCEEXCLUSIONSENDPROPERTIES
{
    GUID guidReceivingFXSlotID;
    long lExclusion;
    float flExclusionLFRatio;
}; // EAXSOURCEEXCLUSIONSENDPROPERTIES

extern const EAX50ACTIVEFXSLOTS EAX40SOURCE_DEFAULTACTIVEFXSLOTID;

extern const EAX50ACTIVEFXSLOTS EAX50SOURCE_3DDEFAULTACTIVEFXSLOTID;

extern const EAX50ACTIVEFXSLOTS EAX50SOURCE_2DDEFAULTACTIVEFXSLOTID;


// EAX Reverb Effect

extern const GUID EAX_REVERB_EFFECT;

// Reverb effect properties
enum EAXREVERB_PROPERTY :
    unsigned int
{
    EAXREVERB_NONE,
    EAXREVERB_ALLPARAMETERS,
    EAXREVERB_ENVIRONMENT,
    EAXREVERB_ENVIRONMENTSIZE,
    EAXREVERB_ENVIRONMENTDIFFUSION,
    EAXREVERB_ROOM,
    EAXREVERB_ROOMHF,
    EAXREVERB_ROOMLF,
    EAXREVERB_DECAYTIME,
    EAXREVERB_DECAYHFRATIO,
    EAXREVERB_DECAYLFRATIO,
    EAXREVERB_REFLECTIONS,
    EAXREVERB_REFLECTIONSDELAY,
    EAXREVERB_REFLECTIONSPAN,
    EAXREVERB_REVERB,
    EAXREVERB_REVERBDELAY,
    EAXREVERB_REVERBPAN,
    EAXREVERB_ECHOTIME,
    EAXREVERB_ECHODEPTH,
    EAXREVERB_MODULATIONTIME,
    EAXREVERB_MODULATIONDEPTH,
    EAXREVERB_AIRABSORPTIONHF,
    EAXREVERB_HFREFERENCE,
    EAXREVERB_LFREFERENCE,
    EAXREVERB_ROOMROLLOFFFACTOR,
    EAXREVERB_FLAGS,
}; // EAXREVERB_PROPERTY

// used by EAXREVERB_ENVIRONMENT
enum :
    unsigned long
{
    EAX_ENVIRONMENT_GENERIC,
    EAX_ENVIRONMENT_PADDEDCELL,
    EAX_ENVIRONMENT_ROOM,
    EAX_ENVIRONMENT_BATHROOM,
    EAX_ENVIRONMENT_LIVINGROOM,
    EAX_ENVIRONMENT_STONEROOM,
    EAX_ENVIRONMENT_AUDITORIUM,
    EAX_ENVIRONMENT_CONCERTHALL,
    EAX_ENVIRONMENT_CAVE,
    EAX_ENVIRONMENT_ARENA,
    EAX_ENVIRONMENT_HANGAR,
    EAX_ENVIRONMENT_CARPETEDHALLWAY,
    EAX_ENVIRONMENT_HALLWAY,
    EAX_ENVIRONMENT_STONECORRIDOR,
    EAX_ENVIRONMENT_ALLEY,
    EAX_ENVIRONMENT_FOREST,
    EAX_ENVIRONMENT_CITY,
    EAX_ENVIRONMENT_MOUNTAINS,
    EAX_ENVIRONMENT_QUARRY,
    EAX_ENVIRONMENT_PLAIN,
    EAX_ENVIRONMENT_PARKINGLOT,
    EAX_ENVIRONMENT_SEWERPIPE,
    EAX_ENVIRONMENT_UNDERWATER,
    EAX_ENVIRONMENT_DRUGGED,
    EAX_ENVIRONMENT_DIZZY,
    EAX_ENVIRONMENT_PSYCHOTIC,

    EAX1_ENVIRONMENT_COUNT,

    // EAX30
    EAX_ENVIRONMENT_UNDEFINED = EAX1_ENVIRONMENT_COUNT,

    EAX3_ENVIRONMENT_COUNT,
};


// reverberation decay time
constexpr auto EAXREVERBFLAGS_DECAYTIMESCALE = 0x00000001UL;

// reflection level
constexpr auto EAXREVERBFLAGS_REFLECTIONSSCALE = 0x00000002UL;

// initial reflection delay time
constexpr auto EAXREVERBFLAGS_REFLECTIONSDELAYSCALE = 0x00000004UL;

// reflections level
constexpr auto EAXREVERBFLAGS_REVERBSCALE = 0x00000008UL;

// late reverberation delay time
constexpr auto EAXREVERBFLAGS_REVERBDELAYSCALE = 0x00000010UL;

// echo time
// EAX30+
constexpr auto EAXREVERBFLAGS_ECHOTIMESCALE = 0x00000040UL;

// modulation time
// EAX30+
constexpr auto EAXREVERBFLAGS_MODULATIONTIMESCALE = 0x00000080UL;

// This flag limits high-frequency decay time according to air absorption.
constexpr auto EAXREVERBFLAGS_DECAYHFLIMIT = 0x00000020UL;

constexpr auto EAXREVERBFLAGS_RESERVED = 0xFFFFFF00UL; // reserved future use


struct EAXREVERBPROPERTIES
{
    unsigned long ulEnvironment; // sets all reverb properties
    float flEnvironmentSize; // environment size in meters
    float flEnvironmentDiffusion; // environment diffusion
    long lRoom; // room effect level (at mid frequencies)
    long lRoomHF; // relative room effect level at high frequencies
    long lRoomLF; // relative room effect level at low frequencies  
    float flDecayTime; // reverberation decay time at mid frequencies
    float flDecayHFRatio; // high-frequency to mid-frequency decay time ratio
    float flDecayLFRatio; // low-frequency to mid-frequency decay time ratio   
    long lReflections; // early reflections level relative to room effect
    float flReflectionsDelay; // initial reflection delay time
    EAXVECTOR vReflectionsPan; // early reflections panning vector
    long lReverb; // late reverberation level relative to room effect
    float flReverbDelay; // late reverberation delay time relative to initial reflection
    EAXVECTOR vReverbPan; // late reverberation panning vector
    float flEchoTime; // echo time
    float flEchoDepth; // echo depth
    float flModulationTime; // modulation time
    float flModulationDepth; // modulation depth
    float flAirAbsorptionHF; // change in level per meter at high frequencies
    float flHFReference; // reference high frequency
    float flLFReference; // reference low frequency 
    float flRoomRolloffFactor; // like DS3D flRolloffFactor but for room effect
    unsigned long ulFlags; // modifies the behavior of properties
}; // EAXREVERBPROPERTIES

bool operator==(
    const EAXREVERBPROPERTIES& lhs,
    const EAXREVERBPROPERTIES& rhs) noexcept;

bool operator!=(
    const EAXREVERBPROPERTIES& lhs,
    const EAXREVERBPROPERTIES& rhs) noexcept;


constexpr auto EAXREVERB_MINENVIRONMENT = static_cast<unsigned long>(EAX_ENVIRONMENT_GENERIC);
constexpr auto EAX1REVERB_MAXENVIRONMENT = static_cast<unsigned long>(EAX_ENVIRONMENT_PSYCHOTIC);
constexpr auto EAX30REVERB_MAXENVIRONMENT = static_cast<unsigned long>(EAX_ENVIRONMENT_UNDEFINED);
constexpr auto EAXREVERB_DEFAULTENVIRONMENT = static_cast<unsigned long>(EAX_ENVIRONMENT_GENERIC);

constexpr auto EAXREVERB_MINENVIRONMENTSIZE = 1.0F;
constexpr auto EAXREVERB_MAXENVIRONMENTSIZE = 100.0F;
constexpr auto EAXREVERB_DEFAULTENVIRONMENTSIZE = 7.5F;

constexpr auto EAXREVERB_MINENVIRONMENTDIFFUSION = 0.0F;
constexpr auto EAXREVERB_MAXENVIRONMENTDIFFUSION = 1.0F;
constexpr auto EAXREVERB_DEFAULTENVIRONMENTDIFFUSION = 1.0F;

constexpr auto EAXREVERB_MINROOM = -10'000L;
constexpr auto EAXREVERB_MAXROOM = 0L;
constexpr auto EAXREVERB_DEFAULTROOM = -1'000L;

constexpr auto EAXREVERB_MINROOMHF = -10'000L;
constexpr auto EAXREVERB_MAXROOMHF = 0L;
constexpr auto EAXREVERB_DEFAULTROOMHF = -100L;

constexpr auto EAXREVERB_MINROOMLF = -10'000L;
constexpr auto EAXREVERB_MAXROOMLF = 0L;
constexpr auto EAXREVERB_DEFAULTROOMLF = 0L;

constexpr auto EAXREVERB_MINDECAYTIME = 0.1F;
constexpr auto EAXREVERB_MAXDECAYTIME = 20.0F;
constexpr auto EAXREVERB_DEFAULTDECAYTIME = 1.49F;

constexpr auto EAXREVERB_MINDECAYHFRATIO = 0.1F;
constexpr auto EAXREVERB_MAXDECAYHFRATIO = 2.0F;
constexpr auto EAXREVERB_DEFAULTDECAYHFRATIO = 0.83F;

constexpr auto EAXREVERB_MINDECAYLFRATIO = 0.1F;
constexpr auto EAXREVERB_MAXDECAYLFRATIO = 2.0F;
constexpr auto EAXREVERB_DEFAULTDECAYLFRATIO = 1.0F;

constexpr auto EAXREVERB_MINREFLECTIONS = -10'000L;
constexpr auto EAXREVERB_MAXREFLECTIONS = 1'000L;
constexpr auto EAXREVERB_DEFAULTREFLECTIONS = -2'602L;

constexpr auto EAXREVERB_MINREFLECTIONSDELAY = 0.0F;
constexpr auto EAXREVERB_MAXREFLECTIONSDELAY = 0.3F;
constexpr auto EAXREVERB_DEFAULTREFLECTIONSDELAY = 0.007F;

constexpr auto EAXREVERB_DEFAULTREFLECTIONSPAN = EAXVECTOR{0.0F, 0.0F, 0.0F};

constexpr auto EAXREVERB_MINREVERB = -10'000L;
constexpr auto EAXREVERB_MAXREVERB = 2'000L;
constexpr auto EAXREVERB_DEFAULTREVERB = 200L;

constexpr auto EAXREVERB_MINREVERBDELAY = 0.0F;
constexpr auto EAXREVERB_MAXREVERBDELAY = 0.1F;
constexpr auto EAXREVERB_DEFAULTREVERBDELAY = 0.011F;

constexpr auto EAXREVERB_DEFAULTREVERBPAN = EAXVECTOR{0.0F, 0.0F, 0.0F};

constexpr auto EAXREVERB_MINECHOTIME = 0.075F;
constexpr auto EAXREVERB_MAXECHOTIME = 0.25F;
constexpr auto EAXREVERB_DEFAULTECHOTIME = 0.25F;

constexpr auto EAXREVERB_MINECHODEPTH = 0.0F;
constexpr auto EAXREVERB_MAXECHODEPTH = 1.0F;
constexpr auto EAXREVERB_DEFAULTECHODEPTH = 0.0F;

constexpr auto EAXREVERB_MINMODULATIONTIME = 0.04F;
constexpr auto EAXREVERB_MAXMODULATIONTIME = 4.0F;
constexpr auto EAXREVERB_DEFAULTMODULATIONTIME = 0.25F;

constexpr auto EAXREVERB_MINMODULATIONDEPTH = 0.0F;
constexpr auto EAXREVERB_MAXMODULATIONDEPTH = 1.0F;
constexpr auto EAXREVERB_DEFAULTMODULATIONDEPTH = 0.0F;

constexpr auto EAXREVERB_MINAIRABSORPTIONHF = -100.0F;
constexpr auto EAXREVERB_MAXAIRABSORPTIONHF = 0.0F;
constexpr auto EAXREVERB_DEFAULTAIRABSORPTIONHF = -5.0F;

constexpr auto EAXREVERB_MINHFREFERENCE = 1'000.0F;
constexpr auto EAXREVERB_MAXHFREFERENCE = 20'000.0F;
constexpr auto EAXREVERB_DEFAULTHFREFERENCE = 5'000.0F;

constexpr auto EAXREVERB_MINLFREFERENCE = 20.0F;
constexpr auto EAXREVERB_MAXLFREFERENCE = 1'000.0F;
constexpr auto EAXREVERB_DEFAULTLFREFERENCE = 250.0F;

constexpr auto EAXREVERB_MINROOMROLLOFFFACTOR = 0.0F;
constexpr auto EAXREVERB_MAXROOMROLLOFFFACTOR = 10.0F;
constexpr auto EAXREVERB_DEFAULTROOMROLLOFFFACTOR = 0.0F;

constexpr auto EAX1REVERB_MINVOLUME = 0.0F;
constexpr auto EAX1REVERB_MAXVOLUME = 1.0F;

constexpr auto EAX1REVERB_MINDAMPING = 0.0F;
constexpr auto EAX1REVERB_MAXDAMPING = 2.0F;

constexpr auto EAXREVERB_DEFAULTFLAGS =
    EAXREVERBFLAGS_DECAYTIMESCALE |
    EAXREVERBFLAGS_REFLECTIONSSCALE |
    EAXREVERBFLAGS_REFLECTIONSDELAYSCALE |
    EAXREVERBFLAGS_REVERBSCALE |
    EAXREVERBFLAGS_REVERBDELAYSCALE |
    EAXREVERBFLAGS_DECAYHFLIMIT;


using EaxReverbPresets = std::array<EAXREVERBPROPERTIES, EAX1_ENVIRONMENT_COUNT>;
extern const EaxReverbPresets EAXREVERB_PRESETS;


using Eax1ReverbPresets = std::array<EAX_REVERBPROPERTIES, EAX1_ENVIRONMENT_COUNT>;
extern const Eax1ReverbPresets EAX1REVERB_PRESETS;


// AGC Compressor Effect

extern const GUID EAX_AGCCOMPRESSOR_EFFECT;

enum EAXAGCCOMPRESSOR_PROPERTY :
    unsigned int
{
    EAXAGCCOMPRESSOR_NONE,
    EAXAGCCOMPRESSOR_ALLPARAMETERS,
    EAXAGCCOMPRESSOR_ONOFF,
}; // EAXAGCCOMPRESSOR_PROPERTY

struct EAXAGCCOMPRESSORPROPERTIES
{
    unsigned long ulOnOff; // Switch Compressor on or off
}; // EAXAGCCOMPRESSORPROPERTIES


constexpr auto EAXAGCCOMPRESSOR_MINONOFF = 0UL;
constexpr auto EAXAGCCOMPRESSOR_MAXONOFF = 1UL;
constexpr auto EAXAGCCOMPRESSOR_DEFAULTONOFF = EAXAGCCOMPRESSOR_MAXONOFF;


// Autowah Effect

extern const GUID EAX_AUTOWAH_EFFECT;

enum EAXAUTOWAH_PROPERTY :
    unsigned int
{
    EAXAUTOWAH_NONE,
    EAXAUTOWAH_ALLPARAMETERS,
    EAXAUTOWAH_ATTACKTIME,
    EAXAUTOWAH_RELEASETIME,
    EAXAUTOWAH_RESONANCE,
    EAXAUTOWAH_PEAKLEVEL,
}; // EAXAUTOWAH_PROPERTY

struct EAXAUTOWAHPROPERTIES
{
    float flAttackTime; // Attack time (seconds)
    float flReleaseTime; // Release time (seconds)
    long lResonance; // Resonance (mB)
    long lPeakLevel; // Peak level (mB)
}; // EAXAUTOWAHPROPERTIES


constexpr auto EAXAUTOWAH_MINATTACKTIME = 0.0001F;
constexpr auto EAXAUTOWAH_MAXATTACKTIME = 1.0F;
constexpr auto EAXAUTOWAH_DEFAULTATTACKTIME = 0.06F;

constexpr auto EAXAUTOWAH_MINRELEASETIME = 0.0001F;
constexpr auto EAXAUTOWAH_MAXRELEASETIME = 1.0F;
constexpr auto EAXAUTOWAH_DEFAULTRELEASETIME = 0.06F;

constexpr auto EAXAUTOWAH_MINRESONANCE = 600L;
constexpr auto EAXAUTOWAH_MAXRESONANCE = 6000L;
constexpr auto EAXAUTOWAH_DEFAULTRESONANCE = 6000L;

constexpr auto EAXAUTOWAH_MINPEAKLEVEL = -9000L;
constexpr auto EAXAUTOWAH_MAXPEAKLEVEL = 9000L;
constexpr auto EAXAUTOWAH_DEFAULTPEAKLEVEL = 2100L;


// Chorus Effect

extern const GUID EAX_CHORUS_EFFECT;


enum EAXCHORUS_PROPERTY :
    unsigned int
{
    EAXCHORUS_NONE,
    EAXCHORUS_ALLPARAMETERS,
    EAXCHORUS_WAVEFORM,
    EAXCHORUS_PHASE,
    EAXCHORUS_RATE,
    EAXCHORUS_DEPTH,
    EAXCHORUS_FEEDBACK,
    EAXCHORUS_DELAY,
}; // EAXCHORUS_PROPERTY

enum :
    unsigned long
{
    EAX_CHORUS_SINUSOID,
    EAX_CHORUS_TRIANGLE,
};

struct EAXCHORUSPROPERTIES
{
    unsigned long ulWaveform; // Waveform selector - see enum above
    long lPhase; // Phase (Degrees)
    float flRate; // Rate (Hz)
    float flDepth; // Depth (0 to 1)
    float flFeedback; // Feedback (-1 to 1)
    float flDelay; // Delay (seconds)
}; // EAXCHORUSPROPERTIES


constexpr auto EAXCHORUS_MINWAVEFORM = 0UL;
constexpr auto EAXCHORUS_MAXWAVEFORM = 1UL;
constexpr auto EAXCHORUS_DEFAULTWAVEFORM = 1UL;

constexpr auto EAXCHORUS_MINPHASE = -180L;
constexpr auto EAXCHORUS_MAXPHASE = 180L;
constexpr auto EAXCHORUS_DEFAULTPHASE = 90L;

constexpr auto EAXCHORUS_MINRATE = 0.0F;
constexpr auto EAXCHORUS_MAXRATE = 10.0F;
constexpr auto EAXCHORUS_DEFAULTRATE = 1.1F;

constexpr auto EAXCHORUS_MINDEPTH = 0.0F;
constexpr auto EAXCHORUS_MAXDEPTH = 1.0F;
constexpr auto EAXCHORUS_DEFAULTDEPTH = 0.1F;

constexpr auto EAXCHORUS_MINFEEDBACK = -1.0F;
constexpr auto EAXCHORUS_MAXFEEDBACK = 1.0F;
constexpr auto EAXCHORUS_DEFAULTFEEDBACK = 0.25F;

constexpr auto EAXCHORUS_MINDELAY = 0.0002F;
constexpr auto EAXCHORUS_MAXDELAY = 0.016F;
constexpr auto EAXCHORUS_DEFAULTDELAY = 0.016F;


// Distortion Effect

extern const GUID EAX_DISTORTION_EFFECT;

enum EAXDISTORTION_PROPERTY :
    unsigned int
{
    EAXDISTORTION_NONE,
    EAXDISTORTION_ALLPARAMETERS,
    EAXDISTORTION_EDGE,
    EAXDISTORTION_GAIN,
    EAXDISTORTION_LOWPASSCUTOFF,
    EAXDISTORTION_EQCENTER,
    EAXDISTORTION_EQBANDWIDTH,
}; // EAXDISTORTION_PROPERTY


struct EAXDISTORTIONPROPERTIES
{
    float flEdge; // Controls the shape of the distortion (0 to 1)
    long lGain; // Controls the post distortion gain (mB)
    float flLowPassCutOff; // Controls the cut-off of the filter pre-distortion (Hz)
    float flEQCenter; // Controls the center frequency of the EQ post-distortion (Hz)
    float flEQBandwidth; // Controls the bandwidth of the EQ post-distortion (Hz)
}; // EAXDISTORTIONPROPERTIES


constexpr auto EAXDISTORTION_MINEDGE = 0.0F;
constexpr auto EAXDISTORTION_MAXEDGE = 1.0F;
constexpr auto EAXDISTORTION_DEFAULTEDGE = 0.2F;

constexpr auto EAXDISTORTION_MINGAIN = -6000L;
constexpr auto EAXDISTORTION_MAXGAIN = 0L;
constexpr auto EAXDISTORTION_DEFAULTGAIN = -2600L;

constexpr auto EAXDISTORTION_MINLOWPASSCUTOFF = 80.0F;
constexpr auto EAXDISTORTION_MAXLOWPASSCUTOFF = 24000.0F;
constexpr auto EAXDISTORTION_DEFAULTLOWPASSCUTOFF = 8000.0F;

constexpr auto EAXDISTORTION_MINEQCENTER = 80.0F;
constexpr auto EAXDISTORTION_MAXEQCENTER = 24000.0F;
constexpr auto EAXDISTORTION_DEFAULTEQCENTER = 3600.0F;

constexpr auto EAXDISTORTION_MINEQBANDWIDTH = 80.0F;
constexpr auto EAXDISTORTION_MAXEQBANDWIDTH = 24000.0F;
constexpr auto EAXDISTORTION_DEFAULTEQBANDWIDTH = 3600.0F;


// Echo Effect

extern const GUID EAX_ECHO_EFFECT;


enum EAXECHO_PROPERTY :
    unsigned int
{
    EAXECHO_NONE,
    EAXECHO_ALLPARAMETERS,
    EAXECHO_DELAY,
    EAXECHO_LRDELAY,
    EAXECHO_DAMPING,
    EAXECHO_FEEDBACK,
    EAXECHO_SPREAD,
}; // EAXECHO_PROPERTY


struct EAXECHOPROPERTIES
{
    float flDelay; // Controls the initial delay time (seconds)
    float flLRDelay; // Controls the delay time between the first and second taps (seconds)
    float flDamping; // Controls a low-pass filter that dampens the echoes (0 to 1)
    float flFeedback; // Controls the duration of echo repetition (0 to 1)
    float flSpread; // Controls the left-right spread of the echoes
}; // EAXECHOPROPERTIES


constexpr auto EAXECHO_MINDAMPING = 0.0F;
constexpr auto EAXECHO_MAXDAMPING = 0.99F;
constexpr auto EAXECHO_DEFAULTDAMPING = 0.5F;

constexpr auto EAXECHO_MINDELAY = 0.002F;
constexpr auto EAXECHO_MAXDELAY = 0.207F;
constexpr auto EAXECHO_DEFAULTDELAY = 0.1F;

constexpr auto EAXECHO_MINLRDELAY = 0.0F;
constexpr auto EAXECHO_MAXLRDELAY = 0.404F;
constexpr auto EAXECHO_DEFAULTLRDELAY = 0.1F;

constexpr auto EAXECHO_MINFEEDBACK = 0.0F;
constexpr auto EAXECHO_MAXFEEDBACK = 1.0F;
constexpr auto EAXECHO_DEFAULTFEEDBACK = 0.5F;

constexpr auto EAXECHO_MINSPREAD = -1.0F;
constexpr auto EAXECHO_MAXSPREAD = 1.0F;
constexpr auto EAXECHO_DEFAULTSPREAD = -1.0F;


// Equalizer Effect

extern const GUID EAX_EQUALIZER_EFFECT;


enum EAXEQUALIZER_PROPERTY :
    unsigned int
{
    EAXEQUALIZER_NONE,
    EAXEQUALIZER_ALLPARAMETERS,
    EAXEQUALIZER_LOWGAIN,
    EAXEQUALIZER_LOWCUTOFF,
    EAXEQUALIZER_MID1GAIN,
    EAXEQUALIZER_MID1CENTER,
    EAXEQUALIZER_MID1WIDTH,
    EAXEQUALIZER_MID2GAIN,
    EAXEQUALIZER_MID2CENTER,
    EAXEQUALIZER_MID2WIDTH,
    EAXEQUALIZER_HIGHGAIN,
    EAXEQUALIZER_HIGHCUTOFF,
}; // EAXEQUALIZER_PROPERTY


struct EAXEQUALIZERPROPERTIES
{
    long lLowGain; // (mB)
    float flLowCutOff; // (Hz)
    long lMid1Gain; // (mB)
    float flMid1Center; // (Hz)
    float flMid1Width; // (octaves)
    long lMid2Gain; // (mB)
    float flMid2Center; // (Hz)
    float flMid2Width; // (octaves)
    long lHighGain; // (mB)
    float flHighCutOff; // (Hz)
}; // EAXEQUALIZERPROPERTIES


constexpr auto EAXEQUALIZER_MINLOWGAIN = -1800L;
constexpr auto EAXEQUALIZER_MAXLOWGAIN = 1800L;
constexpr auto EAXEQUALIZER_DEFAULTLOWGAIN = 0L;

constexpr auto EAXEQUALIZER_MINLOWCUTOFF = 50.0F;
constexpr auto EAXEQUALIZER_MAXLOWCUTOFF = 800.0F;
constexpr auto EAXEQUALIZER_DEFAULTLOWCUTOFF = 200.0F;

constexpr auto EAXEQUALIZER_MINMID1GAIN = -1800L;
constexpr auto EAXEQUALIZER_MAXMID1GAIN = 1800L;
constexpr auto EAXEQUALIZER_DEFAULTMID1GAIN = 0L;

constexpr auto EAXEQUALIZER_MINMID1CENTER = 200.0F;
constexpr auto EAXEQUALIZER_MAXMID1CENTER = 3000.0F;
constexpr auto EAXEQUALIZER_DEFAULTMID1CENTER = 500.0F;

constexpr auto EAXEQUALIZER_MINMID1WIDTH = 0.01F;
constexpr auto EAXEQUALIZER_MAXMID1WIDTH = 1.0F;
constexpr auto EAXEQUALIZER_DEFAULTMID1WIDTH = 1.0F;

constexpr auto EAXEQUALIZER_MINMID2GAIN = -1800L;
constexpr auto EAXEQUALIZER_MAXMID2GAIN = 1800L;
constexpr auto EAXEQUALIZER_DEFAULTMID2GAIN = 0L;

constexpr auto EAXEQUALIZER_MINMID2CENTER = 1000.0F;
constexpr auto EAXEQUALIZER_MAXMID2CENTER = 8000.0F;
constexpr auto EAXEQUALIZER_DEFAULTMID2CENTER = 3000.0F;

constexpr auto EAXEQUALIZER_MINMID2WIDTH = 0.01F;
constexpr auto EAXEQUALIZER_MAXMID2WIDTH = 1.0F;
constexpr auto EAXEQUALIZER_DEFAULTMID2WIDTH = 1.0F;

constexpr auto EAXEQUALIZER_MINHIGHGAIN = -1800L;
constexpr auto EAXEQUALIZER_MAXHIGHGAIN = 1800L;
constexpr auto EAXEQUALIZER_DEFAULTHIGHGAIN = 0L;

constexpr auto EAXEQUALIZER_MINHIGHCUTOFF = 4000.0F;
constexpr auto EAXEQUALIZER_MAXHIGHCUTOFF = 16000.0F;
constexpr auto EAXEQUALIZER_DEFAULTHIGHCUTOFF = 6000.0F;


// Flanger Effect

extern const GUID EAX_FLANGER_EFFECT;

enum EAXFLANGER_PROPERTY :
    unsigned int
{
    EAXFLANGER_NONE,
    EAXFLANGER_ALLPARAMETERS,
    EAXFLANGER_WAVEFORM,
    EAXFLANGER_PHASE,
    EAXFLANGER_RATE,
    EAXFLANGER_DEPTH,
    EAXFLANGER_FEEDBACK,
    EAXFLANGER_DELAY,
}; // EAXFLANGER_PROPERTY

enum :
    unsigned long
{
    EAX_FLANGER_SINUSOID,
    EAX_FLANGER_TRIANGLE,
};

struct EAXFLANGERPROPERTIES
{
    unsigned long ulWaveform; // Waveform selector - see enum above
    long lPhase; // Phase (Degrees)
    float flRate; // Rate (Hz)
    float flDepth; // Depth (0 to 1)
    float flFeedback; // Feedback (0 to 1)
    float flDelay; // Delay (seconds)
}; // EAXFLANGERPROPERTIES


constexpr auto EAXFLANGER_MINWAVEFORM = 0UL;
constexpr auto EAXFLANGER_MAXWAVEFORM = 1UL;
constexpr auto EAXFLANGER_DEFAULTWAVEFORM = 1UL;

constexpr auto EAXFLANGER_MINPHASE = -180L;
constexpr auto EAXFLANGER_MAXPHASE = 180L;
constexpr auto EAXFLANGER_DEFAULTPHASE = 0L;

constexpr auto EAXFLANGER_MINRATE = 0.0F;
constexpr auto EAXFLANGER_MAXRATE = 10.0F;
constexpr auto EAXFLANGER_DEFAULTRATE = 0.27F;

constexpr auto EAXFLANGER_MINDEPTH = 0.0F;
constexpr auto EAXFLANGER_MAXDEPTH = 1.0F;
constexpr auto EAXFLANGER_DEFAULTDEPTH = 1.0F;

constexpr auto EAXFLANGER_MINFEEDBACK = -1.0F;
constexpr auto EAXFLANGER_MAXFEEDBACK = 1.0F;
constexpr auto EAXFLANGER_DEFAULTFEEDBACK = -0.5F;

constexpr auto EAXFLANGER_MINDELAY = 0.0002F;
constexpr auto EAXFLANGER_MAXDELAY = 0.004F;
constexpr auto EAXFLANGER_DEFAULTDELAY = 0.002F;


// Frequency Shifter Effect

extern const GUID EAX_FREQUENCYSHIFTER_EFFECT;

enum EAXFREQUENCYSHIFTER_PROPERTY :
    unsigned int
{
    EAXFREQUENCYSHIFTER_NONE,
    EAXFREQUENCYSHIFTER_ALLPARAMETERS,
    EAXFREQUENCYSHIFTER_FREQUENCY,
    EAXFREQUENCYSHIFTER_LEFTDIRECTION,
    EAXFREQUENCYSHIFTER_RIGHTDIRECTION,
}; // EAXFREQUENCYSHIFTER_PROPERTY

enum :
    unsigned long
{
    EAX_FREQUENCYSHIFTER_DOWN,
    EAX_FREQUENCYSHIFTER_UP,
    EAX_FREQUENCYSHIFTER_OFF
};

struct EAXFREQUENCYSHIFTERPROPERTIES
{
    float flFrequency; // (Hz)
    unsigned long ulLeftDirection; // see enum above
    unsigned long ulRightDirection; // see enum above
}; // EAXFREQUENCYSHIFTERPROPERTIES


constexpr auto EAXFREQUENCYSHIFTER_MINFREQUENCY = 0.0F;
constexpr auto EAXFREQUENCYSHIFTER_MAXFREQUENCY = 24000.0F;
constexpr auto EAXFREQUENCYSHIFTER_DEFAULTFREQUENCY = EAXFREQUENCYSHIFTER_MINFREQUENCY;

constexpr auto EAXFREQUENCYSHIFTER_MINLEFTDIRECTION = 0UL;
constexpr auto EAXFREQUENCYSHIFTER_MAXLEFTDIRECTION = 2UL;
constexpr auto EAXFREQUENCYSHIFTER_DEFAULTLEFTDIRECTION = EAXFREQUENCYSHIFTER_MINLEFTDIRECTION;

constexpr auto EAXFREQUENCYSHIFTER_MINRIGHTDIRECTION = 0UL;
constexpr auto EAXFREQUENCYSHIFTER_MAXRIGHTDIRECTION = 2UL;
constexpr auto EAXFREQUENCYSHIFTER_DEFAULTRIGHTDIRECTION = EAXFREQUENCYSHIFTER_MINRIGHTDIRECTION;


// Vocal Morpher Effect

extern const GUID EAX_VOCALMORPHER_EFFECT;

enum EAXVOCALMORPHER_PROPERTY :
    unsigned int
{
    EAXVOCALMORPHER_NONE,
    EAXVOCALMORPHER_ALLPARAMETERS,
    EAXVOCALMORPHER_PHONEMEA,
    EAXVOCALMORPHER_PHONEMEACOARSETUNING,
    EAXVOCALMORPHER_PHONEMEB,
    EAXVOCALMORPHER_PHONEMEBCOARSETUNING,
    EAXVOCALMORPHER_WAVEFORM,
    EAXVOCALMORPHER_RATE,
}; // EAXVOCALMORPHER_PROPERTY

enum :
    unsigned long
{
    A,
    E,
    I,
    O,
    U,
    AA,
    AE,
    AH,
    AO,
    EH,
    ER,
    IH,
    IY,
    UH,
    UW,
    B,
    D,
    F,
    G,
    J,
    K,
    L,
    M,
    N,
    P,
    R,
    S,
    T,
    V,
    Z,
};

enum :
    unsigned long
{
    EAX_VOCALMORPHER_SINUSOID,
    EAX_VOCALMORPHER_TRIANGLE,
    EAX_VOCALMORPHER_SAWTOOTH
};

// Use this structure for EAXVOCALMORPHER_ALLPARAMETERS
struct EAXVOCALMORPHERPROPERTIES
{
    unsigned long ulPhonemeA; // see enum above
    long lPhonemeACoarseTuning; // (semitones)
    unsigned long ulPhonemeB; // see enum above
    long lPhonemeBCoarseTuning; // (semitones)
    unsigned long ulWaveform; // Waveform selector - see enum above
    float flRate; // (Hz)
}; // EAXVOCALMORPHERPROPERTIES


constexpr auto EAXVOCALMORPHER_MINPHONEMEA = 0UL;
constexpr auto EAXVOCALMORPHER_MAXPHONEMEA = 29UL;
constexpr auto EAXVOCALMORPHER_DEFAULTPHONEMEA = EAXVOCALMORPHER_MINPHONEMEA;

constexpr auto EAXVOCALMORPHER_MINPHONEMEACOARSETUNING = -24L;
constexpr auto EAXVOCALMORPHER_MAXPHONEMEACOARSETUNING = 24L;
constexpr auto EAXVOCALMORPHER_DEFAULTPHONEMEACOARSETUNING = 0L;

constexpr auto EAXVOCALMORPHER_MINPHONEMEB = 0UL;
constexpr auto EAXVOCALMORPHER_MAXPHONEMEB = 29UL;
constexpr auto EAXVOCALMORPHER_DEFAULTPHONEMEB = 10UL;

constexpr auto EAXVOCALMORPHER_MINPHONEMEBCOARSETUNING = -24L;
constexpr auto EAXVOCALMORPHER_MAXPHONEMEBCOARSETUNING = 24L;
constexpr auto EAXVOCALMORPHER_DEFAULTPHONEMEBCOARSETUNING = 0L;

constexpr auto EAXVOCALMORPHER_MINWAVEFORM = 0UL;
constexpr auto EAXVOCALMORPHER_MAXWAVEFORM = 2UL;
constexpr auto EAXVOCALMORPHER_DEFAULTWAVEFORM = EAXVOCALMORPHER_MINWAVEFORM;

constexpr auto EAXVOCALMORPHER_MINRATE = 0.0F;
constexpr auto EAXVOCALMORPHER_MAXRATE = 10.0F;
constexpr auto EAXVOCALMORPHER_DEFAULTRATE = 1.41F;


// Pitch Shifter Effect

extern const GUID EAX_PITCHSHIFTER_EFFECT;

enum EAXPITCHSHIFTER_PROPERTY :
    unsigned int
{
    EAXPITCHSHIFTER_NONE,
    EAXPITCHSHIFTER_ALLPARAMETERS,
    EAXPITCHSHIFTER_COARSETUNE,
    EAXPITCHSHIFTER_FINETUNE,
}; // EAXPITCHSHIFTER_PROPERTY

struct EAXPITCHSHIFTERPROPERTIES
{
    long lCoarseTune; // Amount of pitch shift (semitones)
    long lFineTune; // Amount of pitch shift (cents)
}; // EAXPITCHSHIFTERPROPERTIES


constexpr auto EAXPITCHSHIFTER_MINCOARSETUNE = -12L;
constexpr auto EAXPITCHSHIFTER_MAXCOARSETUNE = 12L;
constexpr auto EAXPITCHSHIFTER_DEFAULTCOARSETUNE = 12L;

constexpr auto EAXPITCHSHIFTER_MINFINETUNE = -50L;
constexpr auto EAXPITCHSHIFTER_MAXFINETUNE = 50L;
constexpr auto EAXPITCHSHIFTER_DEFAULTFINETUNE = 0L;


// Ring Modulator Effect

extern const GUID EAX_RINGMODULATOR_EFFECT;

enum EAXRINGMODULATOR_PROPERTY :
    unsigned int
{
    EAXRINGMODULATOR_NONE,
    EAXRINGMODULATOR_ALLPARAMETERS,
    EAXRINGMODULATOR_FREQUENCY,
    EAXRINGMODULATOR_HIGHPASSCUTOFF,
    EAXRINGMODULATOR_WAVEFORM,
}; // EAXRINGMODULATOR_PROPERTY

enum :
    unsigned long
{
    EAX_RINGMODULATOR_SINUSOID,
    EAX_RINGMODULATOR_SAWTOOTH,
    EAX_RINGMODULATOR_SQUARE,
};

// Use this structure for EAXRINGMODULATOR_ALLPARAMETERS
struct EAXRINGMODULATORPROPERTIES
{
    float flFrequency; // Frequency of modulation (Hz)
    float flHighPassCutOff; // Cut-off frequency of high-pass filter (Hz)
    unsigned long ulWaveform; // Waveform selector - see enum above
}; // EAXRINGMODULATORPROPERTIES


constexpr auto EAXRINGMODULATOR_MINFREQUENCY = 0.0F;
constexpr auto EAXRINGMODULATOR_MAXFREQUENCY = 8000.0F;
constexpr auto EAXRINGMODULATOR_DEFAULTFREQUENCY = 440.0F;

constexpr auto EAXRINGMODULATOR_MINHIGHPASSCUTOFF = 0.0F;
constexpr auto EAXRINGMODULATOR_MAXHIGHPASSCUTOFF = 24000.0F;
constexpr auto EAXRINGMODULATOR_DEFAULTHIGHPASSCUTOFF = 800.0F;

constexpr auto EAXRINGMODULATOR_MINWAVEFORM = 0UL;
constexpr auto EAXRINGMODULATOR_MAXWAVEFORM = 2UL;
constexpr auto EAXRINGMODULATOR_DEFAULTWAVEFORM = EAXRINGMODULATOR_MINWAVEFORM;


using LPEAXSET = ALenum(AL_APIENTRY*)(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_buffer,
    ALuint property_size);

using LPEAXGET = ALenum(AL_APIENTRY*)(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_buffer,
    ALuint property_size);


#endif // !EAX_API_INCLUDED
