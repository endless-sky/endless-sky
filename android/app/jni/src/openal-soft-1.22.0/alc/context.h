#ifndef ALC_CONTEXT_H
#define ALC_CONTEXT_H

#include <atomic>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <utility>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "al/listener.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "atomic.h"
#include "core/context.h"
#include "intrusive_ptr.h"
#include "vector.h"

#ifdef ALSOFT_EAX
#include "al/eax_eax_call.h"
#include "al/eax_fx_slot_index.h"
#include "al/eax_fx_slots.h"
#include "al/eax_utils.h"


using EaxContextSharedDirtyFlagsValue = std::uint_least8_t;

struct EaxContextSharedDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    EaxContextSharedDirtyFlagsValue primary_fx_slot_id : 1;
}; // EaxContextSharedDirtyFlags


using ContextDirtyFlagsValue = std::uint_least8_t;

struct ContextDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

    ContextDirtyFlagsValue guidPrimaryFXSlotID : 1;
    ContextDirtyFlagsValue flDistanceFactor : 1;
    ContextDirtyFlagsValue flAirAbsorptionHF : 1;
    ContextDirtyFlagsValue flHFReference : 1;
    ContextDirtyFlagsValue flMacroFXFactor : 1;
}; // ContextDirtyFlags


struct EaxAlIsExtensionPresentResult
{
    ALboolean is_present;
    bool is_return;
}; // EaxAlIsExtensionPresentResult
#endif // ALSOFT_EAX

struct ALeffect;
struct ALeffectslot;
struct ALsource;

using uint = unsigned int;


struct SourceSubList {
    uint64_t FreeMask{~0_u64};
    ALsource *Sources{nullptr}; /* 64 */

    SourceSubList() noexcept = default;
    SourceSubList(const SourceSubList&) = delete;
    SourceSubList(SourceSubList&& rhs) noexcept : FreeMask{rhs.FreeMask}, Sources{rhs.Sources}
    { rhs.FreeMask = ~0_u64; rhs.Sources = nullptr; }
    ~SourceSubList();

    SourceSubList& operator=(const SourceSubList&) = delete;
    SourceSubList& operator=(SourceSubList&& rhs) noexcept
    { std::swap(FreeMask, rhs.FreeMask); std::swap(Sources, rhs.Sources); return *this; }
};

struct EffectSlotSubList {
    uint64_t FreeMask{~0_u64};
    ALeffectslot *EffectSlots{nullptr}; /* 64 */

    EffectSlotSubList() noexcept = default;
    EffectSlotSubList(const EffectSlotSubList&) = delete;
    EffectSlotSubList(EffectSlotSubList&& rhs) noexcept
      : FreeMask{rhs.FreeMask}, EffectSlots{rhs.EffectSlots}
    { rhs.FreeMask = ~0_u64; rhs.EffectSlots = nullptr; }
    ~EffectSlotSubList();

    EffectSlotSubList& operator=(const EffectSlotSubList&) = delete;
    EffectSlotSubList& operator=(EffectSlotSubList&& rhs) noexcept
    { std::swap(FreeMask, rhs.FreeMask); std::swap(EffectSlots, rhs.EffectSlots); return *this; }
};

struct ALCcontext : public al::intrusive_ref<ALCcontext>, ContextBase {
    const al::intrusive_ptr<ALCdevice> mALDevice;

    /* Wet buffers used by effect slots. */
    al::vector<WetBufferPtr> mWetBuffers;


    bool mPropsDirty{true};
    bool mDeferUpdates{false};

    std::mutex mPropLock;

    std::atomic<ALenum> mLastError{AL_NO_ERROR};

    DistanceModel mDistanceModel{DistanceModel::Default};
    bool mSourceDistanceModel{false};

    float mDopplerFactor{1.0f};
    float mDopplerVelocity{1.0f};
    float mSpeedOfSound{SpeedOfSoundMetersPerSec};
    float mAirAbsorptionGainHF{AirAbsorbGainHF};

    std::mutex mEventCbLock;
    ALEVENTPROCSOFT mEventCb{};
    void *mEventParam{nullptr};

    ALlistener mListener{};

    al::vector<SourceSubList> mSourceList;
    ALuint mNumSources{0};
    std::mutex mSourceLock;

    al::vector<EffectSlotSubList> mEffectSlotList;
    ALuint mNumEffectSlots{0u};
    std::mutex mEffectSlotLock;

    /* Default effect slot */
    std::unique_ptr<ALeffectslot> mDefaultSlot;

    const char *mExtensionList{nullptr};


    ALCcontext(al::intrusive_ptr<ALCdevice> device);
    ALCcontext(const ALCcontext&) = delete;
    ALCcontext& operator=(const ALCcontext&) = delete;
    ~ALCcontext();

