
#include "config.h"

#include <stdexcept>

#include "AL/al.h"
#include "AL/efx.h"

#include "alc/effects/base.h"
#include "aloptional.h"
#include "effects.h"

#ifdef ALSOFT_EAX
#include <cassert>

#include "alnumeric.h"

#include "al/eax_exception.h"
#include "al/eax_utils.h"
#endif // ALSOFT_EAX


namespace {

al::optional<FShifterDirection> DirectionFromEmum(ALenum value)
{
    switch(value)
    {
    case AL_FREQUENCY_SHIFTER_DIRECTION_DOWN: return al::make_optional(FShifterDirection::Down);
    case AL_FREQUENCY_SHIFTER_DIRECTION_UP: return al::make_optional(FShifterDirection::Up);
    case AL_FREQUENCY_SHIFTER_DIRECTION_OFF: return al::make_optional(FShifterDirection::Off);
    }
    return al::nullopt;
}
ALenum EnumFromDirection(FShifterDirection dir)
{
    switch(dir)
    {
    case FShifterDirection::Down: return AL_FREQUENCY_SHIFTER_DIRECTION_DOWN;
    case FShifterDirection::Up: return AL_FREQUENCY_SHIFTER_DIRECTION_UP;
    case FShifterDirection::Off: return AL_FREQUENCY_SHIFTER_DIRECTION_OFF;
    }
    throw std::runtime_error{"Invalid direction: "+std::to_string(static_cast<int>(dir))};
}

void Fshifter_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_FREQUENCY_SHIFTER_FREQUENCY:
        if(!(val >= AL_FREQUENCY_SHIFTER_MIN_FREQUENCY && val <= AL_FREQUENCY_SHIFTER_MAX_FREQUENCY))
            throw effect_exception{AL_INVALID_VALUE, "Frequency shifter frequency out of range"};
        props->Fshifter.Frequency = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid frequency shifter float property 0x%04x",
            param};
    }
}
void Fshifter_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Fshifter_setParamf(props, param, vals[0]); }

void Fshifter_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_FREQUENCY_SHIFTER_LEFT_DIRECTION:
        if(auto diropt = DirectionFromEmum(val))
            props->Fshifter.LeftDirection = *diropt;
        else
            throw effect_exception{AL_INVALID_VALUE,
                "Unsupported frequency shifter left direction: 0x%04x", val};
        break;

    case AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION:
        if(auto diropt = DirectionFromEmum(val))
            props->Fshifter.RightDirection = *diropt;
        else
            throw effect_exception{AL_INVALID_VALUE,
                "Unsupported frequency shifter right direction: 0x%04x", val};
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM,
            "Invalid frequency shifter integer property 0x%04x", param};
    }
}
void Fshifter_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Fshifter_setParami(props, param, vals[0]); }

void Fshifter_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_FREQUENCY_SHIFTER_LEFT_DIRECTION:
        *val = EnumFromDirection(props->Fshifter.LeftDirection);
        break;
    case AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION:
        *val = EnumFromDirection(props->Fshifter.RightDirection);
        break;
    default:
        throw effect_exception{AL_INVALID_ENUM,
            "Invalid frequency shifter integer property 0x%04x", param};
    }
}
void Fshifter_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Fshifter_getParami(props, param, vals); }

void Fshifter_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_FREQUENCY_SHIFTER_FREQUENCY:
        *val = props->Fshifter.Frequency;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid frequency shifter float property 0x%04x",
            param};
    }
}
void Fshifter_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Fshifter_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Fshifter.Frequency      = AL_FREQUENCY_SHIFTER_DEFAULT_FREQUENCY;
    props.Fshifter.LeftDirection  = *DirectionFromEmum(AL_FREQUENCY_SHIFTER_DEFAULT_LEFT_DIRECTION);
    props.Fshifter.RightDirection = *DirectionFromEmum(AL_FREQUENCY_SHIFTER_DEFAULT_RIGHT_DIRECTION);
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Fshifter);

const EffectProps FshifterEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxFrequencyShifterEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxFrequencyShifterEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxFrequencyShifterEffectDirtyFlagsValue flFrequency : 1;
    EaxFrequencyShifterEffectDirtyFlagsValue ulLeftDirection : 1;
    EaxFrequencyShifterEffectDirtyFlagsValue ulRightDirection : 1;
}; // EaxFrequencyShifterEffectDirtyFlags


