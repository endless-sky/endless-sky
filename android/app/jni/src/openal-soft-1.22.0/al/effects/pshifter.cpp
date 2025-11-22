
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

void Pshifter_setParamf(EffectProps*, ALenum param, float)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid pitch shifter float property 0x%04x", param}; }
void Pshifter_setParamfv(EffectProps*, ALenum param, const float*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid pitch shifter float-vector property 0x%04x",
        param};
}

void Pshifter_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_PITCH_SHIFTER_COARSE_TUNE:
        if(!(val >= AL_PITCH_SHIFTER_MIN_COARSE_TUNE && val <= AL_PITCH_SHIFTER_MAX_COARSE_TUNE))
            throw effect_exception{AL_INVALID_VALUE, "Pitch shifter coarse tune out of range"};
        props->Pshifter.CoarseTune = val;
        break;

    case AL_PITCH_SHIFTER_FINE_TUNE:
        if(!(val >= AL_PITCH_SHIFTER_MIN_FINE_TUNE && val <= AL_PITCH_SHIFTER_MAX_FINE_TUNE))
            throw effect_exception{AL_INVALID_VALUE, "Pitch shifter fine tune out of range"};
        props->Pshifter.FineTune = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid pitch shifter integer property 0x%04x",
            param};
    }
}
void Pshifter_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Pshifter_setParami(props, param, vals[0]); }

void Pshifter_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_PITCH_SHIFTER_COARSE_TUNE:
        *val = props->Pshifter.CoarseTune;
        break;
    case AL_PITCH_SHIFTER_FINE_TUNE:
        *val = props->Pshifter.FineTune;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid pitch shifter integer property 0x%04x",
            param};
    }
}
void Pshifter_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Pshifter_getParami(props, param, vals); }

void Pshifter_getParamf(const EffectProps*, ALenum param, float*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid pitch shifter float property 0x%04x", param}; }
void Pshifter_getParamfv(const EffectProps*, ALenum param, float*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid pitch shifter float vector-property 0x%04x",
        param};
}

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Pshifter.CoarseTune = AL_PITCH_SHIFTER_DEFAULT_COARSE_TUNE;
    props.Pshifter.FineTune   = AL_PITCH_SHIFTER_DEFAULT_FINE_TUNE;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Pshifter);

const EffectProps PshifterEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxPitchShifterEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxPitchShifterEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxPitchShifterEffectDirtyFlagsValue lCoarseTune : 1;
    EaxPitchShifterEffectDirtyFlagsValue lFineTune : 1;
}; // EaxPitchShifterEffectDirtyFlags


class EaxPitchShifterEffect final :
    public EaxEffect
{
public:
    EaxPitchShifterEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXPITCHSHIFTERPROPERTIES eax_{};
    EAXPITCHSHIFTERPROPERTIES eax_d_{};
    EaxPitchShifterEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_coarse_tune();
    void set_efx_fine_tune();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_coarse_tune(long lCoarseTune);
    void validate_fine_tune(long lFineTune);
    void validate_all(const EAXPITCHSHIFTERPROPERTIES& all);

    void defer_coarse_tune(long lCoarseTune);
    void defer_fine_tune(long lFineTune);
    void defer_all(const EAXPITCHSHIFTERPROPERTIES& all);

    void defer_coarse_tune(const EaxEaxCall& eax_call);
    void defer_fine_tune(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxPitchShifterEffect


class EaxPitchShifterEffectException :
    public EaxException
{
public:
    explicit EaxPitchShifterEffectException(
        const char* message)
        :
        EaxException{"EAX_PITCH_SHIFTER_EFFECT", message}
    {
    }
}; // EaxPitchShifterEffectException


EaxPitchShifterEffect::EaxPitchShifterEffect()
    : EaxEffect{AL_EFFECT_PITCH_SHIFTER}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxPitchShifterEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxPitchShifterEffect::set_eax_defaults()
{
    eax_.lCoarseTune = EAXPITCHSHIFTER_DEFAULTCOARSETUNE;
    eax_.lFineTune = EAXPITCHSHIFTER_DEFAULTFINETUNE;

    eax_d_ = eax_;
}

void EaxPitchShifterEffect::set_efx_coarse_tune()
{
    const auto coarse_tune = clamp(
        static_cast<ALint>(eax_.lCoarseTune),
        AL_PITCH_SHIFTER_MIN_COARSE_TUNE,
        AL_PITCH_SHIFTER_MAX_COARSE_TUNE);

    al_effect_props_.Pshifter.CoarseTune = coarse_tune;
}

void EaxPitchShifterEffect::set_efx_fine_tune()
{
    const auto fine_tune = clamp(
        static_cast<ALint>(eax_.lFineTune),
        AL_PITCH_SHIFTER_MIN_FINE_TUNE,
        AL_PITCH_SHIFTER_MAX_FINE_TUNE);

    al_effect_props_.Pshifter.FineTune = fine_tune;
}

void EaxPitchShifterEffect::set_efx_defaults()
{
    set_efx_coarse_tune();
    set_efx_fine_tune();
}

void EaxPitchShifterEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXPITCHSHIFTER_NONE:
            break;

        case EAXPITCHSHIFTER_ALLPARAMETERS:
            eax_call.set_value<EaxPitchShifterEffectException>(eax_);
            break;

        case EAXPITCHSHIFTER_COARSETUNE:
            eax_call.set_value<EaxPitchShifterEffectException>(eax_.lCoarseTune);
            break;

        case EAXPITCHSHIFTER_FINETUNE:
            eax_call.set_value<EaxPitchShifterEffectException>(eax_.lFineTune);
            break;

        default:
            throw EaxPitchShifterEffectException{"Unsupported property id."};
    }
}

