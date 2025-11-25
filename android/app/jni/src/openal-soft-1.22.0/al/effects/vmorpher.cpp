
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

al::optional<VMorpherPhenome> PhenomeFromEnum(ALenum val)
{
#define HANDLE_PHENOME(x) case AL_VOCAL_MORPHER_PHONEME_ ## x:                \
    return al::make_optional(VMorpherPhenome::x)
    switch(val)
    {
    HANDLE_PHENOME(A);
    HANDLE_PHENOME(E);
    HANDLE_PHENOME(I);
    HANDLE_PHENOME(O);
    HANDLE_PHENOME(U);
    HANDLE_PHENOME(AA);
    HANDLE_PHENOME(AE);
    HANDLE_PHENOME(AH);
    HANDLE_PHENOME(AO);
    HANDLE_PHENOME(EH);
    HANDLE_PHENOME(ER);
    HANDLE_PHENOME(IH);
    HANDLE_PHENOME(IY);
    HANDLE_PHENOME(UH);
    HANDLE_PHENOME(UW);
    HANDLE_PHENOME(B);
    HANDLE_PHENOME(D);
    HANDLE_PHENOME(F);
    HANDLE_PHENOME(G);
    HANDLE_PHENOME(J);
    HANDLE_PHENOME(K);
    HANDLE_PHENOME(L);
    HANDLE_PHENOME(M);
    HANDLE_PHENOME(N);
    HANDLE_PHENOME(P);
    HANDLE_PHENOME(R);
    HANDLE_PHENOME(S);
    HANDLE_PHENOME(T);
    HANDLE_PHENOME(V);
    HANDLE_PHENOME(Z);
    }
    return al::nullopt;
#undef HANDLE_PHENOME
}
ALenum EnumFromPhenome(VMorpherPhenome phenome)
{
#define HANDLE_PHENOME(x) case VMorpherPhenome::x: return AL_VOCAL_MORPHER_PHONEME_ ## x
    switch(phenome)
    {
    HANDLE_PHENOME(A);
    HANDLE_PHENOME(E);
    HANDLE_PHENOME(I);
    HANDLE_PHENOME(O);
    HANDLE_PHENOME(U);
    HANDLE_PHENOME(AA);
    HANDLE_PHENOME(AE);
    HANDLE_PHENOME(AH);
    HANDLE_PHENOME(AO);
    HANDLE_PHENOME(EH);
    HANDLE_PHENOME(ER);
    HANDLE_PHENOME(IH);
    HANDLE_PHENOME(IY);
    HANDLE_PHENOME(UH);
    HANDLE_PHENOME(UW);
    HANDLE_PHENOME(B);
    HANDLE_PHENOME(D);
    HANDLE_PHENOME(F);
    HANDLE_PHENOME(G);
    HANDLE_PHENOME(J);
    HANDLE_PHENOME(K);
    HANDLE_PHENOME(L);
    HANDLE_PHENOME(M);
    HANDLE_PHENOME(N);
    HANDLE_PHENOME(P);
    HANDLE_PHENOME(R);
    HANDLE_PHENOME(S);
    HANDLE_PHENOME(T);
    HANDLE_PHENOME(V);
    HANDLE_PHENOME(Z);
    }
    throw std::runtime_error{"Invalid phenome: "+std::to_string(static_cast<int>(phenome))};
#undef HANDLE_PHENOME
}

al::optional<VMorpherWaveform> WaveformFromEmum(ALenum value)
{
    switch(value)
    {
    case AL_VOCAL_MORPHER_WAVEFORM_SINUSOID: return al::make_optional(VMorpherWaveform::Sinusoid);
    case AL_VOCAL_MORPHER_WAVEFORM_TRIANGLE: return al::make_optional(VMorpherWaveform::Triangle);
    case AL_VOCAL_MORPHER_WAVEFORM_SAWTOOTH: return al::make_optional(VMorpherWaveform::Sawtooth);
    }
    return al::nullopt;
}
ALenum EnumFromWaveform(VMorpherWaveform type)
{
    switch(type)
    {
    case VMorpherWaveform::Sinusoid: return AL_VOCAL_MORPHER_WAVEFORM_SINUSOID;
    case VMorpherWaveform::Triangle: return AL_VOCAL_MORPHER_WAVEFORM_TRIANGLE;
    case VMorpherWaveform::Sawtooth: return AL_VOCAL_MORPHER_WAVEFORM_SAWTOOTH;
    }
    throw std::runtime_error{"Invalid vocal morpher waveform: " +
        std::to_string(static_cast<int>(type))};
}

