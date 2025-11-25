
#include "config.h"

#include <stdexcept>

#include "AL/al.h"
#include "AL/efx.h"

#include "alc/effects/base.h"
#include "aloptional.h"
#include "core/logging.h"
#include "effects.h"

#ifdef ALSOFT_EAX
#include <cassert>

#include "alnumeric.h"

#include "al/eax_exception.h"
#include "al/eax_utils.h"
#endif // ALSOFT_EAX


namespace {

static_assert(ChorusMaxDelay >= AL_CHORUS_MAX_DELAY, "Chorus max delay too small");
static_assert(FlangerMaxDelay >= AL_FLANGER_MAX_DELAY, "Flanger max delay too small");

static_assert(AL_CHORUS_WAVEFORM_SINUSOID == AL_FLANGER_WAVEFORM_SINUSOID, "Chorus/Flanger waveform value mismatch");
static_assert(AL_CHORUS_WAVEFORM_TRIANGLE == AL_FLANGER_WAVEFORM_TRIANGLE, "Chorus/Flanger waveform value mismatch");

inline al::optional<ChorusWaveform> WaveformFromEnum(ALenum type)
{
    switch(type)
    {
    case AL_CHORUS_WAVEFORM_SINUSOID: return al::make_optional(ChorusWaveform::Sinusoid);
    case AL_CHORUS_WAVEFORM_TRIANGLE: return al::make_optional(ChorusWaveform::Triangle);
    }
    return al::nullopt;
}
inline ALenum EnumFromWaveform(ChorusWaveform type)
{
    switch(type)
    {
    case ChorusWaveform::Sinusoid: return AL_CHORUS_WAVEFORM_SINUSOID;
    case ChorusWaveform::Triangle: return AL_CHORUS_WAVEFORM_TRIANGLE;
    }
    throw std::runtime_error{"Invalid chorus waveform: "+std::to_string(static_cast<int>(type))};
}

void Chorus_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_CHORUS_WAVEFORM:
        if(auto formopt = WaveformFromEnum(val))
            props->Chorus.Waveform = *formopt;
        else
            throw effect_exception{AL_INVALID_VALUE, "Invalid chorus waveform: 0x%04x", val};
        break;

    case AL_CHORUS_PHASE:
        if(!(val >= AL_CHORUS_MIN_PHASE && val <= AL_CHORUS_MAX_PHASE))
            throw effect_exception{AL_INVALID_VALUE, "Chorus phase out of range: %d", val};
        props->Chorus.Phase = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid chorus integer property 0x%04x", param};
    }
}
void Chorus_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Chorus_setParami(props, param, vals[0]); }
void Chorus_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_CHORUS_RATE:
        if(!(val >= AL_CHORUS_MIN_RATE && val <= AL_CHORUS_MAX_RATE))
            throw effect_exception{AL_INVALID_VALUE, "Chorus rate out of range: %f", val};
        props->Chorus.Rate = val;
        break;

    case AL_CHORUS_DEPTH:
        if(!(val >= AL_CHORUS_MIN_DEPTH && val <= AL_CHORUS_MAX_DEPTH))
            throw effect_exception{AL_INVALID_VALUE, "Chorus depth out of range: %f", val};
        props->Chorus.Depth = val;
        break;

    case AL_CHORUS_FEEDBACK:
        if(!(val >= AL_CHORUS_MIN_FEEDBACK && val <= AL_CHORUS_MAX_FEEDBACK))
            throw effect_exception{AL_INVALID_VALUE, "Chorus feedback out of range: %f", val};
        props->Chorus.Feedback = val;
        break;

    case AL_CHORUS_DELAY:
        if(!(val >= AL_CHORUS_MIN_DELAY && val <= AL_CHORUS_MAX_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "Chorus delay out of range: %f", val};
        props->Chorus.Delay = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid chorus float property 0x%04x", param};
    }
}
void Chorus_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Chorus_setParamf(props, param, vals[0]); }

void Chorus_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_CHORUS_WAVEFORM:
        *val = EnumFromWaveform(props->Chorus.Waveform);
        break;

    case AL_CHORUS_PHASE:
        *val = props->Chorus.Phase;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid chorus integer property 0x%04x", param};
    }
}
void Chorus_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Chorus_getParami(props, param, vals); }
void Chorus_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_CHORUS_RATE:
        *val = props->Chorus.Rate;
        break;

    case AL_CHORUS_DEPTH:
        *val = props->Chorus.Depth;
        break;

    case AL_CHORUS_FEEDBACK:
        *val = props->Chorus.Feedback;
        break;

    case AL_CHORUS_DELAY:
        *val = props->Chorus.Delay;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid chorus float property 0x%04x", param};
    }
}
void Chorus_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Chorus_getParamf(props, param, vals); }

