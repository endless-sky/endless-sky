
#include "config.h"

#include <cmath>

#include "AL/al.h"
#include "AL/efx.h"

#include "alc/effects/base.h"
#include "effects.h"

#ifdef ALSOFT_EAX
#include <tuple>
#include "alnumeric.h"
#include "AL/efx-presets.h"
#include "al/eax_exception.h"
#include "al/eax_utils.h"
#endif // ALSOFT_EAX


namespace {

void Reverb_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_EAXREVERB_DECAY_HFLIMIT:
        if(!(val >= AL_EAXREVERB_MIN_DECAY_HFLIMIT && val <= AL_EAXREVERB_MAX_DECAY_HFLIMIT))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb decay hflimit out of range"};
        props->Reverb.DecayHFLimit = val != AL_FALSE;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid EAX reverb integer property 0x%04x",
            param};
    }
}
void Reverb_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Reverb_setParami(props, param, vals[0]); }
void Reverb_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_EAXREVERB_DENSITY:
        if(!(val >= AL_EAXREVERB_MIN_DENSITY && val <= AL_EAXREVERB_MAX_DENSITY))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb density out of range"};
        props->Reverb.Density = val;
        break;

    case AL_EAXREVERB_DIFFUSION:
        if(!(val >= AL_EAXREVERB_MIN_DIFFUSION && val <= AL_EAXREVERB_MAX_DIFFUSION))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb diffusion out of range"};
        props->Reverb.Diffusion = val;
        break;

    case AL_EAXREVERB_GAIN:
        if(!(val >= AL_EAXREVERB_MIN_GAIN && val <= AL_EAXREVERB_MAX_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb gain out of range"};
        props->Reverb.Gain = val;
        break;

    case AL_EAXREVERB_GAINHF:
        if(!(val >= AL_EAXREVERB_MIN_GAINHF && val <= AL_EAXREVERB_MAX_GAINHF))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb gainhf out of range"};
        props->Reverb.GainHF = val;
        break;

    case AL_EAXREVERB_GAINLF:
        if(!(val >= AL_EAXREVERB_MIN_GAINLF && val <= AL_EAXREVERB_MAX_GAINLF))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb gainlf out of range"};
        props->Reverb.GainLF = val;
        break;

    case AL_EAXREVERB_DECAY_TIME:
        if(!(val >= AL_EAXREVERB_MIN_DECAY_TIME && val <= AL_EAXREVERB_MAX_DECAY_TIME))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb decay time out of range"};
        props->Reverb.DecayTime = val;
        break;

    case AL_EAXREVERB_DECAY_HFRATIO:
        if(!(val >= AL_EAXREVERB_MIN_DECAY_HFRATIO && val <= AL_EAXREVERB_MAX_DECAY_HFRATIO))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb decay hfratio out of range"};
        props->Reverb.DecayHFRatio = val;
        break;

    case AL_EAXREVERB_DECAY_LFRATIO:
        if(!(val >= AL_EAXREVERB_MIN_DECAY_LFRATIO && val <= AL_EAXREVERB_MAX_DECAY_LFRATIO))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb decay lfratio out of range"};
        props->Reverb.DecayLFRatio = val;
        break;

    case AL_EAXREVERB_REFLECTIONS_GAIN:
        if(!(val >= AL_EAXREVERB_MIN_REFLECTIONS_GAIN && val <= AL_EAXREVERB_MAX_REFLECTIONS_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb reflections gain out of range"};
        props->Reverb.ReflectionsGain = val;
        break;

    case AL_EAXREVERB_REFLECTIONS_DELAY:
        if(!(val >= AL_EAXREVERB_MIN_REFLECTIONS_DELAY && val <= AL_EAXREVERB_MAX_REFLECTIONS_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb reflections delay out of range"};
        props->Reverb.ReflectionsDelay = val;
        break;

    case AL_EAXREVERB_LATE_REVERB_GAIN:
        if(!(val >= AL_EAXREVERB_MIN_LATE_REVERB_GAIN && val <= AL_EAXREVERB_MAX_LATE_REVERB_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb late reverb gain out of range"};
        props->Reverb.LateReverbGain = val;
        break;

    case AL_EAXREVERB_LATE_REVERB_DELAY:
        if(!(val >= AL_EAXREVERB_MIN_LATE_REVERB_DELAY && val <= AL_EAXREVERB_MAX_LATE_REVERB_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb late reverb delay out of range"};
        props->Reverb.LateReverbDelay = val;
        break;

    case AL_EAXREVERB_AIR_ABSORPTION_GAINHF:
        if(!(val >= AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF && val <= AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb air absorption gainhf out of range"};
        props->Reverb.AirAbsorptionGainHF = val;
        break;

    case AL_EAXREVERB_ECHO_TIME:
        if(!(val >= AL_EAXREVERB_MIN_ECHO_TIME && val <= AL_EAXREVERB_MAX_ECHO_TIME))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb echo time out of range"};
        props->Reverb.EchoTime = val;
        break;

    case AL_EAXREVERB_ECHO_DEPTH:
        if(!(val >= AL_EAXREVERB_MIN_ECHO_DEPTH && val <= AL_EAXREVERB_MAX_ECHO_DEPTH))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb echo depth out of range"};
        props->Reverb.EchoDepth = val;
        break;

    case AL_EAXREVERB_MODULATION_TIME:
        if(!(val >= AL_EAXREVERB_MIN_MODULATION_TIME && val <= AL_EAXREVERB_MAX_MODULATION_TIME))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb modulation time out of range"};
        props->Reverb.ModulationTime = val;
        break;

    case AL_EAXREVERB_MODULATION_DEPTH:
        if(!(val >= AL_EAXREVERB_MIN_MODULATION_DEPTH && val <= AL_EAXREVERB_MAX_MODULATION_DEPTH))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb modulation depth out of range"};
        props->Reverb.ModulationDepth = val;
        break;

    case AL_EAXREVERB_HFREFERENCE:
        if(!(val >= AL_EAXREVERB_MIN_HFREFERENCE && val <= AL_EAXREVERB_MAX_HFREFERENCE))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb hfreference out of range"};
        props->Reverb.HFReference = val;
        break;

    case AL_EAXREVERB_LFREFERENCE:
        if(!(val >= AL_EAXREVERB_MIN_LFREFERENCE && val <= AL_EAXREVERB_MAX_LFREFERENCE))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb lfreference out of range"};
        props->Reverb.LFReference = val;
        break;

    case AL_EAXREVERB_ROOM_ROLLOFF_FACTOR:
        if(!(val >= AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR && val <= AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb room rolloff factor out of range"};
        props->Reverb.RoomRolloffFactor = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid EAX reverb float property 0x%04x", param};
    }
}
void Reverb_setParamfv(EffectProps *props, ALenum param, const float *vals)
{
    switch(param)
    {
    case AL_EAXREVERB_REFLECTIONS_PAN:
        if(!(std::isfinite(vals[0]) && std::isfinite(vals[1]) && std::isfinite(vals[2])))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb reflections pan out of range"};
        props->Reverb.ReflectionsPan[0] = vals[0];
        props->Reverb.ReflectionsPan[1] = vals[1];
        props->Reverb.ReflectionsPan[2] = vals[2];
        break;
    case AL_EAXREVERB_LATE_REVERB_PAN:
        if(!(std::isfinite(vals[0]) && std::isfinite(vals[1]) && std::isfinite(vals[2])))
            throw effect_exception{AL_INVALID_VALUE, "EAX Reverb late reverb pan out of range"};
        props->Reverb.LateReverbPan[0] = vals[0];
        props->Reverb.LateReverbPan[1] = vals[1];
        props->Reverb.LateReverbPan[2] = vals[2];
        break;

    default:
        Reverb_setParamf(props, param, vals[0]);
        break;
    }
}

void Reverb_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_EAXREVERB_DECAY_HFLIMIT:
        *val = props->Reverb.DecayHFLimit;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid EAX reverb integer property 0x%04x",
            param};
    }
}
void Reverb_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Reverb_getParami(props, param, vals); }
void Reverb_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_EAXREVERB_DENSITY:
        *val = props->Reverb.Density;
        break;

    case AL_EAXREVERB_DIFFUSION:
        *val = props->Reverb.Diffusion;
        break;

    case AL_EAXREVERB_GAIN:
        *val = props->Reverb.Gain;
        break;

    case AL_EAXREVERB_GAINHF:
        *val = props->Reverb.GainHF;
        break;

    case AL_EAXREVERB_GAINLF:
        *val = props->Reverb.GainLF;
        break;

    case AL_EAXREVERB_DECAY_TIME:
        *val = props->Reverb.DecayTime;
        break;

    case AL_EAXREVERB_DECAY_HFRATIO:
        *val = props->Reverb.DecayHFRatio;
        break;

    case AL_EAXREVERB_DECAY_LFRATIO:
        *val = props->Reverb.DecayLFRatio;
        break;

    case AL_EAXREVERB_REFLECTIONS_GAIN:
        *val = props->Reverb.ReflectionsGain;
        break;

    case AL_EAXREVERB_REFLECTIONS_DELAY:
        *val = props->Reverb.ReflectionsDelay;
        break;

    case AL_EAXREVERB_LATE_REVERB_GAIN:
        *val = props->Reverb.LateReverbGain;
        break;

    case AL_EAXREVERB_LATE_REVERB_DELAY:
        *val = props->Reverb.LateReverbDelay;
        break;

    case AL_EAXREVERB_AIR_ABSORPTION_GAINHF:
        *val = props->Reverb.AirAbsorptionGainHF;
        break;

    case AL_EAXREVERB_ECHO_TIME:
        *val = props->Reverb.EchoTime;
        break;

    case AL_EAXREVERB_ECHO_DEPTH:
        *val = props->Reverb.EchoDepth;
        break;

    case AL_EAXREVERB_MODULATION_TIME:
        *val = props->Reverb.ModulationTime;
        break;

    case AL_EAXREVERB_MODULATION_DEPTH:
        *val = props->Reverb.ModulationDepth;
        break;

    case AL_EAXREVERB_HFREFERENCE:
        *val = props->Reverb.HFReference;
        break;

    case AL_EAXREVERB_LFREFERENCE:
        *val = props->Reverb.LFReference;
        break;

    case AL_EAXREVERB_ROOM_ROLLOFF_FACTOR:
        *val = props->Reverb.RoomRolloffFactor;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid EAX reverb float property 0x%04x", param};
    }
}
void Reverb_getParamfv(const EffectProps *props, ALenum param, float *vals)
{
    switch(param)
    {
    case AL_EAXREVERB_REFLECTIONS_PAN:
        vals[0] = props->Reverb.ReflectionsPan[0];
        vals[1] = props->Reverb.ReflectionsPan[1];
        vals[2] = props->Reverb.ReflectionsPan[2];
        break;
    case AL_EAXREVERB_LATE_REVERB_PAN:
        vals[0] = props->Reverb.LateReverbPan[0];
        vals[1] = props->Reverb.LateReverbPan[1];
        vals[2] = props->Reverb.LateReverbPan[2];
        break;

    default:
        Reverb_getParamf(props, param, vals);
        break;
    }
}

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Reverb.Density   = AL_EAXREVERB_DEFAULT_DENSITY;
    props.Reverb.Diffusion = AL_EAXREVERB_DEFAULT_DIFFUSION;
    props.Reverb.Gain   = AL_EAXREVERB_DEFAULT_GAIN;
    props.Reverb.GainHF = AL_EAXREVERB_DEFAULT_GAINHF;
    props.Reverb.GainLF = AL_EAXREVERB_DEFAULT_GAINLF;
    props.Reverb.DecayTime    = AL_EAXREVERB_DEFAULT_DECAY_TIME;
    props.Reverb.DecayHFRatio = AL_EAXREVERB_DEFAULT_DECAY_HFRATIO;
    props.Reverb.DecayLFRatio = AL_EAXREVERB_DEFAULT_DECAY_LFRATIO;
    props.Reverb.ReflectionsGain   = AL_EAXREVERB_DEFAULT_REFLECTIONS_GAIN;
    props.Reverb.ReflectionsDelay  = AL_EAXREVERB_DEFAULT_REFLECTIONS_DELAY;
    props.Reverb.ReflectionsPan[0] = AL_EAXREVERB_DEFAULT_REFLECTIONS_PAN_XYZ;
    props.Reverb.ReflectionsPan[1] = AL_EAXREVERB_DEFAULT_REFLECTIONS_PAN_XYZ;
    props.Reverb.ReflectionsPan[2] = AL_EAXREVERB_DEFAULT_REFLECTIONS_PAN_XYZ;
    props.Reverb.LateReverbGain   = AL_EAXREVERB_DEFAULT_LATE_REVERB_GAIN;
    props.Reverb.LateReverbDelay  = AL_EAXREVERB_DEFAULT_LATE_REVERB_DELAY;
    props.Reverb.LateReverbPan[0] = AL_EAXREVERB_DEFAULT_LATE_REVERB_PAN_XYZ;
    props.Reverb.LateReverbPan[1] = AL_EAXREVERB_DEFAULT_LATE_REVERB_PAN_XYZ;
    props.Reverb.LateReverbPan[2] = AL_EAXREVERB_DEFAULT_LATE_REVERB_PAN_XYZ;
    props.Reverb.EchoTime  = AL_EAXREVERB_DEFAULT_ECHO_TIME;
    props.Reverb.EchoDepth = AL_EAXREVERB_DEFAULT_ECHO_DEPTH;
    props.Reverb.ModulationTime  = AL_EAXREVERB_DEFAULT_MODULATION_TIME;
    props.Reverb.ModulationDepth = AL_EAXREVERB_DEFAULT_MODULATION_DEPTH;
    props.Reverb.AirAbsorptionGainHF = AL_EAXREVERB_DEFAULT_AIR_ABSORPTION_GAINHF;
    props.Reverb.HFReference = AL_EAXREVERB_DEFAULT_HFREFERENCE;
    props.Reverb.LFReference = AL_EAXREVERB_DEFAULT_LFREFERENCE;
    props.Reverb.RoomRolloffFactor = AL_EAXREVERB_DEFAULT_ROOM_ROLLOFF_FACTOR;
    props.Reverb.DecayHFLimit = AL_EAXREVERB_DEFAULT_DECAY_HFLIMIT;
    return props;
}


void StdReverb_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_REVERB_DECAY_HFLIMIT:
        if(!(val >= AL_REVERB_MIN_DECAY_HFLIMIT && val <= AL_REVERB_MAX_DECAY_HFLIMIT))
            throw effect_exception{AL_INVALID_VALUE, "Reverb decay hflimit out of range"};
        props->Reverb.DecayHFLimit = val != AL_FALSE;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid reverb integer property 0x%04x", param};
    }
}
void StdReverb_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ StdReverb_setParami(props, param, vals[0]); }
void StdReverb_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_REVERB_DENSITY:
        if(!(val >= AL_REVERB_MIN_DENSITY && val <= AL_REVERB_MAX_DENSITY))
            throw effect_exception{AL_INVALID_VALUE, "Reverb density out of range"};
        props->Reverb.Density = val;
        break;

    case AL_REVERB_DIFFUSION:
        if(!(val >= AL_REVERB_MIN_DIFFUSION && val <= AL_REVERB_MAX_DIFFUSION))
            throw effect_exception{AL_INVALID_VALUE, "Reverb diffusion out of range"};
        props->Reverb.Diffusion = val;
        break;

    case AL_REVERB_GAIN:
        if(!(val >= AL_REVERB_MIN_GAIN && val <= AL_REVERB_MAX_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Reverb gain out of range"};
        props->Reverb.Gain = val;
        break;

    case AL_REVERB_GAINHF:
        if(!(val >= AL_REVERB_MIN_GAINHF && val <= AL_REVERB_MAX_GAINHF))
            throw effect_exception{AL_INVALID_VALUE, "Reverb gainhf out of range"};
        props->Reverb.GainHF = val;
        break;

    case AL_REVERB_DECAY_TIME:
        if(!(val >= AL_REVERB_MIN_DECAY_TIME && val <= AL_REVERB_MAX_DECAY_TIME))
            throw effect_exception{AL_INVALID_VALUE, "Reverb decay time out of range"};
        props->Reverb.DecayTime = val;
        break;

    case AL_REVERB_DECAY_HFRATIO:
        if(!(val >= AL_REVERB_MIN_DECAY_HFRATIO && val <= AL_REVERB_MAX_DECAY_HFRATIO))
            throw effect_exception{AL_INVALID_VALUE, "Reverb decay hfratio out of range"};
        props->Reverb.DecayHFRatio = val;
        break;

    case AL_REVERB_REFLECTIONS_GAIN:
        if(!(val >= AL_REVERB_MIN_REFLECTIONS_GAIN && val <= AL_REVERB_MAX_REFLECTIONS_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Reverb reflections gain out of range"};
        props->Reverb.ReflectionsGain = val;
        break;

    case AL_REVERB_REFLECTIONS_DELAY:
        if(!(val >= AL_REVERB_MIN_REFLECTIONS_DELAY && val <= AL_REVERB_MAX_REFLECTIONS_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "Reverb reflections delay out of range"};
        props->Reverb.ReflectionsDelay = val;
        break;

    case AL_REVERB_LATE_REVERB_GAIN:
        if(!(val >= AL_REVERB_MIN_LATE_REVERB_GAIN && val <= AL_REVERB_MAX_LATE_REVERB_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Reverb late reverb gain out of range"};
        props->Reverb.LateReverbGain = val;
        break;

    case AL_REVERB_LATE_REVERB_DELAY:
        if(!(val >= AL_REVERB_MIN_LATE_REVERB_DELAY && val <= AL_REVERB_MAX_LATE_REVERB_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "Reverb late reverb delay out of range"};
        props->Reverb.LateReverbDelay = val;
        break;

    case AL_REVERB_AIR_ABSORPTION_GAINHF:
        if(!(val >= AL_REVERB_MIN_AIR_ABSORPTION_GAINHF && val <= AL_REVERB_MAX_AIR_ABSORPTION_GAINHF))
            throw effect_exception{AL_INVALID_VALUE, "Reverb air absorption gainhf out of range"};
        props->Reverb.AirAbsorptionGainHF = val;
        break;

    case AL_REVERB_ROOM_ROLLOFF_FACTOR:
        if(!(val >= AL_REVERB_MIN_ROOM_ROLLOFF_FACTOR && val <= AL_REVERB_MAX_ROOM_ROLLOFF_FACTOR))
            throw effect_exception{AL_INVALID_VALUE, "Reverb room rolloff factor out of range"};
        props->Reverb.RoomRolloffFactor = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid reverb float property 0x%04x", param};
    }
}
void StdReverb_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ StdReverb_setParamf(props, param, vals[0]); }

void StdReverb_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_REVERB_DECAY_HFLIMIT:
        *val = props->Reverb.DecayHFLimit;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid reverb integer property 0x%04x", param};
    }
}
void StdReverb_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ StdReverb_getParami(props, param, vals); }
void StdReverb_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_REVERB_DENSITY:
        *val = props->Reverb.Density;
        break;

    case AL_REVERB_DIFFUSION:
        *val = props->Reverb.Diffusion;
        break;

    case AL_REVERB_GAIN:
        *val = props->Reverb.Gain;
        break;

    case AL_REVERB_GAINHF:
        *val = props->Reverb.GainHF;
        break;

    case AL_REVERB_DECAY_TIME:
        *val = props->Reverb.DecayTime;
        break;

    case AL_REVERB_DECAY_HFRATIO:
        *val = props->Reverb.DecayHFRatio;
        break;

    case AL_REVERB_REFLECTIONS_GAIN:
        *val = props->Reverb.ReflectionsGain;
        break;

    case AL_REVERB_REFLECTIONS_DELAY:
        *val = props->Reverb.ReflectionsDelay;
        break;

    case AL_REVERB_LATE_REVERB_GAIN:
        *val = props->Reverb.LateReverbGain;
        break;

    case AL_REVERB_LATE_REVERB_DELAY:
        *val = props->Reverb.LateReverbDelay;
        break;

    case AL_REVERB_AIR_ABSORPTION_GAINHF:
        *val = props->Reverb.AirAbsorptionGainHF;
        break;

    case AL_REVERB_ROOM_ROLLOFF_FACTOR:
        *val = props->Reverb.RoomRolloffFactor;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid reverb float property 0x%04x", param};
    }
}
void StdReverb_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ StdReverb_getParamf(props, param, vals); }

EffectProps genDefaultStdProps() noexcept
{
    EffectProps props{};
    props.Reverb.Density   = AL_REVERB_DEFAULT_DENSITY;
    props.Reverb.Diffusion = AL_REVERB_DEFAULT_DIFFUSION;
    props.Reverb.Gain   = AL_REVERB_DEFAULT_GAIN;
    props.Reverb.GainHF = AL_REVERB_DEFAULT_GAINHF;
    props.Reverb.GainLF = 1.0f;
    props.Reverb.DecayTime    = AL_REVERB_DEFAULT_DECAY_TIME;
    props.Reverb.DecayHFRatio = AL_REVERB_DEFAULT_DECAY_HFRATIO;
    props.Reverb.DecayLFRatio = 1.0f;
    props.Reverb.ReflectionsGain   = AL_REVERB_DEFAULT_REFLECTIONS_GAIN;
    props.Reverb.ReflectionsDelay  = AL_REVERB_DEFAULT_REFLECTIONS_DELAY;
    props.Reverb.ReflectionsPan[0] = 0.0f;
    props.Reverb.ReflectionsPan[1] = 0.0f;
    props.Reverb.ReflectionsPan[2] = 0.0f;
    props.Reverb.LateReverbGain   = AL_REVERB_DEFAULT_LATE_REVERB_GAIN;
    props.Reverb.LateReverbDelay  = AL_REVERB_DEFAULT_LATE_REVERB_DELAY;
    props.Reverb.LateReverbPan[0] = 0.0f;
    props.Reverb.LateReverbPan[1] = 0.0f;
    props.Reverb.LateReverbPan[2] = 0.0f;
    props.Reverb.EchoTime  = 0.25f;
    props.Reverb.EchoDepth = 0.0f;
    props.Reverb.ModulationTime  = 0.25f;
    props.Reverb.ModulationDepth = 0.0f;
    props.Reverb.AirAbsorptionGainHF = AL_REVERB_DEFAULT_AIR_ABSORPTION_GAINHF;
    props.Reverb.HFReference = 5000.0f;
    props.Reverb.LFReference = 250.0f;
    props.Reverb.RoomRolloffFactor = AL_REVERB_DEFAULT_ROOM_ROLLOFF_FACTOR;
    props.Reverb.DecayHFLimit = AL_REVERB_DEFAULT_DECAY_HFLIMIT;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Reverb);

const EffectProps ReverbEffectProps{genDefaultProps()};

DEFINE_ALEFFECT_VTABLE(StdReverb);

const EffectProps StdReverbEffectProps{genDefaultStdProps()};

#ifdef ALSOFT_EAX
namespace {

extern const EFXEAXREVERBPROPERTIES eax_efx_reverb_presets[];

using EaxReverbEffectDirtyFlagsValue = std::uint_least32_t;

struct EaxReverbEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxReverbEffectDirtyFlagsValue ulEnvironment : 1;
    EaxReverbEffectDirtyFlagsValue flEnvironmentSize : 1;
    EaxReverbEffectDirtyFlagsValue flEnvironmentDiffusion : 1;
    EaxReverbEffectDirtyFlagsValue lRoom : 1;
    EaxReverbEffectDirtyFlagsValue lRoomHF : 1;
    EaxReverbEffectDirtyFlagsValue lRoomLF : 1;
    EaxReverbEffectDirtyFlagsValue flDecayTime : 1;
    EaxReverbEffectDirtyFlagsValue flDecayHFRatio : 1;
    EaxReverbEffectDirtyFlagsValue flDecayLFRatio : 1;
    EaxReverbEffectDirtyFlagsValue lReflections : 1;
    EaxReverbEffectDirtyFlagsValue flReflectionsDelay : 1;
    EaxReverbEffectDirtyFlagsValue vReflectionsPan : 1;
    EaxReverbEffectDirtyFlagsValue lReverb : 1;
    EaxReverbEffectDirtyFlagsValue flReverbDelay : 1;
    EaxReverbEffectDirtyFlagsValue vReverbPan : 1;
    EaxReverbEffectDirtyFlagsValue flEchoTime : 1;
    EaxReverbEffectDirtyFlagsValue flEchoDepth : 1;
    EaxReverbEffectDirtyFlagsValue flModulationTime : 1;
    EaxReverbEffectDirtyFlagsValue flModulationDepth : 1;
    EaxReverbEffectDirtyFlagsValue flAirAbsorptionHF : 1;
    EaxReverbEffectDirtyFlagsValue flHFReference : 1;
    EaxReverbEffectDirtyFlagsValue flLFReference : 1;
    EaxReverbEffectDirtyFlagsValue flRoomRolloffFactor : 1;
    EaxReverbEffectDirtyFlagsValue ulFlags : 1;
}; // EaxReverbEffectDirtyFlags

struct Eax1ReverbEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxReverbEffectDirtyFlagsValue ulEnvironment : 1;
    EaxReverbEffectDirtyFlagsValue flVolume : 1;
    EaxReverbEffectDirtyFlagsValue flDecayTime : 1;
    EaxReverbEffectDirtyFlagsValue flDamping : 1;
}; // Eax1ReverbEffectDirtyFlags