void Vmorpher_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_VOCAL_MORPHER_PHONEMEA:
        if(auto phenomeopt = PhenomeFromEnum(val))
            props->Vmorpher.PhonemeA = *phenomeopt;
        else
            throw effect_exception{AL_INVALID_VALUE, "Vocal morpher phoneme-a out of range: 0x%04x", val};
        break;

    case AL_VOCAL_MORPHER_PHONEMEA_COARSE_TUNING:
        if(!(val >= AL_VOCAL_MORPHER_MIN_PHONEMEA_COARSE_TUNING && val <= AL_VOCAL_MORPHER_MAX_PHONEMEA_COARSE_TUNING))
            throw effect_exception{AL_INVALID_VALUE, "Vocal morpher phoneme-a coarse tuning out of range"};
        props->Vmorpher.PhonemeACoarseTuning = val;
        break;

    case AL_VOCAL_MORPHER_PHONEMEB:
        if(auto phenomeopt = PhenomeFromEnum(val))
            props->Vmorpher.PhonemeB = *phenomeopt;
        else
            throw effect_exception{AL_INVALID_VALUE, "Vocal morpher phoneme-b out of range: 0x%04x", val};
        break;

    case AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING:
        if(!(val >= AL_VOCAL_MORPHER_MIN_PHONEMEB_COARSE_TUNING && val <= AL_VOCAL_MORPHER_MAX_PHONEMEB_COARSE_TUNING))
            throw effect_exception{AL_INVALID_VALUE, "Vocal morpher phoneme-b coarse tuning out of range"};
        props->Vmorpher.PhonemeBCoarseTuning = val;
        break;

    case AL_VOCAL_MORPHER_WAVEFORM:
        if(auto formopt = WaveformFromEmum(val))
            props->Vmorpher.Waveform = *formopt;
        else
            throw effect_exception{AL_INVALID_VALUE, "Vocal morpher waveform out of range: 0x%04x", val};
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid vocal morpher integer property 0x%04x",
            param};
    }
}
void Vmorpher_setParamiv(EffectProps*, ALenum param, const int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid vocal morpher integer-vector property 0x%04x",
        param};
}
void Vmorpher_setParamf(EffectProps *props, ALenum param, float val)
{
    switch(param)
    {
    case AL_VOCAL_MORPHER_RATE:
        if(!(val >= AL_VOCAL_MORPHER_MIN_RATE && val <= AL_VOCAL_MORPHER_MAX_RATE))
            throw effect_exception{AL_INVALID_VALUE, "Vocal morpher rate out of range"};
        props->Vmorpher.Rate = val;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid vocal morpher float property 0x%04x",
            param};
    }
}
void Vmorpher_setParamfv(EffectProps *props, ALenum param, const float *vals)
{ Vmorpher_setParamf(props, param, vals[0]); }