const EffectProps genDefaultChorusProps() noexcept
{
    EffectProps props{};
    props.Chorus.Waveform = *WaveformFromEnum(AL_CHORUS_DEFAULT_WAVEFORM);
    props.Chorus.Phase = AL_CHORUS_DEFAULT_PHASE;
    props.Chorus.Rate = AL_CHORUS_DEFAULT_RATE;
    props.Chorus.Depth = AL_CHORUS_DEFAULT_DEPTH;
    props.Chorus.Feedback = AL_CHORUS_DEFAULT_FEEDBACK;
    props.Chorus.Delay = AL_CHORUS_DEFAULT_DELAY;
    return props;
}


void Flanger_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_FLANGER_WAVEFORM:
        if(auto formopt = WaveformFromEnum(val))
            props->Chorus.Waveform = *formopt;
        else
            throw effect_exception{AL_INVALID_VALUE, "Invalid flanger waveform: 0x%04x", val};
        break;

    case AL_FLANGER_PHASE:
        if(!(val >= AL_FLANGER_MIN_PHASE && val <= AL_FLANGER_MAX_PHASE))
            throw effect_exception{AL_INVALID_VALUE, "Flanger phase out of range: %d", val};
        props->Chorus.Phase = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid flanger integer property 0x%04x", param};
    }
}
void Flanger_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Flanger_setParami(props, param, vals[0]); }
void Flanger_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_FLANGER_RATE:
        if(!(val >= AL_FLANGER_MIN_RATE && val <= AL_FLANGER_MAX_RATE))
            throw effect_exception{AL_INVALID_VALUE, "Flanger rate out of range: %f", val};
        props->Chorus.Rate = val;
        break;

    case AL_FLANGER_DEPTH:
        if(!(val >= AL_FLANGER_MIN_DEPTH && val <= AL_FLANGER_MAX_DEPTH))
            throw effect_exception{AL_INVALID_VALUE, "Flanger depth out of range: %f", val};
        props->Chorus.Depth = val;
        break;

    case AL_FLANGER_FEEDBACK:
        if(!(val >= AL_FLANGER_MIN_FEEDBACK && val <= AL_FLANGER_MAX_FEEDBACK))
            throw effect_exception{AL_INVALID_VALUE, "Flanger feedback out of range: %f", val};
        props->Chorus.Feedback = val;
        break;

    case AL_FLANGER_DELAY:
        if(!(val >= AL_FLANGER_MIN_DELAY && val <= AL_FLANGER_MAX_DELAY))
            throw effect_exception{AL_INVALID_VALUE, "Flanger delay out of range: %f", val};
        props->Chorus.Delay = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid flanger float property 0x%04x", param};
    }
}
void Flanger_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Flanger_setParamf(props, param, vals[0]); }

void Flanger_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_FLANGER_WAVEFORM:
        *val = EnumFromWaveform(props->Chorus.Waveform);
        break;

    case AL_FLANGER_PHASE:
        *val = props->Chorus.Phase;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid flanger integer property 0x%04x", param};
    }
}
void Flanger_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Flanger_getParami(props, param, vals); }
void Flanger_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_FLANGER_RATE:
        *val = props->Chorus.Rate;
        break;

    case AL_FLANGER_DEPTH:
        *val = props->Chorus.Depth;
        break;

    case AL_FLANGER_FEEDBACK:
        *val = props->Chorus.Feedback;
        break;

    case AL_FLANGER_DELAY:
        *val = props->Chorus.Delay;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid flanger float property 0x%04x", param};
    }
}
void Flanger_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Flanger_getParamf(props, param, vals); }

EffectProps genDefaultFlangerProps() noexcept
{
    EffectProps props{};
    props.Chorus.Waveform = *WaveformFromEnum(AL_FLANGER_DEFAULT_WAVEFORM);
    props.Chorus.Phase = AL_FLANGER_DEFAULT_PHASE;
    props.Chorus.Rate = AL_FLANGER_DEFAULT_RATE;
    props.Chorus.Depth = AL_FLANGER_DEFAULT_DEPTH;
    props.Chorus.Feedback = AL_FLANGER_DEFAULT_FEEDBACK;
    props.Chorus.Delay = AL_FLANGER_DEFAULT_DELAY;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Chorus);

const EffectProps ChorusEffectProps{genDefaultChorusProps()};

DEFINE_ALEFFECT_VTABLE(Flanger);

