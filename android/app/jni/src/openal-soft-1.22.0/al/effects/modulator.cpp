
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

al::optional<ModulatorWaveform> WaveformFromEmum(ALenum value)
{
    switch(value)
    {
    case AL_RING_MODULATOR_SINUSOID: return al::make_optional(ModulatorWaveform::Sinusoid);
    case AL_RING_MODULATOR_SAWTOOTH: return al::make_optional(ModulatorWaveform::Sawtooth);
    case AL_RING_MODULATOR_SQUARE: return al::make_optional(ModulatorWaveform::Square);
    }
    return al::nullopt;
}
ALenum EnumFromWaveform(ModulatorWaveform type)
{
    switch(type)
    {
    case ModulatorWaveform::Sinusoid: return AL_RING_MODULATOR_SINUSOID;
    case ModulatorWaveform::Sawtooth: return AL_RING_MODULATOR_SAWTOOTH;
    case ModulatorWaveform::Square: return AL_RING_MODULATOR_SQUARE;
    }
    throw std::runtime_error{"Invalid modulator waveform: " +
        std::to_string(static_cast<int>(type))};
}

void Modulator_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_RING_MODULATOR_FREQUENCY:
        if(!(val >= AL_RING_MODULATOR_MIN_FREQUENCY && val <= AL_RING_MODULATOR_MAX_FREQUENCY))
            throw effect_exception{AL_INVALID_VALUE, "Modulator frequency out of range: %f", val};
        props->Modulator.Frequency = val;
        break;

    case AL_RING_MODULATOR_HIGHPASS_CUTOFF:
        if(!(val >= AL_RING_MODULATOR_MIN_HIGHPASS_CUTOFF && val <= AL_RING_MODULATOR_MAX_HIGHPASS_CUTOFF))
            throw effect_exception{AL_INVALID_VALUE, "Modulator high-pass cutoff out of range: %f", val};
        props->Modulator.HighPassCutoff = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid modulator float property 0x%04x", param};
    }
}
void Modulator_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Modulator_setParamf(props, param, vals[0]); }
void Modulator_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_RING_MODULATOR_FREQUENCY:
    case AL_RING_MODULATOR_HIGHPASS_CUTOFF:
        Modulator_setParamf(props, param, static_cast<float>(val));
        break;

    case AL_RING_MODULATOR_WAVEFORM:
        if(auto formopt = WaveformFromEmum(val))
            props->Modulator.Waveform = *formopt;
        else
            throw effect_exception{AL_INVALID_VALUE, "Invalid modulator waveform: 0x%04x", val};
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid modulator integer property 0x%04x",
            param};
    }
}
void Modulator_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Modulator_setParami(props, param, vals[0]); }

void Modulator_getParami(const EffectProps *props, ALenum param, int *val)
{
    switch(param)
    {
    case AL_RING_MODULATOR_FREQUENCY:
        *val = static_cast<int>(props->Modulator.Frequency);
        break;
    case AL_RING_MODULATOR_HIGHPASS_CUTOFF:
        *val = static_cast<int>(props->Modulator.HighPassCutoff);
        break;
    case AL_RING_MODULATOR_WAVEFORM:
        *val = EnumFromWaveform(props->Modulator.Waveform);
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid modulator integer property 0x%04x",
            param};
    }
}
void Modulator_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Modulator_getParami(props, param, vals); }
void Modulator_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_RING_MODULATOR_FREQUENCY:
        *val = props->Modulator.Frequency;
        break;
    case AL_RING_MODULATOR_HIGHPASS_CUTOFF:
        *val = props->Modulator.HighPassCutoff;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid modulator float property 0x%04x", param};
    }
}
void Modulator_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Modulator_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Modulator.Frequency      = AL_RING_MODULATOR_DEFAULT_FREQUENCY;
    props.Modulator.HighPassCutoff = AL_RING_MODULATOR_DEFAULT_HIGHPASS_CUTOFF;
    props.Modulator.Waveform       = *WaveformFromEmum(AL_RING_MODULATOR_DEFAULT_WAVEFORM);
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Modulator);

const EffectProps ModulatorEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxRingModulatorEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxRingModulatorEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxRingModulatorEffectDirtyFlagsValue flFrequency : 1;
    EaxRingModulatorEffectDirtyFlagsValue flHighPassCutOff : 1;
    EaxRingModulatorEffectDirtyFlagsValue ulWaveform : 1;
}; // EaxPitchShifterEffectDirtyFlags


