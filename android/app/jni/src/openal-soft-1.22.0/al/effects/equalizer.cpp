
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

void Equalizer_setParami(EffectProps*, ALenum param, int)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid equalizer integer property 0x%04x", param}; }
void Equalizer_setParamiv(EffectProps*, ALenum param, const int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid equalizer integer-vector property 0x%04x",
        param};
}
void Equalizer_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_EQUALIZER_LOW_GAIN:
        if(!(val >= AL_EQUALIZER_MIN_LOW_GAIN && val <= AL_EQUALIZER_MAX_LOW_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer low-band gain out of range"};
        props->Equalizer.LowGain = val;
        break;

    case AL_EQUALIZER_LOW_CUTOFF:
        if(!(val >= AL_EQUALIZER_MIN_LOW_CUTOFF && val <= AL_EQUALIZER_MAX_LOW_CUTOFF))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer low-band cutoff out of range"};
        props->Equalizer.LowCutoff = val;
        break;

    case AL_EQUALIZER_MID1_GAIN:
        if(!(val >= AL_EQUALIZER_MIN_MID1_GAIN && val <= AL_EQUALIZER_MAX_MID1_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer mid1-band gain out of range"};
        props->Equalizer.Mid1Gain = val;
        break;

    case AL_EQUALIZER_MID1_CENTER:
        if(!(val >= AL_EQUALIZER_MIN_MID1_CENTER && val <= AL_EQUALIZER_MAX_MID1_CENTER))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer mid1-band center out of range"};
        props->Equalizer.Mid1Center = val;
        break;

    case AL_EQUALIZER_MID1_WIDTH:
        if(!(val >= AL_EQUALIZER_MIN_MID1_WIDTH && val <= AL_EQUALIZER_MAX_MID1_WIDTH))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer mid1-band width out of range"};
        props->Equalizer.Mid1Width = val;
        break;

    case AL_EQUALIZER_MID2_GAIN:
        if(!(val >= AL_EQUALIZER_MIN_MID2_GAIN && val <= AL_EQUALIZER_MAX_MID2_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer mid2-band gain out of range"};
        props->Equalizer.Mid2Gain = val;
        break;

    case AL_EQUALIZER_MID2_CENTER:
        if(!(val >= AL_EQUALIZER_MIN_MID2_CENTER && val <= AL_EQUALIZER_MAX_MID2_CENTER))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer mid2-band center out of range"};
        props->Equalizer.Mid2Center = val;
        break;

    case AL_EQUALIZER_MID2_WIDTH:
        if(!(val >= AL_EQUALIZER_MIN_MID2_WIDTH && val <= AL_EQUALIZER_MAX_MID2_WIDTH))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer mid2-band width out of range"};
        props->Equalizer.Mid2Width = val;
        break;

    case AL_EQUALIZER_HIGH_GAIN:
        if(!(val >= AL_EQUALIZER_MIN_HIGH_GAIN && val <= AL_EQUALIZER_MAX_HIGH_GAIN))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer high-band gain out of range"};
        props->Equalizer.HighGain = val;
        break;

    case AL_EQUALIZER_HIGH_CUTOFF:
        if(!(val >= AL_EQUALIZER_MIN_HIGH_CUTOFF && val <= AL_EQUALIZER_MAX_HIGH_CUTOFF))
            throw effect_exception{AL_INVALID_VALUE, "Equalizer high-band cutoff out of range"};
        props->Equalizer.HighCutoff = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid equalizer float property 0x%04x", param};
    }
}
void Equalizer_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Equalizer_setParamf(props, param, vals[0]); }

void Equalizer_getParami(const EffectProps*, ALenum param, int*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid equalizer integer property 0x%04x", param}; }
void Equalizer_getParamiv(const EffectProps*, ALenum param, int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid equalizer integer-vector property 0x%04x",
        param};
}
void Equalizer_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_EQUALIZER_LOW_GAIN:
        *val = props->Equalizer.LowGain;
        break;

    case AL_EQUALIZER_LOW_CUTOFF:
        *val = props->Equalizer.LowCutoff;
        break;

    case AL_EQUALIZER_MID1_GAIN:
        *val = props->Equalizer.Mid1Gain;
        break;

    case AL_EQUALIZER_MID1_CENTER:
        *val = props->Equalizer.Mid1Center;
        break;

    case AL_EQUALIZER_MID1_WIDTH:
        *val = props->Equalizer.Mid1Width;
        break;

    case AL_EQUALIZER_MID2_GAIN:
        *val = props->Equalizer.Mid2Gain;
        break;

    case AL_EQUALIZER_MID2_CENTER:
        *val = props->Equalizer.Mid2Center;
        break;

    case AL_EQUALIZER_MID2_WIDTH:
        *val = props->Equalizer.Mid2Width;
        break;

    case AL_EQUALIZER_HIGH_GAIN:
        *val = props->Equalizer.HighGain;
        break;

    case AL_EQUALIZER_HIGH_CUTOFF:
        *val = props->Equalizer.HighCutoff;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid equalizer float property 0x%04x", param};
    }
}
void Equalizer_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Equalizer_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Equalizer.LowCutoff = AL_EQUALIZER_DEFAULT_LOW_CUTOFF;
    props.Equalizer.LowGain = AL_EQUALIZER_DEFAULT_LOW_GAIN;
    props.Equalizer.Mid1Center = AL_EQUALIZER_DEFAULT_MID1_CENTER;
    props.Equalizer.Mid1Gain = AL_EQUALIZER_DEFAULT_MID1_GAIN;
    props.Equalizer.Mid1Width = AL_EQUALIZER_DEFAULT_MID1_WIDTH;
    props.Equalizer.Mid2Center = AL_EQUALIZER_DEFAULT_MID2_CENTER;
    props.Equalizer.Mid2Gain = AL_EQUALIZER_DEFAULT_MID2_GAIN;
    props.Equalizer.Mid2Width = AL_EQUALIZER_DEFAULT_MID2_WIDTH;
    props.Equalizer.HighCutoff = AL_EQUALIZER_DEFAULT_HIGH_CUTOFF;
    props.Equalizer.HighGain = AL_EQUALIZER_DEFAULT_HIGH_GAIN;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Equalizer);