class EaxReverbEffect final :
    public EaxEffect
{
public:
    EaxReverbEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAX_REVERBPROPERTIES eax1_{};
    EAX_REVERBPROPERTIES eax1_d_{};
    Eax1ReverbEffectDirtyFlags eax1_dirty_flags_{};
    EAXREVERBPROPERTIES eax_{};
    EAXREVERBPROPERTIES eax_d_{};
    EaxReverbEffectDirtyFlags eax_dirty_flags_{};

    [[noreturn]] static void eax_fail(const char* message);

    void set_eax_defaults();

    void set_efx_density_from_environment_size();
    void set_efx_diffusion();
    void set_efx_gain();
    void set_efx_gain_hf();
    void set_efx_gain_lf();
    void set_efx_decay_time();
    void set_efx_decay_hf_ratio();
    void set_efx_decay_lf_ratio();
    void set_efx_reflections_gain();
    void set_efx_reflections_delay();
    void set_efx_reflections_pan();
    void set_efx_late_reverb_gain();
    void set_efx_late_reverb_delay();
    void set_efx_late_reverb_pan();
    void set_efx_echo_time();
    void set_efx_echo_depth();
    void set_efx_modulation_time();
    void set_efx_modulation_depth();
    void set_efx_air_absorption_gain_hf();
    void set_efx_hf_reference();
    void set_efx_lf_reference();
    void set_efx_room_rolloff_factor();
    void set_efx_flags();
    void set_efx_defaults();

    void v1_get(const EaxEaxCall& eax_call) const;

    void get_all(const EaxEaxCall& eax_call) const;

    void get(const EaxEaxCall& eax_call) const;

    static void v1_validate_environment(unsigned long environment);
    static void v1_validate_volume(float volume);
    static void v1_validate_decay_time(float decay_time);
    static void v1_validate_damping(float damping);
    static void v1_validate_all(const EAX_REVERBPROPERTIES& all);

    void v1_defer_environment(unsigned long environment);
    void v1_defer_volume(float volume);
    void v1_defer_decay_time(float decay_time);
    void v1_defer_damping(float damping);
    void v1_defer_all(const EAX_REVERBPROPERTIES& all);

    void v1_defer_environment(const EaxEaxCall& eax_call);
    void v1_defer_volume(const EaxEaxCall& eax_call);
    void v1_defer_decay_time(const EaxEaxCall& eax_call);
    void v1_defer_damping(const EaxEaxCall& eax_call);
    void v1_defer_all(const EaxEaxCall& eax_call);
    void v1_defer(const EaxEaxCall& eax_call);

    void v1_set_efx();

    static void validate_environment(unsigned long ulEnvironment, int version, bool is_standalone);
    static void validate_environment_size(float flEnvironmentSize);
    static void validate_environment_diffusion(float flEnvironmentDiffusion);
    static void validate_room(long lRoom);
    static void validate_room_hf(long lRoomHF);
    static void validate_room_lf(long lRoomLF);
    static void validate_decay_time(float flDecayTime);
    static void validate_decay_hf_ratio(float flDecayHFRatio);
    static void validate_decay_lf_ratio(float flDecayLFRatio);
    static void validate_reflections(long lReflections);
    static void validate_reflections_delay(float flReflectionsDelay);
    static void validate_reflections_pan(const EAXVECTOR& vReflectionsPan);
    static void validate_reverb(long lReverb);
    static void validate_reverb_delay(float flReverbDelay);
    static void validate_reverb_pan(const EAXVECTOR& vReverbPan);
    static void validate_echo_time(float flEchoTime);
    static void validate_echo_depth(float flEchoDepth);
    static void validate_modulation_time(float flModulationTime);
    static void validate_modulation_depth(float flModulationDepth);
    static void validate_air_absorbtion_hf(float air_absorbtion_hf);
    static void validate_hf_reference(float flHFReference);
    static void validate_lf_reference(float flLFReference);
    static void validate_room_rolloff_factor(float flRoomRolloffFactor);
    static void validate_flags(unsigned long ulFlags);
    static void validate_all(const EAX20LISTENERPROPERTIES& all, int version);
    static void validate_all(const EAXREVERBPROPERTIES& all, int version);

    void defer_environment(unsigned long ulEnvironment);
    void defer_environment_size(float flEnvironmentSize);
    void defer_environment_diffusion(float flEnvironmentDiffusion);
    void defer_room(long lRoom);
    void defer_room_hf(long lRoomHF);
    void defer_room_lf(long lRoomLF);
    void defer_decay_time(float flDecayTime);
    void defer_decay_hf_ratio(float flDecayHFRatio);
    void defer_decay_lf_ratio(float flDecayLFRatio);
    void defer_reflections(long lReflections);
    void defer_reflections_delay(float flReflectionsDelay);
    void defer_reflections_pan(const EAXVECTOR& vReflectionsPan);
    void defer_reverb(long lReverb);
    void defer_reverb_delay(float flReverbDelay);
    void defer_reverb_pan(const EAXVECTOR& vReverbPan);
    void defer_echo_time(float flEchoTime);
    void defer_echo_depth(float flEchoDepth);
    void defer_modulation_time(float flModulationTime);
    void defer_modulation_depth(float flModulationDepth);
    void defer_air_absorbtion_hf(float flAirAbsorptionHF);
    void defer_hf_reference(float flHFReference);
    void defer_lf_reference(float flLFReference);
    void defer_room_rolloff_factor(float flRoomRolloffFactor);
    void defer_flags(unsigned long ulFlags);
    void defer_all(const EAX20LISTENERPROPERTIES& all);
    void defer_all(const EAXREVERBPROPERTIES& all);

    void defer_environment(const EaxEaxCall& eax_call);
    void defer_environment_size(const EaxEaxCall& eax_call);
    void defer_environment_diffusion(const EaxEaxCall& eax_call);
    void defer_room(const EaxEaxCall& eax_call);
    void defer_room_hf(const EaxEaxCall& eax_call);
    void defer_room_lf(const EaxEaxCall& eax_call);
    void defer_decay_time(const EaxEaxCall& eax_call);
    void defer_decay_hf_ratio(const EaxEaxCall& eax_call);
    void defer_decay_lf_ratio(const EaxEaxCall& eax_call);
    void defer_reflections(const EaxEaxCall& eax_call);
    void defer_reflections_delay(const EaxEaxCall& eax_call);
    void defer_reflections_pan(const EaxEaxCall& eax_call);
    void defer_reverb(const EaxEaxCall& eax_call);
    void defer_reverb_delay(const EaxEaxCall& eax_call);
    void defer_reverb_pan(const EaxEaxCall& eax_call);
    void defer_echo_time(const EaxEaxCall& eax_call);
    void defer_echo_depth(const EaxEaxCall& eax_call);
    void defer_modulation_time(const EaxEaxCall& eax_call);
    void defer_modulation_depth(const EaxEaxCall& eax_call);
    void defer_air_absorbtion_hf(const EaxEaxCall& eax_call);
    void defer_hf_reference(const EaxEaxCall& eax_call);
    void defer_lf_reference(const EaxEaxCall& eax_call);
    void defer_room_rolloff_factor(const EaxEaxCall& eax_call);
    void defer_flags(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxReverbEffect


class EaxReverbEffectException :
    public EaxException
{
public:
    explicit EaxReverbEffectException(
        const char* message)
        :
        EaxException{"EAX_REVERB_EFFECT", message}
    {
    }
}; // EaxReverbEffectException


EaxReverbEffect::EaxReverbEffect()
    : EaxEffect{AL_EFFECT_EAXREVERB}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxReverbEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

[[noreturn]] void EaxReverbEffect::eax_fail(const char* message)
{
    throw EaxReverbEffectException{message};
}

void EaxReverbEffect::set_eax_defaults()
{
    eax1_ = EAX1REVERB_PRESETS[EAX_ENVIRONMENT_GENERIC];
    eax1_d_ = eax1_;
    eax_ = EAXREVERB_PRESETS[EAX_ENVIRONMENT_GENERIC];
    /* HACK: EAX2 has a default room volume of -10,000dB (silence), although
     * newer versions use -1,000dB. What should be happening is properties for
     * each EAX version is tracked separately, with the last version used for
     * the properties to apply (presumably v2 or nothing being the default).
     */
    eax_.lRoom = EAXREVERB_MINROOM;
    eax_d_ = eax_;
}

void EaxReverbEffect::set_efx_density_from_environment_size()
{
    const auto eax_environment_size = eax_.flEnvironmentSize;

    const auto efx_density = clamp(
        (eax_environment_size * eax_environment_size * eax_environment_size) / 16.0F,
        AL_EAXREVERB_MIN_DENSITY,
        AL_EAXREVERB_MAX_DENSITY);

    al_effect_props_.Reverb.Density = efx_density;
}

void EaxReverbEffect::set_efx_diffusion()
{
    const auto efx_diffusion = clamp(
        eax_.flEnvironmentDiffusion,
        AL_EAXREVERB_MIN_DIFFUSION,
        AL_EAXREVERB_MAX_DIFFUSION);

    al_effect_props_.Reverb.Diffusion = efx_diffusion;
}

void EaxReverbEffect::set_efx_gain()
{
    const auto efx_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lRoom)),
        AL_EAXREVERB_MIN_GAIN,
        AL_EAXREVERB_MAX_GAIN);

    al_effect_props_.Reverb.Gain = efx_gain;
}

