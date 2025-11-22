//
// EAX API.
//
// Based on headers `eax[2-5].h` included in Doom 3 source code:
// https://github.com/id-Software/DOOM-3/tree/master/neo/openal/include
//

#include "config.h"

#include <algorithm>

#include "al/eax_api.h"


const GUID DSPROPSETID_EAX_ReverbProperties =
{
    0x4A4E6FC1,
    0xC341,
    0x11D1,
    {0xB7, 0x3A, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
};

const GUID DSPROPSETID_EAXBUFFER_ReverbProperties =
{
    0x4A4E6FC0,
    0xC341,
    0x11D1,
    {0xB7, 0x3A, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
};

const GUID DSPROPSETID_EAX20_ListenerProperties =
{
    0x306A6A8,
    0xB224,
    0x11D2,
    {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
};

const GUID DSPROPSETID_EAX20_BufferProperties =
{
    0x306A6A7,
    0xB224,
    0x11D2,
    {0x99, 0xE5, 0x00, 0x00, 0xE8, 0xD8, 0xC7, 0x22}
};

const GUID DSPROPSETID_EAX30_ListenerProperties =
{
    0xA8FA6882,
    0xB476,
    0x11D3,
    {0xBD, 0xB9, 0x00, 0xC0, 0xF0, 0x2D, 0xDF, 0x87}
};

const GUID DSPROPSETID_EAX30_BufferProperties =
{
    0xA8FA6881,
    0xB476,
    0x11D3,
    {0xBD, 0xB9, 0x00, 0xC0, 0xF0, 0x2D, 0xDF, 0x87}
};

const GUID EAX_NULL_GUID =
{
    0x00000000,
    0x0000,
    0x0000,
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

const GUID EAX_PrimaryFXSlotID =
{
    0xF317866D,
    0x924C,
    0x450C,
    {0x86, 0x1B, 0xE6, 0xDA, 0xA2, 0x5E, 0x7C, 0x20}
};

const GUID EAXPROPERTYID_EAX40_Context =
{
    0x1D4870AD,
    0xDEF,
    0x43C0,
    {0xA4, 0xC, 0x52, 0x36, 0x32, 0x29, 0x63, 0x42}
};

const GUID EAXPROPERTYID_EAX50_Context =
{
    0x57E13437,
    0xB932,
    0x4AB2,
    {0xB8, 0xBD, 0x52, 0x66, 0xC1, 0xA8, 0x87, 0xEE}
};

const GUID EAXPROPERTYID_EAX40_FXSlot0 =
{
    0xC4D79F1E,
    0xF1AC,
    0x436B,
    {0xA8, 0x1D, 0xA7, 0x38, 0xE7, 0x04, 0x54, 0x69}
};

const GUID EAXPROPERTYID_EAX50_FXSlot0 =
{
    0x91F9590F,
    0xC388,
    0x407A,
    {0x84, 0xB0, 0x1B, 0xAE, 0xE, 0xF7, 0x1A, 0xBC}
};

const GUID EAXPROPERTYID_EAX40_FXSlot1 =
{
    0x8C00E96,
    0x74BE,
    0x4491,
    {0x93, 0xAA, 0xE8, 0xAD, 0x35, 0xA4, 0x91, 0x17}
};

const GUID EAXPROPERTYID_EAX50_FXSlot1 =
{
    0x8F5F7ACA,
    0x9608,
    0x4965,
    {0x81, 0x37, 0x82, 0x13, 0xC7, 0xB9, 0xD9, 0xDE}
};

const GUID EAXPROPERTYID_EAX40_FXSlot2 =
{
    0x1D433B88,
    0xF0F6,
    0x4637,
    {0x91, 0x9F, 0x60, 0xE7, 0xE0, 0x6B, 0x5E, 0xDD}
};

const GUID EAXPROPERTYID_EAX50_FXSlot2 =
{
    0x3C0F5252,
    0x9834,
    0x46F0,
    {0xA1, 0xD8, 0x5B, 0x95, 0xC4, 0xA0, 0xA, 0x30}
};

const GUID EAXPROPERTYID_EAX40_FXSlot3 =
{
    0xEFFF08EA,
    0xC7D8,
    0x44AB,
    {0x93, 0xAD, 0x6D, 0xBD, 0x5F, 0x91, 0x00, 0x64}
};

const GUID EAXPROPERTYID_EAX50_FXSlot3 =
{
    0xE2EB0EAA,
    0xE806,
    0x45E7,
    {0x9F, 0x86, 0x06, 0xC1, 0x57, 0x1A, 0x6F, 0xA3}
};

const GUID EAXPROPERTYID_EAX40_Source =
{
    0x1B86B823,
    0x22DF,
    0x4EAE,
    {0x8B, 0x3C, 0x12, 0x78, 0xCE, 0x54, 0x42, 0x27}
};

const GUID EAXPROPERTYID_EAX50_Source =
{
    0x5EDF82F0,
    0x24A7,
    0x4F38,
    {0x8E, 0x64, 0x2F, 0x09, 0xCA, 0x05, 0xDE, 0xE1}
};

const GUID EAX_REVERB_EFFECT =
{
    0xCF95C8F,
    0xA3CC,
    0x4849,
    {0xB0, 0xB6, 0x83, 0x2E, 0xCC, 0x18, 0x22, 0xDF}
};

const GUID EAX_AGCCOMPRESSOR_EFFECT =
{
    0xBFB7A01E,
    0x7825,
    0x4039,
    {0x92, 0x7F, 0x03, 0xAA, 0xBD, 0xA0, 0xC5, 0x60}
};

const GUID EAX_AUTOWAH_EFFECT =
{
    0xEC3130C0,
    0xAC7A,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_CHORUS_EFFECT =
{
    0xDE6D6FE0,
    0xAC79,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_DISTORTION_EFFECT =
{
    0x975A4CE0,
    0xAC7E,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_ECHO_EFFECT =
{
    0xE9F1BC0,
    0xAC82,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_EQUALIZER_EFFECT =
{
    0x65F94CE0,
    0x9793,
    0x11D3,
    {0x93, 0x9D, 0x00, 0xC0, 0xF0, 0x2D, 0xD6, 0xF0}
};

const GUID EAX_FLANGER_EFFECT =
{
    0xA70007C0,
    0x7D2,
    0x11D3,
    {0x9B, 0x1E, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_FREQUENCYSHIFTER_EFFECT =
{
    0xDC3E1880,
    0x9212,
    0x11D3,
    {0x93, 0x9D, 0x00, 0xC0, 0xF0, 0x2D, 0xD6, 0xF0}
};

const GUID EAX_VOCALMORPHER_EFFECT =
{
    0xE41CF10C,
    0x3383,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_PITCHSHIFTER_EFFECT =
{
    0xE7905100,
    0xAFB2,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};

const GUID EAX_RINGMODULATOR_EFFECT =
{
    0xB89FE60,
    0xAFB5,
    0x11D2,
    {0x88, 0xDD, 0x00, 0xA0, 0x24, 0xD1, 0x3C, 0xE1}
};


bool operator==(
    const EAX40CONTEXTPROPERTIES& lhs,
    const EAX40CONTEXTPROPERTIES& rhs) noexcept
{
    return
        lhs.guidPrimaryFXSlotID == rhs.guidPrimaryFXSlotID &&
        lhs.flDistanceFactor == rhs.flDistanceFactor &&
        lhs.flAirAbsorptionHF == rhs.flAirAbsorptionHF &&
        lhs.flHFReference == rhs.flHFReference;
}

bool operator==(
    const EAX50CONTEXTPROPERTIES& lhs,
    const EAX50CONTEXTPROPERTIES& rhs) noexcept
{
    return
        static_cast<const EAX40CONTEXTPROPERTIES&>(lhs) == static_cast<const EAX40CONTEXTPROPERTIES&>(rhs) &&
        lhs.flMacroFXFactor == rhs.flMacroFXFactor;
}


const GUID EAXCONTEXT_DEFAULTPRIMARYFXSLOTID = EAXPROPERTYID_EAX40_FXSlot0;

bool operator==(
    const EAX40FXSLOTPROPERTIES& lhs,
    const EAX40FXSLOTPROPERTIES& rhs) noexcept
{
    return
        lhs.guidLoadEffect == rhs.guidLoadEffect &&
        lhs.lVolume == rhs.lVolume &&
        lhs.lLock == rhs.lLock &&
        lhs.ulFlags == rhs.ulFlags;
}

bool operator==(
    const EAX50FXSLOTPROPERTIES& lhs,
    const EAX50FXSLOTPROPERTIES& rhs) noexcept
{
    return
        static_cast<const EAX40FXSLOTPROPERTIES&>(lhs) == static_cast<const EAX40FXSLOTPROPERTIES&>(rhs) &&
        lhs.lOcclusion == rhs.lOcclusion &&
        lhs.flOcclusionLFRatio == rhs.flOcclusionLFRatio;
}

const EAX50ACTIVEFXSLOTS EAX40SOURCE_DEFAULTACTIVEFXSLOTID = EAX50ACTIVEFXSLOTS
{{
    EAX_NULL_GUID,
    EAXPROPERTYID_EAX40_FXSlot0,
}};

bool operator==(
    const EAX50ACTIVEFXSLOTS& lhs,
    const EAX50ACTIVEFXSLOTS& rhs) noexcept
{
    return std::equal(
        std::cbegin(lhs.guidActiveFXSlots),
        std::cend(lhs.guidActiveFXSlots),
        std::begin(rhs.guidActiveFXSlots));
}

bool operator!=(
    const EAX50ACTIVEFXSLOTS& lhs,
    const EAX50ACTIVEFXSLOTS& rhs) noexcept
{
    return !(lhs == rhs);
}


const EAX50ACTIVEFXSLOTS EAX50SOURCE_3DDEFAULTACTIVEFXSLOTID = EAX50ACTIVEFXSLOTS
{{
    EAX_NULL_GUID,
    EAX_PrimaryFXSlotID,
    EAX_NULL_GUID,
    EAX_NULL_GUID,
}};


const EAX50ACTIVEFXSLOTS EAX50SOURCE_2DDEFAULTACTIVEFXSLOTID = EAX50ACTIVEFXSLOTS
{{
    EAX_NULL_GUID,
    EAX_NULL_GUID,
    EAX_NULL_GUID,
    EAX_NULL_GUID,
}};

bool operator==(
    const EAXREVERBPROPERTIES& lhs,
    const EAXREVERBPROPERTIES& rhs) noexcept
{
    return
        lhs.ulEnvironment == rhs.ulEnvironment &&
        lhs.flEnvironmentSize == rhs.flEnvironmentSize &&
        lhs.flEnvironmentDiffusion == rhs.flEnvironmentDiffusion &&
        lhs.lRoom == rhs.lRoom &&
        lhs.lRoomHF == rhs.lRoomHF &&
        lhs.lRoomLF == rhs.lRoomLF &&
        lhs.flDecayTime == rhs.flDecayTime &&
        lhs.flDecayHFRatio == rhs.flDecayHFRatio &&
        lhs.flDecayLFRatio == rhs.flDecayLFRatio &&
        lhs.lReflections == rhs.lReflections &&
        lhs.flReflectionsDelay == rhs.flReflectionsDelay &&
        lhs.vReflectionsPan == rhs.vReflectionsPan &&
        lhs.lReverb == rhs.lReverb &&
        lhs.flReverbDelay == rhs.flReverbDelay &&
        lhs.vReverbPan == rhs.vReverbPan &&
        lhs.flEchoTime == rhs.flEchoTime &&
        lhs.flEchoDepth == rhs.flEchoDepth &&
        lhs.flModulationTime == rhs.flModulationTime &&
        lhs.flModulationDepth == rhs.flModulationDepth &&
        lhs.flAirAbsorptionHF == rhs.flAirAbsorptionHF &&
        lhs.flHFReference == rhs.flHFReference &&
        lhs.flLFReference == rhs.flLFReference &&
        lhs.flRoomRolloffFactor == rhs.flRoomRolloffFactor &&
        lhs.ulFlags == rhs.ulFlags;
}

bool operator!=(
    const EAXREVERBPROPERTIES& lhs,
    const EAXREVERBPROPERTIES& rhs) noexcept
{
    return !(lhs == rhs);
}


namespace {

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_GENERIC =
{
    EAXREVERB_DEFAULTENVIRONMENT,
    EAXREVERB_DEFAULTENVIRONMENTSIZE,
    EAXREVERB_DEFAULTENVIRONMENTDIFFUSION,
    EAXREVERB_DEFAULTROOM,
    EAXREVERB_DEFAULTROOMHF,
    EAXREVERB_DEFAULTROOMLF,
    EAXREVERB_DEFAULTDECAYTIME,
    EAXREVERB_DEFAULTDECAYHFRATIO,
    EAXREVERB_DEFAULTDECAYLFRATIO,
    EAXREVERB_DEFAULTREFLECTIONS,
    EAXREVERB_DEFAULTREFLECTIONSDELAY,
    EAXREVERB_DEFAULTREFLECTIONSPAN,
    EAXREVERB_DEFAULTREVERB,
    EAXREVERB_DEFAULTREVERBDELAY,
    EAXREVERB_DEFAULTREVERBPAN,
    EAXREVERB_DEFAULTECHOTIME,
    EAXREVERB_DEFAULTECHODEPTH,
    EAXREVERB_DEFAULTMODULATIONTIME,
    EAXREVERB_DEFAULTMODULATIONDEPTH,
    EAXREVERB_DEFAULTAIRABSORPTIONHF,
    EAXREVERB_DEFAULTHFREFERENCE,
    EAXREVERB_DEFAULTLFREFERENCE,
    EAXREVERB_DEFAULTROOMROLLOFFFACTOR,
    EAXREVERB_DEFAULTFLAGS,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_PADDEDCELL =
{
    EAX_ENVIRONMENT_PADDEDCELL,
    1.4F,
    1.0F,
    -1'000L,
    -6'000L,
    0L,
    0.17F,
    0.10F,
    1.0F,
    -1'204L,
    0.001F,
    EAXVECTOR{},
    207L,
    0.002F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_ROOM =
{
    EAX_ENVIRONMENT_ROOM,
    1.9F,
    1.0F,
    -1'000L,
    -454L,
    0L,
    0.40F,
    0.83F,
    1.0F,
    -1'646L,
    0.002F,
    EAXVECTOR{},
    53L,
    0.003F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_BATHROOM =
{
    EAX_ENVIRONMENT_BATHROOM,
    1.4F,
    1.0F,
    -1'000L,
    -1'200L,
    0L,
    1.49F,
    0.54F,
    1.0F,
    -370L,
    0.007F,
    EAXVECTOR{},
    1'030L,
    0.011F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_LIVINGROOM =
{
    EAX_ENVIRONMENT_LIVINGROOM,
    2.5F,
    1.0F,
    -1'000L,
    -6'000L,
    0L,
    0.50F,
    0.10F,
    1.0F,
    -1'376,
    0.003F,
    EAXVECTOR{},
    -1'104L,
    0.004F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_STONEROOM =
{
    EAX_ENVIRONMENT_STONEROOM,
    11.6F,
    1.0F,
    -1'000L,
    -300L,
    0L,
    2.31F,
    0.64F,
    1.0F,
    -711L,
    0.012F,
    EAXVECTOR{},
    83L,
    0.017F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_AUDITORIUM =
{
    EAX_ENVIRONMENT_AUDITORIUM,
    21.6F,
    1.0F,
    -1'000L,
    -476L,
    0L,
    4.32F,
    0.59F,
    1.0F,
    -789L,
    0.020F,
    EAXVECTOR{},
    -289L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_CONCERTHALL =
{
    EAX_ENVIRONMENT_CONCERTHALL,
    19.6F,
    1.0F,
    -1'000L,
    -500L,
    0L,
    3.92F,
    0.70F,
    1.0F,
    -1'230L,
    0.020F,
    EAXVECTOR{},
    -2L,
    0.029F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_CAVE =
{
    EAX_ENVIRONMENT_CAVE,
    14.6F,
    1.0F,
    -1'000L,
    0L,
    0L,
    2.91F,
    1.30F,
    1.0F,
    -602L,
    0.015F,
    EAXVECTOR{},
    -302L,
    0.022F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x1FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_ARENA =
{
    EAX_ENVIRONMENT_ARENA,
    36.2F,
    1.0F,
    -1'000L,
    -698L,
    0L,
    7.24F,
    0.33F,
    1.0F,
    -1'166L,
    0.020F,
    EAXVECTOR{},
    16L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_HANGAR =
{
    EAX_ENVIRONMENT_HANGAR,
    50.3F,
    1.0F,
    -1'000L,
    -1'000L,
    0L,
    10.05F,
    0.23F,
    1.0F,
    -602L,
    0.020F,
    EAXVECTOR{},
    198L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_CARPETTEDHALLWAY =
{
    EAX_ENVIRONMENT_CARPETEDHALLWAY,
    1.9F,
    1.0F,
    -1'000L,
    -4'000L,
    0L,
    0.30F,
    0.10F,
    1.0F,
    -1'831L,
    0.002F,
    EAXVECTOR{},
    -1'630L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_HALLWAY =
{
    EAX_ENVIRONMENT_HALLWAY,
    1.8F,
    1.0F,
    -1'000L,
    -300L,
    0L,
    1.49F,
    0.59F,
    1.0F,
    -1'219L,
    0.007F,
    EAXVECTOR{},
    441L,
    0.011F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_STONECORRIDOR =
{
    EAX_ENVIRONMENT_STONECORRIDOR,
    13.5F,
    1.0F,
    -1'000L,
    -237L,
    0L,
    2.70F,
    0.79F,
    1.0F,
    -1'214L,
    0.013F,
    EAXVECTOR{},
    395L,
    0.020F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_ALLEY =
{
    EAX_ENVIRONMENT_ALLEY,
    7.5F,
    0.300F,
    -1'000L,
    -270L,
    0L,
    1.49F,
    0.86F,
    1.0F,
    -1'204L,
    0.007F,
    EAXVECTOR{},
    -4L,
    0.011F,
    EAXVECTOR{},
    0.125F,
    0.950F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_FOREST =
{
    EAX_ENVIRONMENT_FOREST,
    38.0F,
    0.300F,
    -1'000L,
    -3'300L,
    0L,
    1.49F,
    0.54F,
    1.0F,
    -2'560L,
    0.162F,
    EAXVECTOR{},
    -229L,
    0.088F,
    EAXVECTOR{},
    0.125F,
    1.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_CITY =
{
    EAX_ENVIRONMENT_CITY,
    7.5F,
    0.500F,
    -1'000L,
    -800L,
    0L,
    1.49F,
    0.67F,
    1.0F,
    -2'273L,
    0.007F,
    EAXVECTOR{},
    -1'691L,
    0.011F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_MOUNTAINS =
{
    EAX_ENVIRONMENT_MOUNTAINS,
    100.0F,
    0.270F,
    -1'000L,
    -2'500L,
    0L,
    1.49F,
    0.21F,
    1.0F,
    -2'780L,
    0.300F,
    EAXVECTOR{},
    -1'434L,
    0.100F,
    EAXVECTOR{},
    0.250F,
    1.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x1FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_QUARRY =
{
    EAX_ENVIRONMENT_QUARRY,
    17.5F,
    1.0F,
    -1'000L,
    -1'000L,
    0L,
    1.49F,
    0.83F,
    1.0F,
    -10'000L,
    0.061F,
    EAXVECTOR{},
    500L,
    0.025F,
    EAXVECTOR{},
    0.125F,
    0.700F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_PLAIN =
{
    EAX_ENVIRONMENT_PLAIN,
    42.5F,
    0.210F,
    -1'000L,
    -2'000L,
    0L,
    1.49F,
    0.50F,
    1.0F,
    -2'466L,
    0.179F,
    EAXVECTOR{},
    -1'926L,
    0.100F,
    EAXVECTOR{},
    0.250F,
    1.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_PARKINGLOT =
{
    EAX_ENVIRONMENT_PARKINGLOT,
    8.3F,
    1.0F,
    -1'000L,
    0L,
    0L,
    1.65F,
    1.50F,
    1.0F,
    -1'363L,
    0.008F,
    EAXVECTOR{},
    -1'153L,
    0.012F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x1FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_SEWERPIPE =
{
    EAX_ENVIRONMENT_SEWERPIPE,
    1.7F,
    0.800F,
    -1'000L,
    -1'000L,
    0L,
    2.81F,
    0.14F,
    1.0F,
    429L,
    0.014F,
    EAXVECTOR{},
    1'023L,
    0.021F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    0.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_UNDERWATER =
{
    EAX_ENVIRONMENT_UNDERWATER,
    1.8F,
    1.0F,
    -1'000L,
    -4'000L,
    0L,
    1.49F,
    0.10F,
    1.0F,
    -449L,
    0.007F,
    EAXVECTOR{},
    1'700L,
    0.011F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    1.180F,
    0.348F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x3FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_DRUGGED =
{
    EAX_ENVIRONMENT_DRUGGED,
    1.9F,
    0.500F,
    -1'000L,
    0L,
    0L,
    8.39F,
    1.39F,
    1.0F,
    -115L,
    0.002F,
    EAXVECTOR{},
    985L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    0.250F,
    1.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x1FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_DIZZY =
{
    EAX_ENVIRONMENT_DIZZY,
    1.8F,
    0.600F,
    -1'000L,
    -400L,
    0L,
    17.23F,
    0.56F,
    1.0F,
    -1'713L,
    0.020F,
    EAXVECTOR{},
    -613L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    1.0F,
    0.810F,
    0.310F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x1FUL,
};

constexpr EAXREVERBPROPERTIES EAXREVERB_PRESET_PSYCHOTIC =
{
    EAX_ENVIRONMENT_PSYCHOTIC,
    1.0F,
    0.500F,
    -1'000L,
    -151L,
    0L,
    7.56F,
    0.91F,
    1.0F,
    -626L,
    0.020F,
    EAXVECTOR{},
    774L,
    0.030F,
    EAXVECTOR{},
    0.250F,
    0.0F,
    4.0F,
    1.0F,
    -5.0F,
    5'000.0F,
    250.0F,
    0.0F,
    0x1FUL,
};

} // namespace

const EaxReverbPresets EAXREVERB_PRESETS{{
    EAXREVERB_PRESET_GENERIC,
    EAXREVERB_PRESET_PADDEDCELL,
    EAXREVERB_PRESET_ROOM,
    EAXREVERB_PRESET_BATHROOM,
    EAXREVERB_PRESET_LIVINGROOM,
    EAXREVERB_PRESET_STONEROOM,
    EAXREVERB_PRESET_AUDITORIUM,
    EAXREVERB_PRESET_CONCERTHALL,
    EAXREVERB_PRESET_CAVE,
    EAXREVERB_PRESET_ARENA,
    EAXREVERB_PRESET_HANGAR,
    EAXREVERB_PRESET_CARPETTEDHALLWAY,
    EAXREVERB_PRESET_HALLWAY,
    EAXREVERB_PRESET_STONECORRIDOR,
    EAXREVERB_PRESET_ALLEY,
    EAXREVERB_PRESET_FOREST,
    EAXREVERB_PRESET_CITY,
    EAXREVERB_PRESET_MOUNTAINS,
    EAXREVERB_PRESET_QUARRY,
    EAXREVERB_PRESET_PLAIN,
    EAXREVERB_PRESET_PARKINGLOT,
    EAXREVERB_PRESET_SEWERPIPE,
    EAXREVERB_PRESET_UNDERWATER,
    EAXREVERB_PRESET_DRUGGED,
    EAXREVERB_PRESET_DIZZY,
    EAXREVERB_PRESET_PSYCHOTIC,
}};

namespace {
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_GENERIC = {EAX_ENVIRONMENT_GENERIC, 0.5F, 1.493F, 0.5F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_PADDEDCELL = {EAX_ENVIRONMENT_PADDEDCELL, 0.25F, 0.1F, 0.0F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_ROOM = {EAX_ENVIRONMENT_ROOM, 0.417F, 0.4F, 0.666F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_BATHROOM = {EAX_ENVIRONMENT_BATHROOM, 0.653F, 1.499F, 0.166F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_LIVINGROOM = {EAX_ENVIRONMENT_LIVINGROOM, 0.208F, 0.478F, 0.0F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_STONEROOM = {EAX_ENVIRONMENT_STONEROOM, 0.5F, 2.309F, 0.888F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_AUDITORIUM = {EAX_ENVIRONMENT_AUDITORIUM, 0.403F, 4.279F, 0.5F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_CONCERTHALL = {EAX_ENVIRONMENT_CONCERTHALL, 0.5F, 3.961F, 0.5F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_CAVE = {EAX_ENVIRONMENT_CAVE, 0.5F, 2.886F, 1.304F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_ARENA = {EAX_ENVIRONMENT_ARENA, 0.361F, 7.284F, 0.332F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_HANGAR = {EAX_ENVIRONMENT_HANGAR, 0.5F, 10.0F, 0.3F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_CARPETTEDHALLWAY = {EAX_ENVIRONMENT_CARPETEDHALLWAY, 0.153F, 0.259F, 2.0F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_HALLWAY = {EAX_ENVIRONMENT_HALLWAY, 0.361F, 1.493F, 0.0F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_STONECORRIDOR = {EAX_ENVIRONMENT_STONECORRIDOR, 0.444F, 2.697F, 0.638F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_ALLEY = {EAX_ENVIRONMENT_ALLEY, 0.25F, 1.752F, 0.776F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_FOREST = {EAX_ENVIRONMENT_FOREST, 0.111F, 3.145F, 0.472F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_CITY = {EAX_ENVIRONMENT_CITY, 0.111F, 2.767F, 0.224F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_MOUNTAINS = {EAX_ENVIRONMENT_MOUNTAINS, 0.194F, 7.841F, 0.472F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_QUARRY = {EAX_ENVIRONMENT_QUARRY, 1.0F, 1.499F, 0.5F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_PLAIN = {EAX_ENVIRONMENT_PLAIN, 0.097F, 2.767F, 0.224F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_PARKINGLOT = {EAX_ENVIRONMENT_PARKINGLOT, 0.208F, 1.652F, 1.5F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_SEWERPIPE = {EAX_ENVIRONMENT_SEWERPIPE, 0.652F, 2.886F, 0.25F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_UNDERWATER = {EAX_ENVIRONMENT_UNDERWATER, 1.0F, 1.499F, 0.0F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_DRUGGED = {EAX_ENVIRONMENT_DRUGGED, 0.875F, 8.392F, 1.388F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_DIZZY = {EAX_ENVIRONMENT_DIZZY, 0.139F, 17.234F, 0.666F};
constexpr EAX_REVERBPROPERTIES EAX1REVERB_PRESET_PSYCHOTIC = {EAX_ENVIRONMENT_PSYCHOTIC, 0.486F, 7.563F, 0.806F};
} // namespace

const Eax1ReverbPresets EAX1REVERB_PRESETS{{
    EAX1REVERB_PRESET_GENERIC,
    EAX1REVERB_PRESET_PADDEDCELL,
    EAX1REVERB_PRESET_ROOM,
    EAX1REVERB_PRESET_BATHROOM,
    EAX1REVERB_PRESET_LIVINGROOM,
    EAX1REVERB_PRESET_STONEROOM,
    EAX1REVERB_PRESET_AUDITORIUM,
    EAX1REVERB_PRESET_CONCERTHALL,
    EAX1REVERB_PRESET_CAVE,
    EAX1REVERB_PRESET_ARENA,
    EAX1REVERB_PRESET_HANGAR,
    EAX1REVERB_PRESET_CARPETTEDHALLWAY,
    EAX1REVERB_PRESET_HALLWAY,
    EAX1REVERB_PRESET_STONECORRIDOR,
    EAX1REVERB_PRESET_ALLEY,
    EAX1REVERB_PRESET_FOREST,
    EAX1REVERB_PRESET_CITY,
    EAX1REVERB_PRESET_MOUNTAINS,
    EAX1REVERB_PRESET_QUARRY,
    EAX1REVERB_PRESET_PLAIN,
    EAX1REVERB_PRESET_PARKINGLOT,
    EAX1REVERB_PRESET_SEWERPIPE,
    EAX1REVERB_PRESET_UNDERWATER,
    EAX1REVERB_PRESET_DRUGGED,
    EAX1REVERB_PRESET_DIZZY,
    EAX1REVERB_PRESET_PSYCHOTIC,
}};