void Vmorpher_getParami(const EffectProps *props, ALenum param, int* val)
{
    switch(param)
    {
    case AL_VOCAL_MORPHER_PHONEMEA:
        *val = EnumFromPhenome(props->Vmorpher.PhonemeA);
        break;

    case AL_VOCAL_MORPHER_PHONEMEA_COARSE_TUNING:
        *val = props->Vmorpher.PhonemeACoarseTuning;
        break;

    case AL_VOCAL_MORPHER_PHONEMEB:
        *val = EnumFromPhenome(props->Vmorpher.PhonemeB);
        break;

    case AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING:
        *val = props->Vmorpher.PhonemeBCoarseTuning;
        break;

    case AL_VOCAL_MORPHER_WAVEFORM:
        *val = EnumFromWaveform(props->Vmorpher.Waveform);
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid vocal morpher integer property 0x%04x",
            param};
    }
}
void Vmorpher_getParamiv(const EffectProps*, ALenum param, int*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid vocal morpher integer-vector property 0x%04x",
        param};
}
void Vmorpher_getParamf(const EffectProps *props, ALenum param, float *val)
{
    switch(param)
    {
    case AL_VOCAL_MORPHER_RATE:
        *val = props->Vmorpher.Rate;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid vocal morpher float property 0x%04x",
            param};
    }
}
void Vmorpher_getParamfv(const EffectProps *props, ALenum param, float *vals)
{ Vmorpher_getParamf(props, param, vals); }

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Vmorpher.Rate                 = AL_VOCAL_MORPHER_DEFAULT_RATE;
    props.Vmorpher.PhonemeA             = *PhenomeFromEnum(AL_VOCAL_MORPHER_DEFAULT_PHONEMEA);
    props.Vmorpher.PhonemeB             = *PhenomeFromEnum(AL_VOCAL_MORPHER_DEFAULT_PHONEMEB);
    props.Vmorpher.PhonemeACoarseTuning = AL_VOCAL_MORPHER_DEFAULT_PHONEMEA_COARSE_TUNING;
    props.Vmorpher.PhonemeBCoarseTuning = AL_VOCAL_MORPHER_DEFAULT_PHONEMEB_COARSE_TUNING;
    props.Vmorpher.Waveform             = *WaveformFromEmum(AL_VOCAL_MORPHER_DEFAULT_WAVEFORM);
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Vmorpher);

const EffectProps VmorpherEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxVocalMorpherEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxVocalMorpherEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxVocalMorpherEffectDirtyFlagsValue ulPhonemeA : 1;
    EaxVocalMorpherEffectDirtyFlagsValue lPhonemeACoarseTuning : 1;
    EaxVocalMorpherEffectDirtyFlagsValue ulPhonemeB : 1;
    EaxVocalMorpherEffectDirtyFlagsValue lPhonemeBCoarseTuning : 1;
    EaxVocalMorpherEffectDirtyFlagsValue ulWaveform : 1;
    EaxVocalMorpherEffectDirtyFlagsValue flRate : 1;
}; // EaxPitchShifterEffectDirtyFlags