void EaxReverbEffect::set_efx_gain_hf()
{
    const auto efx_gain_hf = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lRoomHF)),
        AL_EAXREVERB_MIN_GAINHF,
        AL_EAXREVERB_MAX_GAINHF);

    al_effect_props_.Reverb.GainHF = efx_gain_hf;
}

void EaxReverbEffect::set_efx_gain_lf()
{
    const auto efx_gain_lf = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lRoomLF)),
        AL_EAXREVERB_MIN_GAINLF,
        AL_EAXREVERB_MAX_GAINLF);

    al_effect_props_.Reverb.GainLF = efx_gain_lf;
}

void EaxReverbEffect::set_efx_decay_time()
{
    const auto efx_decay_time = clamp(
        eax_.flDecayTime,
        AL_EAXREVERB_MIN_DECAY_TIME,
        AL_EAXREVERB_MAX_DECAY_TIME);

    al_effect_props_.Reverb.DecayTime = efx_decay_time;
}

void EaxReverbEffect::set_efx_decay_hf_ratio()
{
    const auto efx_decay_hf_ratio = clamp(
        eax_.flDecayHFRatio,
        AL_EAXREVERB_MIN_DECAY_HFRATIO,
        AL_EAXREVERB_MAX_DECAY_HFRATIO);

    al_effect_props_.Reverb.DecayHFRatio = efx_decay_hf_ratio;
}