    void init();
    /**
     * Removes the context from its device and removes it from being current on
     * the running thread or globally. Returns true if other contexts still
     * exist on the device.
     */
    bool deinit();

    /**
     * Defers/suspends updates for the given context's listener and sources.
     * This does *NOT* stop mixing, but rather prevents certain property
     * changes from taking effect. mPropLock must be held when called.
     */
    void deferUpdates() noexcept { mDeferUpdates = true; }

    /**
     * Resumes update processing after being deferred. mPropLock must be held
     * when called.
     */
    void processUpdates()
    {
        if(std::exchange(mDeferUpdates, false))
            applyAllUpdates();
    }

    /**
     * Applies all pending updates for the context, listener, effect slots, and
     * sources.
     */
    void applyAllUpdates();

#ifdef __USE_MINGW_ANSI_STDIO
    [[gnu::format(gnu_printf, 3, 4)]]
#else
    [[gnu::format(printf, 3, 4)]]
#endif
    void setError(ALenum errorCode, const char *msg, ...);

    /* Process-wide current context */
    static std::atomic<ALCcontext*> sGlobalContext;

private:
    /* Thread-local current context. */
    static thread_local ALCcontext *sLocalContext;

    /* Thread-local context handling. This handles attempting to release the
     * context which may have been left current when the thread is destroyed.
     */
    class ThreadCtx {
    public:
        ~ThreadCtx();
        void set(ALCcontext *ctx) const noexcept { sLocalContext = ctx; }
    };
    static thread_local ThreadCtx sThreadContext;

public:
    /* HACK: MinGW generates bad code when accessing an extern thread_local
     * object. Add a wrapper function for it that only accesses it where it's
     * defined.
     */
#ifdef __MINGW32__
    static ALCcontext *getThreadContext() noexcept;
    static void setThreadContext(ALCcontext *context) noexcept;
#else
    static ALCcontext *getThreadContext() noexcept { return sLocalContext; }
    static void setThreadContext(ALCcontext *context) noexcept { sThreadContext.set(context); }
#endif

    /* Default effect that applies to sources that don't have an effect on send 0. */
    static ALeffect sDefaultEffect;

    DEF_NEWDEL(ALCcontext)

#ifdef ALSOFT_EAX
public:
    bool has_eax() const noexcept { return eax_is_initialized_; }

    bool eax_is_capable() const noexcept;


    void eax_uninitialize() noexcept;


    ALenum eax_eax_set(
        const GUID* property_set_id,
        ALuint property_id,
        ALuint property_source_id,
        ALvoid* property_value,
        ALuint property_value_size);

    ALenum eax_eax_get(
        const GUID* property_set_id,
        ALuint property_id,
        ALuint property_source_id,
        ALvoid* property_value,
        ALuint property_value_size);


    void eax_update_filters();

    void eax_commit_and_update_sources();


    void eax_set_last_error() noexcept;


    EaxFxSlotIndex eax_get_previous_primary_fx_slot_index() const noexcept
    { return eax_previous_primary_fx_slot_index_; }
    EaxFxSlotIndex eax_get_primary_fx_slot_index() const noexcept
    { return eax_primary_fx_slot_index_; }

    const ALeffectslot& eax_get_fx_slot(EaxFxSlotIndexValue fx_slot_index) const
    { return eax_fx_slots_.get(fx_slot_index); }
    ALeffectslot& eax_get_fx_slot(EaxFxSlotIndexValue fx_slot_index)
    { return eax_fx_slots_.get(fx_slot_index); }

    void eax_commit_fx_slots()
    { eax_fx_slots_.commit(); }

private:
    struct Eax
    {
        EAX50CONTEXTPROPERTIES context{};
    }; // Eax


    bool eax_is_initialized_{};
    bool eax_is_tried_{};
    bool eax_are_legacy_fx_slots_unlocked_{};

    long eax_last_error_{};
    unsigned long eax_speaker_config_{};

    EaxFxSlotIndex eax_previous_primary_fx_slot_index_{};
    EaxFxSlotIndex eax_primary_fx_slot_index_{};
    EaxFxSlots eax_fx_slots_{};

    EaxContextSharedDirtyFlags eax_context_shared_dirty_flags_{};

    Eax eax_{};
    Eax eax_d_{};
    EAXSESSIONPROPERTIES eax_session_{};

    ContextDirtyFlags eax_context_dirty_flags_{};

    std::string eax_extension_list_{};


    [[noreturn]]
    static void eax_fail(
        const char* message);


    void eax_initialize_extensions();

    void eax_initialize();


    bool eax_has_no_default_effect_slot() const noexcept;

