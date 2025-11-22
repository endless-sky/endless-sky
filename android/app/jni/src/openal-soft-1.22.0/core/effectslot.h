#ifndef CORE_EFFECTSLOT_H
#define CORE_EFFECTSLOT_H

#include <atomic>

#include "almalloc.h"
#include "device.h"
#include "effects/base.h"
#include "intrusive_ptr.h"

struct EffectSlot;
struct WetBuffer;

using EffectSlotArray = al::FlexArray<EffectSlot*>;


enum class EffectSlotType : unsigned char {
    None,
    Reverb,
    Chorus,
    Distortion,
    Echo,
    Flanger,
    FrequencyShifter,
    VocalMorpher,
    PitchShifter,
    RingModulator,
    Autowah,
    Compressor,
    Equalizer,
    EAXReverb,
    DedicatedLFE,
    DedicatedDialog,
    Convolution
};

struct EffectSlotProps {
    float Gain;
    bool  AuxSendAuto;
    EffectSlot *Target;

    EffectSlotType Type;
    EffectProps Props;

    al::intrusive_ptr<EffectState> State;

    std::atomic<EffectSlotProps*> next;

    DEF_NEWDEL(EffectSlotProps)
};


struct EffectSlot {
    std::atomic<EffectSlotProps*> Update{nullptr};

    /* Wet buffer configuration is ACN channel order with N3D scaling.
     * Consequently, effects that only want to work with mono input can use
     * channel 0 by itself. Effects that want multichannel can process the
     * ambisonics signal and make a B-Format source pan.
     */
    MixParams Wet;

    float Gain{1.0f};
    bool  AuxSendAuto{true};
    EffectSlot *Target{nullptr};

    EffectSlotType EffectType{EffectSlotType::None};
    EffectProps mEffectProps{};
    EffectState *mEffectState{nullptr};

    float RoomRolloff{0.0f}; /* Added to the source's room rolloff, not multiplied. */
    float DecayTime{0.0f};
    float DecayLFRatio{0.0f};
    float DecayHFRatio{0.0f};
    bool DecayHFLimit{false};
    float AirAbsorptionGainHF{1.0f};

    /* Mixing buffer used by the Wet mix. */
    WetBuffer *mWetBuffer{nullptr};

    ~EffectSlot();

    static EffectSlotArray *CreatePtrArray(size_t count) noexcept;

    DISABLE_ALLOC()
};

#endif /* CORE_EFFECTSLOT_H */
