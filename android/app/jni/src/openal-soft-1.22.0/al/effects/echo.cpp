
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

static_assert(EchoMaxDelay >= AL_ECHO_MAX_DELAY, "Echo max delay too short");
static_assert(EchoMaxLRDelay >= AL_ECHO_MAX_LRDELAY, "Echo max left-right delay too short");

void Echo_setParami(EffectProps*, ALenum param, int)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid echo integer property 0x%04x", param}; }
void Echo_setParamiv(EffectProps*, ALenum param, const int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid echo integer-vector property 0x%04x", param}; }
void Echo_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_ECHO_DELAY:
        if(!(val >= AL_ECHO_MIN_DELAY && val <= AL_ECHO_MAX_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "Echo delay out of range"};
        props->Echo.Delay = val;
        break;

    case AL_ECHO_LRDELAY:
        if(!(val >= AL_ECHO_MIN_LRDELAY && val <= AL_ECHO_MAX_LRDELAY))
            throw effect_exception{AL_INVALID_VALUE, "Echo LR delay out of range"};
        props->Echo.LRDelay = val;
        break;

    case AL_ECHO_DAMPING:
        if(!(val >= AL_ECHO_MIN_DAMPING && val <= AL_ECHO_MAX_DAMPING))
            throw effect_exception{AL_INVALID_VALUE, "Echo damping out of range"};
        props->Echo.Damping = val;
        break;

    case AL_ECHO_FEEDBACK:
        if(!(val >= AL_ECHO_MIN_FEEDBACK && val <= AL_ECHO_MAX_FEEDBACK))
            throw effect_exception{AL_INVALID_VALUE, "Echo feedback out of range"};
        props->Echo.Feedback = val;
        break;

    case AL_ECHO_SPREAD:
        if(!(val >= AL_ECHO_MIN_SPREAD && val <= AL_ECHO_MAX_SPREAD))
            throw effect_exception{AL_INVALID_VALUE, "Echo spread out of range"};
        props->Echo.Spread = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid echo float property 0x%04x", param};
    }
}
void Echo_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Echo_setParamf(props, param, vals[0]); }

void Echo_getParami(const EffectProps*, ALenum param, int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid echo integer property 0x%04x", param}; }
void Echo_getParamiv(const EffectProps*, ALenum param, int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid echo integer-vector property 0x%04x", param}; }
void Echo_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_ECHO_DELAY:
        *val = props->Echo.Delay;
        break;

    case AL_ECHO_LRDELAY:
        *val = props->Echo.LRDelay;
        break;

    case AL_ECHO_DAMPING:
        *val = props->Echo.Damping;
        break;

    case AL_ECHO_FEEDBACK:
        *val = props->Echo.Feedback;
        break;

    case AL_ECHO_SPREAD:
        *val = props->Echo.Spread;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid echo float property 0x%04x", param};
    }
}
void Echo_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Echo_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Echo.Delay    = AL_ECHO_DEFAULT_DELAY;
    props.Echo.LRDelay  = AL_ECHO_DEFAULT_LRDELAY;
    props.Echo.Damping  = AL_ECHO_DEFAULT_DAMPING;
    props.Echo.Feedback = AL_ECHO_DEFAULT_FEEDBACK;
    props.Echo.Spread   = AL_ECHO_DEFAULT_SPREAD;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Echo);

const EffectProps EchoEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxEchoEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxEchoEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxEchoEffectDirtyFlagsValue flDelay : 1;
    EaxEchoEffectDirtyFlagsValue flLRDelay : 1;
    EaxEchoEffectDirtyFlagsValue flDamping : 1;
    EaxEchoEffectDirtyFlagsValue flFeedback : 1;
    EaxEchoEffectDirtyFlagsValue flSpread : 1;
}; // EaxEchoEffectDirtyFlags