class EaxRingModulatorEffect final :
    public EaxEffect
{
public:
    EaxRingModulatorEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXRINGMODULATORPROPERTIES eax_{};
    EAXRINGMODULATORPROPERTIES eax_d_{};
    EaxRingModulatorEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_frequency();
    void set_efx_high_pass_cutoff();
    void set_efx_waveform();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_frequency(float flFrequency);
    void validate_high_pass_cutoff(float flHighPassCutOff);
    void validate_waveform(unsigned long ulWaveform);
    void validate_all(const EAXRINGMODULATORPROPERTIES& all);

    void defer_frequency(float flFrequency);
    void defer_high_pass_cutoff(float flHighPassCutOff);
    void defer_waveform(unsigned long ulWaveform);
    void defer_all(const EAXRINGMODULATORPROPERTIES& all);

    void defer_frequency(const EaxEaxCall& eax_call);
    void defer_high_pass_cutoff(const EaxEaxCall& eax_call);
    void defer_waveform(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxRingModulatorEffect


class EaxRingModulatorEffectException :
    public EaxException
{
public:
    explicit EaxRingModulatorEffectException(
        const char* message)
        :
        EaxException{"EAX_RING_MODULATOR_EFFECT", message}
    {
    }
}; // EaxRingModulatorEffectException


EaxRingModulatorEffect::EaxRingModulatorEffect()
    : EaxEffect{AL_EFFECT_RING_MODULATOR}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxRingModulatorEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxRingModulatorEffect::set_eax_defaults()
{
    eax_.flFrequency = EAXRINGMODULATOR_DEFAULTFREQUENCY;
    eax_.flHighPassCutOff = EAXRINGMODULATOR_DEFAULTHIGHPASSCUTOFF;
    eax_.ulWaveform = EAXRINGMODULATOR_DEFAULTWAVEFORM;

    eax_d_ = eax_;
}

void EaxRingModulatorEffect::set_efx_frequency()
{
    const auto frequency = clamp(
        eax_.flFrequency,
        AL_RING_MODULATOR_MIN_FREQUENCY,
        AL_RING_MODULATOR_MAX_FREQUENCY);

    al_effect_props_.Modulator.Frequency = frequency;
}

void EaxRingModulatorEffect::set_efx_high_pass_cutoff()
{
    const auto high_pass_cutoff = clamp(
        eax_.flHighPassCutOff,
        AL_RING_MODULATOR_MIN_HIGHPASS_CUTOFF,
        AL_RING_MODULATOR_MAX_HIGHPASS_CUTOFF);

    al_effect_props_.Modulator.HighPassCutoff = high_pass_cutoff;
}

void EaxRingModulatorEffect::set_efx_waveform()
{
    const auto waveform = clamp(
        static_cast<ALint>(eax_.ulWaveform),
        AL_RING_MODULATOR_MIN_WAVEFORM,
        AL_RING_MODULATOR_MAX_WAVEFORM);

    const auto efx_waveform = WaveformFromEmum(waveform);
    assert(efx_waveform.has_value());
    al_effect_props_.Modulator.Waveform = *efx_waveform;
}

void EaxRingModulatorEffect::set_efx_defaults()
{
    set_efx_frequency();
    set_efx_high_pass_cutoff();
    set_efx_waveform();
}

void EaxRingModulatorEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXRINGMODULATOR_NONE:
            break;

        case EAXRINGMODULATOR_ALLPARAMETERS:
            eax_call.set_value<EaxRingModulatorEffectException>(eax_);
            break;

        case EAXRINGMODULATOR_FREQUENCY:
            eax_call.set_value<EaxRingModulatorEffectException>(eax_.flFrequency);
            break;

        case EAXRINGMODULATOR_HIGHPASSCUTOFF:
            eax_call.set_value<EaxRingModulatorEffectException>(eax_.flHighPassCutOff);
            break;

        case EAXRINGMODULATOR_WAVEFORM:
            eax_call.set_value<EaxRingModulatorEffectException>(eax_.ulWaveform);
            break;

        default:
            throw EaxRingModulatorEffectException{"Unsupported property id."};
    }
}

void EaxRingModulatorEffect::validate_frequency(
    float flFrequency)
{
    eax_validate_range<EaxRingModulatorEffectException>(
        "Frequency",
        flFrequency,
        EAXRINGMODULATOR_MINFREQUENCY,
        EAXRINGMODULATOR_MAXFREQUENCY);
}