void EaxReverbEffect::set_efx_decay_lf_ratio()
{
    const auto efx_decay_lf_ratio = clamp(
        eax_.flDecayLFRatio,
        AL_EAXREVERB_MIN_DECAY_LFRATIO,
        AL_EAXREVERB_MAX_DECAY_LFRATIO);

    al_effect_props_.Reverb.DecayLFRatio = efx_decay_lf_ratio;
}

void EaxReverbEffect::set_efx_reflections_gain()
{
    const auto efx_reflections_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lReflections)),
        AL_EAXREVERB_MIN_REFLECTIONS_GAIN,
        AL_EAXREVERB_MAX_REFLECTIONS_GAIN);

    al_effect_props_.Reverb.ReflectionsGain = efx_reflections_gain;
}

void EaxReverbEffect::set_efx_reflections_delay()
{
    const auto efx_reflections_delay = clamp(
        eax_.flReflectionsDelay,
        AL_EAXREVERB_MIN_REFLECTIONS_DELAY,
        AL_EAXREVERB_MAX_REFLECTIONS_DELAY);

    al_effect_props_.Reverb.ReflectionsDelay = efx_reflections_delay;
}

void EaxReverbEffect::set_efx_reflections_pan()
{
    al_effect_props_.Reverb.ReflectionsPan[0] = eax_.vReflectionsPan.x;
    al_effect_props_.Reverb.ReflectionsPan[1] = eax_.vReflectionsPan.y;
    al_effect_props_.Reverb.ReflectionsPan[2] = eax_.vReflectionsPan.z;
}

void EaxReverbEffect::set_efx_late_reverb_gain()
{
    const auto efx_late_reverb_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lReverb)),
        AL_EAXREVERB_MIN_LATE_REVERB_GAIN,
        AL_EAXREVERB_MAX_LATE_REVERB_GAIN);

    al_effect_props_.Reverb.LateReverbGain = efx_late_reverb_gain;
}

void EaxReverbEffect::set_efx_late_reverb_delay()
{
    const auto efx_late_reverb_delay = clamp(
        eax_.flReverbDelay,
        AL_EAXREVERB_MIN_LATE_REVERB_DELAY,
        AL_EAXREVERB_MAX_LATE_REVERB_DELAY);

    al_effect_props_.Reverb.LateReverbDelay = efx_late_reverb_delay;
}

void EaxReverbEffect::set_efx_late_reverb_pan()
{
    al_effect_props_.Reverb.LateReverbPan[0] = eax_.vReverbPan.x;
    al_effect_props_.Reverb.LateReverbPan[1] = eax_.vReverbPan.y;
    al_effect_props_.Reverb.LateReverbPan[2] = eax_.vReverbPan.z;
}

void EaxReverbEffect::set_efx_echo_time()
{
    const auto efx_echo_time = clamp(
        eax_.flEchoTime,
        AL_EAXREVERB_MIN_ECHO_TIME,
        AL_EAXREVERB_MAX_ECHO_TIME);

    al_effect_props_.Reverb.EchoTime = efx_echo_time;
}

void EaxReverbEffect::set_efx_echo_depth()
{
    const auto efx_echo_depth = clamp(
        eax_.flEchoDepth,
        AL_EAXREVERB_MIN_ECHO_DEPTH,
        AL_EAXREVERB_MAX_ECHO_DEPTH);

    al_effect_props_.Reverb.EchoDepth = efx_echo_depth;
}

void EaxReverbEffect::set_efx_modulation_time()
{
    const auto efx_modulation_time = clamp(
        eax_.flModulationTime,
        AL_EAXREVERB_MIN_MODULATION_TIME,
        AL_EAXREVERB_MAX_MODULATION_TIME);

    al_effect_props_.Reverb.ModulationTime = efx_modulation_time;
}

void EaxReverbEffect::set_efx_modulation_depth()
{
    const auto efx_modulation_depth = clamp(
        eax_.flModulationDepth,
        AL_EAXREVERB_MIN_MODULATION_DEPTH,
        AL_EAXREVERB_MAX_MODULATION_DEPTH);

    al_effect_props_.Reverb.ModulationDepth = efx_modulation_depth;
}

void EaxReverbEffect::set_efx_air_absorption_gain_hf()
{
    const auto efx_air_absorption_hf = clamp(
        level_mb_to_gain(eax_.flAirAbsorptionHF),
        AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF,
        AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF);

    al_effect_props_.Reverb.AirAbsorptionGainHF = efx_air_absorption_hf;
}

void EaxReverbEffect::set_efx_hf_reference()
{
    const auto efx_hf_reference = clamp(
        eax_.flHFReference,
        AL_EAXREVERB_MIN_HFREFERENCE,
        AL_EAXREVERB_MAX_HFREFERENCE);

    al_effect_props_.Reverb.HFReference = efx_hf_reference;
}

void EaxReverbEffect::set_efx_lf_reference()
{
    const auto efx_lf_reference = clamp(
        eax_.flLFReference,
        AL_EAXREVERB_MIN_LFREFERENCE,
        AL_EAXREVERB_MAX_LFREFERENCE);

    al_effect_props_.Reverb.LFReference = efx_lf_reference;
}

void EaxReverbEffect::set_efx_room_rolloff_factor()
{
    const auto efx_room_rolloff_factor = clamp(
        eax_.flRoomRolloffFactor,
        AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR,
        AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR);

    al_effect_props_.Reverb.RoomRolloffFactor = efx_room_rolloff_factor;
}

void EaxReverbEffect::set_efx_flags()
{
    al_effect_props_.Reverb.DecayHFLimit = ((eax_.ulFlags & EAXREVERBFLAGS_DECAYHFLIMIT) != 0);
}

void EaxReverbEffect::set_efx_defaults()
{
    set_efx_density_from_environment_size();
    set_efx_diffusion();
    set_efx_gain();
    set_efx_gain_hf();
    set_efx_gain_lf();
    set_efx_decay_time();
    set_efx_decay_hf_ratio();
    set_efx_decay_lf_ratio();
    set_efx_reflections_gain();
    set_efx_reflections_delay();
    set_efx_reflections_pan();
    set_efx_late_reverb_gain();
    set_efx_late_reverb_delay();
    set_efx_late_reverb_pan();
    set_efx_echo_time();
    set_efx_echo_depth();
    set_efx_modulation_time();
    set_efx_modulation_depth();
    set_efx_air_absorption_gain_hf();
    set_efx_hf_reference();
    set_efx_lf_reference();
    set_efx_room_rolloff_factor();
    set_efx_flags();
}