const EffectProps FlangerEffectProps{genDefaultFlangerProps()};


#ifdef ALSOFT_EAX
namespace {

void eax_set_efx_waveform(
    ALenum waveform,
    EffectProps& al_effect_props)
{
    const auto efx_waveform = WaveformFromEnum(waveform);
    assert(efx_waveform.has_value());
    al_effect_props.Chorus.Waveform = *efx_waveform;
}

void eax_set_efx_phase(
    ALint phase,
    EffectProps& al_effect_props)
{
    al_effect_props.Chorus.Phase = phase;
}

void eax_set_efx_rate(
    ALfloat rate,
    EffectProps& al_effect_props)
{
    al_effect_props.Chorus.Rate = rate;
}

void eax_set_efx_depth(
    ALfloat depth,
    EffectProps& al_effect_props)
{
    al_effect_props.Chorus.Depth = depth;
}

void eax_set_efx_feedback(
    ALfloat feedback,
    EffectProps& al_effect_props)
{
    al_effect_props.Chorus.Feedback = feedback;
}

void eax_set_efx_delay(
    ALfloat delay,
    EffectProps& al_effect_props)
{
    al_effect_props.Chorus.Delay = delay;
}


using EaxChorusEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxChorusEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxChorusEffectDirtyFlagsValue ulWaveform : 1;
    EaxChorusEffectDirtyFlagsValue lPhase : 1;
    EaxChorusEffectDirtyFlagsValue flRate : 1;
    EaxChorusEffectDirtyFlagsValue flDepth : 1;
    EaxChorusEffectDirtyFlagsValue flFeedback : 1;
    EaxChorusEffectDirtyFlagsValue flDelay : 1;
}; // EaxChorusEffectDirtyFlags


class EaxChorusEffect final :
    public EaxEffect
{
public:
    EaxChorusEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXCHORUSPROPERTIES eax_{};
    EAXCHORUSPROPERTIES eax_d_{};
    EaxChorusEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults() noexcept;

    void set_efx_waveform();
    void set_efx_phase();
    void set_efx_rate();
    void set_efx_depth();
    void set_efx_feedback();
    void set_efx_delay();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_waveform(unsigned long ulWaveform);
    void validate_phase(long lPhase);
    void validate_rate(float flRate);
    void validate_depth(float flDepth);
    void validate_feedback(float flFeedback);
    void validate_delay(float flDelay);
    void validate_all(const EAXCHORUSPROPERTIES& eax_all);

    void defer_waveform(unsigned long ulWaveform);
    void defer_phase(long lPhase);
    void defer_rate(float flRate);
    void defer_depth(float flDepth);
    void defer_feedback(float flFeedback);
    void defer_delay(float flDelay);
    void defer_all(const EAXCHORUSPROPERTIES& eax_all);

    void defer_waveform(const EaxEaxCall& eax_call);
    void defer_phase(const EaxEaxCall& eax_call);
    void defer_rate(const EaxEaxCall& eax_call);
    void defer_depth(const EaxEaxCall& eax_call);
    void defer_feedback(const EaxEaxCall& eax_call);
    void defer_delay(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxChorusEffect


class EaxChorusEffectException :
    public EaxException
{
public:
    explicit EaxChorusEffectException(
        const char* message)
        :
        EaxException{"EAX_CHORUS_EFFECT", message}
    {
    }
}; // EaxChorusEffectException


EaxChorusEffect::EaxChorusEffect()
    : EaxEffect{AL_EFFECT_CHORUS}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxChorusEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxChorusEffect::set_eax_defaults() noexcept
{
    eax_.ulWaveform = EAXCHORUS_DEFAULTWAVEFORM;
    eax_.lPhase = EAXCHORUS_DEFAULTPHASE;
    eax_.flRate = EAXCHORUS_DEFAULTRATE;
    eax_.flDepth = EAXCHORUS_DEFAULTDEPTH;
    eax_.flFeedback = EAXCHORUS_DEFAULTFEEDBACK;
    eax_.flDelay = EAXCHORUS_DEFAULTDELAY;

    eax_d_ = eax_;
}

void EaxChorusEffect::set_efx_waveform()
{
    const auto waveform = clamp(
        static_cast<ALint>(eax_.ulWaveform),
        AL_CHORUS_MIN_WAVEFORM,
        AL_CHORUS_MAX_WAVEFORM);

    eax_set_efx_waveform(waveform, al_effect_props_);
}

void EaxChorusEffect::set_efx_phase()
{
    const auto phase = clamp(
        static_cast<ALint>(eax_.lPhase),
        AL_CHORUS_MIN_PHASE,
        AL_CHORUS_MAX_PHASE);

    eax_set_efx_phase(phase, al_effect_props_);
}

void EaxChorusEffect::set_efx_rate()
{
    const auto rate = clamp(
        eax_.flRate,
        AL_CHORUS_MIN_RATE,
        AL_CHORUS_MAX_RATE);

    eax_set_efx_rate(rate, al_effect_props_);
}

void EaxChorusEffect::set_efx_depth()
{
    const auto depth = clamp(
        eax_.flDepth,
        AL_CHORUS_MIN_DEPTH,
        AL_CHORUS_MAX_DEPTH);

    eax_set_efx_depth(depth, al_effect_props_);
}

void EaxChorusEffect::set_efx_feedback()
{
    const auto feedback = clamp(
        eax_.flFeedback,
        AL_CHORUS_MIN_FEEDBACK,
        AL_CHORUS_MAX_FEEDBACK);

    eax_set_efx_feedback(feedback, al_effect_props_);
}

void EaxChorusEffect::set_efx_delay()
{
    const auto delay = clamp(
        eax_.flDelay,
        AL_CHORUS_MIN_DELAY,
        AL_CHORUS_MAX_DELAY);

    eax_set_efx_delay(delay, al_effect_props_);
}

void EaxChorusEffect::set_efx_defaults()
{
    set_efx_waveform();
    set_efx_phase();
    set_efx_rate();
    set_efx_depth();
    set_efx_feedback();
    set_efx_delay();
}

void EaxChorusEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXCHORUS_NONE:
            break;

        case EAXCHORUS_ALLPARAMETERS:
            eax_call.set_value<EaxChorusEffectException>(eax_);
            break;

        case EAXCHORUS_WAVEFORM:
            eax_call.set_value<EaxChorusEffectException>(eax_.ulWaveform);
            break;

        case EAXCHORUS_PHASE:
            eax_call.set_value<EaxChorusEffectException>(eax_.lPhase);
            break;

        case EAXCHORUS_RATE:
            eax_call.set_value<EaxChorusEffectException>(eax_.flRate);
            break;

        case EAXCHORUS_DEPTH:
            eax_call.set_value<EaxChorusEffectException>(eax_.flDepth);
            break;

        case EAXCHORUS_FEEDBACK:
            eax_call.set_value<EaxChorusEffectException>(eax_.flFeedback);
            break;

        case EAXCHORUS_DELAY:
            eax_call.set_value<EaxChorusEffectException>(eax_.flDelay);
            break;

        default:
            throw EaxChorusEffectException{"Unsupported property id."};
    }
}