class EaxVocalMorpherEffect final :
    public EaxEffect
{
public:
    EaxVocalMorpherEffect();

    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXVOCALMORPHERPROPERTIES eax_{};
    EAXVOCALMORPHERPROPERTIES eax_d_{};
    EaxVocalMorpherEffectDirtyFlags eax_dirty_flags_{};

    void set_eax_defaults();

    void set_efx_phoneme_a();
    void set_efx_phoneme_a_coarse_tuning();
    void set_efx_phoneme_b();
    void set_efx_phoneme_b_coarse_tuning();
    void set_efx_waveform();
    void set_efx_rate();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_phoneme_a(unsigned long ulPhonemeA);
    void validate_phoneme_a_coarse_tuning(long lPhonemeACoarseTuning);
    void validate_phoneme_b(unsigned long ulPhonemeB);
    void validate_phoneme_b_coarse_tuning(long lPhonemeBCoarseTuning);
    void validate_waveform(unsigned long ulWaveform);
    void validate_rate(float flRate);
    void validate_all(const EAXVOCALMORPHERPROPERTIES& all);

    void defer_phoneme_a(unsigned long ulPhonemeA);
    void defer_phoneme_a_coarse_tuning(long lPhonemeACoarseTuning);
    void defer_phoneme_b(unsigned long ulPhonemeB);
    void defer_phoneme_b_coarse_tuning(long lPhonemeBCoarseTuning);
    void defer_waveform(unsigned long ulWaveform);
    void defer_rate(float flRate);
    void defer_all(const EAXVOCALMORPHERPROPERTIES& all);

    void defer_phoneme_a(const EaxEaxCall& eax_call);
    void defer_phoneme_a_coarse_tuning(const EaxEaxCall& eax_call);
    void defer_phoneme_b(const EaxEaxCall& eax_call);
    void defer_phoneme_b_coarse_tuning(const EaxEaxCall& eax_call);
    void defer_waveform(const EaxEaxCall& eax_call);
    void defer_rate(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxVocalMorpherEffect


class EaxVocalMorpherEffectException :
    public EaxException
{
public:
    explicit EaxVocalMorpherEffectException(
        const char* message)
        :
        EaxException{"EAX_VOCAL_MORPHER_EFFECT", message}
    {
    }
}; // EaxVocalMorpherEffectException


EaxVocalMorpherEffect::EaxVocalMorpherEffect()
    : EaxEffect{AL_EFFECT_VOCAL_MORPHER}
{
    set_eax_defaults();
    set_efx_defaults();
}

void EaxVocalMorpherEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxVocalMorpherEffect::set_eax_defaults()
{
    eax_.ulPhonemeA = EAXVOCALMORPHER_DEFAULTPHONEMEA;
    eax_.lPhonemeACoarseTuning = EAXVOCALMORPHER_DEFAULTPHONEMEACOARSETUNING;
    eax_.ulPhonemeB = EAXVOCALMORPHER_DEFAULTPHONEMEB;
    eax_.lPhonemeBCoarseTuning = EAXVOCALMORPHER_DEFAULTPHONEMEBCOARSETUNING;
    eax_.ulWaveform = EAXVOCALMORPHER_DEFAULTWAVEFORM;
    eax_.flRate = EAXVOCALMORPHER_DEFAULTRATE;

    eax_d_ = eax_;
}

void EaxVocalMorpherEffect::set_efx_phoneme_a()
{
    const auto phoneme_a = clamp(
        static_cast<ALint>(eax_.ulPhonemeA),
        AL_VOCAL_MORPHER_MIN_PHONEMEA,
        AL_VOCAL_MORPHER_MAX_PHONEMEA);

    const auto efx_phoneme_a = PhenomeFromEnum(phoneme_a);
    assert(efx_phoneme_a.has_value());
    al_effect_props_.Vmorpher.PhonemeA = *efx_phoneme_a;
}

void EaxVocalMorpherEffect::set_efx_phoneme_a_coarse_tuning()
{
    const auto phoneme_a_coarse_tuning = clamp(
        static_cast<ALint>(eax_.lPhonemeACoarseTuning),
        AL_VOCAL_MORPHER_MIN_PHONEMEA_COARSE_TUNING,
        AL_VOCAL_MORPHER_MAX_PHONEMEA_COARSE_TUNING);

    al_effect_props_.Vmorpher.PhonemeACoarseTuning = phoneme_a_coarse_tuning;
}

void EaxVocalMorpherEffect::set_efx_phoneme_b()
{
    const auto phoneme_b = clamp(
        static_cast<ALint>(eax_.ulPhonemeB),
        AL_VOCAL_MORPHER_MIN_PHONEMEB,
        AL_VOCAL_MORPHER_MAX_PHONEMEB);

    const auto efx_phoneme_b = PhenomeFromEnum(phoneme_b);
    assert(efx_phoneme_b.has_value());
    al_effect_props_.Vmorpher.PhonemeB = *efx_phoneme_b;
}

void EaxVocalMorpherEffect::set_efx_phoneme_b_coarse_tuning()
{
    const auto phoneme_b_coarse_tuning = clamp(
        static_cast<ALint>(eax_.lPhonemeBCoarseTuning),
        AL_VOCAL_MORPHER_MIN_PHONEMEB_COARSE_TUNING,
        AL_VOCAL_MORPHER_MAX_PHONEMEB_COARSE_TUNING);

    al_effect_props_.Vmorpher.PhonemeBCoarseTuning = phoneme_b_coarse_tuning;
}

void EaxVocalMorpherEffect::set_efx_waveform()
{
    const auto waveform = clamp(
        static_cast<ALint>(eax_.ulWaveform),
        AL_VOCAL_MORPHER_MIN_WAVEFORM,
        AL_VOCAL_MORPHER_MAX_WAVEFORM);

    const auto wfx_waveform = WaveformFromEmum(waveform);
    assert(wfx_waveform.has_value());
    al_effect_props_.Vmorpher.Waveform = *wfx_waveform;
}

void EaxVocalMorpherEffect::set_efx_rate()
{
    const auto rate = clamp(
        eax_.flRate,
        AL_VOCAL_MORPHER_MIN_RATE,
        AL_VOCAL_MORPHER_MAX_RATE);

    al_effect_props_.Vmorpher.Rate = rate;
}

void EaxVocalMorpherEffect::set_efx_defaults()
{
    set_efx_phoneme_a();
    set_efx_phoneme_a_coarse_tuning();
    set_efx_phoneme_b();
    set_efx_phoneme_b_coarse_tuning();
    set_efx_waveform();
    set_efx_rate();
}

void EaxVocalMorpherEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXVOCALMORPHER_NONE:
            break;

        case EAXVOCALMORPHER_ALLPARAMETERS:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_);
            break;

        case EAXVOCALMORPHER_PHONEMEA:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_.ulPhonemeA);
            break;

        case EAXVOCALMORPHER_PHONEMEACOARSETUNING:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_.lPhonemeACoarseTuning);
            break;

        case EAXVOCALMORPHER_PHONEMEB:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_.ulPhonemeB);
            break;

        case EAXVOCALMORPHER_PHONEMEBCOARSETUNING:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_.lPhonemeBCoarseTuning);
            break;

        case EAXVOCALMORPHER_WAVEFORM:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_.ulWaveform);
            break;

        case EAXVOCALMORPHER_RATE:
            eax_call.set_value<EaxVocalMorpherEffectException>(eax_.flRate);
            break;

        default:
            throw EaxVocalMorpherEffectException{"Unsupported property id."};
    }
}

