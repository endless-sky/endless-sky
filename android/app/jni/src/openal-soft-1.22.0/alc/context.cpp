
#include "config.h"

#include "context.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <stddef.h>
#include <stdexcept>

#include "AL/efx.h"

#include "al/auxeffectslot.h"
#include "al/source.h"
#include "al/effect.h"
#include "al/event.h"
#include "al/listener.h"
#include "albit.h"
#include "alc/alu.h"
#include "core/async_event.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/logging.h"
#include "core/voice.h"
#include "core/voice_change.h"
#include "device.h"
#include "ringbuffer.h"
#include "vecmat.h"

#ifdef ALSOFT_EAX
#include <cassert>
#include <cstring>

#include "alstring.h"
#include "al/eax_exception.h"
#include "al/eax_globals.h"
#endif // ALSOFT_EAX

namespace {

using namespace std::placeholders;

using voidp = void*;

/* Default context extensions */
constexpr ALchar alExtList[] =
    "AL_EXT_ALAW "
    "AL_EXT_BFORMAT "
    "AL_EXT_DOUBLE "
    "AL_EXT_EXPONENT_DISTANCE "
    "AL_EXT_FLOAT32 "
    "AL_EXT_IMA4 "
    "AL_EXT_LINEAR_DISTANCE "
    "AL_EXT_MCFORMATS "
    "AL_EXT_MULAW "
    "AL_EXT_MULAW_BFORMAT "
    "AL_EXT_MULAW_MCFORMATS "
    "AL_EXT_OFFSET "
    "AL_EXT_source_distance_model "
    "AL_EXT_SOURCE_RADIUS "
    "AL_EXT_STEREO_ANGLES "
    "AL_LOKI_quadriphonic "
    "AL_SOFT_bformat_ex "
    "AL_SOFTX_bformat_hoa "
    "AL_SOFT_block_alignment "
    "AL_SOFT_callback_buffer "
    "AL_SOFTX_convolution_reverb "
    "AL_SOFT_deferred_updates "
    "AL_SOFT_direct_channels "
    "AL_SOFT_direct_channels_remix "
    "AL_SOFT_effect_target "
    "AL_SOFT_events "
    "AL_SOFT_gain_clamp_ex "
    "AL_SOFTX_hold_on_disconnect "
    "AL_SOFT_loop_points "
    "AL_SOFTX_map_buffer "
    "AL_SOFT_MSADPCM "
    "AL_SOFT_source_latency "
    "AL_SOFT_source_length "
    "AL_SOFT_source_resampler "
    "AL_SOFT_source_spatialize "
    "AL_SOFT_UHJ";

} // namespace


std::atomic<ALCcontext*> ALCcontext::sGlobalContext{nullptr};

thread_local ALCcontext *ALCcontext::sLocalContext{nullptr};
ALCcontext::ThreadCtx::~ThreadCtx()
{
    if(ALCcontext *ctx{ALCcontext::sLocalContext})
    {
        const bool result{ctx->releaseIfNoDelete()};
        ERR("Context %p current for thread being destroyed%s!\n", voidp{ctx},
            result ? "" : ", leak detected");
    }
}
thread_local ALCcontext::ThreadCtx ALCcontext::sThreadContext;

ALeffect ALCcontext::sDefaultEffect;


#ifdef __MINGW32__
ALCcontext *ALCcontext::getThreadContext() noexcept
{ return sLocalContext; }
void ALCcontext::setThreadContext(ALCcontext *context) noexcept
{ sThreadContext.set(context); }
#endif

ALCcontext::ALCcontext(al::intrusive_ptr<ALCdevice> device)
  : ContextBase{device.get()}, mALDevice{std::move(device)}
{
}

ALCcontext::~ALCcontext()
{
    TRACE("Freeing context %p\n", voidp{this});

    size_t count{std::accumulate(mSourceList.cbegin(), mSourceList.cend(), size_t{0u},
        [](size_t cur, const SourceSubList &sublist) noexcept -> size_t
        { return cur + static_cast<uint>(al::popcount(~sublist.FreeMask)); })};
    if(count > 0)
        WARN("%zu Source%s not deleted\n", count, (count==1)?"":"s");
    mSourceList.clear();
    mNumSources = 0;

#ifdef ALSOFT_EAX
    eax_uninitialize();
#endif // ALSOFT_EAX

    mDefaultSlot = nullptr;
    count = std::accumulate(mEffectSlotList.cbegin(), mEffectSlotList.cend(), size_t{0u},
        [](size_t cur, const EffectSlotSubList &sublist) noexcept -> size_t
        { return cur + static_cast<uint>(al::popcount(~sublist.FreeMask)); });
    if(count > 0)
        WARN("%zu AuxiliaryEffectSlot%s not deleted\n", count, (count==1)?"":"s");
    mEffectSlotList.clear();
    mNumEffectSlots = 0;
}