void EaxChorusEffect::validate_waveform(
    unsigned long ulWaveform)
{
    eax_validate_range<EaxChorusEffectException>(
        "Waveform",
        ulWaveform,
        EAXCHORUS_MINWAVEFORM,
        EAXCHORUS_MAXWAVEFORM);
}

void EaxChorusEffect::validate_phase(
    long lPhase)
{
    eax_validate_range<EaxChorusEffectException>(
        "Phase",
        lPhase,
        EAXCHORUS_MINPHASE,
        EAXCHORUS_MAXPHASE);
}

void EaxChorusEffect::validate_rate(
    float flRate)
{
    eax_validate_range<EaxChorusEffectException>(
        "Rate",
        flRate,
        EAXCHORUS_MINRATE,
        EAXCHORUS_MAXRATE);
}

void EaxChorusEffect::validate_depth(
    float flDepth)
{
    eax_validate_range<EaxChorusEffectException>(
        "Depth",
        flDepth,
        EAXCHORUS_MINDEPTH,
        EAXCHORUS_MAXDEPTH);
}

void EaxChorusEffect::validate_feedback(
    float flFeedback)
{
    eax_validate_range<EaxChorusEffectException>(
        "Feedback",
        flFeedback,
        EAXCHORUS_MINFEEDBACK,
        EAXCHORUS_MAXFEEDBACK);
}

void EaxChorusEffect::validate_delay(
    float flDelay)
{
    eax_validate_range<EaxChorusEffectException>(
        "Delay",
        flDelay,
        EAXCHORUS_MINDELAY,
        EAXCHORUS_MAXDELAY);
}

void EaxChorusEffect::validate_all(
    const EAXCHORUSPROPERTIES& eax_all)
{
    validate_waveform(eax_all.ulWaveform);
    validate_phase(eax_all.lPhase);
    validate_rate(eax_all.flRate);
    validate_depth(eax_all.flDepth);
    validate_feedback(eax_all.flFeedback);
    validate_delay(eax_all.flDelay);
}