const EffectProps EqualizerEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxEqualizerEffectDirtyFlagsValue = std::uint_least16_t;

struct EaxEqualizerEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxEqualizerEffectDirtyFlagsValue lLowGain : 1;
    EaxEqualizerEffectDirtyFlagsValue flLowCutOff : 1;
    EaxEqualizerEffectDirtyFlagsValue lMid1Gain : 1;
    EaxEqualizerEffectDirtyFlagsValue flMid1Center : 1;
    EaxEqualizerEffectDirtyFlagsValue flMid1Width : 1;
    EaxEqualizerEffectDirtyFlagsValue lMid2Gain : 1;
    EaxEqualizerEffectDirtyFlagsValue flMid2Center : 1;
    EaxEqualizerEffectDirtyFlagsValue flMid2Width : 1;
    EaxEqualizerEffectDirtyFlagsValue lHighGain : 1;
    EaxEqualizerEffectDirtyFlagsValue flHighCutOff : 1;
}; // EaxEqualizerEffectDirtyFlags


class EaxEqualizerEffect final :
    public EaxEffect
{
public:
    EaxEqualizerEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXEQUALIZERPROPERTIES eax_{};
    EAXEQUALIZERPROPERTIES eax_d_{};
    EaxEqualizerEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_low_gain();
    void set_efx_low_cutoff();
    void set_efx_mid1_gain();
    void set_efx_mid1_center();
    void set_efx_mid1_width();
    void set_efx_mid2_gain();
    void set_efx_mid2_center();
    void set_efx_mid2_width();
    void set_efx_high_gain();
    void set_efx_high_cutoff();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_low_gain(long lLowGain);
    void validate_low_cutoff(float flLowCutOff);
    void validate_mid1_gain(long lMid1Gain);
    void validate_mid1_center(float flMid1Center);
    void validate_mid1_width(float flMid1Width);
    void validate_mid2_gain(long lMid2Gain);
    void validate_mid2_center(float flMid2Center);
    void validate_mid2_width(float flMid2Width);
    void validate_high_gain(long lHighGain);
    void validate_high_cutoff(float flHighCutOff);
    void validate_all(const EAXEQUALIZERPROPERTIES& all);

    void defer_low_gain(long lLowGain);
    void defer_low_cutoff(float flLowCutOff);
    void defer_mid1_gain(long lMid1Gain);
    void defer_mid1_center(float flMid1Center);
    void defer_mid1_width(float flMid1Width);
    void defer_mid2_gain(long lMid2Gain);
    void defer_mid2_center(float flMid2Center);
    void defer_mid2_width(float flMid2Width);
    void defer_high_gain(long lHighGain);
    void defer_high_cutoff(float flHighCutOff);
    void defer_all(const EAXEQUALIZERPROPERTIES& all);

    void defer_low_gain(const EaxEaxCall& eax_call);
    void defer_low_cutoff(const EaxEaxCall& eax_call);
    void defer_mid1_gain(const EaxEaxCall& eax_call);
    void defer_mid1_center(const EaxEaxCall& eax_call);
    void defer_mid1_width(const EaxEaxCall& eax_call);
    void defer_mid2_gain(const EaxEaxCall& eax_call);
    void defer_mid2_center(const EaxEaxCall& eax_call);
    void defer_mid2_width(const EaxEaxCall& eax_call);
    void defer_high_gain(const EaxEaxCall& eax_call);
    void defer_high_cutoff(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxEqualizerEffect


class EaxEqualizerEffectException :
    public EaxException
{
public:
    explicit EaxEqualizerEffectException(
        const char* message)
        :
        EaxException{"EAX_EQUALIZER_EFFECT", message}
    {
    }
}; // EaxEqualizerEffectException


EaxEqualizerEffect::EaxEqualizerEffect()
    : EaxEffect{AL_EFFECT_EQUALIZER}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxEqualizerEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxEqualizerEffect::set_eax_defaults()
{
    eax_.lLowGain = EAXEQUALIZER_DEFAULTLOWGAIN;
    eax_.flLowCutOff = EAXEQUALIZER_DEFAULTLOWCUTOFF;
    eax_.lMid1Gain = EAXEQUALIZER_DEFAULTMID1GAIN;
    eax_.flMid1Center = EAXEQUALIZER_DEFAULTMID1CENTER;
    eax_.flMid1Width = EAXEQUALIZER_DEFAULTMID1WIDTH;
    eax_.lMid2Gain = EAXEQUALIZER_DEFAULTMID2GAIN;
    eax_.flMid2Center = EAXEQUALIZER_DEFAULTMID2CENTER;
    eax_.flMid2Width = EAXEQUALIZER_DEFAULTMID2WIDTH;
    eax_.lHighGain = EAXEQUALIZER_DEFAULTHIGHGAIN;
    eax_.flHighCutOff = EAXEQUALIZER_DEFAULTHIGHCUTOFF;

    eax_d_ = eax_;
}

void EaxEqualizerEffect::set_efx_low_gain()
{
    const auto low_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lLowGain)),
        AL_EQUALIZER_MIN_LOW_GAIN,
        AL_EQUALIZER_MAX_LOW_GAIN);

    al_effect_props_.Equalizer.LowGain = low_gain;
}

