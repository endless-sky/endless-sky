
#include "config.h"

#include <cmath>
#include <cstdlib>

#include <algorithm>

#include "AL/efx.h"

#include "alc/effects/base.h"
#include "effects.h"

#ifdef ALSOFT_EAX
#include "alnumeric.h"

#include "al/eax_exception.h"
#include "al/eax_utils.h"
#endif // ALSOFT_EAX


namespace {

void Autowah_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_AUTOWAH_ATTACK_TIME:
        if(!(val >= AL_AUTOWAH_MIN_ATTACK_TIME && val <= AL_AUTOWAH_MAX_ATTACK_TIME))
            throw effect_exception{AL_INVALID_VALUE, "Autowah attack time out of range"};
        props->Autowah.AttackTime = val;
        break;

    case AL_AUTOWAH_RELEASE_TIME:
        if(!(val >= AL_AUTOWAH_MIN_RELEASE_TIME && val <= AL_AUTOWAH_MAX_RELEASE_TIME))
            throw effect_exception{AL_INVALID_VALUE, "Autowah release time out of range"};
        props->Autowah.ReleaseTime = val;
        break;

    case AL_AUTOWAH_RESONANCE:
        if(!(val >= AL_AUTOWAH_MIN_RESONANCE && val <= AL_AUTOWAH_MAX_RESONANCE))
            throw effect_exception{AL_INVALID_VALUE, "Autowah resonance out of range"};
        props->Autowah.Resonance = val;
        break;

    case AL_AUTOWAH_PEAK_GAIN:
        if(!(val >= AL_AUTOWAH_MIN_PEAK_GAIN && val <= AL_AUTOWAH_MAX_PEAK_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Autowah peak gain out of range"};
        props->Autowah.PeakGain = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid autowah float property 0x%04x", param};
    }
}
void Autowah_setParamfv(EffectProps *props,  ALenum param, const float *vals)
{ Autowah_setParamf(props, param, vals[0]); }

void Autowah_setParami(EffectProps*, ALenum param, int)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid autowah integer property 0x%04x", param}; }
void Autowah_setParamiv(EffectProps*, ALenum param, const int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid autowah integer vector property 0x%04x",
        param};
}

void Autowah_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_AUTOWAH_ATTACK_TIME:
        *val = props->Autowah.AttackTime;
        break;

    case AL_AUTOWAH_RELEASE_TIME:
        *val = props->Autowah.ReleaseTime;
        break;

    case AL_AUTOWAH_RESONANCE:
        *val = props->Autowah.Resonance;
        break;

    case AL_AUTOWAH_PEAK_GAIN:
        *val = props->Autowah.PeakGain;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid autowah float property 0x%04x", param};
    }

}
void Autowah_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Autowah_getParamf(props, param, vals); }

void Autowah_getParami(const EffectProps*, ALenum param, int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid autowah integer property 0x%04x", param}; }
void Autowah_getParamiv(const EffectProps*, ALenum param, int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid autowah integer vector property 0x%04x",
        param};
}

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Autowah.AttackTime = AL_AUTOWAH_DEFAULT_ATTACK_TIME;
    props.Autowah.ReleaseTime = AL_AUTOWAH_DEFAULT_RELEASE_TIME;
    props.Autowah.Resonance = AL_AUTOWAH_DEFAULT_RESONANCE;
    props.Autowah.PeakGain = AL_AUTOWAH_DEFAULT_PEAK_GAIN;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Autowah);

const EffectProps AutowahEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxAutoWahEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxAutoWahEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxAutoWahEffectDirtyFlagsValue flAttackTime : 1;
    EaxAutoWahEffectDirtyFlagsValue flReleaseTime : 1;
    EaxAutoWahEffectDirtyFlagsValue lResonance : 1;
    EaxAutoWahEffectDirtyFlagsValue lPeakLevel : 1;
}; // EaxAutoWahEffectDirtyFlags


