
#include "config.h"

#include <cmath>

#include "AL/al.h"
#include "AL/alext.h"

#include "alc/effects/base.h"
#include "effects.h"


namespace {

void Dedicated_setParami(EffectProps*, ALenum param, int)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid dedicated integer property 0x%04x", param}; }
void Dedicated_setParamiv(EffectProps*, ALenum param, const int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid dedicated integer-vector property 0x%04x",
        param};
}
void Dedicated_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_DEDICATED_GAIN:
        if(!(val >= 0.0f && std::isfinite(val)))
            throw effect_exception{AL_INVALID_VALUE, "Dedicated gain out of range"};
        props->Dedicated.Gain = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid dedicated float property 0x%04x", param};
    }
}
void Dedicated_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Dedicated_setParamf(props, param, vals[0]); }

void Dedicated_getParami(const EffectProps*, ALenum param, int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid dedicated integer property 0x%04x", param}; }
void Dedicated_getParamiv(const EffectProps*, ALenum param, int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid dedicated integer-vector property 0x%04x",
        param};
}
void Dedicated_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_DEDICATED_GAIN:
        *val = props->Dedicated.Gain;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid dedicated float property 0x%04x", param};
    }
}
void Dedicated_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Dedicated_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Dedicated.Gain = 1.0f;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Dedicated);

const EffectProps DedicatedEffectProps{genDefaultProps()};