void EaxEqualizerEffect::set_efx_low_cutoff()
{
    const auto low_cutoff = clamp(
        eax_.flLowCutOff,
        AL_EQUALIZER_MIN_LOW_CUTOFF,
        AL_EQUALIZER_MAX_LOW_CUTOFF);

    al_effect_props_.Equalizer.LowCutoff = low_cutoff;
}

void EaxEqualizerEffect::set_efx_mid1_gain()
{
    const auto mid1_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lMid1Gain)),
        AL_EQUALIZER_MIN_MID1_GAIN,
        AL_EQUALIZER_MAX_MID1_GAIN);

    al_effect_props_.Equalizer.Mid1Gain = mid1_gain;
}

void EaxEqualizerEffect::set_efx_mid1_center()
{
    const auto mid1_center = clamp(
        eax_.flMid1Center,
        AL_EQUALIZER_MIN_MID1_CENTER,
        AL_EQUALIZER_MAX_MID1_CENTER);

    al_effect_props_.Equalizer.Mid1Center = mid1_center;
}

void EaxEqualizerEffect::set_efx_mid1_width()
{
    const auto mid1_width = clamp(
        eax_.flMid1Width,
        AL_EQUALIZER_MIN_MID1_WIDTH,
        AL_EQUALIZER_MAX_MID1_WIDTH);

    al_effect_props_.Equalizer.Mid1Width = mid1_width;
}

