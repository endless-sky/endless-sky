
#include "config.h"

#include "AL/al.h"
#include "alc/inprogext.h"

#include "alc/effects/base.h"
#include "effects.h"


namespace {

void Convolution_setParami(EffectProps* /*props*/, ALenum param, int /*val*/)
{
    switch(param)
    {
    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid null effect integer property 0x%04x",
            param};
    }
}
void Convolution_setParamiv(EffectProps *props, ALenum param, const int *vals)
{
    switch(param)
    {
    default:
        Convolution_setParami(props, param, vals[0]);
    }
}
void Convolution_setParamf(EffectProps* /*props*/, ALenum param, float /*val*/)
{
    switch(param)
    {
    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid null effect float property 0x%04x",
            param};
    }
}
void Convolution_setParamfv(EffectProps *props, ALenum param, const float *vals)
{
    switch(param)
    {
    default:
        Convolution_setParamf(props, param, vals[0]);
    }
}

void Convolution_getParami(const EffectProps* /*props*/, ALenum param, int* /*val*/)
{
    switch(param)
    {
    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid null effect integer property 0x%04x",
            param};
    }
}
void Convolution_getParamiv(const EffectProps *props, ALenum param, int *vals)
{
    switch(param)
    {
    default:
        Convolution_getParami(props, param, vals);
    }
}
void Convolution_getParamf(const EffectProps* /*props*/, ALenum param, float* /*val*/)
{
    switch(param)
    {
    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid null effect float property 0x%04x",
            param};
    }
}
void Convolution_getParamfv(const EffectProps *props, ALenum param, float *vals)
{
    switch(param)
    {
    default:
        Convolution_getParamf(props, param, vals);
    }
}

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Convolution);

const EffectProps ConvolutionEffectProps{genDefaultProps()};