void EaxVocalMorpherEffect::validate_phoneme_a(
    unsigned long ulPhonemeA)
{
    eax_validate_range<EaxVocalMorpherEffectException>(
        "Phoneme A",
        ulPhonemeA,
        EAXVOCALMORPHER_MINPHONEMEA,
        EAXVOCALMORPHER_MAXPHONEMEA);
}

void EaxVocalMorpherEffect::validate_phoneme_a_coarse_tuning(
    long lPhonemeACoarseTuning)
{
    eax_validate_range<EaxVocalMorpherEffectException>(
        "Phoneme A Coarse Tuning",
        lPhonemeACoarseTuning,
        EAXVOCALMORPHER_MINPHONEMEACOARSETUNING,
        EAXVOCALMORPHER_MAXPHONEMEACOARSETUNING);
}

void EaxVocalMorpherEffect::validate_phoneme_b(
    unsigned long ulPhonemeB)
{
    eax_validate_range<EaxVocalMorpherEffectException>(
        "Phoneme B",
        ulPhonemeB,
        EAXVOCALMORPHER_MINPHONEMEB,
        EAXVOCALMORPHER_MAXPHONEMEB);
}

void EaxVocalMorpherEffect::validate_phoneme_b_coarse_tuning(
    long lPhonemeBCoarseTuning)
{
    eax_validate_range<EaxVocalMorpherEffectException>(
        "Phoneme B Coarse Tuning",
        lPhonemeBCoarseTuning,
        EAXVOCALMORPHER_MINPHONEMEBCOARSETUNING,
        EAXVOCALMORPHER_MAXPHONEMEBCOARSETUNING);
}

void EaxVocalMorpherEffect::validate_waveform(
    unsigned long ulWaveform)
{
    eax_validate_range<EaxVocalMorpherEffectException>(
        "Waveform",
        ulWaveform,
        EAXVOCALMORPHER_MINWAVEFORM,
        EAXVOCALMORPHER_MAXWAVEFORM);
}