void EaxEqualizerEffect::set_efx_mid2_gain()
{
    const auto mid2_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lMid2Gain)),
        AL_EQUALIZER_MIN_MID2_GAIN,
        AL_EQUALIZER_MAX_MID2_GAIN);

    al_effect_props_.Equalizer.Mid2Gain = mid2_gain;
}

void EaxEqualizerEffect::set_efx_mid2_center()
{
    const auto mid2_center = clamp(
        eax_.flMid2Center,
        AL_EQUALIZER_MIN_MID2_CENTER,
        AL_EQUALIZER_MAX_MID2_CENTER);

    al_effect_props_.Equalizer.Mid2Center = mid2_center;
}

void EaxEqualizerEffect::set_efx_mid2_width()
{
    const auto mid2_width = clamp(
        eax_.flMid2Width,
        AL_EQUALIZER_MIN_MID2_WIDTH,
        AL_EQUALIZER_MAX_MID2_WIDTH);

    al_effect_props_.Equalizer.Mid2Width = mid2_width;
}

void EaxEqualizerEffect::set_efx_high_gain()
{
    const auto high_gain = clamp(
        level_mb_to_gain(static_cast<float>(eax_.lHighGain)),
        AL_EQUALIZER_MIN_HIGH_GAIN,
        AL_EQUALIZER_MAX_HIGH_GAIN);

    al_effect_props_.Equalizer.HighGain = high_gain;
}

void EaxEqualizerEffect::set_efx_high_cutoff()
{
    const auto high_cutoff = clamp(
        eax_.flHighCutOff,
        AL_EQUALIZER_MIN_HIGH_CUTOFF,
        AL_EQUALIZER_MAX_HIGH_CUTOFF);

    al_effect_props_.Equalizer.HighCutoff = high_cutoff;
}

void EaxEqualizerEffect::set_efx_defaults()
{
    set_efx_low_gain();
    set_efx_low_cutoff();
    set_efx_mid1_gain();
    set_efx_mid1_center();
    set_efx_mid1_width();
    set_efx_mid2_gain();
    set_efx_mid2_center();
    set_efx_mid2_width();
    set_efx_high_gain();
    set_efx_high_cutoff();
}

void EaxEqualizerEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXEQUALIZER_NONE:
            break;

        case EAXEQUALIZER_ALLPARAMETERS:
            eax_call.set_value<EaxEqualizerEffectException>(eax_);
            break;

        case EAXEQUALIZER_LOWGAIN:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.lLowGain);
            break;

        case EAXEQUALIZER_LOWCUTOFF:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.flLowCutOff);
            break;

        case EAXEQUALIZER_MID1GAIN:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.lMid1Gain);
            break;

        case EAXEQUALIZER_MID1CENTER:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.flMid1Center);
            break;

        case EAXEQUALIZER_MID1WIDTH:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.flMid1Width);
            break;

        case EAXEQUALIZER_MID2GAIN:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.lMid2Gain);
            break;

        case EAXEQUALIZER_MID2CENTER:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.flMid2Center);
            break;

        case EAXEQUALIZER_MID2WIDTH:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.flMid2Width);
            break;

        case EAXEQUALIZER_HIGHGAIN:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.lHighGain);
            break;

        case EAXEQUALIZER_HIGHCUTOFF:
            eax_call.set_value<EaxEqualizerEffectException>(eax_.flHighCutOff);
            break;

        default:
            throw EaxEqualizerEffectException{"Unsupported property id."};
    }
}