void EaxChorusEffect::defer_waveform(
    unsigned long ulWaveform)
{
    eax_d_.ulWaveform = ulWaveform;
    eax_dirty_flags_.ulWaveform = (eax_.ulWaveform != eax_d_.ulWaveform);
}

void EaxChorusEffect::defer_phase(
    long lPhase)
{
    eax_d_.lPhase = lPhase;
    eax_dirty_flags_.lPhase = (eax_.lPhase != eax_d_.lPhase);
}

void EaxChorusEffect::defer_rate(
    float flRate)
{
    eax_d_.flRate = flRate;
    eax_dirty_flags_.flRate = (eax_.flRate != eax_d_.flRate);
}

void EaxChorusEffect::defer_depth(
    float flDepth)
{
    eax_d_.flDepth = flDepth;
    eax_dirty_flags_.flDepth = (eax_.flDepth != eax_d_.flDepth);
}

void EaxChorusEffect::defer_feedback(
    float flFeedback)
{
    eax_d_.flFeedback = flFeedback;
    eax_dirty_flags_.flFeedback = (eax_.flFeedback != eax_d_.flFeedback);
}

void EaxChorusEffect::defer_delay(
    float flDelay)
{
    eax_d_.flDelay = flDelay;
    eax_dirty_flags_.flDelay = (eax_.flDelay != eax_d_.flDelay);
}

void EaxChorusEffect::defer_all(
    const EAXCHORUSPROPERTIES& eax_all)
{
    defer_waveform(eax_all.ulWaveform);
    defer_phase(eax_all.lPhase);
    defer_rate(eax_all.flRate);
    defer_depth(eax_all.flDepth);
    defer_feedback(eax_all.flFeedback);
    defer_delay(eax_all.flDelay);
}

void EaxChorusEffect::defer_waveform(
    const EaxEaxCall& eax_call)
{
    const auto& waveform =
        eax_call.get_value<EaxChorusEffectException, const decltype(EAXCHORUSPROPERTIES::ulWaveform)>();

    validate_waveform(waveform);
    defer_waveform(waveform);
}

void EaxChorusEffect::defer_phase(
    const EaxEaxCall& eax_call)
{
    const auto& phase =
        eax_call.get_value<EaxChorusEffectException, const decltype(EAXCHORUSPROPERTIES::lPhase)>();

    validate_phase(phase);
    defer_phase(phase);
}

void EaxChorusEffect::defer_rate(
    const EaxEaxCall& eax_call)
{
    const auto& rate =
        eax_call.get_value<EaxChorusEffectException, const decltype(EAXCHORUSPROPERTIES::flRate)>();

    validate_rate(rate);
    defer_rate(rate);
}

void EaxChorusEffect::defer_depth(
    const EaxEaxCall& eax_call)
{
    const auto& depth =
        eax_call.get_value<EaxChorusEffectException, const decltype(EAXCHORUSPROPERTIES::flDepth)>();

    validate_depth(depth);
    defer_depth(depth);
}

void EaxChorusEffect::defer_feedback(
    const EaxEaxCall& eax_call)
{
    const auto& feedback =
        eax_call.get_value<EaxChorusEffectException, const decltype(EAXCHORUSPROPERTIES::flFeedback)>();

    validate_feedback(feedback);
    defer_feedback(feedback);
}

void EaxChorusEffect::defer_delay(
    const EaxEaxCall& eax_call)
{
    const auto& delay =
        eax_call.get_value<EaxChorusEffectException, const decltype(EAXCHORUSPROPERTIES::flDelay)>();

    validate_delay(delay);
    defer_delay(delay);
}

void EaxChorusEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxChorusEffectException, const EAXCHORUSPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxChorusEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxChorusEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.ulWaveform)
    {
        set_efx_waveform();
    }

    if (eax_dirty_flags_.lPhase)
    {
        set_efx_phase();
    }

    if (eax_dirty_flags_.flRate)
    {
        set_efx_rate();
    }

    if (eax_dirty_flags_.flDepth)
    {
        set_efx_depth();
    }

    if (eax_dirty_flags_.flFeedback)
    {
        set_efx_feedback();
    }

    if (eax_dirty_flags_.flDelay)
    {
        set_efx_delay();
    }

    eax_dirty_flags_ = EaxChorusEffectDirtyFlags{};

    return true;
}

void EaxChorusEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXCHORUS_NONE:
            break;

        case EAXCHORUS_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXCHORUS_WAVEFORM:
            defer_waveform(eax_call);
            break;

        case EAXCHORUS_PHASE:
            defer_phase(eax_call);
            break;

        case EAXCHORUS_RATE:
            defer_rate(eax_call);
            break;

        case EAXCHORUS_DEPTH:
            defer_depth(eax_call);
            break;

        case EAXCHORUS_FEEDBACK:
            defer_feedback(eax_call);
            break;

        case EAXCHORUS_DELAY:
            defer_delay(eax_call);
            break;

        default:
            throw EaxChorusEffectException{"Unsupported property id."};
    }
}


} // namespace


EaxEffectUPtr eax_create_eax_chorus_effect()
{
    return std::make_unique<::EaxChorusEffect>();
}


namespace
{


using EaxFlangerEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxFlangerEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxFlangerEffectDirtyFlagsValue ulWaveform : 1;
    EaxFlangerEffectDirtyFlagsValue lPhase : 1;
    EaxFlangerEffectDirtyFlagsValue flRate : 1;
    EaxFlangerEffectDirtyFlagsValue flDepth : 1;
    EaxFlangerEffectDirtyFlagsValue flFeedback : 1;
    EaxFlangerEffectDirtyFlagsValue flDelay : 1;
}; // EaxFlangerEffectDirtyFlags


class EaxFlangerEffect final :
    public EaxEffect
{
public:
    EaxFlangerEffect();


    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXFLANGERPROPERTIES eax_{};
    EAXFLANGERPROPERTIES eax_d_{};
    EaxFlangerEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_waveform();
    void set_efx_phase();
    void set_efx_rate();
    void set_efx_depth();
    void set_efx_feedback();
    void set_efx_delay();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_waveform(unsigned long ulWaveform);
    void validate_phase(long lPhase);
    void validate_rate(float flRate);
    void validate_depth(float flDepth);
    void validate_feedback(float flFeedback);
    void validate_delay(float flDelay);
    void validate_all(const EAXFLANGERPROPERTIES& all);

    void defer_waveform(unsigned long ulWaveform);
    void defer_phase(long lPhase);
    void defer_rate(float flRate);
    void defer_depth(float flDepth);
    void defer_feedback(float flFeedback);
    void defer_delay(float flDelay);
    void defer_all(const EAXFLANGERPROPERTIES& all);

    void defer_waveform(const EaxEaxCall& eax_call);
    void defer_phase(const EaxEaxCall& eax_call);
    void defer_rate(const EaxEaxCall& eax_call);
    void defer_depth(const EaxEaxCall& eax_call);
    void defer_feedback(const EaxEaxCall& eax_call);
    void defer_delay(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxFlangerEffect


class EaxFlangerEffectException :
    public EaxException
{
public:
    explicit EaxFlangerEffectException(
        const char* message)
        :
        EaxException{"EAX_FLANGER_EFFECT", message}
    {
    }
}; // EaxFlangerEffectException


EaxFlangerEffect::EaxFlangerEffect()
    : EaxEffect{AL_EFFECT_FLANGER}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxFlangerEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxFlangerEffect::set_eax_defaults()
{
    eax_.ulWaveform = EAXFLANGER_DEFAULTWAVEFORM;
    eax_.lPhase = EAXFLANGER_DEFAULTPHASE;
    eax_.flRate = EAXFLANGER_DEFAULTRATE;
    eax_.flDepth = EAXFLANGER_DEFAULTDEPTH;
    eax_.flFeedback = EAXFLANGER_DEFAULTFEEDBACK;
    eax_.flDelay = EAXFLANGER_DEFAULTDELAY;

    eax_d_ = eax_;
}

void EaxFlangerEffect::set_efx_waveform()
{
    const auto waveform = clamp(
        static_cast<ALint>(eax_.ulWaveform),
        AL_FLANGER_MIN_WAVEFORM,
        AL_FLANGER_MAX_WAVEFORM);

    eax_set_efx_waveform(waveform, al_effect_props_);
}

void EaxFlangerEffect::set_efx_phase()
{
    const auto phase = clamp(
        static_cast<ALint>(eax_.lPhase),
        AL_FLANGER_MIN_PHASE,
        AL_FLANGER_MAX_PHASE);

    eax_set_efx_phase(phase, al_effect_props_);
}

void EaxFlangerEffect::set_efx_rate()
{
    const auto rate = clamp(
        eax_.flRate,
        AL_FLANGER_MIN_RATE,
        AL_FLANGER_MAX_RATE);

    eax_set_efx_rate(rate, al_effect_props_);
}

void EaxFlangerEffect::set_efx_depth()
{
    const auto depth = clamp(
        eax_.flDepth,
        AL_FLANGER_MIN_DEPTH,
        AL_FLANGER_MAX_DEPTH);

    eax_set_efx_depth(depth, al_effect_props_);
}

void EaxFlangerEffect::set_efx_feedback()
{
    const auto feedback = clamp(
        eax_.flFeedback,
        AL_FLANGER_MIN_FEEDBACK,
        AL_FLANGER_MAX_FEEDBACK);

    eax_set_efx_feedback(feedback, al_effect_props_);
}

void EaxFlangerEffect::set_efx_delay()
{
    const auto delay = clamp(
        eax_.flDelay,
        AL_FLANGER_MIN_DELAY,
        AL_FLANGER_MAX_DELAY);

    eax_set_efx_delay(delay, al_effect_props_);
}

void EaxFlangerEffect::set_efx_defaults()
{
    set_efx_waveform();
    set_efx_phase();
    set_efx_rate();
    set_efx_depth();
    set_efx_feedback();
    set_efx_delay();
}

void EaxFlangerEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXFLANGER_NONE:
            break;

        case EAXFLANGER_ALLPARAMETERS:
            eax_call.set_value<EaxFlangerEffectException>(eax_);
            break;

        case EAXFLANGER_WAVEFORM:
            eax_call.set_value<EaxFlangerEffectException>(eax_.ulWaveform);
            break;

        case EAXFLANGER_PHASE:
            eax_call.set_value<EaxFlangerEffectException>(eax_.lPhase);
            break;

        case EAXFLANGER_RATE:
            eax_call.set_value<EaxFlangerEffectException>(eax_.flRate);
            break;

        case EAXFLANGER_DEPTH:
            eax_call.set_value<EaxFlangerEffectException>(eax_.flDepth);
            break;

        case EAXFLANGER_FEEDBACK:
            eax_call.set_value<EaxFlangerEffectException>(eax_.flFeedback);
            break;

        case EAXFLANGER_DELAY:
            eax_call.set_value<EaxFlangerEffectException>(eax_.flDelay);
            break;

        default:
            throw EaxFlangerEffectException{"Unsupported property id."};
    }
}

