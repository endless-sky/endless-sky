#ifndef CORE_EFFECTS_BASE_H
#define CORE_EFFECTS_BASE_H

#include <stddef.h>

#include "albyte.h"
#include "almalloc.h"
#include "alspan.h"
#include "atomic.h"
#include "core/bufferline.h"
#include "intrusive_ptr.h"

struct BufferStorage;
struct ContextBase;
struct DeviceBase;
struct EffectSlot;
struct MixParams;
struct RealMixParams;


/** Target gain for the reverb decay feedback reaching the decay time. */
constexpr float ReverbDecayGain{0.001f}; /* -60 dB */

constexpr float ReverbMaxReflectionsDelay{0.3f};
constexpr float ReverbMaxLateReverbDelay{0.1f};

enum class ChorusWaveform {
    Sinusoid,
    Triangle
};

constexpr float ChorusMaxDelay{0.016f};
constexpr float FlangerMaxDelay{0.004f};

constexpr float EchoMaxDelay{0.207f};
constexpr float EchoMaxLRDelay{0.404f};

enum class FShifterDirection {
    Down,
    Up,
    Off
};

enum class ModulatorWaveform {
    Sinusoid,
    Sawtooth,
    Square
};

enum class VMorpherPhenome {
    A, E, I, O, U,
    AA, AE, AH, AO, EH, ER, IH, IY, UH, UW,
    B, D, F, G, J, K, L, M, N, P, R, S, T, V, Z
};

enum class VMorpherWaveform {
    Sinusoid,
    Triangle,
    Sawtooth
};

union EffectProps {
    struct {
        // Shared Reverb Properties
        float Density;
        float Diffusion;
        float Gain;
        float GainHF;
        float DecayTime;
        float DecayHFRatio;
        float ReflectionsGain;
        float ReflectionsDelay;
        float LateReverbGain;
        float LateReverbDelay;
        float AirAbsorptionGainHF;
        float RoomRolloffFactor;
        bool DecayHFLimit;

        // Additional EAX Reverb Properties
        float GainLF;
        float DecayLFRatio;
        float ReflectionsPan[3];
        float LateReverbPan[3];
        float EchoTime;
        float EchoDepth;
        float ModulationTime;
        float ModulationDepth;
        float HFReference;
        float LFReference;
    } Reverb;

    struct {
        float AttackTime;
        float ReleaseTime;
        float Resonance;
        float PeakGain;
    } Autowah;

    struct {
        ChorusWaveform Waveform;
        int Phase;
        float Rate;
        float Depth;
        float Feedback;
        float Delay;
    } Chorus; /* Also Flanger */

    struct {
        bool OnOff;
    } Compressor;

    struct {
        float Edge;
        float Gain;
        float LowpassCutoff;
        float EQCenter;
        float EQBandwidth;
    } Distortion;

    struct {
        float Delay;
        float LRDelay;

        float Damping;
        float Feedback;

        float Spread;
    } Echo;

    struct {
        float LowCutoff;
        float LowGain;
        float Mid1Center;
        float Mid1Gain;
        float Mid1Width;
        float Mid2Center;
        float Mid2Gain;
        float Mid2Width;
        float HighCutoff;
        float HighGain;
    } Equalizer;

    struct {
        float Frequency;
        FShifterDirection LeftDirection;
        FShifterDirection RightDirection;
    } Fshifter;

    struct {
        float Frequency;
        float HighPassCutoff;
        ModulatorWaveform Waveform;
    } Modulator;

    struct {
        int CoarseTune;
        int FineTune;
    } Pshifter;

    struct {
        float Rate;
        VMorpherPhenome PhonemeA;
        VMorpherPhenome PhonemeB;
        int PhonemeACoarseTuning;
        int PhonemeBCoarseTuning;
        VMorpherWaveform Waveform;
    } Vmorpher;

    struct {
        float Gain;
    } Dedicated;
};


struct EffectTarget {
    MixParams *Main;
    RealMixParams *RealOut;
};

struct EffectState : public al::intrusive_ref<EffectState> {
    struct Buffer {
        const BufferStorage *storage;
        al::span<const al::byte> samples;
    };

    al::span<FloatBufferLine> mOutTarget;


    virtual ~EffectState() = default;

    virtual void deviceUpdate(const DeviceBase *device, const Buffer &buffer) = 0;
    virtual void update(const ContextBase *context, const EffectSlot *slot,
        const EffectProps *props, const EffectTarget target) = 0;
    virtual void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) = 0;
};


struct EffectStateFactory {
    virtual ~EffectStateFactory() = default;

    virtual al::intrusive_ptr<EffectState> create() = 0;
};

#endif /* CORE_EFFECTS_BASE_H */