void EaxEqualizerEffect::validate_low_gain(
    long lLowGain)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Low Gain",
        lLowGain,
        EAXEQUALIZER_MINLOWGAIN,
        EAXEQUALIZER_MAXLOWGAIN);
}

void EaxEqualizerEffect::validate_low_cutoff(
    float flLowCutOff)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Low Cutoff",
        flLowCutOff,
        EAXEQUALIZER_MINLOWCUTOFF,
        EAXEQUALIZER_MAXLOWCUTOFF);
}

void EaxEqualizerEffect::validate_mid1_gain(
    long lMid1Gain)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Mid1 Gain",
        lMid1Gain,
        EAXEQUALIZER_MINMID1GAIN,
        EAXEQUALIZER_MAXMID1GAIN);
}

void EaxEqualizerEffect::validate_mid1_center(
    float flMid1Center)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Mid1 Center",
        flMid1Center,
        EAXEQUALIZER_MINMID1CENTER,
        EAXEQUALIZER_MAXMID1CENTER);
}

void EaxEqualizerEffect::validate_mid1_width(
    float flMid1Width)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Mid1 Width",
        flMid1Width,
        EAXEQUALIZER_MINMID1WIDTH,
        EAXEQUALIZER_MAXMID1WIDTH);
}

void EaxEqualizerEffect::validate_mid2_gain(
    long lMid2Gain)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Mid2 Gain",
        lMid2Gain,
        EAXEQUALIZER_MINMID2GAIN,
        EAXEQUALIZER_MAXMID2GAIN);
}

void EaxEqualizerEffect::validate_mid2_center(
    float flMid2Center)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Mid2 Center",
        flMid2Center,
        EAXEQUALIZER_MINMID2CENTER,
        EAXEQUALIZER_MAXMID2CENTER);
}

void EaxEqualizerEffect::validate_mid2_width(
    float flMid2Width)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "Mid2 Width",
        flMid2Width,
        EAXEQUALIZER_MINMID2WIDTH,
        EAXEQUALIZER_MAXMID2WIDTH);
}

void EaxEqualizerEffect::validate_high_gain(
    long lHighGain)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "High Gain",
        lHighGain,
        EAXEQUALIZER_MINHIGHGAIN,
        EAXEQUALIZER_MAXHIGHGAIN);
}

void EaxEqualizerEffect::validate_high_cutoff(
    float flHighCutOff)
{
    eax_validate_range<EaxEqualizerEffectException>(
        "High Cutoff",
        flHighCutOff,
        EAXEQUALIZER_MINHIGHCUTOFF,
        EAXEQUALIZER_MAXHIGHCUTOFF);
}

void EaxEqualizerEffect::validate_all(
    const EAXEQUALIZERPROPERTIES& all)
{
    validate_low_gain(all.lLowGain);
    validate_low_cutoff(all.flLowCutOff);
    validate_mid1_gain(all.lMid1Gain);
    validate_mid1_center(all.flMid1Center);
    validate_mid1_width(all.flMid1Width);
    validate_mid2_gain(all.lMid2Gain);
    validate_mid2_center(all.flMid2Center);
    validate_mid2_width(all.flMid2Width);
    validate_high_gain(all.lHighGain);
    validate_high_cutoff(all.flHighCutOff);
}

void EaxEqualizerEffect::defer_low_gain(
    long lLowGain)
{
    eax_d_.lLowGain = lLowGain;
    eax_dirty_flags_.lLowGain = (eax_.lLowGain != eax_d_.lLowGain);
}

void EaxEqualizerEffect::defer_low_cutoff(
    float flLowCutOff)
{
    eax_d_.flLowCutOff = flLowCutOff;
    eax_dirty_flags_.flLowCutOff = (eax_.flLowCutOff != eax_d_.flLowCutOff);
}

