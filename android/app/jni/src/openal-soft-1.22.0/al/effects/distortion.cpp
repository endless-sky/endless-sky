
#include "config.h"

#include "AL/al.h"
#include "AL/efx.h"

#include "alc/effects/base.h"
#include "effects.h"

#ifdef ALSOFT_EAX
#include "alnumeric.h"

#include "al/eax_exception.h"
#include "al/eax_utils.h"
#endif // ALSOFT_EAX


namespace {

void Distortion_setParami(EffectProps*, ALenum param, int)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid distortion integer property 0x%04x", param}; }
void Distortion_setParamiv(EffectProps*, ALenum param, const int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid distortion integer-vector property 0x%04x",
        param};
}
void Distortion_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_DISTORTION_EDGE:
        if(!(val >= AL_DISTORTION_MIN_EDGE && val <= AL_DISTORTION_MAX_EDGE))
            throw effect_exception{AL_INVALID_VALUE, "Distortion edge out of range"};
        props->Distortion.Edge = val;
        break;

    case AL_DISTORTION_GAIN:
        if(!(val >= AL_DISTORTION_MIN_GAIN && val <= AL_DISTORTION_MAX_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Distortion gain out of range"};
        props->Distortion.Gain = val;
        break;

    case AL_DISTORTION_LOWPASS_CUTOFF:
        if(!(val >= AL_DISTORTION_MIN_LOWPASS_CUTOFF && val <= AL_DISTORTION_MAX_LOWPASS_CUTOFF))
            throw effect_exception{AL_INVALID_VALUE, "Distortion low-pass cutoff out of range"};
        props->Distortion.LowpassCutoff = val;
        break;

    case AL_DISTORTION_EQCENTER:
        if(!(val >= AL_DISTORTION_MIN_EQCENTER && val <= AL_DISTORTION_MAX_EQCENTER))
            throw effect_exception{AL_INVALID_VALUE, "Distortion EQ center out of range"};
        props->Distortion.EQCenter = val;
        break;

    case AL_DISTORTION_EQBANDWIDTH:
        if(!(val >= AL_DISTORTION_MIN_EQBANDWIDTH && val <= AL_DISTORTION_MAX_EQBANDWIDTH))
            throw effect_exception{AL_INVALID_VALUE, "Distortion EQ bandwidth out of range"};
        props->Distortion.EQBandwidth = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid distortion float property 0x%04x", param};
    }
}
void Distortion_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Distortion_setParamf(props, param, vals[0]); }

void Distortion_getParami(const EffectProps*, ALenum param, int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid distortion integer property 0x%04x", param}; }
void Distortion_getParamiv(const EffectProps*, ALenum param, int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid distortion integer-vector property 0x%04x",
        param};
}
void Distortion_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_DISTORTION_EDGE:
        *val = props->Distortion.Edge;
        break;

    case AL_DISTORTION_GAIN:
        *val = props->Distortion.Gain;
        break;

    case AL_DISTORTION_LOWPASS_CUTOFF:
        *val = props->Distortion.LowpassCutoff;
        break;

    case AL_DISTORTION_EQCENTER:
        *val = props->Distortion.EQCenter;
        break;

    case AL_DISTORTION_EQBANDWIDTH:
        *val = props->Distortion.EQBandwidth;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid distortion float property 0x%04x", param};
    }
}
void Distortion_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Distortion_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Distortion.Edge = AL_DISTORTION_DEFAULT_EDGE;
    props.Distortion.Gain = AL_DISTORTION_DEFAULT_GAIN;
    props.Distortion.LowpassCutoff = AL_DISTORTION_DEFAULT_LOWPASS_CUTOFF;
    props.Distortion.EQCenter = AL_DISTORTION_DEFAULT_EQCENTER;
    props.Distortion.EQBandwidth = AL_DISTORTION_DEFAULT_EQBANDWIDTH;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Distortion);

const EffectProps DistortionEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxDistortionEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxDistortionEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxDistortionEffectDirtyFlagsValue flEdge : 1;
    EaxDistortionEffectDirtyFlagsValue lGain : 1;
    EaxDistortionEffectDirtyFlagsValue flLowPassCutOff : 1;
    EaxDistortionEffectDirtyFlagsValue flEQCenter : 1;
    EaxDistortionEffectDirtyFlagsValue flEQBandwidth : 1;
}; // EaxDistortionEffectDirtyFlags