void ALCcontext::init()
{
    if(sDefaultEffect.type != AL_EFFECT_NULL && mDevice->Type == DeviceType::Playback)
    {
        mDefaultSlot = std::make_unique<ALeffectslot>();
        aluInitEffectPanning(&mDefaultSlot->mSlot, this);
    }

    EffectSlotArray *auxslots;
    if(!mDefaultSlot)
        auxslots = EffectSlot::CreatePtrArray(0);
    else
    {
        auxslots = EffectSlot::CreatePtrArray(1);
        (*auxslots)[0] = &mDefaultSlot->mSlot;
        mDefaultSlot->mState = SlotState::Playing;
    }
    mActiveAuxSlots.store(auxslots, std::memory_order_relaxed);

    allocVoiceChanges();
    {
        VoiceChange *cur{mVoiceChangeTail};
        while(VoiceChange *next{cur->mNext.load(std::memory_order_relaxed)})
            cur = next;
        mCurrentVoiceChange.store(cur, std::memory_order_relaxed);
    }

    mExtensionList = alExtList;

#ifdef ALSOFT_EAX
    eax_initialize_extensions();
#endif // ALSOFT_EAX

    mParams.Position = alu::Vector{0.0f, 0.0f, 0.0f, 1.0f};
    mParams.Matrix = alu::Matrix::Identity();
    mParams.Velocity = alu::Vector{};
    mParams.Gain = mListener.Gain;
    mParams.MetersPerUnit = mListener.mMetersPerUnit;
    mParams.AirAbsorptionGainHF = mAirAbsorptionGainHF;
    mParams.DopplerFactor = mDopplerFactor;
    mParams.SpeedOfSound = mSpeedOfSound * mDopplerVelocity;
    mParams.SourceDistanceModel = mSourceDistanceModel;
    mParams.mDistanceModel = mDistanceModel;


    mAsyncEvents = RingBuffer::Create(511, sizeof(AsyncEvent), false);
    StartEventThrd(this);


    allocVoices(256);
    mActiveVoiceCount.store(64, std::memory_order_relaxed);
}

bool ALCcontext::deinit()
{
    if(sLocalContext == this)
    {
        WARN("%p released while current on thread\n", voidp{this});
        sThreadContext.set(nullptr);
        release();
    }

    ALCcontext *origctx{this};
    if(sGlobalContext.compare_exchange_strong(origctx, nullptr))
        release();

    bool ret{};
    /* First make sure this context exists in the device's list. */
    auto *oldarray = mDevice->mContexts.load(std::memory_order_acquire);
    if(auto toremove = static_cast<size_t>(std::count(oldarray->begin(), oldarray->end(), this)))
    {
        using ContextArray = al::FlexArray<ContextBase*>;
        auto alloc_ctx_array = [](const size_t count) -> ContextArray*
        {
            if(count == 0) return &DeviceBase::sEmptyContextArray;
            return ContextArray::Create(count).release();
        };
        auto *newarray = alloc_ctx_array(oldarray->size() - toremove);

        /* Copy the current/old context handles to the new array, excluding the
         * given context.
         */
        std::copy_if(oldarray->begin(), oldarray->end(), newarray->begin(),
            std::bind(std::not_equal_to<>{}, _1, this));

        /* Store the new context array in the device. Wait for any current mix
         * to finish before deleting the old array.
         */
        mDevice->mContexts.store(newarray);
        if(oldarray != &DeviceBase::sEmptyContextArray)
        {
            mDevice->waitForMix();
            delete oldarray;
        }

        ret = !newarray->empty();
    }
    else
        ret = !oldarray->empty();

    StopEventThrd(this);

    return ret;
}

void ALCcontext::applyAllUpdates()
{
    /* Tell the mixer to stop applying updates, then wait for any active
     * updating to finish, before providing updates.
     */
    mHoldUpdates.store(true, std::memory_order_release);
    while((mUpdateCount.load(std::memory_order_acquire)&1) != 0) {
        /* busy-wait */
    }

#ifdef ALSOFT_EAX
    eax_apply_deferred();
#endif
    if(std::exchange(mPropsDirty, false))
        UpdateContextProps(this);
    UpdateAllEffectSlotProps(this);
    UpdateAllSourceProps(this);

    /* Now with all updates declared, let the mixer continue applying them so
     * they all happen at once.
     */
    mHoldUpdates.store(false, std::memory_order_release);
}