void EaxVocalMorpherEffect::validate_rate(
    float flRate)
{
    eax_validate_range<EaxVocalMorpherEffectException>(
        "Rate",
        flRate,
        EAXVOCALMORPHER_MINRATE,
        EAXVOCALMORPHER_MAXRATE);
}

void EaxVocalMorpherEffect::validate_all(
    const EAXVOCALMORPHERPROPERTIES& all)
{
    validate_phoneme_a(all.ulPhonemeA);
    validate_phoneme_a_coarse_tuning(all.lPhonemeACoarseTuning);
    validate_phoneme_b(all.ulPhonemeB);
    validate_phoneme_b_coarse_tuning(all.lPhonemeBCoarseTuning);
    validate_waveform(all.ulWaveform);
    validate_rate(all.flRate);
}

void EaxVocalMorpherEffect::defer_phoneme_a(
    unsigned long ulPhonemeA)
{
    eax_d_.ulPhonemeA = ulPhonemeA;
    eax_dirty_flags_.ulPhonemeA = (eax_.ulPhonemeA != eax_d_.ulPhonemeA);
}

void EaxVocalMorpherEffect::defer_phoneme_a_coarse_tuning(
    long lPhonemeACoarseTuning)
{
    eax_d_.lPhonemeACoarseTuning = lPhonemeACoarseTuning;
    eax_dirty_flags_.lPhonemeACoarseTuning = (eax_.lPhonemeACoarseTuning != eax_d_.lPhonemeACoarseTuning);
}

void EaxVocalMorpherEffect::defer_phoneme_b(
    unsigned long ulPhonemeB)
{
    eax_d_.ulPhonemeB = ulPhonemeB;
    eax_dirty_flags_.ulPhonemeB = (eax_.ulPhonemeB != eax_d_.ulPhonemeB);
}

void EaxVocalMorpherEffect::defer_phoneme_b_coarse_tuning(
    long lPhonemeBCoarseTuning)
{
    eax_d_.lPhonemeBCoarseTuning = lPhonemeBCoarseTuning;
    eax_dirty_flags_.lPhonemeBCoarseTuning = (eax_.lPhonemeBCoarseTuning != eax_d_.lPhonemeBCoarseTuning);
}

void EaxVocalMorpherEffect::defer_waveform(
    unsigned long ulWaveform)
{
    eax_d_.ulWaveform = ulWaveform;
    eax_dirty_flags_.ulWaveform = (eax_.ulWaveform != eax_d_.ulWaveform);
}

void EaxVocalMorpherEffect::defer_rate(
    float flRate)
{
    eax_d_.flRate = flRate;
    eax_dirty_flags_.flRate = (eax_.flRate != eax_d_.flRate);
}

void EaxVocalMorpherEffect::defer_all(
    const EAXVOCALMORPHERPROPERTIES& all)
{
    defer_phoneme_a(all.ulPhonemeA);
    defer_phoneme_a_coarse_tuning(all.lPhonemeACoarseTuning);
    defer_phoneme_b(all.ulPhonemeB);
    defer_phoneme_b_coarse_tuning(all.lPhonemeBCoarseTuning);
    defer_waveform(all.ulWaveform);
    defer_rate(all.flRate);
}

void EaxVocalMorpherEffect::defer_phoneme_a(
    const EaxEaxCall& eax_call)
{
    const auto& phoneme_a = eax_call.get_value<EaxVocalMorpherEffectException,
        const decltype(EAXVOCALMORPHERPROPERTIES::ulPhonemeA)>();

    validate_phoneme_a(phoneme_a);
    defer_phoneme_a(phoneme_a);
}

void EaxVocalMorpherEffect::defer_phoneme_a_coarse_tuning(
    const EaxEaxCall& eax_call)
{
    const auto& phoneme_a_coarse_tuning = eax_call.get_value<
        EaxVocalMorpherEffectException,
        const decltype(EAXVOCALMORPHERPROPERTIES::lPhonemeACoarseTuning)
    >();

    validate_phoneme_a_coarse_tuning(phoneme_a_coarse_tuning);
    defer_phoneme_a_coarse_tuning(phoneme_a_coarse_tuning);
}