class EaxDistortionEffect final :
    public EaxEffect
{
public:
    EaxDistortionEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXDISTORTIONPROPERTIES eax_{};
    EAXDISTORTIONPROPERTIES eax_d_{};
    EaxDistortionEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_edge();
    void set_efx_gain();
    void set_efx_lowpass_cutoff();
    void set_efx_eq_center();
    void set_efx_eq_bandwidth();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_edge(float flEdge);
    void validate_gain(long lGain);
    void validate_lowpass_cutoff(float flLowPassCutOff);
    void validate_eq_center(float flEQCenter);
    void validate_eq_bandwidth(float flEQBandwidth);
    void validate_all(const EAXDISTORTIONPROPERTIES& eax_all);

    void defer_edge(float flEdge);
    void defer_gain(long lGain);
    void defer_low_pass_cutoff(float flLowPassCutOff);
    void defer_eq_center(float flEQCenter);
    void defer_eq_bandwidth(float flEQBandwidth);
    void defer_all(const EAXDISTORTIONPROPERTIES& eax_all);

    void defer_edge(const EaxEaxCall& eax_call);
    void defer_gain(const EaxEaxCall& eax_call);
    void defer_low_pass_cutoff(const EaxEaxCall& eax_call);
    void defer_eq_center(const EaxEaxCall& eax_call);
    void defer_eq_bandwidth(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxDistortionEffect


class EaxDistortionEffectException :
    public EaxException
{
public:
    explicit EaxDistortionEffectException(
        const char* message)
        :
        EaxException{"EAX_DISTORTION_EFFECT", message}
    {
    }
}; // EaxDistortionEffectException


EaxDistortionEffect::EaxDistortionEffect()
    : EaxEffect{AL_EFFECT_DISTORTION}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxDistortionEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxDistortionEffect::set_eax_defaults()
{
    eax_.flEdge = EAXDISTORTION_DEFAULTEDGE;
    eax_.lGain = EAXDISTORTION_DEFAULTGAIN;
    eax_.flLowPassCutOff = EAXDISTORTION_DEFAULTLOWPASSCUTOFF;
    eax_.flEQCenter = EAXDISTORTION_DEFAULTEQCENTER;
    eax_.flEQBandwidth = EAXDISTORTION_DEFAULTEQBANDWIDTH;

    eax_d_ = eax_;
}

void EaxDistortionEffect::set_efx_edge()
{
    const auto edge = clamp(
        eax_.flEdge,
        AL_DISTORTION_MIN_EDGE,
        AL_DISTORTION_MAX_EDGE);

    al_effect_props_.Distortion.Edge = edge;
}

void EaxDistortionEffect::set_efx_gain()
{
    const auto gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lGain)),
        AL_DISTORTION_MIN_GAIN,
        AL_DISTORTION_MAX_GAIN);

    al_effect_props_.Distortion.Gain = gain;
}

void EaxDistortionEffect::set_efx_lowpass_cutoff()
{
    const auto lowpass_cutoff = clamp(
        eax_.flLowPassCutOff,
        AL_DISTORTION_MIN_LOWPASS_CUTOFF,
        AL_DISTORTION_MAX_LOWPASS_CUTOFF);

    al_effect_props_.Distortion.LowpassCutoff = lowpass_cutoff;
}

void EaxDistortionEffect::set_efx_eq_center()
{
    const auto eq_center = clamp(
        eax_.flEQCenter,
        AL_DISTORTION_MIN_EQCENTER,
        AL_DISTORTION_MAX_EQCENTER);

    al_effect_props_.Distortion.EQCenter = eq_center;
}

void EaxDistortionEffect::set_efx_eq_bandwidth()
{
    const auto eq_bandwidth = clamp(
        eax_.flEdge,
        AL_DISTORTION_MIN_EQBANDWIDTH,
        AL_DISTORTION_MAX_EQBANDWIDTH);

    al_effect_props_.Distortion.EQBandwidth = eq_bandwidth;
}