    void eax_ensure_no_default_effect_slot() const;

    bool eax_has_enough_aux_sends() const noexcept;

    void eax_ensure_enough_aux_sends() const;

    void eax_ensure_compatibility();


    unsigned long eax_detect_speaker_configuration() const;
    void eax_update_speaker_configuration();


    void eax_set_last_error_defaults() noexcept;

    void eax_set_session_defaults() noexcept;

    void eax_set_context_defaults() noexcept;

    void eax_set_defaults() noexcept;

    void eax_initialize_sources();


    void eax_unlock_legacy_fx_slots(const EaxEaxCall& eax_call) noexcept;


    void eax_dispatch_fx_slot(
        const EaxEaxCall& eax_call);

    void eax_dispatch_source(
        const EaxEaxCall& eax_call);


    void eax_get_primary_fx_slot_id(
        const EaxEaxCall& eax_call);

    void eax_get_distance_factor(
        const EaxEaxCall& eax_call);

    void eax_get_air_absorption_hf(
        const EaxEaxCall& eax_call);

    void eax_get_hf_reference(
        const EaxEaxCall& eax_call);

    void eax_get_last_error(
        const EaxEaxCall& eax_call);

    void eax_get_speaker_config(
        const EaxEaxCall& eax_call);

    void eax_get_session(
        const EaxEaxCall& eax_call);

    void eax_get_macro_fx_factor(
        const EaxEaxCall& eax_call);

    void eax_get_context_all(
        const EaxEaxCall& eax_call);

    void eax_get(
        const EaxEaxCall& eax_call);


    void eax_set_primary_fx_slot_id();

    void eax_set_distance_factor();

    void eax_set_air_absorbtion_hf();

    void eax_set_hf_reference();

    void eax_set_macro_fx_factor();

    void eax_set_context();

    void eax_initialize_fx_slots();


    void eax_update_sources();


    void eax_validate_primary_fx_slot_id(
        const GUID& primary_fx_slot_id);

    void eax_validate_distance_factor(
        float distance_factor);

    void eax_validate_air_absorption_hf(
        float air_absorption_hf);

    void eax_validate_hf_reference(
        float hf_reference);

    void eax_validate_speaker_config(
        unsigned long speaker_config);

    void eax_validate_session_eax_version(
        unsigned long eax_version);

    void eax_validate_session_max_active_sends(
        unsigned long max_active_sends);

    void eax_validate_session(
        const EAXSESSIONPROPERTIES& eax_session);

    void eax_validate_macro_fx_factor(
        float macro_fx_factor);

    void eax_validate_context_all(
        const EAX40CONTEXTPROPERTIES& context_all);

    void eax_validate_context_all(
        const EAX50CONTEXTPROPERTIES& context_all);


    void eax_defer_primary_fx_slot_id(
        const GUID& primary_fx_slot_id);

    void eax_defer_distance_factor(
        float distance_factor);

    void eax_defer_air_absorption_hf(
        float air_absorption_hf);

    void eax_defer_hf_reference(
        float hf_reference);

    void eax_defer_macro_fx_factor(
        float macro_fx_factor);

    void eax_defer_context_all(
        const EAX40CONTEXTPROPERTIES& context_all);

    void eax_defer_context_all(
        const EAX50CONTEXTPROPERTIES& context_all);


    void eax_defer_context_all(
        const EaxEaxCall& eax_call);

    void eax_defer_primary_fx_slot_id(
        const EaxEaxCall& eax_call);

    void eax_defer_distance_factor(
        const EaxEaxCall& eax_call);

    void eax_defer_air_absorption_hf(
        const EaxEaxCall& eax_call);

    void eax_defer_hf_reference(
        const EaxEaxCall& eax_call);

    void eax_set_session(
        const EaxEaxCall& eax_call);

    void eax_defer_macro_fx_factor(
        const EaxEaxCall& eax_call);

    void eax_set(
        const EaxEaxCall& eax_call);

    void eax_apply_deferred();
#endif // ALSOFT_EAX
};

#define SETERR_RETURN(ctx, err, retval, ...) do {                             \
    (ctx)->setError((err), __VA_ARGS__);                                      \
    return retval;                                                            \
} while(0)


using ContextRef = al::intrusive_ptr<ALCcontext>;

ContextRef GetContextRef(void);

void UpdateContextProps(ALCcontext *context);


extern bool TrapALError;


#ifdef ALSOFT_EAX
ALenum AL_APIENTRY EAXSet(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_value,
    ALuint property_value_size) noexcept;

ALenum AL_APIENTRY EAXGet(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_value,
    ALuint property_value_size) noexcept;
#endif // ALSOFT_EAX

#endif /* ALC_CONTEXT_H */