class EaxFrequencyShifterEffect final :
    public EaxEffect
{
public:
    EaxFrequencyShifterEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXFREQUENCYSHIFTERPROPERTIES eax_{};
    EAXFREQUENCYSHIFTERPROPERTIES eax_d_{};
    EaxFrequencyShifterEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_frequency();
    void set_efx_left_direction();
    void set_efx_right_direction();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_frequency(float flFrequency);
    void validate_left_direction(unsigned long ulLeftDirection);
    void validate_right_direction(unsigned long ulRightDirection);
    void validate_all(const EAXFREQUENCYSHIFTERPROPERTIES& all);

    void defer_frequency(float flFrequency);
    void defer_left_direction(unsigned long ulLeftDirection);
    void defer_right_direction(unsigned long ulRightDirection);
    void defer_all(const EAXFREQUENCYSHIFTERPROPERTIES& all);

    void defer_frequency(const EaxEaxCall& eax_call);
    void defer_left_direction(const EaxEaxCall& eax_call);
    void defer_right_direction(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxFrequencyShifterEffect


class EaxFrequencyShifterEffectException :
    public EaxException
{
public:
    explicit EaxFrequencyShifterEffectException(
        const char* message)
        :
        EaxException{"EAX_FREQUENCY_SHIFTER_EFFECT", message}
    {
    }
}; // EaxFrequencyShifterEffectException


EaxFrequencyShifterEffect::EaxFrequencyShifterEffect()
    : EaxEffect{AL_EFFECT_FREQUENCY_SHIFTER}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxFrequencyShifterEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxFrequencyShifterEffect::set_eax_defaults()
{
    eax_.flFrequency = EAXFREQUENCYSHIFTER_DEFAULTFREQUENCY;
    eax_.ulLeftDirection = EAXFREQUENCYSHIFTER_DEFAULTLEFTDIRECTION;
    eax_.ulRightDirection = EAXFREQUENCYSHIFTER_DEFAULTRIGHTDIRECTION;

    eax_d_ = eax_;
}

void EaxFrequencyShifterEffect::set_efx_frequency()
{
    const auto frequency = clamp(
        eax_.flFrequency,
        AL_FREQUENCY_SHIFTER_MIN_FREQUENCY,
        AL_FREQUENCY_SHIFTER_MAX_FREQUENCY);

    al_effect_props_.Fshifter.Frequency = frequency;
}

void EaxFrequencyShifterEffect::set_efx_left_direction()
{
    const auto left_direction = clamp(
        static_cast<ALint>(eax_.ulLeftDirection),
        AL_FREQUENCY_SHIFTER_MIN_LEFT_DIRECTION,
        AL_FREQUENCY_SHIFTER_MAX_LEFT_DIRECTION);

    const auto efx_left_direction = DirectionFromEmum(left_direction);
    assert(efx_left_direction.has_value());
    al_effect_props_.Fshifter.LeftDirection = *efx_left_direction;
}

void EaxFrequencyShifterEffect::set_efx_right_direction()
{
    const auto right_direction = clamp(
        static_cast<ALint>(eax_.ulRightDirection),
        AL_FREQUENCY_SHIFTER_MIN_RIGHT_DIRECTION,
        AL_FREQUENCY_SHIFTER_MAX_RIGHT_DIRECTION);

    const auto efx_right_direction = DirectionFromEmum(right_direction);
    assert(efx_right_direction.has_value());
    al_effect_props_.Fshifter.RightDirection = *efx_right_direction;
}

void EaxFrequencyShifterEffect::set_efx_defaults()
{
    set_efx_frequency();
    set_efx_left_direction();
    set_efx_right_direction();
}

void EaxFrequencyShifterEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXFREQUENCYSHIFTER_NONE:
            break;

        case EAXFREQUENCYSHIFTER_ALLPARAMETERS:
            eax_call.set_value<EaxFrequencyShifterEffectException>(eax_);
            break;

        case EAXFREQUENCYSHIFTER_FREQUENCY:
            eax_call.set_value<EaxFrequencyShifterEffectException>(eax_.flFrequency);
            break;

        case EAXFREQUENCYSHIFTER_LEFTDIRECTION:
            eax_call.set_value<EaxFrequencyShifterEffectException>(eax_.ulLeftDirection);
            break;

        case EAXFREQUENCYSHIFTER_RIGHTDIRECTION:
            eax_call.set_value<EaxFrequencyShifterEffectException>(eax_.ulRightDirection);
            break;

        default:
            throw EaxFrequencyShifterEffectException{"Unsupported property id."};
    }
}

void EaxFrequencyShifterEffect::validate_frequency(
    float flFrequency)
{
    eax_validate_range<EaxFrequencyShifterEffectException>(
        "Frequency",
        flFrequency,
        EAXFREQUENCYSHIFTER_MINFREQUENCY,
        EAXFREQUENCYSHIFTER_MAXFREQUENCY);
}