void EaxFlangerEffect::validate_waveform(
    unsigned long ulWaveform)
{
    eax_validate_range<EaxFlangerEffectException>(
        "Waveform",
        ulWaveform,
        EAXFLANGER_MINWAVEFORM,
        EAXFLANGER_MAXWAVEFORM);
}

void EaxFlangerEffect::validate_phase(
    long lPhase)
{
    eax_validate_range<EaxFlangerEffectException>(
        "Phase",
        lPhase,
        EAXFLANGER_MINPHASE,
        EAXFLANGER_MAXPHASE);
}

void EaxFlangerEffect::validate_rate(
    float flRate)
{
    eax_validate_range<EaxFlangerEffectException>(
        "Rate",
        flRate,
        EAXFLANGER_MINRATE,
        EAXFLANGER_MAXRATE);
}

void EaxFlangerEffect::validate_depth(
    float flDepth)
{
    eax_validate_range<EaxFlangerEffectException>(
        "Depth",
        flDepth,
        EAXFLANGER_MINDEPTH,
        EAXFLANGER_MAXDEPTH);
}

void EaxFlangerEffect::validate_feedback(
    float flFeedback)
{
    eax_validate_range<EaxFlangerEffectException>(
        "Feedback",
        flFeedback,
        EAXFLANGER_MINFEEDBACK,
        EAXFLANGER_MAXFEEDBACK);
}

void EaxFlangerEffect::validate_delay(
    float flDelay)
{
    eax_validate_range<EaxFlangerEffectException>(
        "Delay",
        flDelay,
        EAXFLANGER_MINDELAY,
        EAXFLANGER_MAXDELAY);
}

void EaxFlangerEffect::validate_all(
    const EAXFLANGERPROPERTIES& all)
{
    validate_waveform(all.ulWaveform);
    validate_phase(all.lPhase);
    validate_rate(all.flRate);
    validate_depth(all.flDepth);
    validate_feedback(all.flDelay);
    validate_delay(all.flDelay);
}

void EaxFlangerEffect::defer_waveform(
    unsigned long ulWaveform)
{
    eax_d_.ulWaveform = ulWaveform;
    eax_dirty_flags_.ulWaveform = (eax_.ulWaveform != eax_d_.ulWaveform);
}

void EaxFlangerEffect::defer_phase(
    long lPhase)
{
    eax_d_.lPhase = lPhase;
    eax_dirty_flags_.lPhase = (eax_.lPhase != eax_d_.lPhase);
}