void EaxVocalMorpherEffect::defer_phoneme_b(
    const EaxEaxCall& eax_call)
{
    const auto& phoneme_b = eax_call.get_value<
        EaxVocalMorpherEffectException,
        const decltype(EAXVOCALMORPHERPROPERTIES::ulPhonemeB)
    >();

    validate_phoneme_b(phoneme_b);
    defer_phoneme_b(phoneme_b);
}

void EaxVocalMorpherEffect::defer_phoneme_b_coarse_tuning(
    const EaxEaxCall& eax_call)
{
    const auto& phoneme_b_coarse_tuning = eax_call.get_value<
        EaxVocalMorpherEffectException,
        const decltype(EAXVOCALMORPHERPROPERTIES::lPhonemeBCoarseTuning)
    >();

    validate_phoneme_b_coarse_tuning(phoneme_b_coarse_tuning);
    defer_phoneme_b_coarse_tuning(phoneme_b_coarse_tuning);
}

void EaxVocalMorpherEffect::defer_waveform(
    const EaxEaxCall& eax_call)
{
    const auto& waveform = eax_call.get_value<
        EaxVocalMorpherEffectException,
        const decltype(EAXVOCALMORPHERPROPERTIES::ulWaveform)
    >();

    validate_waveform(waveform);
    defer_waveform(waveform);
}

void EaxVocalMorpherEffect::defer_rate(
    const EaxEaxCall& eax_call)
{
    const auto& rate = eax_call.get_value<
        EaxVocalMorpherEffectException,
        const decltype(EAXVOCALMORPHERPROPERTIES::flRate)
    >();

    validate_rate(rate);
    defer_rate(rate);
}

void EaxVocalMorpherEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all = eax_call.get_value<
        EaxVocalMorpherEffectException,
        const EAXVOCALMORPHERPROPERTIES
    >();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxVocalMorpherEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxVocalMorpherEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.ulPhonemeA)
    {
        set_efx_phoneme_a();
    }

    if (eax_dirty_flags_.lPhonemeACoarseTuning)
    {
        set_efx_phoneme_a_coarse_tuning();
    }

    if (eax_dirty_flags_.ulPhonemeB)
    {
        set_efx_phoneme_b();
    }

    if (eax_dirty_flags_.lPhonemeBCoarseTuning)
    {
        set_efx_phoneme_b_coarse_tuning();
    }

    if (eax_dirty_flags_.ulWaveform)
    {
        set_efx_waveform();
    }

    if (eax_dirty_flags_.flRate)
    {
        set_efx_rate();
    }

    eax_dirty_flags_ = EaxVocalMorpherEffectDirtyFlags{};

    return true;
}

void EaxVocalMorpherEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXVOCALMORPHER_NONE:
            break;

        case EAXVOCALMORPHER_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXVOCALMORPHER_PHONEMEA:
            defer_phoneme_a(eax_call);
            break;

        case EAXVOCALMORPHER_PHONEMEACOARSETUNING:
            defer_phoneme_a_coarse_tuning(eax_call);
            break;

        case EAXVOCALMORPHER_PHONEMEB:
            defer_phoneme_b(eax_call);
            break;

        case EAXVOCALMORPHER_PHONEMEBCOARSETUNING:
            defer_phoneme_b_coarse_tuning(eax_call);
            break;

        case EAXVOCALMORPHER_WAVEFORM:
            defer_waveform(eax_call);
            break;

        case EAXVOCALMORPHER_RATE:
            defer_rate(eax_call);
            break;

        default:
            throw EaxVocalMorpherEffectException{"Unsupported property id."};
    }
}

} // namespace


EaxEffectUPtr eax_create_eax_vocal_morpher_effect()
{
    return std::make_unique<EaxVocalMorpherEffect>();
}

#endif // ALSOFT_EAX