void EaxReverbEffect::v1_get(const EaxEaxCall& eax_call) const
{
    switch(eax_call.get_property_id())
    {
        case DSPROPERTY_EAX_ALL:
            eax_call.set_value<EaxReverbEffectException>(eax1_);
            break;

        case DSPROPERTY_EAX_ENVIRONMENT:
            eax_call.set_value<EaxReverbEffectException>(eax1_.environment);
            break;

        case DSPROPERTY_EAX_VOLUME:
            eax_call.set_value<EaxReverbEffectException>(eax1_.fVolume);
            break;

        case DSPROPERTY_EAX_DECAYTIME:
            eax_call.set_value<EaxReverbEffectException>(eax1_.fDecayTime_sec);
            break;

        case DSPROPERTY_EAX_DAMPING:
            eax_call.set_value<EaxReverbEffectException>(eax1_.fDamping);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void EaxReverbEffect::get_all(
    const EaxEaxCall& eax_call) const
{
    if (eax_call.get_version() == 2)
    {
        auto& eax_reverb = eax_call.get_value<EaxReverbEffectException, EAX20LISTENERPROPERTIES>();
        eax_reverb.lRoom = eax_.lRoom;
        eax_reverb.lRoomHF = eax_.lRoomHF;
        eax_reverb.flRoomRolloffFactor = eax_.flRoomRolloffFactor;
        eax_reverb.flDecayTime = eax_.flDecayTime;
        eax_reverb.flDecayHFRatio = eax_.flDecayHFRatio;
        eax_reverb.lReflections = eax_.lReflections;
        eax_reverb.flReflectionsDelay = eax_.flReflectionsDelay;
        eax_reverb.lReverb = eax_.lReverb;
        eax_reverb.flReverbDelay = eax_.flReverbDelay;
        eax_reverb.dwEnvironment = eax_.ulEnvironment;
        eax_reverb.flEnvironmentSize = eax_.flEnvironmentSize;
        eax_reverb.flEnvironmentDiffusion = eax_.flEnvironmentDiffusion;
        eax_reverb.flAirAbsorptionHF = eax_.flAirAbsorptionHF;
        eax_reverb.dwFlags = eax_.ulFlags;
    }
    else
    {
        eax_call.set_value<EaxReverbEffectException>(eax_);
    }
}

void EaxReverbEffect::get(const EaxEaxCall& eax_call) const
{
    if(eax_call.get_version() == 1)
        v1_get(eax_call);
    else switch(eax_call.get_property_id())
    {
        case EAXREVERB_NONE:
            break;

        case EAXREVERB_ALLPARAMETERS:
            get_all(eax_call);
            break;

        case EAXREVERB_ENVIRONMENT:
            eax_call.set_value<EaxReverbEffectException>(eax_.ulEnvironment);
            break;

        case EAXREVERB_ENVIRONMENTSIZE:
            eax_call.set_value<EaxReverbEffectException>(eax_.flEnvironmentSize);
            break;

        case EAXREVERB_ENVIRONMENTDIFFUSION:
            eax_call.set_value<EaxReverbEffectException>(eax_.flEnvironmentDiffusion);
            break;

        case EAXREVERB_ROOM:
            eax_call.set_value<EaxReverbEffectException>(eax_.lRoom);
            break;

        case EAXREVERB_ROOMHF:
            eax_call.set_value<EaxReverbEffectException>(eax_.lRoomHF);
            break;

        case EAXREVERB_ROOMLF:
            eax_call.set_value<EaxReverbEffectException>(eax_.lRoomLF);
            break;

        case EAXREVERB_DECAYTIME:
            eax_call.set_value<EaxReverbEffectException>(eax_.flDecayTime);
            break;

        case EAXREVERB_DECAYHFRATIO:
            eax_call.set_value<EaxReverbEffectException>(eax_.flDecayHFRatio);
            break;

        case EAXREVERB_DECAYLFRATIO:
            eax_call.set_value<EaxReverbEffectException>(eax_.flDecayLFRatio);
            break;

        case EAXREVERB_REFLECTIONS:
            eax_call.set_value<EaxReverbEffectException>(eax_.lReflections);
            break;

        case EAXREVERB_REFLECTIONSDELAY:
            eax_call.set_value<EaxReverbEffectException>(eax_.flReflectionsDelay);
            break;

        case EAXREVERB_REFLECTIONSPAN:
            eax_call.set_value<EaxReverbEffectException>(eax_.vReflectionsPan);
            break;

        case EAXREVERB_REVERB:
            eax_call.set_value<EaxReverbEffectException>(eax_.lReverb);
            break;

        case EAXREVERB_REVERBDELAY:
            eax_call.set_value<EaxReverbEffectException>(eax_.flReverbDelay);
            break;

        case EAXREVERB_REVERBPAN:
            eax_call.set_value<EaxReverbEffectException>(eax_.vReverbPan);
            break;

        case EAXREVERB_ECHOTIME:
            eax_call.set_value<EaxReverbEffectException>(eax_.flEchoTime);
            break;

        case EAXREVERB_ECHODEPTH:
            eax_call.set_value<EaxReverbEffectException>(eax_.flEchoDepth);
            break;

        case EAXREVERB_MODULATIONTIME:
            eax_call.set_value<EaxReverbEffectException>(eax_.flModulationTime);
            break;

        case EAXREVERB_MODULATIONDEPTH:
            eax_call.set_value<EaxReverbEffectException>(eax_.flModulationDepth);
            break;

        case EAXREVERB_AIRABSORPTIONHF:
            eax_call.set_value<EaxReverbEffectException>(eax_.flAirAbsorptionHF);
            break;

        case EAXREVERB_HFREFERENCE:
            eax_call.set_value<EaxReverbEffectException>(eax_.flHFReference);
            break;

        case EAXREVERB_LFREFERENCE:
            eax_call.set_value<EaxReverbEffectException>(eax_.flLFReference);
            break;

        case EAXREVERB_ROOMROLLOFFFACTOR:
            eax_call.set_value<EaxReverbEffectException>(eax_.flRoomRolloffFactor);
            break;

        case EAXREVERB_FLAGS:
            eax_call.set_value<EaxReverbEffectException>(eax_.ulFlags);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void EaxReverbEffect::v1_validate_environment(unsigned long environment)
{
    validate_environment(environment, 1, true);
}

void EaxReverbEffect::v1_validate_volume(float volume)
{
    eax_validate_range<EaxReverbEffectException>("Volume", volume, EAX1REVERB_MINVOLUME, EAX1REVERB_MAXVOLUME);
}

void EaxReverbEffect::v1_validate_decay_time(float decay_time)
{
    validate_decay_time(decay_time);
}

void EaxReverbEffect::v1_validate_damping(float damping)
{
    eax_validate_range<EaxReverbEffectException>("Damping", damping, EAX1REVERB_MINDAMPING, EAX1REVERB_MAXDAMPING);
}

void EaxReverbEffect::v1_validate_all(const EAX_REVERBPROPERTIES& all)
{
    v1_validate_environment(all.environment);
    v1_validate_volume(all.fVolume);
    v1_validate_decay_time(all.fDecayTime_sec);
    v1_validate_damping(all.fDamping);
}

void EaxReverbEffect::validate_environment(
    unsigned long ulEnvironment,
    int version,
    bool is_standalone)
{
    eax_validate_range<EaxReverbEffectException>(
        "Environment",
        ulEnvironment,
        EAXREVERB_MINENVIRONMENT,
        (version <= 2 || is_standalone) ? EAX1REVERB_MAXENVIRONMENT : EAX30REVERB_MAXENVIRONMENT);
}

void EaxReverbEffect::validate_environment_size(
    float flEnvironmentSize)
{
    eax_validate_range<EaxReverbEffectException>(
        "Environment Size",
        flEnvironmentSize,
        EAXREVERB_MINENVIRONMENTSIZE,
        EAXREVERB_MAXENVIRONMENTSIZE);
}

void EaxReverbEffect::validate_environment_diffusion(
    float flEnvironmentDiffusion)
{
    eax_validate_range<EaxReverbEffectException>(
        "Environment Diffusion",
        flEnvironmentDiffusion,
        EAXREVERB_MINENVIRONMENTDIFFUSION,
        EAXREVERB_MAXENVIRONMENTDIFFUSION);
}

void EaxReverbEffect::validate_room(
    long lRoom)
{
    eax_validate_range<EaxReverbEffectException>(
        "Room",
        lRoom,
        EAXREVERB_MINROOM,
        EAXREVERB_MAXROOM);
}

void EaxReverbEffect::validate_room_hf(
    long lRoomHF)
{
    eax_validate_range<EaxReverbEffectException>(
        "Room HF",
        lRoomHF,
        EAXREVERB_MINROOMHF,
        EAXREVERB_MAXROOMHF);
}

void EaxReverbEffect::validate_room_lf(
    long lRoomLF)
{
    eax_validate_range<EaxReverbEffectException>(
        "Room LF",
        lRoomLF,
        EAXREVERB_MINROOMLF,
        EAXREVERB_MAXROOMLF);
}

void EaxReverbEffect::validate_decay_time(
    float flDecayTime)
{
    eax_validate_range<EaxReverbEffectException>(
        "Decay Time",
        flDecayTime,
        EAXREVERB_MINDECAYTIME,
        EAXREVERB_MAXDECAYTIME);
}

void EaxReverbEffect::validate_decay_hf_ratio(
    float flDecayHFRatio)
{
    eax_validate_range<EaxReverbEffectException>(
        "Decay HF Ratio",
        flDecayHFRatio,
        EAXREVERB_MINDECAYHFRATIO,
        EAXREVERB_MAXDECAYHFRATIO);
}

void EaxReverbEffect::validate_decay_lf_ratio(
    float flDecayLFRatio)
{
    eax_validate_range<EaxReverbEffectException>(
        "Decay LF Ratio",
        flDecayLFRatio,
        EAXREVERB_MINDECAYLFRATIO,
        EAXREVERB_MAXDECAYLFRATIO);
}

void EaxReverbEffect::validate_reflections(
    long lReflections)
{
    eax_validate_range<EaxReverbEffectException>(
        "Reflections",
        lReflections,
        EAXREVERB_MINREFLECTIONS,
        EAXREVERB_MAXREFLECTIONS);
}

void EaxReverbEffect::validate_reflections_delay(
    float flReflectionsDelay)
{
    eax_validate_range<EaxReverbEffectException>(
        "Reflections Delay",
        flReflectionsDelay,
        EAXREVERB_MINREFLECTIONSDELAY,
        EAXREVERB_MAXREFLECTIONSDELAY);
}

void EaxReverbEffect::validate_reflections_pan(
    const EAXVECTOR& vReflectionsPan)
{
    std::ignore = vReflectionsPan;
}

void EaxReverbEffect::validate_reverb(
    long lReverb)
{
    eax_validate_range<EaxReverbEffectException>(
        "Reverb",
        lReverb,
        EAXREVERB_MINREVERB,
        EAXREVERB_MAXREVERB);
}

void EaxReverbEffect::validate_reverb_delay(
    float flReverbDelay)
{
    eax_validate_range<EaxReverbEffectException>(
        "Reverb Delay",
        flReverbDelay,
        EAXREVERB_MINREVERBDELAY,
        EAXREVERB_MAXREVERBDELAY);
}

void EaxReverbEffect::validate_reverb_pan(
    const EAXVECTOR& vReverbPan)
{
    std::ignore = vReverbPan;
}

void EaxReverbEffect::validate_echo_time(
    float flEchoTime)
{
    eax_validate_range<EaxReverbEffectException>(
        "Echo Time",
        flEchoTime,
        EAXREVERB_MINECHOTIME,
        EAXREVERB_MAXECHOTIME);
}

void EaxReverbEffect::validate_echo_depth(
    float flEchoDepth)
{
    eax_validate_range<EaxReverbEffectException>(
        "Echo Depth",
        flEchoDepth,
        EAXREVERB_MINECHODEPTH,
        EAXREVERB_MAXECHODEPTH);
}

void EaxReverbEffect::validate_modulation_time(
    float flModulationTime)
{
    eax_validate_range<EaxReverbEffectException>(
        "Modulation Time",
        flModulationTime,
        EAXREVERB_MINMODULATIONTIME,
        EAXREVERB_MAXMODULATIONTIME);
}

void EaxReverbEffect::validate_modulation_depth(
    float flModulationDepth)
{
    eax_validate_range<EaxReverbEffectException>(
        "Modulation Depth",
        flModulationDepth,
        EAXREVERB_MINMODULATIONDEPTH,
        EAXREVERB_MAXMODULATIONDEPTH);
}

void EaxReverbEffect::validate_air_absorbtion_hf(
    float air_absorbtion_hf)
{
    eax_validate_range<EaxReverbEffectException>(
        "Air Absorbtion HF",
        air_absorbtion_hf,
        EAXREVERB_MINAIRABSORPTIONHF,
        EAXREVERB_MAXAIRABSORPTIONHF);
}

void EaxReverbEffect::validate_hf_reference(
    float flHFReference)
{
    eax_validate_range<EaxReverbEffectException>(
        "HF Reference",
        flHFReference,
        EAXREVERB_MINHFREFERENCE,
        EAXREVERB_MAXHFREFERENCE);
}

void EaxReverbEffect::validate_lf_reference(
    float flLFReference)
{
    eax_validate_range<EaxReverbEffectException>(
        "LF Reference",
        flLFReference,
        EAXREVERB_MINLFREFERENCE,
        EAXREVERB_MAXLFREFERENCE);
}

void EaxReverbEffect::validate_room_rolloff_factor(
    float flRoomRolloffFactor)
{
    eax_validate_range<EaxReverbEffectException>(
        "Room Rolloff Factor",
        flRoomRolloffFactor,
        EAXREVERB_MINROOMROLLOFFFACTOR,
        EAXREVERB_MAXROOMROLLOFFFACTOR);
}

void EaxReverbEffect::validate_flags(
    unsigned long ulFlags)
{
    eax_validate_range<EaxReverbEffectException>(
        "Flags",
        ulFlags,
        0UL,
        ~EAXREVERBFLAGS_RESERVED);
}

void EaxReverbEffect::validate_all(
    const EAX20LISTENERPROPERTIES& listener,
    int version)
{
    validate_room(listener.lRoom);
    validate_room_hf(listener.lRoomHF);
    validate_room_rolloff_factor(listener.flRoomRolloffFactor);
    validate_decay_time(listener.flDecayTime);
    validate_decay_hf_ratio(listener.flDecayHFRatio);
    validate_reflections(listener.lReflections);
    validate_reflections_delay(listener.flReflectionsDelay);
    validate_reverb(listener.lReverb);
    validate_reverb_delay(listener.flReverbDelay);
    validate_environment(listener.dwEnvironment, version, false);
    validate_environment_size(listener.flEnvironmentSize);
    validate_environment_diffusion(listener.flEnvironmentDiffusion);
    validate_air_absorbtion_hf(listener.flAirAbsorptionHF);
    validate_flags(listener.dwFlags);
}

void EaxReverbEffect::validate_all(
    const EAXREVERBPROPERTIES& lReverb,
    int version)
{
    validate_environment(lReverb.ulEnvironment, version, false);
    validate_environment_size(lReverb.flEnvironmentSize);
    validate_environment_diffusion(lReverb.flEnvironmentDiffusion);
    validate_room(lReverb.lRoom);
    validate_room_hf(lReverb.lRoomHF);
    validate_room_lf(lReverb.lRoomLF);
    validate_decay_time(lReverb.flDecayTime);
    validate_decay_hf_ratio(lReverb.flDecayHFRatio);
    validate_decay_lf_ratio(lReverb.flDecayLFRatio);
    validate_reflections(lReverb.lReflections);
    validate_reflections_delay(lReverb.flReflectionsDelay);
    validate_reverb(lReverb.lReverb);
    validate_reverb_delay(lReverb.flReverbDelay);
    validate_echo_time(lReverb.flEchoTime);
    validate_echo_depth(lReverb.flEchoDepth);
    validate_modulation_time(lReverb.flModulationTime);
    validate_modulation_depth(lReverb.flModulationDepth);
    validate_air_absorbtion_hf(lReverb.flAirAbsorptionHF);
    validate_hf_reference(lReverb.flHFReference);
    validate_lf_reference(lReverb.flLFReference);
    validate_room_rolloff_factor(lReverb.flRoomRolloffFactor);
    validate_flags(lReverb.ulFlags);
}

void EaxReverbEffect::v1_defer_environment(unsigned long environment)
{
    eax1_d_ = EAX1REVERB_PRESETS[environment];
    eax1_dirty_flags_.ulEnvironment = true;
}

void EaxReverbEffect::v1_defer_volume(float volume)
{
    eax1_d_.fVolume = volume;
    eax1_dirty_flags_.flVolume = (eax1_.fVolume != eax1_d_.fVolume);
}

void EaxReverbEffect::v1_defer_decay_time(float decay_time)
{
    eax1_d_.fDecayTime_sec = decay_time;
    eax1_dirty_flags_.flDecayTime = (eax1_.fDecayTime_sec != eax1_d_.fDecayTime_sec);
}

void EaxReverbEffect::v1_defer_damping(float damping)
{
    eax1_d_.fDamping = damping;
    eax1_dirty_flags_.flDamping = (eax1_.fDamping != eax1_d_.fDamping);
}

void EaxReverbEffect::v1_defer_all(const EAX_REVERBPROPERTIES& lReverb)
{
    v1_defer_environment(lReverb.environment);
    v1_defer_volume(lReverb.fVolume);
    v1_defer_decay_time(lReverb.fDecayTime_sec);
    v1_defer_damping(lReverb.fDamping);
}


void EaxReverbEffect::v1_set_efx()
{
    auto efx_props = eax_efx_reverb_presets[eax1_.environment];
    efx_props.flGain = eax1_.fVolume;
    efx_props.flDecayTime = eax1_.fDecayTime_sec;
    efx_props.flDecayHFRatio = clamp(eax1_.fDamping, AL_EAXREVERB_MIN_DECAY_HFRATIO, AL_EAXREVERB_MAX_DECAY_HFRATIO);

    al_effect_props_.Reverb.Density = efx_props.flDensity;
    al_effect_props_.Reverb.Diffusion = efx_props.flDiffusion;
    al_effect_props_.Reverb.Gain = efx_props.flGain;
    al_effect_props_.Reverb.GainHF = efx_props.flGainHF;
    al_effect_props_.Reverb.GainLF = efx_props.flGainLF;
    al_effect_props_.Reverb.DecayTime = efx_props.flDecayTime;
    al_effect_props_.Reverb.DecayHFRatio = efx_props.flDecayHFRatio;
    al_effect_props_.Reverb.DecayLFRatio = efx_props.flDecayLFRatio;
    al_effect_props_.Reverb.ReflectionsGain = efx_props.flReflectionsGain;
    al_effect_props_.Reverb.ReflectionsDelay = efx_props.flReflectionsDelay;
    al_effect_props_.Reverb.ReflectionsPan[0] = efx_props.flReflectionsPan[0];
    al_effect_props_.Reverb.ReflectionsPan[1] = efx_props.flReflectionsPan[1];
    al_effect_props_.Reverb.ReflectionsPan[2] = efx_props.flReflectionsPan[2];
    al_effect_props_.Reverb.LateReverbGain = efx_props.flLateReverbGain;
    al_effect_props_.Reverb.LateReverbDelay = efx_props.flLateReverbDelay;
    al_effect_props_.Reverb.LateReverbPan[0] = efx_props.flLateReverbPan[0];
    al_effect_props_.Reverb.LateReverbPan[1] = efx_props.flLateReverbPan[1];
    al_effect_props_.Reverb.LateReverbPan[2] = efx_props.flLateReverbPan[2];
    al_effect_props_.Reverb.EchoTime = efx_props.flEchoTime;
    al_effect_props_.Reverb.EchoDepth = efx_props.flEchoDepth;
    al_effect_props_.Reverb.ModulationTime = efx_props.flModulationTime;
    al_effect_props_.Reverb.ModulationDepth = efx_props.flModulationDepth;
    al_effect_props_.Reverb.HFReference = efx_props.flHFReference;
    al_effect_props_.Reverb.LFReference = efx_props.flLFReference;
    al_effect_props_.Reverb.RoomRolloffFactor = efx_props.flRoomRolloffFactor;
    al_effect_props_.Reverb.AirAbsorptionGainHF = efx_props.flAirAbsorptionGainHF;
    al_effect_props_.Reverb.DecayHFLimit = false;
}

void EaxReverbEffect::defer_environment(
    unsigned long ulEnvironment)
{
    eax_d_.ulEnvironment = ulEnvironment;
    eax_dirty_flags_.ulEnvironment = (eax_.ulEnvironment != eax_d_.ulEnvironment);
}

void EaxReverbEffect::defer_environment_size(
    float flEnvironmentSize)
{
    eax_d_.flEnvironmentSize = flEnvironmentSize;
    eax_dirty_flags_.flEnvironmentSize = (eax_.flEnvironmentSize != eax_d_.flEnvironmentSize);
}

void EaxReverbEffect::defer_environment_diffusion(
    float flEnvironmentDiffusion)
{
    eax_d_.flEnvironmentDiffusion = flEnvironmentDiffusion;
    eax_dirty_flags_.flEnvironmentDiffusion = (eax_.flEnvironmentDiffusion != eax_d_.flEnvironmentDiffusion);
}

void EaxReverbEffect::defer_room(
    long lRoom)
{
    eax_d_.lRoom = lRoom;
    eax_dirty_flags_.lRoom = (eax_.lRoom != eax_d_.lRoom);
}

void EaxReverbEffect::defer_room_hf(
    long lRoomHF)
{
    eax_d_.lRoomHF = lRoomHF;
    eax_dirty_flags_.lRoomHF = (eax_.lRoomHF != eax_d_.lRoomHF);
}

void EaxReverbEffect::defer_room_lf(
    long lRoomLF)
{
    eax_d_.lRoomLF = lRoomLF;
    eax_dirty_flags_.lRoomLF = (eax_.lRoomLF != eax_d_.lRoomLF);
}

void EaxReverbEffect::defer_decay_time(
    float flDecayTime)
{
    eax_d_.flDecayTime = flDecayTime;
    eax_dirty_flags_.flDecayTime = (eax_.flDecayTime != eax_d_.flDecayTime);
}

void EaxReverbEffect::defer_decay_hf_ratio(
    float flDecayHFRatio)
{
    eax_d_.flDecayHFRatio = flDecayHFRatio;
    eax_dirty_flags_.flDecayHFRatio = (eax_.flDecayHFRatio != eax_d_.flDecayHFRatio);
}

void EaxReverbEffect::defer_decay_lf_ratio(
    float flDecayLFRatio)
{
    eax_d_.flDecayLFRatio = flDecayLFRatio;
    eax_dirty_flags_.flDecayLFRatio = (eax_.flDecayLFRatio != eax_d_.flDecayLFRatio);
}

void EaxReverbEffect::defer_reflections(
    long lReflections)
{
    eax_d_.lReflections = lReflections;
    eax_dirty_flags_.lReflections = (eax_.lReflections != eax_d_.lReflections);
}

void EaxReverbEffect::defer_reflections_delay(
    float flReflectionsDelay)
{
    eax_d_.flReflectionsDelay = flReflectionsDelay;
    eax_dirty_flags_.flReflectionsDelay = (eax_.flReflectionsDelay != eax_d_.flReflectionsDelay);
}

void EaxReverbEffect::defer_reflections_pan(
    const EAXVECTOR& vReflectionsPan)
{
    eax_d_.vReflectionsPan = vReflectionsPan;
    eax_dirty_flags_.vReflectionsPan = (eax_.vReflectionsPan != eax_d_.vReflectionsPan);
}

void EaxReverbEffect::defer_reverb(
    long lReverb)
{
    eax_d_.lReverb = lReverb;
    eax_dirty_flags_.lReverb = (eax_.lReverb != eax_d_.lReverb);
}

void EaxReverbEffect::defer_reverb_delay(
    float flReverbDelay)
{
    eax_d_.flReverbDelay = flReverbDelay;
    eax_dirty_flags_.flReverbDelay = (eax_.flReverbDelay != eax_d_.flReverbDelay);
}

void EaxReverbEffect::defer_reverb_pan(
    const EAXVECTOR& vReverbPan)
{
    eax_d_.vReverbPan = vReverbPan;
    eax_dirty_flags_.vReverbPan = (eax_.vReverbPan != eax_d_.vReverbPan);
}

void EaxReverbEffect::defer_echo_time(
    float flEchoTime)
{
    eax_d_.flEchoTime = flEchoTime;
    eax_dirty_flags_.flEchoTime = (eax_.flEchoTime != eax_d_.flEchoTime);
}

void EaxReverbEffect::defer_echo_depth(
    float flEchoDepth)
{
    eax_d_.flEchoDepth = flEchoDepth;
    eax_dirty_flags_.flEchoDepth = (eax_.flEchoDepth != eax_d_.flEchoDepth);
}

void EaxReverbEffect::defer_modulation_time(
    float flModulationTime)
{
    eax_d_.flModulationTime = flModulationTime;
    eax_dirty_flags_.flModulationTime = (eax_.flModulationTime != eax_d_.flModulationTime);
}

void EaxReverbEffect::defer_modulation_depth(
    float flModulationDepth)
{
    eax_d_.flModulationDepth = flModulationDepth;
    eax_dirty_flags_.flModulationDepth = (eax_.flModulationDepth != eax_d_.flModulationDepth);
}

void EaxReverbEffect::defer_air_absorbtion_hf(
    float flAirAbsorptionHF)
{
    eax_d_.flAirAbsorptionHF = flAirAbsorptionHF;
    eax_dirty_flags_.flAirAbsorptionHF = (eax_.flAirAbsorptionHF != eax_d_.flAirAbsorptionHF);
}

void EaxReverbEffect::defer_hf_reference(
    float flHFReference)
{
    eax_d_.flHFReference = flHFReference;
    eax_dirty_flags_.flHFReference = (eax_.flHFReference != eax_d_.flHFReference);
}

void EaxReverbEffect::defer_lf_reference(
    float flLFReference)
{
    eax_d_.flLFReference = flLFReference;
    eax_dirty_flags_.flLFReference = (eax_.flLFReference != eax_d_.flLFReference);
}

void EaxReverbEffect::defer_room_rolloff_factor(
    float flRoomRolloffFactor)
{
    eax_d_.flRoomRolloffFactor = flRoomRolloffFactor;
    eax_dirty_flags_.flRoomRolloffFactor = (eax_.flRoomRolloffFactor != eax_d_.flRoomRolloffFactor);
}

void EaxReverbEffect::defer_flags(
    unsigned long ulFlags)
{
    eax_d_.ulFlags = ulFlags;
    eax_dirty_flags_.ulFlags = (eax_.ulFlags != eax_d_.ulFlags);
}

void EaxReverbEffect::defer_all(
    const EAX20LISTENERPROPERTIES& listener)
{
    defer_room(listener.lRoom);
    defer_room_hf(listener.lRoomHF);
    defer_room_rolloff_factor(listener.flRoomRolloffFactor);
    defer_decay_time(listener.flDecayTime);
    defer_decay_hf_ratio(listener.flDecayHFRatio);
    defer_reflections(listener.lReflections);
    defer_reflections_delay(listener.flReflectionsDelay);
    defer_reverb(listener.lReverb);
    defer_reverb_delay(listener.flReverbDelay);
    defer_environment(listener.dwEnvironment);
    defer_environment_size(listener.flEnvironmentSize);
    defer_environment_diffusion(listener.flEnvironmentDiffusion);
    defer_air_absorbtion_hf(listener.flAirAbsorptionHF);
    defer_flags(listener.dwFlags);
}

void EaxReverbEffect::defer_all(
    const EAXREVERBPROPERTIES& lReverb)
{
    defer_environment(lReverb.ulEnvironment);
    defer_environment_size(lReverb.flEnvironmentSize);
    defer_environment_diffusion(lReverb.flEnvironmentDiffusion);
    defer_room(lReverb.lRoom);
    defer_room_hf(lReverb.lRoomHF);
    defer_room_lf(lReverb.lRoomLF);
    defer_decay_time(lReverb.flDecayTime);
    defer_decay_hf_ratio(lReverb.flDecayHFRatio);
    defer_decay_lf_ratio(lReverb.flDecayLFRatio);
    defer_reflections(lReverb.lReflections);
    defer_reflections_delay(lReverb.flReflectionsDelay);
    defer_reflections_pan(lReverb.vReflectionsPan);
    defer_reverb(lReverb.lReverb);
    defer_reverb_delay(lReverb.flReverbDelay);
    defer_reverb_pan(lReverb.vReverbPan);
    defer_echo_time(lReverb.flEchoTime);
    defer_echo_depth(lReverb.flEchoDepth);
    defer_modulation_time(lReverb.flModulationTime);
    defer_modulation_depth(lReverb.flModulationDepth);
    defer_air_absorbtion_hf(lReverb.flAirAbsorptionHF);
    defer_hf_reference(lReverb.flHFReference);
    defer_lf_reference(lReverb.flLFReference);
    defer_room_rolloff_factor(lReverb.flRoomRolloffFactor);
    defer_flags(lReverb.ulFlags);
}


void EaxReverbEffect::v1_defer_environment(const EaxEaxCall& eax_call)
{
    const auto& environment = eax_call.get_value<EaxReverbEffectException,
        const decltype(EAX_REVERBPROPERTIES::environment)>();

    validate_environment(environment, 1, true);

    const auto& reverb_preset = EAX1REVERB_PRESETS[environment];
    v1_defer_all(reverb_preset);
}

void EaxReverbEffect::v1_defer_volume(const EaxEaxCall& eax_call)
{
    const auto& volume = eax_call.get_value<EaxReverbEffectException,
        const decltype(EAX_REVERBPROPERTIES::fVolume)>();

    v1_validate_volume(volume);
    v1_defer_volume(volume);
}

void EaxReverbEffect::v1_defer_decay_time(const EaxEaxCall& eax_call)
{
    const auto& decay_time = eax_call.get_value<EaxReverbEffectException,
        const decltype(EAX_REVERBPROPERTIES::fDecayTime_sec)>();

    v1_validate_decay_time(decay_time);
    v1_defer_decay_time(decay_time);
}

void EaxReverbEffect::v1_defer_damping(const EaxEaxCall& eax_call)
{
    const auto& damping = eax_call.get_value<EaxReverbEffectException,
        const decltype(EAX_REVERBPROPERTIES::fDamping)>();

    v1_validate_damping(damping);
    v1_defer_damping(damping);
}

void EaxReverbEffect::v1_defer_all(const EaxEaxCall& eax_call)
{
    const auto& reverb_all = eax_call.get_value<EaxReverbEffectException,
        const EAX_REVERBPROPERTIES>();

    v1_validate_all(reverb_all);
    v1_defer_all(reverb_all);
}


void EaxReverbEffect::defer_environment(
    const EaxEaxCall& eax_call)
{
    const auto& ulEnvironment =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::ulEnvironment)>();

    validate_environment(ulEnvironment, eax_call.get_version(), true);

    if (eax_d_.ulEnvironment == ulEnvironment)
    {
        return;
    }

    const auto& reverb_preset = EAXREVERB_PRESETS[ulEnvironment];

    defer_all(reverb_preset);
}

void EaxReverbEffect::defer_environment_size(
    const EaxEaxCall& eax_call)
{
    const auto& flEnvironmentSize =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flEnvironmentSize)>();

    validate_environment_size(flEnvironmentSize);

    if (eax_d_.flEnvironmentSize == flEnvironmentSize)
    {
        return;
    }

    const auto scale = flEnvironmentSize / eax_d_.flEnvironmentSize;

    defer_environment_size(flEnvironmentSize);

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_DECAYTIMESCALE) != 0)
    {
        const auto flDecayTime = clamp(
            scale * eax_d_.flDecayTime,
            EAXREVERB_MINDECAYTIME,
            EAXREVERB_MAXDECAYTIME);

        defer_decay_time(flDecayTime);
    }

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_REFLECTIONSSCALE) != 0)
    {
        if ((eax_d_.ulFlags & EAXREVERBFLAGS_REFLECTIONSDELAYSCALE) != 0)
        {
            const auto lReflections = clamp(
                eax_d_.lReflections - static_cast<long>(gain_to_level_mb(scale)),
                EAXREVERB_MINREFLECTIONS,
                EAXREVERB_MAXREFLECTIONS);

            defer_reflections(lReflections);
        }
    }

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_REFLECTIONSDELAYSCALE) != 0)
    {
        const auto flReflectionsDelay = clamp(
            eax_d_.flReflectionsDelay * scale,
            EAXREVERB_MINREFLECTIONSDELAY,
            EAXREVERB_MAXREFLECTIONSDELAY);

        defer_reflections_delay(flReflectionsDelay);
    }

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_REVERBSCALE) != 0)
    {
        const auto log_scalar = ((eax_d_.ulFlags & EAXREVERBFLAGS_DECAYTIMESCALE) != 0) ? 2'000.0F : 3'000.0F;

        const auto lReverb = clamp(
            eax_d_.lReverb - static_cast<long>(std::log10(scale) * log_scalar),
            EAXREVERB_MINREVERB,
            EAXREVERB_MAXREVERB);

        defer_reverb(lReverb);
    }

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_REVERBDELAYSCALE) != 0)
    {
        const auto flReverbDelay = clamp(
            scale * eax_d_.flReverbDelay,
            EAXREVERB_MINREVERBDELAY,
            EAXREVERB_MAXREVERBDELAY);

        defer_reverb_delay(flReverbDelay);
    }

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_ECHOTIMESCALE) != 0)
    {
        const auto flEchoTime = clamp(
            eax_d_.flEchoTime * scale,
            EAXREVERB_MINECHOTIME,
            EAXREVERB_MAXECHOTIME);

        defer_echo_time(flEchoTime);
    }

    if ((eax_d_.ulFlags & EAXREVERBFLAGS_MODULATIONTIMESCALE) != 0)
    {
        const auto flModulationTime = clamp(
            scale * eax_d_.flModulationTime,
            EAXREVERB_MINMODULATIONTIME,
            EAXREVERB_MAXMODULATIONTIME);

        defer_modulation_time(flModulationTime);
    }
}