class EaxAutoWahEffect final :
    public EaxEffect
{
public:
    EaxAutoWahEffect();


    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXAUTOWAHPROPERTIES eax_{};
    EAXAUTOWAHPROPERTIES eax_d_{};
    EaxAutoWahEffectDirtyFlags eax_dirty_flags_{};


    void set_eax_defaults();


    void set_efx_attack_time();

    void set_efx_release_time();

    void set_efx_resonance();

    void set_efx_peak_gain();

    void set_efx_defaults();


    void get(const EaxEaxCall& eax_call);


    void validate_attack_time(
        float flAttackTime);

    void validate_release_time(
        float flReleaseTime);

    void validate_resonance(
        long lResonance);

    void validate_peak_level(
        long lPeakLevel);

    void validate_all(
        const EAXAUTOWAHPROPERTIES& eax_all);


    void defer_attack_time(
        float flAttackTime);

    void defer_release_time(
        float flReleaseTime);

    void defer_resonance(
        long lResonance);

    void defer_peak_level(
        long lPeakLevel);

    void defer_all(
        const EAXAUTOWAHPROPERTIES& eax_all);


    void defer_attack_time(
        const EaxEaxCall& eax_call);

    void defer_release_time(
        const EaxEaxCall& eax_call);

    void defer_resonance(
        const EaxEaxCall& eax_call);

    void defer_peak_level(
        const EaxEaxCall& eax_call);

    void defer_all(
        const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxAutoWahEffect


class EaxAutoWahEffectException :
    public EaxException
{
public:
    explicit EaxAutoWahEffectException(
        const char* message)
        :
        EaxException{"EAX_AUTO_WAH_EFFECT", message}
    {
    }
}; // EaxAutoWahEffectException


EaxAutoWahEffect::EaxAutoWahEffect()
    : EaxEffect{AL_EFFECT_AUTOWAH}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxAutoWahEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxAutoWahEffect::set_eax_defaults()
{
    eax_.flAttackTime = EAXAUTOWAH_DEFAULTATTACKTIME;
    eax_.flReleaseTime = EAXAUTOWAH_DEFAULTRELEASETIME;
    eax_.lResonance = EAXAUTOWAH_DEFAULTRESONANCE;
    eax_.lPeakLevel = EAXAUTOWAH_DEFAULTPEAKLEVEL;

    eax_d_ = eax_;
}

void EaxAutoWahEffect::set_efx_attack_time()
{
    const auto attack_time = clamp(
        eax_.flAttackTime,
        AL_AUTOWAH_MIN_ATTACK_TIME,
        AL_AUTOWAH_MAX_ATTACK_TIME);

    al_effect_props_.Autowah.AttackTime = attack_time;
}

void EaxAutoWahEffect::set_efx_release_time()
{
    const auto release_time = clamp(
        eax_.flReleaseTime,
        AL_AUTOWAH_MIN_RELEASE_TIME,
        AL_AUTOWAH_MAX_RELEASE_TIME);

    al_effect_props_.Autowah.ReleaseTime = release_time;
}

void EaxAutoWahEffect::set_efx_resonance()
{
    const auto resonance = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lResonance)),
        AL_AUTOWAH_MIN_RESONANCE,
        AL_AUTOWAH_MAX_RESONANCE);

    al_effect_props_.Autowah.Resonance = resonance;
}

void EaxAutoWahEffect::set_efx_peak_gain()
{
    const auto peak_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lPeakLevel)),
        AL_AUTOWAH_MIN_PEAK_GAIN,
        AL_AUTOWAH_MAX_PEAK_GAIN);

    al_effect_props_.Autowah.PeakGain = peak_gain;
}

void EaxAutoWahEffect::set_efx_defaults()
{
    set_efx_attack_time();
    set_efx_release_time();
    set_efx_resonance();
    set_efx_peak_gain();
}

void EaxAutoWahEffect::get(const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case EAXAUTOWAH_NONE:
            break;

        case EAXAUTOWAH_ALLPARAMETERS:
            eax_call.set_value<EaxAutoWahEffectException>(eax_);
            break;

        case EAXAUTOWAH_ATTACKTIME:
            eax_call.set_value<EaxAutoWahEffectException>(eax_.flAttackTime);
            break;

        case EAXAUTOWAH_RELEASETIME:
            eax_call.set_value<EaxAutoWahEffectException>(eax_.flReleaseTime);
            break;

        case EAXAUTOWAH_RESONANCE:
            eax_call.set_value<EaxAutoWahEffectException>(eax_.lResonance);
            break;

        case EAXAUTOWAH_PEAKLEVEL:
            eax_call.set_value<EaxAutoWahEffectException>(eax_.lPeakLevel);
            break;

        default:
            throw EaxAutoWahEffectException{"Unsupported property id."};
    }
}

void EaxAutoWahEffect::validate_attack_time(
    float flAttackTime)
{
    eax_validate_range<EaxAutoWahEffectException>(
        "Attack Time",
        flAttackTime,
        EAXAUTOWAH_MINATTACKTIME,
        EAXAUTOWAH_MAXATTACKTIME);
}

void EaxAutoWahEffect::validate_release_time(
    float flReleaseTime)
{
    eax_validate_range<EaxAutoWahEffectException>(
        "Release Time",
        flReleaseTime,
        EAXAUTOWAH_MINRELEASETIME,
        EAXAUTOWAH_MAXRELEASETIME);
}

void EaxAutoWahEffect::validate_resonance(
    long lResonance)
{
    eax_validate_range<EaxAutoWahEffectException>(
        "Resonance",
        lResonance,
        EAXAUTOWAH_MINRESONANCE,
        EAXAUTOWAH_MAXRESONANCE);
}