#ifdef ALSOFT_EAX
namespace {

class ContextException :
    public EaxException
{
public:
    explicit ContextException(
        const char* message)
        :
        EaxException{"EAX_CONTEXT", message}
    {
    }
}; // ContextException


template<typename F>
void ForEachSource(ALCcontext *context, F func)
{
    for(auto &sublist : context->mSourceList)
    {
        uint64_t usemask{~sublist.FreeMask};
        while(usemask)
        {
            const int idx{al::countr_zero(usemask)};
            usemask &= ~(1_u64 << idx);

            func(sublist.Sources[idx]);
        }
    }
}

} // namespace


bool ALCcontext::eax_is_capable() const noexcept
{
    return eax_has_enough_aux_sends();
}

void ALCcontext::eax_uninitialize() noexcept
{
    if (!eax_is_initialized_)
    {
        return;
    }

    eax_is_initialized_ = true;
    eax_is_tried_ = false;

    eax_fx_slots_.uninitialize();
}

ALenum ALCcontext::eax_eax_set(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_value,
    ALuint property_value_size)
{
    eax_initialize();

    const auto eax_call = create_eax_call(
        false,
        property_set_id,
        property_id,
        property_source_id,
        property_value,
        property_value_size
    );

    eax_unlock_legacy_fx_slots(eax_call);

    switch (eax_call.get_property_set_id())
    {
        case EaxEaxCallPropertySetId::context:
            eax_set(eax_call);
            break;

        case EaxEaxCallPropertySetId::fx_slot:
        case EaxEaxCallPropertySetId::fx_slot_effect:
            eax_dispatch_fx_slot(eax_call);
            break;

        case EaxEaxCallPropertySetId::source:
            eax_dispatch_source(eax_call);
            break;

        default:
            eax_fail("Unsupported property set id.");
    }

    static constexpr auto deferred_flag = 0x80000000u;
    if(!(property_id&deferred_flag) && !mDeferUpdates)
        applyAllUpdates();

    return AL_NO_ERROR;
}

ALenum ALCcontext::eax_eax_get(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_value,
    ALuint property_value_size)
{
    eax_initialize();

    const auto eax_call = create_eax_call(
        true,
        property_set_id,
        property_id,
        property_source_id,
        property_value,
        property_value_size
    );

    eax_unlock_legacy_fx_slots(eax_call);

    switch (eax_call.get_property_set_id())
    {
        case EaxEaxCallPropertySetId::context:
            eax_get(eax_call);
            break;

        case EaxEaxCallPropertySetId::fx_slot:
        case EaxEaxCallPropertySetId::fx_slot_effect:
            eax_dispatch_fx_slot(eax_call);
            break;

        case EaxEaxCallPropertySetId::source:
            eax_dispatch_source(eax_call);
            break;

        default:
            eax_fail("Unsupported property set id.");
    }

    return AL_NO_ERROR;
}

void ALCcontext::eax_update_filters()
{
    ForEachSource(this, std::mem_fn(&ALsource::eax_update_filters));
}

void ALCcontext::eax_commit_and_update_sources()
{
    std::unique_lock<std::mutex> source_lock{mSourceLock};
    ForEachSource(this, std::mem_fn(&ALsource::eax_commit_and_update));
}

void ALCcontext::eax_set_last_error() noexcept
{
    eax_last_error_ = EAXERR_INVALID_OPERATION;
}

[[noreturn]]
void ALCcontext::eax_fail(
    const char* message)
{
    throw ContextException{message};
}

void ALCcontext::eax_initialize_extensions()
{
    if (!eax_g_is_enabled)
    {
        return;
    }

    const auto string_max_capacity =
        std::strlen(mExtensionList) + 1 +
        std::strlen(eax1_ext_name) + 1 +
        std::strlen(eax2_ext_name) + 1 +
        std::strlen(eax3_ext_name) + 1 +
        std::strlen(eax4_ext_name) + 1 +
        std::strlen(eax5_ext_name) + 1 +
        std::strlen(eax_x_ram_ext_name) + 1 +
        0;

    eax_extension_list_.reserve(string_max_capacity);

    if (eax_is_capable())
    {
        eax_extension_list_ += eax1_ext_name;
        eax_extension_list_ += ' ';

        eax_extension_list_ += eax2_ext_name;
        eax_extension_list_ += ' ';

        eax_extension_list_ += eax3_ext_name;
        eax_extension_list_ += ' ';

        eax_extension_list_ += eax4_ext_name;
        eax_extension_list_ += ' ';

        eax_extension_list_ += eax5_ext_name;
        eax_extension_list_ += ' ';
    }

    eax_extension_list_ += eax_x_ram_ext_name;
    eax_extension_list_ += ' ';

    eax_extension_list_ += mExtensionList;
    mExtensionList = eax_extension_list_.c_str();
}