void EaxReverbEffect::defer_environment_diffusion(
    const EaxEaxCall& eax_call)
{
    const auto& flEnvironmentDiffusion =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flEnvironmentDiffusion)>();

    validate_environment_diffusion(flEnvironmentDiffusion);
    defer_environment_diffusion(flEnvironmentDiffusion);
}

void EaxReverbEffect::defer_room(
    const EaxEaxCall& eax_call)
{
    const auto& lRoom =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::lRoom)>();

    validate_room(lRoom);
    defer_room(lRoom);
}

void EaxReverbEffect::defer_room_hf(
    const EaxEaxCall& eax_call)
{
    const auto& lRoomHF =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::lRoomHF)>();

    validate_room_hf(lRoomHF);
    defer_room_hf(lRoomHF);
}

void EaxReverbEffect::defer_room_lf(
    const EaxEaxCall& eax_call)
{
    const auto& lRoomLF =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::lRoomLF)>();

    validate_room_lf(lRoomLF);
    defer_room_lf(lRoomLF);
}

void EaxReverbEffect::defer_decay_time(
    const EaxEaxCall& eax_call)
{
    const auto& flDecayTime =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flDecayTime)>();

    validate_decay_time(flDecayTime);
    defer_decay_time(flDecayTime);
}

void EaxReverbEffect::defer_decay_hf_ratio(
    const EaxEaxCall& eax_call)
{
    const auto& flDecayHFRatio =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flDecayHFRatio)>();

    validate_decay_hf_ratio(flDecayHFRatio);
    defer_decay_hf_ratio(flDecayHFRatio);
}

