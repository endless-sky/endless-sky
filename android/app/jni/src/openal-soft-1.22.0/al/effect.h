#ifndef AL_EFFECT_H
#define AL_EFFECT_H

#include "AL/al.h"
#include "AL/efx.h"

#include "al/effects/effects.h"
#include "alc/effects/base.h"


enum {
    EAXREVERB_EFFECT = 0,
    REVERB_EFFECT,
    AUTOWAH_EFFECT,
    CHORUS_EFFECT,
    COMPRESSOR_EFFECT,
    DISTORTION_EFFECT,
    ECHO_EFFECT,
    EQUALIZER_EFFECT,
    FLANGER_EFFECT,
    FSHIFTER_EFFECT,
    MODULATOR_EFFECT,
    PSHIFTER_EFFECT,
    VMORPHER_EFFECT,
    DEDICATED_EFFECT,
    CONVOLUTION_EFFECT,

    MAX_EFFECTS
};
extern bool DisabledEffects[MAX_EFFECTS];

extern float ReverbBoost;

struct EffectList {
    const char name[16];
    int type;
    ALenum val;
};
extern const EffectList gEffectList[16];


struct ALeffect {
    // Effect type (AL_EFFECT_NULL, ...)
    ALenum type{AL_EFFECT_NULL};

    EffectProps Props{};

    const EffectVtable *vtab{nullptr};

    /* Self ID */
    ALuint id{0u};

    DISABLE_ALLOC()
};

void InitEffect(ALeffect *effect);

void LoadReverbPreset(const char *name, ALeffect *effect);

bool IsValidEffectType(ALenum type) noexcept;

#endif