void ALCcontext::eax_initialize()
{
    if (eax_is_initialized_)
    {
        return;
    }

    if (eax_is_tried_)
    {
        eax_fail("No EAX.");
    }

    eax_is_tried_ = true;

    if (!eax_g_is_enabled)
    {
        eax_fail("EAX disabled by a configuration.");
    }

    eax_ensure_compatibility();
    eax_set_defaults();
    eax_set_air_absorbtion_hf();
    eax_update_speaker_configuration();
    eax_initialize_fx_slots();
    eax_initialize_sources();

    eax_is_initialized_ = true;
}

bool ALCcontext::eax_has_no_default_effect_slot() const noexcept
{
    return mDefaultSlot == nullptr;
}

void ALCcontext::eax_ensure_no_default_effect_slot() const
{
    if (!eax_has_no_default_effect_slot())
    {
        eax_fail("There is a default effect slot in the context.");
    }
}

bool ALCcontext::eax_has_enough_aux_sends() const noexcept
{
    return mALDevice->NumAuxSends >= EAX_MAX_FXSLOTS;
}

void ALCcontext::eax_ensure_enough_aux_sends() const
{
    if (!eax_has_enough_aux_sends())
    {
        eax_fail("Not enough aux sends.");
    }
}

void ALCcontext::eax_ensure_compatibility()
{
    eax_ensure_enough_aux_sends();
}

unsigned long ALCcontext::eax_detect_speaker_configuration() const
{
#define EAX_PREFIX "[EAX_DETECT_SPEAKER_CONFIG]"

    switch(mDevice->FmtChans)
    {
    case DevFmtMono: return SPEAKERS_2;
    case DevFmtStereo:
        /* Pretend 7.1 if using UHJ output, since they both provide full
         * horizontal surround.
         */
        if(mDevice->mUhjEncoder)
            return SPEAKERS_7;
        if(mDevice->Flags.test(DirectEar))
            return HEADPHONES;
        return SPEAKERS_2;
    case DevFmtQuad: return SPEAKERS_4;
    case DevFmtX51: return SPEAKERS_5;
    case DevFmtX61: return SPEAKERS_6;
    case DevFmtX71: return SPEAKERS_7;
    /* This could also be HEADPHONES, since headphones-based HRTF and Ambi3D
     * provide full-sphere surround sound. Depends if apps are more likely to
     * consider headphones or 7.1 for surround sound support.
     */
    case DevFmtAmbi3D: return SPEAKERS_7;
    }
    ERR(EAX_PREFIX "Unexpected device channel format 0x%x.\n", mDevice->FmtChans);
    return HEADPHONES;

#undef EAX_PREFIX
}

void ALCcontext::eax_update_speaker_configuration()
{
    eax_speaker_config_ = eax_detect_speaker_configuration();
}

void ALCcontext::eax_set_last_error_defaults() noexcept
{
    eax_last_error_ = EAX_OK;
}

void ALCcontext::eax_set_session_defaults() noexcept
{
    eax_session_.ulEAXVersion = EAXCONTEXT_MINEAXSESSION;
    eax_session_.ulMaxActiveSends = EAXCONTEXT_DEFAULTMAXACTIVESENDS;
}

void ALCcontext::eax_set_context_defaults() noexcept
{
    eax_.context.guidPrimaryFXSlotID = EAXCONTEXT_DEFAULTPRIMARYFXSLOTID;
    eax_.context.flDistanceFactor = EAXCONTEXT_DEFAULTDISTANCEFACTOR;
    eax_.context.flAirAbsorptionHF = EAXCONTEXT_DEFAULTAIRABSORPTIONHF;
    eax_.context.flHFReference = EAXCONTEXT_DEFAULTHFREFERENCE;
}

void ALCcontext::eax_set_defaults() noexcept
{
    eax_set_last_error_defaults();
    eax_set_session_defaults();
    eax_set_context_defaults();

    eax_d_ = eax_;
}

void ALCcontext::eax_unlock_legacy_fx_slots(const EaxEaxCall& eax_call) noexcept
{
    if (eax_call.get_version() != 5 || eax_are_legacy_fx_slots_unlocked_)
        return;

    eax_are_legacy_fx_slots_unlocked_ = true;
    eax_fx_slots_.unlock_legacy();
}