void EaxReverbEffect::defer_decay_lf_ratio(
    const EaxEaxCall& eax_call)
{
    const auto& flDecayLFRatio =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flDecayLFRatio)>();

    validate_decay_lf_ratio(flDecayLFRatio);
    defer_decay_lf_ratio(flDecayLFRatio);
}

void EaxReverbEffect::defer_reflections(
    const EaxEaxCall& eax_call)
{
    const auto& lReflections =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::lReflections)>();

    validate_reflections(lReflections);
    defer_reflections(lReflections);
}

void EaxReverbEffect::defer_reflections_delay(
    const EaxEaxCall& eax_call)
{
    const auto& flReflectionsDelay =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flReflectionsDelay)>();

    validate_reflections_delay(flReflectionsDelay);
    defer_reflections_delay(flReflectionsDelay);
}

void EaxReverbEffect::defer_reflections_pan(
    const EaxEaxCall& eax_call)
{
    const auto& vReflectionsPan =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::vReflectionsPan)>();

    validate_reflections_pan(vReflectionsPan);
    defer_reflections_pan(vReflectionsPan);
}

void EaxReverbEffect::defer_reverb(
    const EaxEaxCall& eax_call)
{
    const auto& lReverb =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::lReverb)>();

    validate_reverb(lReverb);
    defer_reverb(lReverb);
}

void EaxReverbEffect::defer_reverb_delay(
    const EaxEaxCall& eax_call)
{
    const auto& flReverbDelay =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flReverbDelay)>();

    validate_reverb_delay(flReverbDelay);
    defer_reverb_delay(flReverbDelay);
}