void EaxFrequencyShifterEffect::validate_left_direction(
    unsigned long ulLeftDirection)
{
    eax_validate_range<EaxFrequencyShifterEffectException>(
        "Left Direction",
        ulLeftDirection,
        EAXFREQUENCYSHIFTER_MINLEFTDIRECTION,
        EAXFREQUENCYSHIFTER_MAXLEFTDIRECTION);
}

void EaxFrequencyShifterEffect::validate_right_direction(
    unsigned long ulRightDirection)
{
    eax_validate_range<EaxFrequencyShifterEffectException>(
        "Right Direction",
        ulRightDirection,
        EAXFREQUENCYSHIFTER_MINRIGHTDIRECTION,
        EAXFREQUENCYSHIFTER_MAXRIGHTDIRECTION);
}

void EaxFrequencyShifterEffect::validate_all(
    const EAXFREQUENCYSHIFTERPROPERTIES& all)
{
    validate_frequency(all.flFrequency);
    validate_left_direction(all.ulLeftDirection);
    validate_right_direction(all.ulRightDirection);
}

void EaxFrequencyShifterEffect::defer_frequency(
    float flFrequency)
{
    eax_d_.flFrequency = flFrequency;
    eax_dirty_flags_.flFrequency = (eax_.flFrequency != eax_d_.flFrequency);
}

void EaxFrequencyShifterEffect::defer_left_direction(
    unsigned long ulLeftDirection)
{
    eax_d_.ulLeftDirection = ulLeftDirection;
    eax_dirty_flags_.ulLeftDirection = (eax_.ulLeftDirection != eax_d_.ulLeftDirection);
}

void EaxFrequencyShifterEffect::defer_right_direction(
    unsigned long ulRightDirection)
{
    eax_d_.ulRightDirection = ulRightDirection;
    eax_dirty_flags_.ulRightDirection = (eax_.ulRightDirection != eax_d_.ulRightDirection);
}

void EaxFrequencyShifterEffect::defer_all(
    const EAXFREQUENCYSHIFTERPROPERTIES& all)
{
    defer_frequency(all.flFrequency);
    defer_left_direction(all.ulLeftDirection);
    defer_right_direction(all.ulRightDirection);
}

void EaxFrequencyShifterEffect::defer_frequency(
    const EaxEaxCall& eax_call)
{
    const auto& frequency =
        eax_call.get_value<
        EaxFrequencyShifterEffectException, const decltype(EAXFREQUENCYSHIFTERPROPERTIES::flFrequency)>();

    validate_frequency(frequency);
    defer_frequency(frequency);
}

void EaxFrequencyShifterEffect::defer_left_direction(
    const EaxEaxCall& eax_call)
{
    const auto& left_direction =
        eax_call.get_value<
        EaxFrequencyShifterEffectException, const decltype(EAXFREQUENCYSHIFTERPROPERTIES::ulLeftDirection)>();

    validate_left_direction(left_direction);
    defer_left_direction(left_direction);
}

void EaxFrequencyShifterEffect::defer_right_direction(
    const EaxEaxCall& eax_call)
{
    const auto& right_direction =
        eax_call.get_value<
        EaxFrequencyShifterEffectException, const decltype(EAXFREQUENCYSHIFTERPROPERTIES::ulRightDirection)>();

    validate_right_direction(right_direction);
    defer_right_direction(right_direction);
}

void EaxFrequencyShifterEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<
        EaxFrequencyShifterEffectException, const EAXFREQUENCYSHIFTERPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxFrequencyShifterEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxFrequencyShifterEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.flFrequency)
    {
        set_efx_frequency();
    }

    if (eax_dirty_flags_.ulLeftDirection)
    {
        set_efx_left_direction();
    }

    if (eax_dirty_flags_.ulRightDirection)
    {
        set_efx_right_direction();
    }

    eax_dirty_flags_ = EaxFrequencyShifterEffectDirtyFlags{};

    return true;
}

void EaxFrequencyShifterEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXFREQUENCYSHIFTER_NONE:
            break;

        case EAXFREQUENCYSHIFTER_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXFREQUENCYSHIFTER_FREQUENCY:
            defer_frequency(eax_call);
            break;

        case EAXFREQUENCYSHIFTER_LEFTDIRECTION:
            defer_left_direction(eax_call);
            break;

        case EAXFREQUENCYSHIFTER_RIGHTDIRECTION:
            defer_right_direction(eax_call);
            break;

        default:
            throw EaxFrequencyShifterEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_frequency_shifter_effect()
{
    return std::make_unique<EaxFrequencyShifterEffect>();
}

#endif // ALSOFT_EAX