void EaxEqualizerEffect::defer_mid1_gain(
    long lMid1Gain)
{
    eax_d_.lMid1Gain = lMid1Gain;
    eax_dirty_flags_.lMid1Gain = (eax_.lMid1Gain != eax_d_.lMid1Gain);
}

void EaxEqualizerEffect::defer_mid1_center(
    float flMid1Center)
{
    eax_d_.flMid1Center = flMid1Center;
    eax_dirty_flags_.flMid1Center = (eax_.flMid1Center != eax_d_.flMid1Center);
}

void EaxEqualizerEffect::defer_mid1_width(
    float flMid1Width)
{
    eax_d_.flMid1Width = flMid1Width;
    eax_dirty_flags_.flMid1Width = (eax_.flMid1Width != eax_d_.flMid1Width);
}

void EaxEqualizerEffect::defer_mid2_gain(
    long lMid2Gain)
{
    eax_d_.lMid2Gain = lMid2Gain;
    eax_dirty_flags_.lMid2Gain = (eax_.lMid2Gain != eax_d_.lMid2Gain);
}

void EaxEqualizerEffect::defer_mid2_center(
    float flMid2Center)
{
    eax_d_.flMid2Center = flMid2Center;
    eax_dirty_flags_.flMid2Center = (eax_.flMid2Center != eax_d_.flMid2Center);
}

void EaxEqualizerEffect::defer_mid2_width(
    float flMid2Width)
{
    eax_d_.flMid2Width = flMid2Width;
    eax_dirty_flags_.flMid2Width = (eax_.flMid2Width != eax_d_.flMid2Width);
}

void EaxEqualizerEffect::defer_high_gain(
    long lHighGain)
{
    eax_d_.lHighGain = lHighGain;
    eax_dirty_flags_.lHighGain = (eax_.lHighGain != eax_d_.lHighGain);
}

void EaxEqualizerEffect::defer_high_cutoff(
    float flHighCutOff)
{
    eax_d_.flHighCutOff = flHighCutOff;
    eax_dirty_flags_.flHighCutOff = (eax_.flHighCutOff != eax_d_.flHighCutOff);
}

void EaxEqualizerEffect::defer_all(
    const EAXEQUALIZERPROPERTIES& all)
{
    defer_low_gain(all.lLowGain);
    defer_low_cutoff(all.flLowCutOff);
    defer_mid1_gain(all.lMid1Gain);
    defer_mid1_center(all.flMid1Center);
    defer_mid1_width(all.flMid1Width);
    defer_mid2_gain(all.lMid2Gain);
    defer_mid2_center(all.flMid2Center);
    defer_mid2_width(all.flMid2Width);
    defer_high_gain(all.lHighGain);
    defer_high_cutoff(all.flHighCutOff);
}

void EaxEqualizerEffect::defer_low_gain(
    const EaxEaxCall& eax_call)
{
    const auto& low_gain =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::lLowGain)>();

    validate_low_gain(low_gain);
    defer_low_gain(low_gain);
}

void EaxEqualizerEffect::defer_low_cutoff(
    const EaxEaxCall& eax_call)
{
    const auto& low_cutoff =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::flLowCutOff)>();

    validate_low_cutoff(low_cutoff);
    defer_low_cutoff(low_cutoff);
}

void EaxEqualizerEffect::defer_mid1_gain(
    const EaxEaxCall& eax_call)
{
    const auto& mid1_gain =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::lMid1Gain)>();

    validate_mid1_gain(mid1_gain);
    defer_mid1_gain(mid1_gain);
}

void EaxEqualizerEffect::defer_mid1_center(
    const EaxEaxCall& eax_call)
{
    const auto& mid1_center =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::flMid1Center)>();

    validate_mid1_center(mid1_center);
    defer_mid1_center(mid1_center);
}

void EaxEqualizerEffect::defer_mid1_width(
    const EaxEaxCall& eax_call)
{
    const auto& mid1_width =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::flMid1Width)>();

    validate_mid1_width(mid1_width);
    defer_mid1_width(mid1_width);
}