void EaxReverbEffect::defer_reverb_pan(
    const EaxEaxCall& eax_call)
{
    const auto& vReverbPan =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::vReverbPan)>();

    validate_reverb_pan(vReverbPan);
    defer_reverb_pan(vReverbPan);
}

void EaxReverbEffect::defer_echo_time(
    const EaxEaxCall& eax_call)
{
    const auto& flEchoTime =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flEchoTime)>();

    validate_echo_time(flEchoTime);
    defer_echo_time(flEchoTime);
}

void EaxReverbEffect::defer_echo_depth(
    const EaxEaxCall& eax_call)
{
    const auto& flEchoDepth =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flEchoDepth)>();

    validate_echo_depth(flEchoDepth);
    defer_echo_depth(flEchoDepth);
}

void EaxReverbEffect::defer_modulation_time(
    const EaxEaxCall& eax_call)
{
    const auto& flModulationTime =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flModulationTime)>();

    validate_modulation_time(flModulationTime);
    defer_modulation_time(flModulationTime);
}

void EaxReverbEffect::defer_modulation_depth(
    const EaxEaxCall& eax_call)
{
    const auto& flModulationDepth =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flModulationDepth)>();

    validate_modulation_depth(flModulationDepth);
    defer_modulation_depth(flModulationDepth);
}

void EaxReverbEffect::defer_air_absorbtion_hf(
    const EaxEaxCall& eax_call)
{
    const auto& air_absorbtion_hf =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flAirAbsorptionHF)>();

    validate_air_absorbtion_hf(air_absorbtion_hf);
    defer_air_absorbtion_hf(air_absorbtion_hf);
}

void EaxReverbEffect::defer_hf_reference(
    const EaxEaxCall& eax_call)
{
    const auto& flHFReference =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flHFReference)>();

    validate_hf_reference(flHFReference);
    defer_hf_reference(flHFReference);
}

void EaxReverbEffect::defer_lf_reference(
    const EaxEaxCall& eax_call)
{
    const auto& flLFReference =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flLFReference)>();

    validate_lf_reference(flLFReference);
    defer_lf_reference(flLFReference);
}

void EaxReverbEffect::defer_room_rolloff_factor(
    const EaxEaxCall& eax_call)
{
    const auto& flRoomRolloffFactor =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::flRoomRolloffFactor)>();

    validate_room_rolloff_factor(flRoomRolloffFactor);
    defer_room_rolloff_factor(flRoomRolloffFactor);
}

void EaxReverbEffect::defer_flags(
    const EaxEaxCall& eax_call)
{
    const auto& ulFlags =
        eax_call.get_value<EaxReverbEffectException, const decltype(EAXREVERBPROPERTIES::ulFlags)>();

    validate_flags(ulFlags);
    defer_flags(ulFlags);
}

void EaxReverbEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto eax_version = eax_call.get_version();

    if (eax_version == 2)
    {
        const auto& listener =
            eax_call.get_value<EaxReverbEffectException, const EAX20LISTENERPROPERTIES>();

        validate_all(listener, eax_version);
        defer_all(listener);
    }
    else
    {
        const auto& reverb_all =
            eax_call.get_value<EaxReverbEffectException, const EAXREVERBPROPERTIES>();

        validate_all(reverb_all, eax_version);
        defer_all(reverb_all);
    }
}


void EaxReverbEffect::v1_defer(const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case DSPROPERTY_EAX_ALL: return v1_defer_all(eax_call);
        case DSPROPERTY_EAX_ENVIRONMENT: return v1_defer_environment(eax_call);
        case DSPROPERTY_EAX_VOLUME: return v1_defer_volume(eax_call);
        case DSPROPERTY_EAX_DECAYTIME: return v1_defer_decay_time(eax_call);
        case DSPROPERTY_EAX_DAMPING: return v1_defer_damping(eax_call);
        default: eax_fail("Unsupported property id.");
    }
}

// [[nodiscard]]
bool EaxReverbEffect::apply_deferred()
{
    bool ret{false};

    if(unlikely(eax1_dirty_flags_ != Eax1ReverbEffectDirtyFlags{}))
    {
        eax1_ = eax1_d_;

        v1_set_efx();

        eax1_dirty_flags_ = Eax1ReverbEffectDirtyFlags{};

        ret = true;
    }

    if(eax_dirty_flags_ == EaxReverbEffectDirtyFlags{})
        return ret;

    eax_ = eax_d_;

    if (eax_dirty_flags_.ulEnvironment)
    {
    }

    if (eax_dirty_flags_.flEnvironmentSize)
    {
        set_efx_density_from_environment_size();
    }

    if (eax_dirty_flags_.flEnvironmentDiffusion)
    {
        set_efx_diffusion();
    }

    if (eax_dirty_flags_.lRoom)
    {
        set_efx_gain();
    }

    if (eax_dirty_flags_.lRoomHF)
    {
        set_efx_gain_hf();
    }

    if (eax_dirty_flags_.lRoomLF)
    {
        set_efx_gain_lf();
    }

    if (eax_dirty_flags_.flDecayTime)
    {
        set_efx_decay_time();
    }

    if (eax_dirty_flags_.flDecayHFRatio)
    {
        set_efx_decay_hf_ratio();
    }

    if (eax_dirty_flags_.flDecayLFRatio)
    {
        set_efx_decay_lf_ratio();
    }

    if (eax_dirty_flags_.lReflections)
    {
        set_efx_reflections_gain();
    }

    if (eax_dirty_flags_.flReflectionsDelay)
    {
        set_efx_reflections_delay();
    }

    if (eax_dirty_flags_.vReflectionsPan)
    {
        set_efx_reflections_pan();
    }

    if (eax_dirty_flags_.lReverb)
    {
        set_efx_late_reverb_gain();
    }

    if (eax_dirty_flags_.flReverbDelay)
    {
        set_efx_late_reverb_delay();
    }

    if (eax_dirty_flags_.vReverbPan)
    {
        set_efx_late_reverb_pan();
    }

    if (eax_dirty_flags_.flEchoTime)
    {
        set_efx_echo_time();
    }

    if (eax_dirty_flags_.flEchoDepth)
    {
        set_efx_echo_depth();
    }

    if (eax_dirty_flags_.flModulationTime)
    {
        set_efx_modulation_time();
    }

    if (eax_dirty_flags_.flModulationDepth)
    {
        set_efx_modulation_depth();
    }

    if (eax_dirty_flags_.flAirAbsorptionHF)
    {
        set_efx_air_absorption_gain_hf();
    }

    if (eax_dirty_flags_.flHFReference)
    {
        set_efx_hf_reference();
    }

    if (eax_dirty_flags_.flLFReference)
    {
        set_efx_lf_reference();
    }

    if (eax_dirty_flags_.flRoomRolloffFactor)
    {
        set_efx_room_rolloff_factor();
    }

    if (eax_dirty_flags_.ulFlags)
    {
        set_efx_flags();
    }

    eax_dirty_flags_ = EaxReverbEffectDirtyFlags{};

    return true;
}

void EaxReverbEffect::set(const EaxEaxCall& eax_call)
{
    if(eax_call.get_version() == 1)
        v1_defer(eax_call);
    else switch(eax_call.get_property_id())
    {
        case EAXREVERB_NONE:
            break;

        case EAXREVERB_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXREVERB_ENVIRONMENT:
            defer_environment(eax_call);
            break;

        case EAXREVERB_ENVIRONMENTSIZE:
            defer_environment_size(eax_call);
            break;

        case EAXREVERB_ENVIRONMENTDIFFUSION:
            defer_environment_diffusion(eax_call);
            break;

        case EAXREVERB_ROOM:
            defer_room(eax_call);
            break;

        case EAXREVERB_ROOMHF:
            defer_room_hf(eax_call);
            break;

        case EAXREVERB_ROOMLF:
            defer_room_lf(eax_call);
            break;

        case EAXREVERB_DECAYTIME:
            defer_decay_time(eax_call);
            break;

        case EAXREVERB_DECAYHFRATIO:
            defer_decay_hf_ratio(eax_call);
            break;

        case EAXREVERB_DECAYLFRATIO:
            defer_decay_lf_ratio(eax_call);
            break;

        case EAXREVERB_REFLECTIONS:
            defer_reflections(eax_call);
            break;

        case EAXREVERB_REFLECTIONSDELAY:
            defer_reflections_delay(eax_call);
            break;

        case EAXREVERB_REFLECTIONSPAN:
            defer_reflections_pan(eax_call);
            break;

        case EAXREVERB_REVERB:
            defer_reverb(eax_call);
            break;

        case EAXREVERB_REVERBDELAY:
            defer_reverb_delay(eax_call);
            break;

        case EAXREVERB_REVERBPAN:
            defer_reverb_pan(eax_call);
            break;

        case EAXREVERB_ECHOTIME:
            defer_echo_time(eax_call);
            break;

        case EAXREVERB_ECHODEPTH:
            defer_echo_depth(eax_call);
            break;

        case EAXREVERB_MODULATIONTIME:
            defer_modulation_time(eax_call);
            break;

        case EAXREVERB_MODULATIONDEPTH:
            defer_modulation_depth(eax_call);
            break;

        case EAXREVERB_AIRABSORPTIONHF:
            defer_air_absorbtion_hf(eax_call);
            break;

        case EAXREVERB_HFREFERENCE:
            defer_hf_reference(eax_call);
            break;

        case EAXREVERB_LFREFERENCE:
            defer_lf_reference(eax_call);
            break;

        case EAXREVERB_ROOMROLLOFFFACTOR:
            defer_room_rolloff_factor(eax_call);
            break;

        case EAXREVERB_FLAGS:
            defer_flags(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

const EFXEAXREVERBPROPERTIES eax_efx_reverb_presets[EAX1_ENVIRONMENT_COUNT] =
{
    EFX_REVERB_PRESET_GENERIC,
    EFX_REVERB_PRESET_PADDEDCELL,
    EFX_REVERB_PRESET_ROOM,
    EFX_REVERB_PRESET_BATHROOM,
    EFX_REVERB_PRESET_LIVINGROOM,
    EFX_REVERB_PRESET_STONEROOM,
    EFX_REVERB_PRESET_AUDITORIUM,
    EFX_REVERB_PRESET_CONCERTHALL,
    EFX_REVERB_PRESET_CAVE,
    EFX_REVERB_PRESET_ARENA,
    EFX_REVERB_PRESET_HANGAR,
    EFX_REVERB_PRESET_CARPETEDHALLWAY,
    EFX_REVERB_PRESET_HALLWAY,
    EFX_REVERB_PRESET_STONECORRIDOR,
    EFX_REVERB_PRESET_ALLEY,
    EFX_REVERB_PRESET_FOREST,
    EFX_REVERB_PRESET_CITY,
    EFX_REVERB_PRESET_MOUNTAINS,
    EFX_REVERB_PRESET_QUARRY,
    EFX_REVERB_PRESET_PLAIN,
    EFX_REVERB_PRESET_PARKINGLOT,
    EFX_REVERB_PRESET_SEWERPIPE,
    EFX_REVERB_PRESET_UNDERWATER,
    EFX_REVERB_PRESET_DRUGGED,
    EFX_REVERB_PRESET_DIZZY,
    EFX_REVERB_PRESET_PSYCHOTIC,
}; // EFXEAXREVERBPROPERTIES

} // namespace

EaxEffectUPtr eax_create_eax_reverb_effect()
{
    return std::make_unique<EaxReverbEffect>();
}

#endif // ALSOFT_EAX