void ALCcontext::eax_dispatch_fx_slot(
    const EaxEaxCall& eax_call)
{
    const auto fx_slot_index = eax_call.get_fx_slot_index();
    if(!fx_slot_index.has_value())
        eax_fail("Invalid fx slot index.");

    auto& fx_slot = eax_get_fx_slot(*fx_slot_index);
    if(fx_slot.eax_dispatch(eax_call))
    {
        std::lock_guard<std::mutex> source_lock{mSourceLock};
        eax_update_filters();
    }
}

void ALCcontext::eax_dispatch_source(
    const EaxEaxCall& eax_call)
{
    const auto source_id = eax_call.get_property_al_name();

    std::lock_guard<std::mutex> source_lock{mSourceLock};

    const auto source = ALsource::eax_lookup_source(*this, source_id);

    if (!source)
    {
        eax_fail("Source not found.");
    }

    source->eax_dispatch(eax_call);
}

void ALCcontext::eax_get_primary_fx_slot_id(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_.context.guidPrimaryFXSlotID);
}

void ALCcontext::eax_get_distance_factor(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_.context.flDistanceFactor);
}

void ALCcontext::eax_get_air_absorption_hf(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_.context.flAirAbsorptionHF);
}

void ALCcontext::eax_get_hf_reference(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_.context.flHFReference);
}

void ALCcontext::eax_get_last_error(
    const EaxEaxCall& eax_call)
{
    const auto eax_last_error = eax_last_error_;
    eax_last_error_ = EAX_OK;
    eax_call.set_value<ContextException>(eax_last_error);
}

void ALCcontext::eax_get_speaker_config(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_speaker_config_);
}

void ALCcontext::eax_get_session(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_session_);
}

void ALCcontext::eax_get_macro_fx_factor(
    const EaxEaxCall& eax_call)
{
    eax_call.set_value<ContextException>(eax_.context.flMacroFXFactor);
}

void ALCcontext::eax_get_context_all(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_version())
    {
        case 4:
            eax_call.set_value<ContextException>(static_cast<const EAX40CONTEXTPROPERTIES&>(eax_.context));
            break;

        case 5:
            eax_call.set_value<ContextException>(static_cast<const EAX50CONTEXTPROPERTIES&>(eax_.context));
            break;

        default:
            eax_fail("Unsupported EAX version.");
    }
}