void EaxDistortionEffect::set_efx_defaults()
{
    set_efx_edge();
    set_efx_gain();
    set_efx_lowpass_cutoff();
    set_efx_eq_center();
    set_efx_eq_bandwidth();
}

void EaxDistortionEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXDISTORTION_NONE:
            break;

        case EAXDISTORTION_ALLPARAMETERS:
            eax_call.set_value<EaxDistortionEffectException>(eax_);
            break;

        case EAXDISTORTION_EDGE:
            eax_call.set_value<EaxDistortionEffectException>(eax_.flEdge);
            break;

        case EAXDISTORTION_GAIN:
            eax_call.set_value<EaxDistortionEffectException>(eax_.lGain);
            break;

        case EAXDISTORTION_LOWPASSCUTOFF:
            eax_call.set_value<EaxDistortionEffectException>(eax_.flLowPassCutOff);
            break;

        case EAXDISTORTION_EQCENTER:
            eax_call.set_value<EaxDistortionEffectException>(eax_.flEQCenter);
            break;

        case EAXDISTORTION_EQBANDWIDTH:
            eax_call.set_value<EaxDistortionEffectException>(eax_.flEQBandwidth);
            break;

        default:
            throw EaxDistortionEffectException{"Unsupported property id."};
    }
}

void EaxDistortionEffect::validate_edge(
    float flEdge)
{
    eax_validate_range<EaxDistortionEffectException>(
        "Edge",
        flEdge,
        EAXDISTORTION_MINEDGE,
        EAXDISTORTION_MAXEDGE);
}

void EaxDistortionEffect::validate_gain(
    long lGain)
{
    eax_validate_range<EaxDistortionEffectException>(
        "Gain",
        lGain,
        EAXDISTORTION_MINGAIN,
        EAXDISTORTION_MAXGAIN);
}

void EaxDistortionEffect::validate_lowpass_cutoff(
    float flLowPassCutOff)
{
    eax_validate_range<EaxDistortionEffectException>(
        "Low-pass Cut-off",
        flLowPassCutOff,
        EAXDISTORTION_MINLOWPASSCUTOFF,
        EAXDISTORTION_MAXLOWPASSCUTOFF);
}

void EaxDistortionEffect::validate_eq_center(
    float flEQCenter)
{
    eax_validate_range<EaxDistortionEffectException>(
        "EQ Center",
        flEQCenter,
        EAXDISTORTION_MINEQCENTER,
        EAXDISTORTION_MAXEQCENTER);
}

void EaxDistortionEffect::validate_eq_bandwidth(
    float flEQBandwidth)
{
    eax_validate_range<EaxDistortionEffectException>(
        "EQ Bandwidth",
        flEQBandwidth,
        EAXDISTORTION_MINEQBANDWIDTH,
        EAXDISTORTION_MAXEQBANDWIDTH);
}

void EaxDistortionEffect::validate_all(
    const EAXDISTORTIONPROPERTIES& eax_all)
{
    validate_edge(eax_all.flEdge);
    validate_gain(eax_all.lGain);
    validate_lowpass_cutoff(eax_all.flLowPassCutOff);
    validate_eq_center(eax_all.flEQCenter);
    validate_eq_bandwidth(eax_all.flEQBandwidth);
}

void EaxDistortionEffect::defer_edge(
    float flEdge)
{
    eax_d_.flEdge = flEdge;
    eax_dirty_flags_.flEdge = (eax_.flEdge != eax_d_.flEdge);
}

void EaxDistortionEffect::defer_gain(
    long lGain)
{
    eax_d_.lGain = lGain;
    eax_dirty_flags_.lGain = (eax_.lGain != eax_d_.lGain);
}

void EaxDistortionEffect::defer_low_pass_cutoff(
    float flLowPassCutOff)
{
    eax_d_.flLowPassCutOff = flLowPassCutOff;
    eax_dirty_flags_.flLowPassCutOff = (eax_.flLowPassCutOff != eax_d_.flLowPassCutOff);
}

