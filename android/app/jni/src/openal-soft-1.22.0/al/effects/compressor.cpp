
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

void Compressor_setParami(EffectProps *props, ALenum param, int val)
{
    switch(param)
    {
    case AL_COMPRESSOR_ONOFF:
        if(!(val >= AL_COMPRESSOR_MIN_ONOFF && val <= AL_COMPRESSOR_MAX_ONOFF))
            throw effect_exception{AL_INVALID_VALUE, "Compressor state out of range"};
        props->Compressor.OnOff = (val != AL_FALSE);
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid compressor integer property 0x%04x",
            param};
    }
}
void Compressor_setParamiv(EffectProps *props, ALenum param, const int *vals)
{ Compressor_setParami(props, param, vals[0]); }
void Compressor_setParamf(EffectProps*, ALenum param, float)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid compressor float property 0x%04x", param}; }
void Compressor_setParamfv(EffectProps*, ALenum param, const float*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid compressor float-vector property 0x%04x",
        param};
}

void Compressor_getParami(const EffectProps *props, ALenum param, int *val)
{ 
    switch(param)
    {
    case AL_COMPRESSOR_ONOFF:
        *val = props->Compressor.OnOff;
        break;

    default:
        throw effect_exception{AL_INVALID_ENUM, "Invalid compressor integer property 0x%04x",
            param};
    }
}
void Compressor_getParamiv(const EffectProps *props, ALenum param, int *vals)
{ Compressor_getParami(props, param, vals); }
void Compressor_getParamf(const EffectProps*, ALenum param, float*)
{ throw effect_exception{AL_INVALID_ENUM, "Invalid compressor float property 0x%04x", param}; }
void Compressor_getParamfv(const EffectProps*, ALenum param, float*)
{
    throw effect_exception{AL_INVALID_ENUM, "Invalid compressor float-vector property 0x%04x",
        param};
}

EffectProps genDefaultProps() noexcept
{
    EffectProps props{};
    props.Compressor.OnOff = AL_COMPRESSOR_DEFAULT_ONOFF;
    return props;
}

} // namespace

DEFINE_ALEFFECT_VTABLE(Compressor);

const EffectProps CompressorEffectProps{genDefaultProps()};

#ifdef ALSOFT_EAX
namespace {

using EaxCompressorEffectDirtyFlagsValue = std::uint_least8_t;

struct EaxCompressorEffectDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxCompressorEffectDirtyFlagsValue ulOnOff : 1;
}; // EaxCompressorEffectDirtyFlags


class EaxCompressorEffect final :
    public EaxEffect
{
public:
    EaxCompressorEffect();


    void dispatch(const EaxEaxCall& eax_call) override;

    // [[nodiscard]]
    bool apply_deferred() override;

private:
    EAXAGCCOMPRESSORPROPERTIES eax_{};
    EAXAGCCOMPRESSORPROPERTIES eax_d_{};
    EaxCompressorEffectDirtyFlags eax_dirty_flags_{};


    void set_eax_defaults();

    void set_efx_on_off();
    void set_efx_defaults();

    void get(const EaxEaxCall& eax_call);

    void validate_on_off(unsigned long ulOnOff);
    void validate_all(const EAXAGCCOMPRESSORPROPERTIES& eax_all);

    void defer_on_off(unsigned long ulOnOff);
    void defer_all(const EAXAGCCOMPRESSORPROPERTIES& eax_all);

    void defer_on_off(const EaxEaxCall& eax_call);
    void defer_all(const EaxEaxCall& eax_call);

    void set(const EaxEaxCall& eax_call);
}; // EaxCompressorEffect


class EaxCompressorEffectException :
    public EaxException
{
public:
    explicit EaxCompressorEffectException(
        const char* message)
        :
        EaxException{"EAX_COMPRESSOR_EFFECT", message}
    {
    }
}; // EaxCompressorEffectException


EaxCompressorEffect::EaxCompressorEffect()
    : EaxEffect{AL_EFFECT_COMPRESSOR}
{
    set_eax_defaults();
    set_efx_defaults();
}