void ALCcontext::eax_get(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case EAXCONTEXT_NONE:
            break;

        case EAXCONTEXT_ALLPARAMETERS:
            eax_get_context_all(eax_call);
            break;

        case EAXCONTEXT_PRIMARYFXSLOTID:
            eax_get_primary_fx_slot_id(eax_call);
            break;

        case EAXCONTEXT_DISTANCEFACTOR:
            eax_get_distance_factor(eax_call);
            break;

        case EAXCONTEXT_AIRABSORPTIONHF:
            eax_get_air_absorption_hf(eax_call);
            break;

        case EAXCONTEXT_HFREFERENCE:
            eax_get_hf_reference(eax_call);
            break;

        case EAXCONTEXT_LASTERROR:
            eax_get_last_error(eax_call);
            break;

        case EAXCONTEXT_SPEAKERCONFIG:
            eax_get_speaker_config(eax_call);
            break;

        case EAXCONTEXT_EAXSESSION:
            eax_get_session(eax_call);
            break;

        case EAXCONTEXT_MACROFXFACTOR:
            eax_get_macro_fx_factor(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void ALCcontext::eax_set_primary_fx_slot_id()
{
    eax_previous_primary_fx_slot_index_ = eax_primary_fx_slot_index_;
    eax_primary_fx_slot_index_ = eax_.context.guidPrimaryFXSlotID;
}

void ALCcontext::eax_set_distance_factor()
{
    mListener.mMetersPerUnit = eax_.context.flDistanceFactor;
    mPropsDirty = true;
}

void ALCcontext::eax_set_air_absorbtion_hf()
{
    mAirAbsorptionGainHF = level_mb_to_gain(eax_.context.flAirAbsorptionHF);
    mPropsDirty = true;
}

void ALCcontext::eax_set_hf_reference()
{
    // TODO
}

void ALCcontext::eax_set_macro_fx_factor()
{
    // TODO
}

void ALCcontext::eax_set_context()
{
    eax_set_primary_fx_slot_id();
    eax_set_distance_factor();
    eax_set_air_absorbtion_hf();
    eax_set_hf_reference();
}

void ALCcontext::eax_initialize_fx_slots()
{
    eax_fx_slots_.initialize(*this);
    eax_previous_primary_fx_slot_index_ = eax_.context.guidPrimaryFXSlotID;
    eax_primary_fx_slot_index_ = eax_.context.guidPrimaryFXSlotID;
}

void ALCcontext::eax_initialize_sources()
{
    std::unique_lock<std::mutex> source_lock{mSourceLock};
    auto init_source = [this](ALsource &source) noexcept
    { source.eax_initialize(this); };
    ForEachSource(this, init_source);
}

void ALCcontext::eax_update_sources()
{
    std::unique_lock<std::mutex> source_lock{mSourceLock};
    auto update_source = [this](ALsource &source)
    { source.eax_update(eax_context_shared_dirty_flags_); };
    ForEachSource(this, update_source);
}

void ALCcontext::eax_validate_primary_fx_slot_id(
    const GUID& primary_fx_slot_id)
{
    if (primary_fx_slot_id != EAX_NULL_GUID &&
        primary_fx_slot_id != EAXPROPERTYID_EAX40_FXSlot0 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX50_FXSlot0 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX40_FXSlot1 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX50_FXSlot1 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX40_FXSlot2 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX50_FXSlot2 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX40_FXSlot3 &&
        primary_fx_slot_id != EAXPROPERTYID_EAX50_FXSlot3)
    {
        eax_fail("Unsupported primary FX slot id.");
    }
}

void ALCcontext::eax_validate_distance_factor(
    float distance_factor)
{
    eax_validate_range<ContextException>(
        "Distance Factor",
        distance_factor,
        EAXCONTEXT_MINDISTANCEFACTOR,
        EAXCONTEXT_MAXDISTANCEFACTOR);
}

void ALCcontext::eax_validate_air_absorption_hf(
    float air_absorption_hf)
{
    eax_validate_range<ContextException>(
        "Air Absorption HF",
        air_absorption_hf,
        EAXCONTEXT_MINAIRABSORPTIONHF,
        EAXCONTEXT_MAXAIRABSORPTIONHF);
}

void ALCcontext::eax_validate_hf_reference(
    float hf_reference)
{
    eax_validate_range<ContextException>(
        "HF Reference",
        hf_reference,
        EAXCONTEXT_MINHFREFERENCE,
        EAXCONTEXT_MAXHFREFERENCE);
}

void ALCcontext::eax_validate_speaker_config(
    unsigned long speaker_config)
{
    switch (speaker_config)
    {
        case HEADPHONES:
        case SPEAKERS_2:
        case SPEAKERS_4:
        case SPEAKERS_5:
        case SPEAKERS_6:
        case SPEAKERS_7:
            break;

        default:
            eax_fail("Unsupported speaker configuration.");
    }
}

void ALCcontext::eax_validate_session_eax_version(
    unsigned long eax_version)
{
    switch (eax_version)
    {
        case EAX_40:
        case EAX_50:
            break;

        default:
            eax_fail("Unsupported session EAX version.");
    }
}

void ALCcontext::eax_validate_session_max_active_sends(
    unsigned long max_active_sends)
{
    eax_validate_range<ContextException>(
        "Max Active Sends",
        max_active_sends,
        EAXCONTEXT_MINMAXACTIVESENDS,
        EAXCONTEXT_MAXMAXACTIVESENDS);
}

void ALCcontext::eax_validate_session(
    const EAXSESSIONPROPERTIES& eax_session)
{
    eax_validate_session_eax_version(eax_session.ulEAXVersion);
    eax_validate_session_max_active_sends(eax_session.ulMaxActiveSends);
}

void ALCcontext::eax_validate_macro_fx_factor(
    float macro_fx_factor)
{
    eax_validate_range<ContextException>(
        "Macro FX Factor",
        macro_fx_factor,
        EAXCONTEXT_MINMACROFXFACTOR,
        EAXCONTEXT_MAXMACROFXFACTOR);
}

void ALCcontext::eax_validate_context_all(
    const EAX40CONTEXTPROPERTIES& context_all)
{
    eax_validate_primary_fx_slot_id(context_all.guidPrimaryFXSlotID);
    eax_validate_distance_factor(context_all.flDistanceFactor);
    eax_validate_air_absorption_hf(context_all.flAirAbsorptionHF);
    eax_validate_hf_reference(context_all.flHFReference);
}

void ALCcontext::eax_validate_context_all(
    const EAX50CONTEXTPROPERTIES& context_all)
{
    eax_validate_context_all(static_cast<const EAX40CONTEXTPROPERTIES>(context_all));
    eax_validate_macro_fx_factor(context_all.flMacroFXFactor);
}

void ALCcontext::eax_defer_primary_fx_slot_id(
    const GUID& primary_fx_slot_id)
{
    eax_d_.context.guidPrimaryFXSlotID = primary_fx_slot_id;

    eax_context_dirty_flags_.guidPrimaryFXSlotID =
        (eax_.context.guidPrimaryFXSlotID != eax_d_.context.guidPrimaryFXSlotID);
}

void ALCcontext::eax_defer_distance_factor(
    float distance_factor)
{
    eax_d_.context.flDistanceFactor = distance_factor;

    eax_context_dirty_flags_.flDistanceFactor =
        (eax_.context.flDistanceFactor != eax_d_.context.flDistanceFactor);
}

void ALCcontext::eax_defer_air_absorption_hf(
    float air_absorption_hf)
{
    eax_d_.context.flAirAbsorptionHF = air_absorption_hf;

    eax_context_dirty_flags_.flAirAbsorptionHF =
        (eax_.context.flAirAbsorptionHF != eax_d_.context.flAirAbsorptionHF);
}

void ALCcontext::eax_defer_hf_reference(
    float hf_reference)
{
    eax_d_.context.flHFReference = hf_reference;

    eax_context_dirty_flags_.flHFReference =
        (eax_.context.flHFReference != eax_d_.context.flHFReference);
}

void ALCcontext::eax_defer_macro_fx_factor(
    float macro_fx_factor)
{
    eax_d_.context.flMacroFXFactor = macro_fx_factor;

    eax_context_dirty_flags_.flMacroFXFactor =
        (eax_.context.flMacroFXFactor != eax_d_.context.flMacroFXFactor);
}

void ALCcontext::eax_defer_context_all(
    const EAX40CONTEXTPROPERTIES& context_all)
{
    eax_defer_primary_fx_slot_id(context_all.guidPrimaryFXSlotID);
    eax_defer_distance_factor(context_all.flDistanceFactor);
    eax_defer_air_absorption_hf(context_all.flAirAbsorptionHF);
    eax_defer_hf_reference(context_all.flHFReference);
}

void ALCcontext::eax_defer_context_all(
    const EAX50CONTEXTPROPERTIES& context_all)
{
    eax_defer_context_all(static_cast<const EAX40CONTEXTPROPERTIES&>(context_all));
    eax_defer_macro_fx_factor(context_all.flMacroFXFactor);
}

void ALCcontext::eax_defer_context_all(
    const EaxEaxCall& eax_call)
{
    switch(eax_call.get_version())
    {
    case 4:
        {
            const auto& context_all =
                eax_call.get_value<ContextException, EAX40CONTEXTPROPERTIES>();

            eax_validate_context_all(context_all);
            eax_defer_context_all(context_all);
        }
        break;

    case 5:
        {
            const auto& context_all =
                eax_call.get_value<ContextException, EAX50CONTEXTPROPERTIES>();

            eax_validate_context_all(context_all);
            eax_defer_context_all(context_all);
        }
        break;

    default:
        eax_fail("Unsupported EAX version.");
    }
}

void ALCcontext::eax_defer_primary_fx_slot_id(
    const EaxEaxCall& eax_call)
{
    const auto& primary_fx_slot_id =
        eax_call.get_value<ContextException, const decltype(EAX50CONTEXTPROPERTIES::guidPrimaryFXSlotID)>();

    eax_validate_primary_fx_slot_id(primary_fx_slot_id);
    eax_defer_primary_fx_slot_id(primary_fx_slot_id);
}

void ALCcontext::eax_defer_distance_factor(
    const EaxEaxCall& eax_call)
{
    const auto& distance_factor =
        eax_call.get_value<ContextException, const decltype(EAX50CONTEXTPROPERTIES::flDistanceFactor)>();

    eax_validate_distance_factor(distance_factor);
    eax_defer_distance_factor(distance_factor);
}

void ALCcontext::eax_defer_air_absorption_hf(
    const EaxEaxCall& eax_call)
{
    const auto& air_absorption_hf =
        eax_call.get_value<ContextException, const decltype(EAX50CONTEXTPROPERTIES::flAirAbsorptionHF)>();

    eax_validate_air_absorption_hf(air_absorption_hf);
    eax_defer_air_absorption_hf(air_absorption_hf);
}

void ALCcontext::eax_defer_hf_reference(
    const EaxEaxCall& eax_call)
{
    const auto& hf_reference =
        eax_call.get_value<ContextException, const decltype(EAX50CONTEXTPROPERTIES::flHFReference)>();

    eax_validate_hf_reference(hf_reference);
    eax_defer_hf_reference(hf_reference);
}

void ALCcontext::eax_set_session(
    const EaxEaxCall& eax_call)
{
    const auto& eax_session =
        eax_call.get_value<ContextException, const EAXSESSIONPROPERTIES>();

    eax_validate_session(eax_session);

    eax_session_ = eax_session;
}

void ALCcontext::eax_defer_macro_fx_factor(
    const EaxEaxCall& eax_call)
{
    const auto& macro_fx_factor =
        eax_call.get_value<ContextException, const decltype(EAX50CONTEXTPROPERTIES::flMacroFXFactor)>();

    eax_validate_macro_fx_factor(macro_fx_factor);
    eax_defer_macro_fx_factor(macro_fx_factor);
}

void ALCcontext::eax_set(
    const EaxEaxCall& eax_call)
{
    switch (eax_call.get_property_id())
    {
        case EAXCONTEXT_NONE:
            break;

        case EAXCONTEXT_ALLPARAMETERS:
            eax_defer_context_all(eax_call);
            break;

        case EAXCONTEXT_PRIMARYFXSLOTID:
            eax_defer_primary_fx_slot_id(eax_call);
            break;

        case EAXCONTEXT_DISTANCEFACTOR:
            eax_defer_distance_factor(eax_call);
            break;

        case EAXCONTEXT_AIRABSORPTIONHF:
            eax_defer_air_absorption_hf(eax_call);
            break;

        case EAXCONTEXT_HFREFERENCE:
            eax_defer_hf_reference(eax_call);
            break;

        case EAXCONTEXT_LASTERROR:
            eax_fail("Last error is read-only.");

        case EAXCONTEXT_SPEAKERCONFIG:
            eax_fail("Speaker configuration is read-only.");

        case EAXCONTEXT_EAXSESSION:
            eax_set_session(eax_call);
            break;

        case EAXCONTEXT_MACROFXFACTOR:
            eax_defer_macro_fx_factor(eax_call);
            break;

        default:
            eax_fail("Unsupported property id.");
    }
}

void ALCcontext::eax_apply_deferred()
{
    if (eax_context_dirty_flags_ == ContextDirtyFlags{})
    {
        return;
    }

    eax_ = eax_d_;

    if (eax_context_dirty_flags_.guidPrimaryFXSlotID)
    {
        eax_context_shared_dirty_flags_.primary_fx_slot_id = true;
        eax_set_primary_fx_slot_id();
    }

    if (eax_context_dirty_flags_.flDistanceFactor)
    {
        eax_set_distance_factor();
    }

    if (eax_context_dirty_flags_.flAirAbsorptionHF)
    {
        eax_set_air_absorbtion_hf();
    }

    if (eax_context_dirty_flags_.flHFReference)
    {
        eax_set_hf_reference();
    }

    if (eax_context_dirty_flags_.flMacroFXFactor)
    {
        eax_set_macro_fx_factor();
    }

    if (eax_context_shared_dirty_flags_ != EaxContextSharedDirtyFlags{})
    {
        eax_update_sources();
    }

    eax_context_shared_dirty_flags_ = EaxContextSharedDirtyFlags{};
    eax_context_dirty_flags_ = ContextDirtyFlags{};
}


namespace
{


class EaxSetException :
    public EaxException
{
public:
    explicit EaxSetException(
        const char* message)
        :
        EaxException{"EAX_SET", message}
    {
    }
}; // EaxSetException


[[noreturn]]
void eax_fail_set(
    const char* message)
{
    throw EaxSetException{message};
}


class EaxGetException :
    public EaxException
{
public:
    explicit EaxGetException(
        const char* message)
        :
        EaxException{"EAX_GET", message}
    {
    }
}; // EaxGetException


[[noreturn]]
void eax_fail_get(
    const char* message)
{
    throw EaxGetException{message};
}


} // namespace


FORCE_ALIGN ALenum AL_APIENTRY EAXSet(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_value,
    ALuint property_value_size) noexcept
try
{
    auto context = GetContextRef();

    if (!context)
    {
        eax_fail_set("No current context.");
    }

    std::lock_guard<std::mutex> prop_lock{context->mPropLock};

    return context->eax_eax_set(
        property_set_id,
        property_id,
        property_source_id,
        property_value,
        property_value_size
    );
}
catch (...)
{
    eax_log_exception(__func__);
    return AL_INVALID_OPERATION;
}

FORCE_ALIGN ALenum AL_APIENTRY EAXGet(
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_value,
    ALuint property_value_size) noexcept
try
{
    auto context = GetContextRef();

    if (!context)
    {
        eax_fail_get("No current context.");
    }

    std::lock_guard<std::mutex> prop_lock{context->mPropLock};

    return context->eax_eax_get(
        property_set_id,
        property_id,
        property_source_id,
        property_value,
        property_value_size
    );
}
catch (...)
{
    eax_log_exception(__func__);
    return AL_INVALID_OPERATION;
}
#endif // ALSOFT_EAX
