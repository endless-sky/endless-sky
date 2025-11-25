#ifndef EAX_EFFECT_INCLUDED
#define EAX_EFFECT_INCLUDED


#include <memory>

#include "AL/al.h"
#include "core/effects/base.h"
#include "eax_eax_call.h"

class EaxEffect
{
public:
    EaxEffect(ALenum type) : al_effect_type_{type} { }
    virtual ~EaxEffect() = default;

    const ALenum al_effect_type_;
    EffectProps al_effect_props_{};

    virtual void dispatch(const EaxEaxCall& eax_call) = 0;

    // Returns "true" if any immediated property was changed.
    // [[nodiscard]]
    virtual bool apply_deferred() = 0;
}; // EaxEffect


using EaxEffectUPtr = std::unique_ptr<EaxEffect>;

EaxEffectUPtr eax_create_eax_null_effect();
EaxEffectUPtr eax_create_eax_chorus_effect();
EaxEffectUPtr eax_create_eax_distortion_effect();
EaxEffectUPtr eax_create_eax_echo_effect();
EaxEffectUPtr eax_create_eax_flanger_effect();
EaxEffectUPtr eax_create_eax_frequency_shifter_effect();
EaxEffectUPtr eax_create_eax_vocal_morpher_effect();
EaxEffectUPtr eax_create_eax_pitch_shifter_effect();
EaxEffectUPtr eax_create_eax_ring_modulator_effect();
EaxEffectUPtr eax_create_eax_auto_wah_effect();
EaxEffectUPtr eax_create_eax_compressor_effect();
EaxEffectUPtr eax_create_eax_equalizer_effect();
EaxEffectUPtr eax_create_eax_reverb_effect();

#endif // !EAX_EFFECT_INCLUDED
