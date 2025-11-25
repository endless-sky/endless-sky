#ifndef AL_EFFECTS_EFFECTS_H
#define AL_EFFECTS_EFFECTS_H

#include "AL/al.h"

#include "core/except.h"

#ifdef ALSOFT_EAX
#include "al/eax_effect.h"
#endif // ALSOFT_EAX

union EffectProps;


class effect_exception final : public al::base_exception {
    ALenum mErrorCode;

public:
#ifdef __USE_MINGW_ANSI_STDIO
    [[gnu::format(gnu_printf, 3, 4)]]
#else
    [[gnu::format(printf, 3, 4)]]
#endif
    effect_exception(ALenum code, const char *msg, ...);

    ALenum errorCode() const noexcept { return mErrorCode; }
};


struct EffectVtable {
    void (*const setParami)(EffectProps *props, ALenum param, int val);
    void (*const setParamiv)(EffectProps *props, ALenum param, const int *vals);
    void (*const setParamf)(EffectProps *props, ALenum param, float val);
    void (*const setParamfv)(EffectProps *props, ALenum param, const float *vals);

    void (*const getParami)(const EffectProps *props, ALenum param, int *val);
    void (*const getParamiv)(const EffectProps *props, ALenum param, int *vals);
    void (*const getParamf)(const EffectProps *props, ALenum param, float *val);
    void (*const getParamfv)(const EffectProps *props, ALenum param, float *vals);
};

#define DEFINE_ALEFFECT_VTABLE(T)           \
const EffectVtable T##EffectVtable = {      \
    T##_setParami, T##_setParamiv,          \
    T##_setParamf, T##_setParamfv,          \
    T##_getParami, T##_getParamiv,          \
    T##_getParamf, T##_getParamfv,          \
}


/* Default properties for the given effect types. */
extern const EffectProps NullEffectProps;
extern const EffectProps ReverbEffectProps;
extern const EffectProps StdReverbEffectProps;
extern const EffectProps AutowahEffectProps;
extern const EffectProps ChorusEffectProps;
extern const EffectProps CompressorEffectProps;
extern const EffectProps DistortionEffectProps;
extern const EffectProps EchoEffectProps;
extern const EffectProps EqualizerEffectProps;
extern const EffectProps FlangerEffectProps;
extern const EffectProps FshifterEffectProps;
extern const EffectProps ModulatorEffectProps;
extern const EffectProps PshifterEffectProps;
extern const EffectProps VmorpherEffectProps;
extern const EffectProps DedicatedEffectProps;
extern const EffectProps ConvolutionEffectProps;

/* Vtables to get/set properties for the given effect types. */
extern const EffectVtable NullEffectVtable;
extern const EffectVtable ReverbEffectVtable;
extern const EffectVtable StdReverbEffectVtable;
extern const EffectVtable AutowahEffectVtable;
extern const EffectVtable ChorusEffectVtable;
extern const EffectVtable CompressorEffectVtable;
extern const EffectVtable DistortionEffectVtable;
extern const EffectVtable EchoEffectVtable;
extern const EffectVtable EqualizerEffectVtable;
extern const EffectVtable FlangerEffectVtable;
extern const EffectVtable FshifterEffectVtable;
extern const EffectVtable ModulatorEffectVtable;
extern const EffectVtable PshifterEffectVtable;
extern const EffectVtable VmorpherEffectVtable;
extern const EffectVtable DedicatedEffectVtable;
extern const EffectVtable ConvolutionEffectVtable;


#ifdef ALSOFT_EAX
EaxEffectUPtr eax_create_eax_effect(ALenum al_effect_type);
#endif // ALSOFT_EAX

#endif /* AL_EFFECTS_EFFECTS_H */