class EaxEchoEffect final :
    public EaxEffect
{
public:
    EaxEchoEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXECHOPROPERTIES eax_{};
    EAXECHOPROPERTIES eax_d_{};
    EaxEchoEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_delay();
    void set_efx_lr_delay();
    void set_efx_damping();
    void set_efx_feedback();
    void set_efx_spread();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_delay(float flDelay);
    void validate_lr_delay(float flLRDelay);
    void validate_damping(float flDamping);
    void validate_feedback(float flFeedback);
    void validate_spread(float flSpread);
    void validate_all(const EAXECHOPROPERTIES& all);

    void defer_delay(float flDelay);
    void defer_lr_delay(float flLRDelay);
    void defer_damping(float flDamping);
    void defer_feedback(float flFeedback);
    void defer_spread(float flSpread);
    void defer_all(const EAXECHOPROPERTIES& all);

    void defer_delay(const EaxEaxCall& eax_call);
    void defer_lr_delay(const EaxEaxCall& eax_call);
    void defer_damping(const EaxEaxCall& eax_call);
    void defer_feedback(const EaxEaxCall& eax_call);
    void defer_spread(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxEchoEffect


class EaxEchoEffectException :
    public EaxException
{
public:
    explicit EaxEchoEffectException(
        const char* message)
        :
        EaxException{"EAX_ECHO_EFFECT", message}
    {
    }
}; // EaxEchoEffectException


EaxEchoEffect::EaxEchoEffect()
    : EaxEffect{AL_EFFECT_ECHO}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxEchoEffect::dispatch(
    const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxEchoEffect::set_eax_defaults()
{
    eax_.flDelay = EAXECHO_DEFAULTDELAY;
    eax_.flLRDelay = EAXECHO_DEFAULTLRDELAY;
    eax_.flDamping = EAXECHO_DEFAULTDAMPING;
    eax_.flFeedback = EAXECHO_DEFAULTFEEDBACK;
    eax_.flSpread = EAXECHO_DEFAULTSPREAD;

    eax_d_ = eax_;
}

void EaxEchoEffect::set_efx_delay()
{
    const auto delay = clamp(
        eax_.flDelay,
        AL_ECHO_MIN_DELAY,
        AL_ECHO_MAX_DELAY);

    al_effect_props_.Echo.Delay = delay;
}

void EaxEchoEffect::set_efx_lr_delay()
{
    const auto lr_delay = clamp(
        eax_.flLRDelay,
        AL_ECHO_MIN_LRDELAY,
        AL_ECHO_MAX_LRDELAY);

    al_effect_props_.Echo.LRDelay = lr_delay;
}

void EaxEchoEffect::set_efx_damping()
{
    const auto damping = clamp(
        eax_.flDamping,
        AL_ECHO_MIN_DAMPING,
        AL_ECHO_MAX_DAMPING);

    al_effect_props_.Echo.Damping = damping;
}

void EaxEchoEffect::set_efx_feedback()
{
    const auto feedback = clamp(
        eax_.flFeedback,
        AL_ECHO_MIN_FEEDBACK,
        AL_ECHO_MAX_FEEDBACK);

    al_effect_props_.Echo.Feedback = feedback;
}

void EaxEchoEffect::set_efx_spread()
{
    const auto spread = clamp(
        eax_.flSpread,
        AL_ECHO_MIN_SPREAD,
        AL_ECHO_MAX_SPREAD);

    al_effect_props_.Echo.Spread = spread;
}

void EaxEchoEffect::set_efx_defaults()
{
    set_efx_delay();
    set_efx_lr_delay();
    set_efx_damping();
    set_efx_feedback();
    set_efx_spread();
}

void EaxEchoEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXECHO_NONE:
            break;

        case EAXECHO_ALLPARAMETERS:
            eax_call.set_value<EaxEchoEffectException>(eax_);
            break;

        case EAXECHO_DELAY:
            eax_call.set_value<EaxEchoEffectException>(eax_.flDelay);
            break;

        case EAXECHO_LRDELAY:
            eax_call.set_value<EaxEchoEffectException>(eax_.flLRDelay);
            break;

        case EAXECHO_DAMPING:
            eax_call.set_value<EaxEchoEffectException>(eax_.flDamping);
            break;

        case EAXECHO_FEEDBACK:
            eax_call.set_value<EaxEchoEffectException>(eax_.flFeedback);
            break;

        case EAXECHO_SPREAD:
            eax_call.set_value<EaxEchoEffectException>(eax_.flSpread);
            break;

        default:
            throw EaxEchoEffectException{"Unsupported property id."};
    }
}

void EaxEchoEffect::validate_delay(
    float flDelay)
{
    eax_validate_range<EaxEchoEffectException>(
        "Delay",
        flDelay,
        EAXECHO_MINDELAY,
        EAXECHO_MAXDELAY);
}

void EaxEchoEffect::validate_lr_delay(
    float flLRDelay)
{
    eax_validate_range<EaxEchoEffectException>(
        "LR Delay",
        flLRDelay,
        EAXECHO_MINLRDELAY,
        EAXECHO_MAXLRDELAY);
}

void EaxEchoEffect::validate_damping(
    float flDamping)
{
    eax_validate_range<EaxEchoEffectException>(
        "Damping",
        flDamping,
        EAXECHO_MINDAMPING,
        EAXECHO_MAXDAMPING);
}

void EaxEchoEffect::validate_feedback(
    float flFeedback)
{
    eax_validate_range<EaxEchoEffectException>(
        "Feedback",
        flFeedback,
        EAXECHO_MINFEEDBACK,
        EAXECHO_MAXFEEDBACK);
}

void EaxEchoEffect::validate_spread(
    float flSpread)
{
    eax_validate_range<EaxEchoEffectException>(
        "Spread",
        flSpread,
        EAXECHO_MINSPREAD,
        EAXECHO_MAXSPREAD);
}

void EaxEchoEffect::validate_all(
    const EAXECHOPROPERTIES& all)
{
    validate_delay(all.flDelay);
    validate_lr_delay(all.flLRDelay);
    validate_damping(all.flDamping);
    validate_feedback(all.flFeedback);
    validate_spread(all.flSpread);
}

void EaxEchoEffect::defer_delay(
    float flDelay)
{
    eax_d_.flDelay = flDelay;
    eax_dirty_flags_.flDelay = (eax_.flDelay != eax_d_.flDelay);
}

void EaxEchoEffect::defer_lr_delay(
    float flLRDelay)
{
    eax_d_.flLRDelay = flLRDelay;
    eax_dirty_flags_.flLRDelay = (eax_.flLRDelay != eax_d_.flLRDelay);
}

void EaxEchoEffect::defer_damping(
    float flDamping)
{
    eax_d_.flDamping = flDamping;
    eax_dirty_flags_.flDamping = (eax_.flDamping != eax_d_.flDamping);
}

void EaxEchoEffect::defer_feedback(
    float flFeedback)
{
    eax_d_.flFeedback = flFeedback;
    eax_dirty_flags_.flFeedback = (eax_.flFeedback != eax_d_.flFeedback);
}

void EaxEchoEffect::defer_spread(
    float flSpread)
{
    eax_d_.flSpread = flSpread;
    eax_dirty_flags_.flSpread = (eax_.flSpread != eax_d_.flSpread);
}

void EaxEchoEffect::defer_all(
    const EAXECHOPROPERTIES& all)
{
    defer_delay(all.flDelay);
    defer_lr_delay(all.flLRDelay);
    defer_damping(all.flDamping);
    defer_feedback(all.flFeedback);
    defer_spread(all.flSpread);
}

void EaxEchoEffect::defer_delay(
    const EaxEaxCall& eax_call)
{
    const auto& delay =
        eax_call.get_value<EaxEchoEffectException, const decltype(EAXECHOPROPERTIES::flDelay)>();

    validate_delay(delay);
    defer_delay(delay);
}

void EaxEchoEffect::defer_lr_delay(
    const EaxEaxCall& eax_call)
{
    const auto& lr_delay =
        eax_call.get_value<EaxEchoEffectException, const decltype(EAXECHOPROPERTIES::flLRDelay)>();

    validate_lr_delay(lr_delay);
    defer_lr_delay(lr_delay);
}

void EaxEchoEffect::defer_damping(
    const EaxEaxCall& eax_call)
{
    const auto& damping =
        eax_call.get_value<EaxEchoEffectException, const decltype(EAXECHOPROPERTIES::flDamping)>();

    validate_damping(damping);
    defer_damping(damping);
}

void EaxEchoEffect::defer_feedback(
    const EaxEaxCall& eax_call)
{
    const auto& feedback =
        eax_call.get_value<EaxEchoEffectException, const decltype(EAXECHOPROPERTIES::flFeedback)>();

    validate_feedback(feedback);
    defer_feedback(feedback);
}

void EaxEchoEffect::defer_spread(
    const EaxEaxCall& eax_call)
{
    const auto& spread =
        eax_call.get_value<EaxEchoEffectException, const decltype(EAXECHOPROPERTIES::flSpread)>();

    validate_spread(spread);
    defer_spread(spread);
}

void EaxEchoEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxEchoEffectException, const EAXECHOPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxEchoEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxEchoEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.flDelay)
    {
        set_efx_delay();
    }

    if (eax_dirty_flags_.flLRDelay)
    {
        set_efx_lr_delay();
    }

    if (eax_dirty_flags_.flDamping)
    {
        set_efx_damping();
    }

    if (eax_dirty_flags_.flFeedback)
    {
        set_efx_feedback();
    }

    if (eax_dirty_flags_.flSpread)
    {
        set_efx_spread();
    }

    eax_dirty_flags_ = EaxEchoEffectDirtyFlags{};

    return true;
}

void EaxEchoEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXECHO_NONE:
            break;

        case EAXECHO_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXECHO_DELAY:
            defer_delay(eax_call);
            break;

        case EAXECHO_LRDELAY:
            defer_lr_delay(eax_call);
            break;

        case EAXECHO_DAMPING:
            defer_damping(eax_call);
            break;

        case EAXECHO_FEEDBACK:
            defer_feedback(eax_call);
            break;

        case EAXECHO_SPREAD:
            defer_spread(eax_call);
            break;

        default:
            throw EaxEchoEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_echo_effect()
{
    return std::make_unique<EaxEchoEffect>();
}

#endif // ALSOFT_EAX