// [[nodiscard]]
void EaxCompressorEffect::dispatch(const EaxEaxCall& eax_call)
{
    eax_call.is_get() ? get(eax_call) : set(eax_call);
}

void EaxCompressorEffect::set_eax_defaults()
{
    eax_.ulOnOff = EAXAGCCOMPRESSOR_DEFAULTONOFF;

    eax_d_ = eax_;
}

void EaxCompressorEffect::set_efx_on_off()
{
    const auto on_off = clamp(
        static_cast<ALint>(eax_.ulOnOff),
        AL_COMPRESSOR_MIN_ONOFF,
        AL_COMPRESSOR_MAX_ONOFF);

    al_effect_props_.Compressor.OnOff = (on_off != AL_FALSE);
}

void EaxCompressorEffect::set_efx_defaults()
{
    set_efx_on_off();
}

void EaxCompressorEffect::get(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXAGCCOMPRESSOR_NONE:
            break;

        case EAXAGCCOMPRESSOR_ALLPARAMETERS:
            eax_call.set_value<EaxCompressorEffectException>(eax_);
            break;

        case EAXAGCCOMPRESSOR_ONOFF:
            eax_call.set_value<EaxCompressorEffectException>(eax_.ulOnOff);
            break;

        default:
            throw EaxCompressorEffectException{"Unsupported property id."};
    }
}

void EaxCompressorEffect::validate_on_off(
    unsigned long ulOnOff)
{
    eax_validate_range<EaxCompressorEffectException>(
        "On-Off",
        ulOnOff,
        EAXAGCCOMPRESSOR_MINONOFF,
        EAXAGCCOMPRESSOR_MAXONOFF);
}

void EaxCompressorEffect::validate_all(
    const EAXAGCCOMPRESSORPROPERTIES& eax_all)
{
    validate_on_off(eax_all.ulOnOff);
}

void EaxCompressorEffect::defer_on_off(
    unsigned long ulOnOff)
{
    eax_d_.ulOnOff = ulOnOff;
    eax_dirty_flags_.ulOnOff = (eax_.ulOnOff != eax_d_.ulOnOff);
}

void EaxCompressorEffect::defer_all(
    const EAXAGCCOMPRESSORPROPERTIES& eax_all)
{
    defer_on_off(eax_all.ulOnOff);
}

void EaxCompressorEffect::defer_on_off(
    const EaxEaxCall& eax_call)
{
    const auto& on_off =
        eax_call.get_value<EaxCompressorEffectException, const decltype(EAXAGCCOMPRESSORPROPERTIES::ulOnOff)>();

    validate_on_off(on_off);
    defer_on_off(on_off);
}

void EaxCompressorEffect::defer_all(
    const EaxEaxCall& eax_call)
{
    const auto& all =
        eax_call.get_value<EaxCompressorEffectException, const EAXAGCCOMPRESSORPROPERTIES>();

    validate_all(all);
    defer_all(all);
}

// [[nodiscard]]
bool EaxCompressorEffect::apply_deferred()
{
    if (eax_dirty_flags_ == EaxCompressorEffectDirtyFlags{})
    {
        return false;
    }

    eax_ = eax_d_;

    if (eax_dirty_flags_.ulOnOff)
    {
        set_efx_on_off();
    }

    eax_dirty_flags_ = EaxCompressorEffectDirtyFlags{};

    return true;
}

void EaxCompressorEffect::set(const EaxEaxCall& eax_call)
{
    switch(eax_call.get_property_id())
    {
        case EAXAGCCOMPRESSOR_NONE:
            break;

        case EAXAGCCOMPRESSOR_ALLPARAMETERS:
            defer_all(eax_call);
            break;

        case EAXAGCCOMPRESSOR_ONOFF:
            defer_on_off(eax_call);
            break;

        default:
            throw EaxCompressorEffectException{"Unsupported property id."};
    }
}

} // namespace

EaxEffectUPtr eax_create_eax_compressor_effect()
{
    return std::make_unique<EaxCompressorEffect>();
}

#endif // ALSOFT_EAX