void EaxPitchShifterEffect::validate_coarse_tune(
    long lCoarseTune)
{
    eax_validate_range<EaxPitchShifterEffectException>(
        "Coarse Tune",
        lCoarseTune,
        EAXPITCHSHIFTER_MINCOARSETUNE,
        EAXPITCHSHIFTER_MAXCOARSETUNE);
}

void EaxPitchShifterEffect::validate_fine_tune(
    long lFineTune)
{
    eax_validate_range<EaxPitchShifterEffectException>(
        "Fine Tune",
        lFineTune,
        EAXPITCHSHIFTER_MINFINETUNE,
        EAXPITCHSHIFTER_MAXFINETUNE);
}

void EaxPitchShifterEffect::validate_all(
    const EAXPITCHSHIFTERPROPERTIES& all)
{
    validate_coarse_tune(all.lCoarseTune);
    validate_fine_tune(all.lFineTune);
}

void EaxPitchShifterEffect::defer_coarse_tune(
    long lCoarseTune)
{
    eax_d_.lCoarseTune = lCoarseTune;
    eax_dirty_flags_.lCoarseTune = (eax_.lCoarseTune != eax_d_.lCoarseTune);
}

void EaxPitchShifterEffect::defer_fine_tune(
    long lFineTune)
{
    eax_d_.lFineTune = lFineTune;
    eax_dirty_flags_.lFineTune = (eax_.lFineTune != eax_d_.lFineTune);
}

void EaxPitchShifterEffect::defer_all(
    const EAXPITCHSHIFTERPROPERTIES& all)
{
    defer_coarse_tune(all.lCoarseTune);
    defer_fine_tune(all.lFineTune);
}

void EaxPitchShifterEffect::defer_coarse_tune(
    const EaxEaxCall& eax_call)
{
    const auto& coarse_tune =
        eax_call.get_value<EaxPitchShifterEffectException, const decltype(EAXPITCHSHIFTERPROPERTIES::lCoarseTune)>();

    validate_coarse_tune(coarse_tune);
    defer_coarse_tune(coarse_tune);
}

void EaxPitchShifterEffect::defer_fine_tune(
    const EaxEaxCall& eax_call)
{
    const auto& fine_tune =
        eax_call.get_value<EaxPitchShifterEffectException, const decltype(EAXPITCHSHIFTERPROPERTIES::lFineTune)>();

    validate_fine_tune(fine_tune);
    defer_fine_tune(fine_tune);
}

void EaxPitchShifterEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxPitchShifterEffectException, const EAXPITCHSHIFTERPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxPitchShifterEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxPitchShifterEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.lCoarseTune)
    {
        set_efx_coarse_tune();
    }

    if (eax_dirty_flags_.lFineTune)
    {
        set_efx_fine_tune();
    }

    eax_dirty_flags_ = EaxPitchShifterEffectDirtyFlags{};

    return true;
}

void EaxPitchShifterEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXPITCHSHIFTER_NONE:
            break;

        case EAXPITCHSHIFTER_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXPITCHSHIFTER_COARSETUNE:
            defer_coarse_tune(eax_call);
            break;

        case EAXPITCHSHIFTER_FINETUNE:
            defer_fine_tune(eax_call);
            break;

        default:
            throw EaxPitchShifterEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_pitch_shifter_effect()
{
    return std::make_unique<EaxPitchShifterEffect>();
}

#endif // ALSOFT_EAX
