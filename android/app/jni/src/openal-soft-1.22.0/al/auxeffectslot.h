#ifndef AL_AUXEFFECTSLOT_H
#define AL_AUXEFFECTSLOT_H

#include <atomic>
#include <cstddef>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/efx.h"

#include "alc/device.h"
#include "alc/effects/base.h"
#include "almalloc.h"
#include "atomic.h"
#include "core/effectslot.h"
#include "intrusive_ptr.h"
#include "vector.h"

#ifdef ALSOFT_EAX
#include <memory>

#include "eax_eax_call.h"
#include "eax_effect.h"
#include "eax_fx_slot_index.h"
#endif // ALSOFT_EAX

struct ALbuffer;
struct ALeffect;
struct WetBuffer;


enum class SlotState : ALenum {
    Initial = AL_INITIAL,
    Playing = AL_PLAYING,
    Stopped = AL_STOPPED,
};

struct ALeffectslot {
    float Gain{1.0f};
    bool  AuxSendAuto{true};
    ALeffectslot *Target{nullptr};
    ALbuffer *Buffer{nullptr};

    struct {
        EffectSlotType Type{EffectSlotType::None};
        EffectProps Props{};

        al::intrusive_ptr<EffectState> State;
    } Effect;

    bool mPropsDirty{true};

    SlotState mState{SlotState::Initial};

    RefCount ref{0u};

    EffectSlot mSlot;

    /* Self ID */
    ALuint id{};

    ALeffectslot();
    ALeffectslot(const ALeffectslot&) = delete;
    ALeffectslot& operator=(const ALeffectslot&) = delete;
    ~ALeffectslot();

    ALenum initEffect(ALenum effectType, const EffectProps &effectProps, ALCcontext *context);
    void updateProps(ALCcontext *context);

    /* This can be new'd for the context's default effect slot. */
    DEF_NEWDEL(ALeffectslot)


#ifdef ALSOFT_EAX
public:
    void eax_initialize(
        ALCcontext& al_context,
        EaxFxSlotIndexValue index);

    const EAX50FXSLOTPROPERTIES& eax_get_eax_fx_slot() const noexcept;


    // [[nodiscard]]
    bool eax_dispatch(const EaxEaxCall& eax_call)
    { return eax_call.is_get() ? eax_get(eax_call) : eax_set(eax_call); }


    void eax_unlock_legacy() noexcept;

    void eax_commit() { eax_apply_deferred(); }

private:
    ALCcontext* eax_al_context_{};

    EaxFxSlotIndexValue eax_fx_slot_index_{};

    EAX50FXSLOTPROPERTIES eax_eax_fx_slot_{};

    EaxEffectUPtr eax_effect_{};
    bool eax_is_locked_{};


    [[noreturn]]
    static void eax_fail(
        const char* message);


    GUID eax_get_eax_default_effect_guid() const noexcept;
    long eax_get_eax_default_lock() const noexcept;

    void eax_set_eax_fx_slot_defaults();

    void eax_initialize_eax();

    void eax_initialize_lock();


    void eax_initialize_effects();


    void eax_get_fx_slot_all(
        const EaxEaxCall& eax_call) const;

    void eax_get_fx_slot(
        const EaxEaxCall& eax_call) const;

    // [[nodiscard]]
    bool eax_get(
        const EaxEaxCall& eax_call);


    void eax_set_fx_slot_effect(
        ALenum effect_type);

    void eax_set_fx_slot_effect();


    void eax_set_efx_effect_slot_gain();

    void eax_set_fx_slot_volume();


    void eax_set_effect_slot_send_auto();

    void eax_set_fx_slot_flags();


    void eax_ensure_is_unlocked() const;


    void eax_validate_fx_slot_effect(
        const GUID& eax_effect_id);

    void eax_validate_fx_slot_volume(
        long eax_volume);

    void eax_validate_fx_slot_lock(
        long eax_lock);

    void eax_validate_fx_slot_flags(
        unsigned long eax_flags,
        int eax_version);

    void eax_validate_fx_slot_occlusion(
        long eax_occlusion);

    void eax_validate_fx_slot_occlusion_lf_ratio(
        float eax_occlusion_lf_ratio);

    void eax_validate_fx_slot_all(
        const EAX40FXSLOTPROPERTIES& fx_slot,
        int eax_version);

    void eax_validate_fx_slot_all(
        const EAX50FXSLOTPROPERTIES& fx_slot,
        int eax_version);


    void eax_set_fx_slot_effect(
        const GUID& eax_effect_id);

    void eax_set_fx_slot_volume(
        long eax_volume);

    void eax_set_fx_slot_lock(
        long eax_lock);

    void eax_set_fx_slot_flags(
        unsigned long eax_flags);

    // [[nodiscard]]
    bool eax_set_fx_slot_occlusion(
        long eax_occlusion);

    // [[nodiscard]]
    bool eax_set_fx_slot_occlusion_lf_ratio(
        float eax_occlusion_lf_ratio);

    void eax_set_fx_slot_all(
        const EAX40FXSLOTPROPERTIES& eax_fx_slot);

    // [[nodiscard]]
    bool eax_set_fx_slot_all(
        const EAX50FXSLOTPROPERTIES& eax_fx_slot);


    void eax_set_fx_slot_effect(
        const EaxEaxCall& eax_call);

    void eax_set_fx_slot_volume(
        const EaxEaxCall& eax_call);

    void eax_set_fx_slot_lock(
        const EaxEaxCall& eax_call);

    void eax_set_fx_slot_flags(
        const EaxEaxCall& eax_call);

    // [[nodiscard]]
    bool eax_set_fx_slot_occlusion(
        const EaxEaxCall& eax_call);

    // [[nodiscard]]
    bool eax_set_fx_slot_occlusion_lf_ratio(
        const EaxEaxCall& eax_call);

    // [[nodiscard]]
    bool eax_set_fx_slot_all(
        const EaxEaxCall& eax_call);

    bool eax_set_fx_slot(
        const EaxEaxCall& eax_call);

    void eax_apply_deferred();

    // [[nodiscard]]
    bool eax_set(
        const EaxEaxCall& eax_call);


    void eax_dispatch_effect(
        const EaxEaxCall& eax_call);


    // `alAuxiliaryEffectSloti(effect_slot, AL_EFFECTSLOT_EFFECT, effect)`
    void eax_set_effect_slot_effect(EaxEffect &effect);

    // `alAuxiliaryEffectSloti(effect_slot, AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, value)`
    void eax_set_effect_slot_send_auto(bool is_send_auto);

    // `alAuxiliaryEffectSlotf(effect_slot, AL_EFFECTSLOT_GAIN, gain)`
    void eax_set_effect_slot_gain(ALfloat gain);

public:
    class EaxDeleter {
    public:
        void operator()(ALeffectslot *effect_slot);
    }; // EaxAlEffectSlotDeleter
#endif // ALSOFT_EAX
};

void UpdateAllEffectSlotProps(ALCcontext *context);

#ifdef ALSOFT_EAX

using EaxAlEffectSlotUPtr = std::unique_ptr<ALeffectslot, ALeffectslot::EaxDeleter>;


EaxAlEffectSlotUPtr eax_create_al_effect_slot(
    ALCcontext& context);

void eax_delete_al_effect_slot(
    ALCcontext& context,
    ALeffectslot& effect_slot);
#endif // ALSOFT_EAX

#endif