void EaxEqualizerEffect::defer_mid2_gain(
    const EaxEaxCall& eax_call)
{
    const auto& mid2_gain =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::lMid2Gain)>();

    validate_mid2_gain(mid2_gain);
    defer_mid2_gain(mid2_gain);
}

void EaxEqualizerEffect::defer_mid2_center(
    const EaxEaxCall& eax_call)
{
    const auto& mid2_center =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::flMid2Center)>();

    validate_mid2_center(mid2_center);
    defer_mid2_center(mid2_center);
}

void EaxEqualizerEffect::defer_mid2_width(
    const EaxEaxCall& eax_call)
{
    const auto& mid2_width =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::flMid2Width)>();

    validate_mid2_width(mid2_width);
    defer_mid2_width(mid2_width);
}

void EaxEqualizerEffect::defer_high_gain(
    const EaxEaxCall& eax_call)
{
    const auto& high_gain =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::lHighGain)>();

    validate_high_gain(high_gain);
    defer_high_gain(high_gain);
}

void EaxEqualizerEffect::defer_high_cutoff(
    const EaxEaxCall& eax_call)
{
    const auto& high_cutoff =
        eax_call.get_value<EaxEqualizerEffectException, const decltype(EAXEQUALIZERPROPERTIES::flHighCutOff)>();

    validate_high_cutoff(high_cutoff);
    defer_high_cutoff(high_cutoff);
}

void EaxEqualizerEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxEqualizerEffectException, const EAXEQUALIZERPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxEqualizerEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxEqualizerEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.lLowGain)
    {
        set_efx_low_gain();
    }

    if (eax_dirty_flags_.flLowCutOff)
    {
        set_efx_low_cutoff();
    }

    if (eax_dirty_flags_.lMid1Gain)
    {
        set_efx_mid1_gain();
    }

    if (eax_dirty_flags_.flMid1Center)
    {
        set_efx_mid1_center();
    }

    if (eax_dirty_flags_.flMid1Width)
    {
        set_efx_mid1_width();
    }

    if (eax_dirty_flags_.lMid2Gain)
    {
        set_efx_mid2_gain();
    }

    if (eax_dirty_flags_.flMid2Center)
    {
        set_efx_mid2_center();
    }

    if (eax_dirty_flags_.flMid2Width)
    {
        set_efx_mid2_width();
    }

    if (eax_dirty_flags_.lHighGain)
    {
        set_efx_high_gain();
    }

    if (eax_dirty_flags_.flHighCutOff)
    {
        set_efx_high_cutoff();
    }

    eax_dirty_flags_ = EaxEqualizerEffectDirtyFlags{};

    return true;
}

void EaxEqualizerEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXEQUALIZER_NONE:
            break;

        case EAXEQUALIZER_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXEQUALIZER_LOWGAIN:
            defer_low_gain(eax_call);
            break;

        case EAXEQUALIZER_LOWCUTOFF:
            defer_low_cutoff(eax_call);
            break;

        case EAXEQUALIZER_MID1GAIN:
            defer_mid1_gain(eax_call);
            break;

        case EAXEQUALIZER_MID1CENTER:
            defer_mid1_center(eax_call);
            break;

        case EAXEQUALIZER_MID1WIDTH:
            defer_mid1_width(eax_call);
            break;

        case EAXEQUALIZER_MID2GAIN:
            defer_mid2_gain(eax_call);
            break;

        case EAXEQUALIZER_MID2CENTER:
            defer_mid2_center(eax_call);
            break;

        case EAXEQUALIZER_MID2WIDTH:
            defer_mid2_width(eax_call);
            break;

        case EAXEQUALIZER_HIGHGAIN:
            defer_high_gain(eax_call);
            break;

        case EAXEQUALIZER_HIGHCUTOFF:
            defer_high_cutoff(eax_call);
            break;

        default:
            throw EaxEqualizerEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_equalizer_effect()
{
    return std::make_unique<EaxEqualizerEffect>();
}

#endif // ALSOFT_EAX