void EaxRingModulatorEffect::validate_high_pass_cutoff(
    float flHighPassCutOff)
{
    eax_validate_range<EaxRingModulatorEffectException>(
        "High-Pass Cutoff",
        flHighPassCutOff,
        EAXRINGMODULATOR_MINHIGHPASSCUTOFF,
        EAXRINGMODULATOR_MAXHIGHPASSCUTOFF);
}

void EaxRingModulatorEffect::validate_waveform(
    unsigned long ulWaveform)
{
    eax_validate_range<EaxRingModulatorEffectException>(
        "Waveform",
        ulWaveform,
        EAXRINGMODULATOR_MINWAVEFORM,
        EAXRINGMODULATOR_MAXWAVEFORM);
}

void EaxRingModulatorEffect::validate_all(
    const EAXRINGMODULATORPROPERTIES& all)
{
    validate_frequency(all.flFrequency);
    validate_high_pass_cutoff(all.flHighPassCutOff);
    validate_waveform(all.ulWaveform);
}

void EaxRingModulatorEffect::defer_frequency(
    float flFrequency)
{
    eax_d_.flFrequency = flFrequency;
    eax_dirty_flags_.flFrequency = (eax_.flFrequency != eax_d_.flFrequency);
}

void EaxRingModulatorEffect::defer_high_pass_cutoff(
    float flHighPassCutOff)
{
    eax_d_.flHighPassCutOff = flHighPassCutOff;
    eax_dirty_flags_.flHighPassCutOff = (eax_.flHighPassCutOff != eax_d_.flHighPassCutOff);
}

void EaxRingModulatorEffect::defer_waveform(
    unsigned long ulWaveform)
{
    eax_d_.ulWaveform = ulWaveform;
    eax_dirty_flags_.ulWaveform = (eax_.ulWaveform != eax_d_.ulWaveform);
}

void EaxRingModulatorEffect::defer_all(
    const EAXRINGMODULATORPROPERTIES& all)
{
    defer_frequency(all.flFrequency);
    defer_high_pass_cutoff(all.flHighPassCutOff);
    defer_waveform(all.ulWaveform);
}

void EaxRingModulatorEffect::defer_frequency(
    const EaxEaxCall& eax_call)
{
    const auto& frequency =
        eax_call.get_value<
        EaxRingModulatorEffectException, const decltype(EAXRINGMODULATORPROPERTIES::flFrequency)>();

    validate_frequency(frequency);
    defer_frequency(frequency);
}

void EaxRingModulatorEffect::defer_high_pass_cutoff(
    const EaxEaxCall& eax_call)
{
    const auto& high_pass_cutoff =
        eax_call.get_value<
        EaxRingModulatorEffectException, const decltype(EAXRINGMODULATORPROPERTIES::flHighPassCutOff)>();

    validate_high_pass_cutoff(high_pass_cutoff);
    defer_high_pass_cutoff(high_pass_cutoff);
}

void EaxRingModulatorEffect::defer_waveform(
    const EaxEaxCall& eax_call)
{
    const auto& waveform =
        eax_call.get_value<
        EaxRingModulatorEffectException, const decltype(EAXRINGMODULATORPROPERTIES::ulWaveform)>();

    validate_waveform(waveform);
    defer_waveform(waveform);
}

void EaxRingModulatorEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxRingModulatorEffectException, const EAXRINGMODULATORPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxRingModulatorEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxRingModulatorEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.flFrequency)
    {
        set_efx_frequency();
    }

    if (eax_dirty_flags_.flHighPassCutOff)
    {
        set_efx_high_pass_cutoff();
    }

    if (eax_dirty_flags_.ulWaveform)
    {
        set_efx_waveform();
    }

    eax_dirty_flags_ = EaxRingModulatorEffectDirtyFlags{};

    return true;
}

void EaxRingModulatorEffect::set(const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case EAXRINGMODULATOR_NONE:
            break;

        case EAXRINGMODULATOR_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXRINGMODULATOR_FREQUENCY:
            defer_frequency(eax_call);
            break;

        case EAXRINGMODULATOR_HIGHPASSCUTOFF:
            defer_high_pass_cutoff(eax_call);
            break;

        case EAXRINGMODULATOR_WAVEFORM:
            defer_waveform(eax_call);
            break;

        default:
            throw EaxRingModulatorEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_ring_modulator_effect()
{
    return std::make_unique<EaxRingModulatorEffect>();
}

#endif // ALSOFT_EAX