void EaxFlangerEffect::defer_rate(
    float flRate)
{
    eax_d_.flRate = flRate;
    eax_dirty_flags_.flRate = (eax_.flRate != eax_d_.flRate);
}

void EaxFlangerEffect::defer_depth(
    float flDepth)
{
    eax_d_.flDepth = flDepth;
    eax_dirty_flags_.flDepth = (eax_.flDepth != eax_d_.flDepth);
}

void EaxFlangerEffect::defer_feedback(
    float flFeedback)
{
    eax_d_.flFeedback = flFeedback;
    eax_dirty_flags_.flFeedback = (eax_.flFeedback != eax_d_.flFeedback);
}

void EaxFlangerEffect::defer_delay(
    float flDelay)
{
    eax_d_.flDelay = flDelay;
    eax_dirty_flags_.flDelay = (eax_.flDelay != eax_d_.flDelay);
}

void EaxFlangerEffect::defer_all(
    const EAXFLANGERPROPERTIES& all)
{
    defer_waveform(all.ulWaveform);
    defer_phase(all.lPhase);
    defer_rate(all.flRate);
    defer_depth(all.flDepth);
    defer_feedback(all.flDelay);
    defer_delay(all.flDelay);
}

void EaxFlangerEffect::defer_waveform(
    const EaxEaxCall& eax_call)
{
    const auto& waveform =
        eax_call.get_value<EaxFlangerEffectException, const decltype(EAXFLANGERPROPERTIES::ulWaveform)>();

    validate_waveform(waveform);
    defer_waveform(waveform);
}

void EaxFlangerEffect::defer_phase(
    const EaxEaxCall& eax_call)
{
    const auto& phase =
        eax_call.get_value<EaxFlangerEffectException, const decltype(EAXFLANGERPROPERTIES::lPhase)>();

    validate_phase(phase);
    defer_phase(phase);
}

void EaxFlangerEffect::defer_rate(
    const EaxEaxCall& eax_call)
{
    const auto& rate =
        eax_call.get_value<EaxFlangerEffectException, const decltype(EAXFLANGERPROPERTIES::flRate)>();

    validate_rate(rate);
    defer_rate(rate);
}

void EaxFlangerEffect::defer_depth(
    const EaxEaxCall& eax_call)
{
    const auto& depth =
        eax_call.get_value<EaxFlangerEffectException, const decltype(EAXFLANGERPROPERTIES::flDepth)>();

    validate_depth(depth);
    defer_depth(depth);
}

void EaxFlangerEffect::defer_feedback(
    const EaxEaxCall& eax_call)
{
    const auto& feedback =
        eax_call.get_value<EaxFlangerEffectException, const decltype(EAXFLANGERPROPERTIES::flFeedback)>();

    validate_feedback(feedback);
    defer_feedback(feedback);
}

void EaxFlangerEffect::defer_delay(
    const EaxEaxCall& eax_call)
{
    const auto& delay =
        eax_call.get_value<EaxFlangerEffectException, const decltype(EAXFLANGERPROPERTIES::flDelay)>();

    validate_delay(delay);
    defer_delay(delay);
}

void EaxFlangerEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxFlangerEffectException, const EAXFLANGERPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxFlangerEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxFlangerEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.ulWaveform)
    {
        set_efx_waveform();
    }

    if (eax_dirty_flags_.lPhase)
    {
        set_efx_phase();
    }

    if (eax_dirty_flags_.flRate)
    {
        set_efx_rate();
    }

    if (eax_dirty_flags_.flDepth)
    {
        set_efx_depth();
    }

    if (eax_dirty_flags_.flFeedback)
    {
        set_efx_feedback();
    }

    if (eax_dirty_flags_.flDelay)
    {
        set_efx_delay();
    }

    eax_dirty_flags_ = EaxFlangerEffectDirtyFlags{};

    return true;
}

void EaxFlangerEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXFLANGER_NONE:
            break;

        case EAXFLANGER_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXFLANGER_WAVEFORM:
            defer_waveform(eax_call);
            break;

        case EAXFLANGER_PHASE:
            defer_phase(eax_call);
            break;

        case EAXFLANGER_RATE:
            defer_rate(eax_call);
            break;

        case EAXFLANGER_DEPTH:
            defer_depth(eax_call);
            break;

        case EAXFLANGER_FEEDBACK:
            defer_feedback(eax_call);
            break;

        case EAXFLANGER_DELAY:
            defer_delay(eax_call);
            break;

        default:
            throw EaxFlangerEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_flanger_effect()
{
    return std::make_unique<EaxFlangerEffect>();
}

#endif // ALSOFT_EAX