void EaxDistortionEffect::defer_eq_center(
    float flEQCenter)
{
    eax_d_.flEQCenter = flEQCenter;
    eax_dirty_flags_.flEQCenter = (eax_.flEQCenter != eax_d_.flEQCenter);
}

void EaxDistortionEffect::defer_eq_bandwidth(
    float flEQBandwidth)
{
    eax_d_.flEQBandwidth = flEQBandwidth;
    eax_dirty_flags_.flEQBandwidth = (eax_.flEQBandwidth != eax_d_.flEQBandwidth);
}

void EaxDistortionEffect::defer_all(
    const EAXDISTORTIONPROPERTIES& eax_all)
{
    defer_edge(eax_all.flEdge);
    defer_gain(eax_all.lGain);
    defer_low_pass_cutoff(eax_all.flLowPassCutOff);
    defer_eq_center(eax_all.flEQCenter);
    defer_eq_bandwidth(eax_all.flEQBandwidth);
}

void EaxDistortionEffect::defer_edge(
    const EaxEaxCall& eax_call)
{
    const auto& edge =
        eax_call.get_value<EaxDistortionEffectException, const decltype(EAXDISTORTIONPROPERTIES::flEdge)>();

    validate_edge(edge);
    defer_edge(edge);
}

void EaxDistortionEffect::defer_gain(
    const EaxEaxCall& eax_call)
{
    const auto& gain =
        eax_call.get_value<EaxDistortionEffectException, const decltype(EAXDISTORTIONPROPERTIES::lGain)>();

    validate_gain(gain);
    defer_gain(gain);
}

void EaxDistortionEffect::defer_low_pass_cutoff(
    const EaxEaxCall& eax_call)
{
    const auto& lowpass_cutoff =
        eax_call.get_value<EaxDistortionEffectException, const decltype(EAXDISTORTIONPROPERTIES::flLowPassCutOff)>();

    validate_lowpass_cutoff(lowpass_cutoff);
    defer_low_pass_cutoff(lowpass_cutoff);
}

void EaxDistortionEffect::defer_eq_center(
    const EaxEaxCall& eax_call)
{
    const auto& eq_center =
        eax_call.get_value<EaxDistortionEffectException, const decltype(EAXDISTORTIONPROPERTIES::flEQCenter)>();

    validate_eq_center(eq_center);
    defer_eq_center(eq_center);
}

void EaxDistortionEffect::defer_eq_bandwidth(
    const EaxEaxCall& eax_call)
{
    const auto& eq_bandwidth =
        eax_call.get_value<EaxDistortionEffectException, const decltype(EAXDISTORTIONPROPERTIES::flEQBandwidth)>();

    validate_eq_bandwidth(eq_bandwidth);
    defer_eq_bandwidth(eq_bandwidth);
}

void EaxDistortionEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxDistortionEffectException, const EAXDISTORTIONPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxDistortionEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxDistortionEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.flEdge)
    {
        set_efx_edge();
    }

    if (eax_dirty_flags_.lGain)
    {
        set_efx_gain();
    }

    if (eax_dirty_flags_.flLowPassCutOff)
    {
        set_efx_lowpass_cutoff();
    }

    if (eax_dirty_flags_.flEQCenter)
    {
        set_efx_eq_center();
    }

    if (eax_dirty_flags_.flEQBandwidth)
    {
        set_efx_eq_bandwidth();
    }

    eax_dirty_flags_ = EaxDistortionEffectDirtyFlags{};

    return true;
}

void EaxDistortionEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXDISTORTION_NONE:
            break;

        case EAXDISTORTION_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXDISTORTION_EDGE:
            defer_edge(eax_call);
            break;

        case EAXDISTORTION_GAIN:
            defer_gain(eax_call);
            break;

        case EAXDISTORTION_LOWPASSCUTOFF:
            defer_low_pass_cutoff(eax_call);
            break;

        case EAXDISTORTION_EQCENTER:
            defer_eq_center(eax_call);
            break;

        case EAXDISTORTION_EQBANDWIDTH:
            defer_eq_bandwidth(eax_call);
            break;

        default:
            throw EaxDistortionEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_distortion_effect()
{
    return std::make_unique<EaxDistortionEffect>();
}

#endif // ALSOFT_EAX