void EaxAutoWahEffect::validate_peak_level(
    long lPeakLevel)
{
    eax_validate_range<EaxAutoWahEffectException>(
        "Peak Level",
        lPeakLevel,
        EAXAUTOWAH_MINPEAKLEVEL,
        EAXAUTOWAH_MAXPEAKLEVEL);
}

void EaxAutoWahEffect::validate_all(
    const EAXAUTOWAHPROPERTIES& eax_all)
{
    validate_attack_time(eax_all.flAttackTime);
    validate_release_time(eax_all.flReleaseTime);
    validate_resonance(eax_all.lResonance);
    validate_peak_level(eax_all.lPeakLevel);
}

void EaxAutoWahEffect::defer_attack_time(
    float flAttackTime)
{
    eax_d_.flAttackTime = flAttackTime;
    eax_dirty_flags_.flAttackTime = (eax_.flAttackTime != eax_d_.flAttackTime);
}

void EaxAutoWahEffect::defer_release_time(
    float flReleaseTime)
{
    eax_d_.flReleaseTime = flReleaseTime;
    eax_dirty_flags_.flReleaseTime = (eax_.flReleaseTime != eax_d_.flReleaseTime);
}

void EaxAutoWahEffect::defer_resonance(
    long lResonance)
{
    eax_d_.lResonance = lResonance;
    eax_dirty_flags_.lResonance = (eax_.lResonance != eax_d_.lResonance);
}

void EaxAutoWahEffect::defer_peak_level(
    long lPeakLevel)
{
    eax_d_.lPeakLevel = lPeakLevel;
    eax_dirty_flags_.lPeakLevel = (eax_.lPeakLevel != eax_d_.lPeakLevel);
}

void EaxAutoWahEffect::defer_all(
    const EAXAUTOWAHPROPERTIES& eax_all)
{
    validate_all(eax_all);

    defer_attack_time(eax_all.flAttackTime);
    defer_release_time(eax_all.flReleaseTime);
    defer_resonance(eax_all.lResonance);
    defer_peak_level(eax_all.lPeakLevel);
}

void EaxAutoWahEffect::defer_attack_time(
    const EaxEaxCall& eax_call)
{
    const auto& attack_time =
        eax_call.get_value<EaxAutoWahEffectException, const decltype(EAXAUTOWAHPROPERTIES::flAttackTime)>();

    validate_attack_time(attack_time);
    defer_attack_time(attack_time);
}

void EaxAutoWahEffect::defer_release_time(
    const EaxEaxCall& eax_call)
{
    const auto& release_time =
        eax_call.get_value<EaxAutoWahEffectException, const decltype(EAXAUTOWAHPROPERTIES::flReleaseTime)>();

    validate_release_time(release_time);
    defer_release_time(release_time);
}

void EaxAutoWahEffect::defer_resonance(
    const EaxEaxCall& eax_call)
{
    const auto& resonance =
        eax_call.get_value<EaxAutoWahEffectException, const decltype(EAXAUTOWAHPROPERTIES::lResonance)>();

    validate_resonance(resonance);
    defer_resonance(resonance);
}

void EaxAutoWahEffect::defer_peak_level(
    const EaxEaxCall& eax_call)
{
    const auto& peak_level =
        eax_call.get_value<EaxAutoWahEffectException, const decltype(EAXAUTOWAHPROPERTIES::lPeakLevel)>();

    validate_peak_level(peak_level);
    defer_peak_level(peak_level);
}

void EaxAutoWahEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxAutoWahEffectException, const EAXAUTOWAHPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxAutoWahEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxAutoWahEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.flAttackTime)
    {
        set_efx_attack_time();
    }

    if (eax_dirty_flags_.flReleaseTime)
    {
        set_efx_release_time();
    }

    if (eax_dirty_flags_.lResonance)
    {
        set_efx_resonance();
    }

    if (eax_dirty_flags_.lPeakLevel)
    {
        set_efx_peak_gain();
    }

    eax_dirty_flags_ = EaxAutoWahEffectDirtyFlags{};

    return true;
}

void EaxAutoWahEffect::set(const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case EAXAUTOWAH_NONE:
            break;

        case EAXAUTOWAH_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXAUTOWAH_ATTACKTIME:
            defer_attack_time(eax_call);
            break;

        case EAXAUTOWAH_RELEASETIME:
            defer_release_time(eax_call);
            break;

        case EAXAUTOWAH_RESONANCE:
            defer_resonance(eax_call);
            break;

        case EAXAUTOWAH_PEAKLEVEL:
            defer_peak_level(eax_call);
            break;

        default:
            throw EaxAutoWahEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_auto_wah_effect()
{
    return std::make_unique<::EaxAutoWahEffect>();
}

#endif // ALSOFT_EAX
