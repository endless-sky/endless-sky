#ifndef AL_SOURCE_H
#define AL_SOURCE_H

#include <array>
#include <atomic>
#include <cstddef>
#include <iterator>
#include <limits>
#include <deque>

#include "AL/al.h"
#include "AL/alc.h"

#include "alc/alu.h"
#include "alc/context.h"
#include "alc/inprogext.h"
#include "aldeque.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "atomic.h"
#include "core/voice.h"
#include "vector.h"

#ifdef ALSOFT_EAX
#include "eax_eax_call.h"
#include "eax_fx_slot_index.h"
#include "eax_utils.h"
#endif // ALSOFT_EAX

struct ALbuffer;
struct ALeffectslot;


enum class SourceStereo : bool {
    Normal = AL_NORMAL_SOFT,
    Enhanced = AL_SUPER_STEREO_SOFT
};

#define DEFAULT_SENDS  2

#define INVALID_VOICE_IDX static_cast<ALuint>(-1)

struct ALbufferQueueItem : public VoiceBufferItem {
    ALbuffer *mBuffer{nullptr};

    DISABLE_ALLOC()
};


#ifdef ALSOFT_EAX
using EaxSourceSourceFilterDirtyFlagsValue = std::uint_least16_t;

struct EaxSourceSourceFilterDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

	EaxSourceSourceFilterDirtyFlagsValue lDirect : 1;
	EaxSourceSourceFilterDirtyFlagsValue lDirectHF : 1;
	EaxSourceSourceFilterDirtyFlagsValue lRoom : 1;
	EaxSourceSourceFilterDirtyFlagsValue lRoomHF : 1;
	EaxSourceSourceFilterDirtyFlagsValue lObstruction : 1;
	EaxSourceSourceFilterDirtyFlagsValue flObstructionLFRatio : 1;
	EaxSourceSourceFilterDirtyFlagsValue lOcclusion : 1;
	EaxSourceSourceFilterDirtyFlagsValue flOcclusionLFRatio : 1;
	EaxSourceSourceFilterDirtyFlagsValue flOcclusionRoomRatio : 1;
	EaxSourceSourceFilterDirtyFlagsValue flOcclusionDirectRatio : 1;
	EaxSourceSourceFilterDirtyFlagsValue lExclusion : 1;
	EaxSourceSourceFilterDirtyFlagsValue flExclusionLFRatio : 1;
}; // EaxSourceSourceFilterDirtyFlags


using EaxSourceSourceMiscDirtyFlagsValue = std::uint_least8_t;

struct EaxSourceSourceMiscDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

	EaxSourceSourceMiscDirtyFlagsValue lOutsideVolumeHF : 1;
	EaxSourceSourceMiscDirtyFlagsValue flDopplerFactor : 1;
	EaxSourceSourceMiscDirtyFlagsValue flRolloffFactor : 1;
	EaxSourceSourceMiscDirtyFlagsValue flRoomRolloffFactor : 1;
	EaxSourceSourceMiscDirtyFlagsValue flAirAbsorptionFactor : 1;
	EaxSourceSourceMiscDirtyFlagsValue ulFlags : 1;
	EaxSourceSourceMiscDirtyFlagsValue flMacroFXFactor : 1;
	EaxSourceSourceMiscDirtyFlagsValue speaker_levels : 1;
}; // EaxSourceSourceMiscDirtyFlags


using EaxSourceSendDirtyFlagsValue = std::uint_least8_t;

struct EaxSourceSendDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

	EaxSourceSendDirtyFlagsValue lSend : 1;
	EaxSourceSendDirtyFlagsValue lSendHF : 1;
	EaxSourceSendDirtyFlagsValue lOcclusion : 1;
	EaxSourceSendDirtyFlagsValue flOcclusionLFRatio : 1;
	EaxSourceSendDirtyFlagsValue flOcclusionRoomRatio : 1;
	EaxSourceSendDirtyFlagsValue flOcclusionDirectRatio : 1;
	EaxSourceSendDirtyFlagsValue lExclusion : 1;
	EaxSourceSendDirtyFlagsValue flExclusionLFRatio : 1;
}; // EaxSourceSendDirtyFlags


struct EaxSourceSendsDirtyFlags
{
    using EaxIsBitFieldStruct = bool;

	EaxSourceSendDirtyFlags sends[EAX_MAX_FXSLOTS];
}; // EaxSourceSendsDirtyFlags
#endif // ALSOFT_EAX

struct ALsource {
    /** Source properties. */
    float Pitch{1.0f};
    float Gain{1.0f};
    float OuterGain{0.0f};
    float MinGain{0.0f};
    float MaxGain{1.0f};
    float InnerAngle{360.0f};
    float OuterAngle{360.0f};
    float RefDistance{1.0f};
    float MaxDistance{std::numeric_limits<float>::max()};
    float RolloffFactor{1.0f};
#ifdef ALSOFT_EAX
    // For EAXSOURCE_ROLLOFFFACTOR, which is distinct from and added to
    // AL_ROLLOFF_FACTOR
    float RolloffFactor2{0.0f};
#endif
    std::array<float,3> Position{{0.0f, 0.0f, 0.0f}};
    std::array<float,3> Velocity{{0.0f, 0.0f, 0.0f}};
    std::array<float,3> Direction{{0.0f, 0.0f, 0.0f}};
    std::array<float,3> OrientAt{{0.0f, 0.0f, -1.0f}};
    std::array<float,3> OrientUp{{0.0f, 1.0f,  0.0f}};
    bool HeadRelative{false};
    bool Looping{false};
    DistanceModel mDistanceModel{DistanceModel::Default};
    Resampler mResampler{ResamplerDefault};
    DirectMode DirectChannels{DirectMode::Off};
    SpatializeMode mSpatialize{SpatializeMode::Auto};
    SourceStereo mStereoMode{SourceStereo::Normal};

    bool DryGainHFAuto{true};
    bool WetGainAuto{true};
    bool WetGainHFAuto{true};
    float OuterGainHF{1.0f};

    float AirAbsorptionFactor{0.0f};
    float RoomRolloffFactor{0.0f};
    float DopplerFactor{1.0f};

    /* NOTE: Stereo pan angles are specified in radians, counter-clockwise
     * rather than clockwise.
     */
    std::array<float,2> StereoPan{{al::numbers::pi_v<float>/6.0f, -al::numbers::pi_v<float>/6.0f}};

    float Radius{0.0f};
    float EnhWidth{0.593f};

    /** Direct filter and auxiliary send info. */
    struct {
        float Gain;
        float GainHF;
        float HFReference;
        float GainLF;
        float LFReference;
    } Direct;
    struct SendData {
        ALeffectslot *Slot;
        float Gain;
        float GainHF;
        float HFReference;
        float GainLF;
        float LFReference;
    };
    std::array<SendData,MAX_SENDS> Send;

    /**
     * Last user-specified offset, and the offset type (bytes, samples, or
     * seconds).
     */
    double Offset{0.0};
    ALenum OffsetType{AL_NONE};

    /** Source type (static, streaming, or undetermined) */
    ALenum SourceType{AL_UNDETERMINED};

    /** Source state (initial, playing, paused, or stopped) */
    ALenum state{AL_INITIAL};

    /** Source Buffer Queue head. */
    al::deque<ALbufferQueueItem> mQueue;

    bool mPropsDirty{true};

    /* Index into the context's Voices array. Lazily updated, only checked and
     * reset when looking up the voice.
     */
    ALuint VoiceIdx{INVALID_VOICE_IDX};

    /** Self ID */
    ALuint id{0};


    ALsource();
    ~ALsource();

    ALsource(const ALsource&) = delete;
    ALsource& operator=(const ALsource&) = delete;

    DISABLE_ALLOC()

#ifdef ALSOFT_EAX
public:
    void eax_initialize(ALCcontext *context) noexcept;


    void eax_dispatch(const EaxEaxCall& eax_call)
    { eax_call.is_get() ? eax_get(eax_call) : eax_set(eax_call); }


    void eax_update_filters();

    void eax_update(
        EaxContextSharedDirtyFlags dirty_flags);

    void eax_commit() { eax_apply_deferred(); }
    void eax_commit_and_update();

    bool eax_is_initialized() const noexcept { return eax_al_context_; }


    static ALsource* eax_lookup_source(
        ALCcontext& al_context,
        ALuint source_id) noexcept;


private:
    static constexpr auto eax_max_speakers = 9;


    using EaxActiveFxSlots = std::array<bool, EAX_MAX_FXSLOTS>;
    using EaxSpeakerLevels = std::array<long, eax_max_speakers>;

    struct Eax
    {
        using Sends = std::array<EAXSOURCEALLSENDPROPERTIES, EAX_MAX_FXSLOTS>;

        EAX50ACTIVEFXSLOTS active_fx_slots{};
        EAX50SOURCEPROPERTIES source{};
        Sends sends{};
        EaxSpeakerLevels speaker_levels{};
    }; // Eax


    bool eax_uses_primary_id_{};
    bool eax_has_active_fx_slots_{};
    bool eax_are_active_fx_slots_dirty_{};

    ALCcontext* eax_al_context_{};

    EAXBUFFER_REVERBPROPERTIES eax1_{};
    Eax eax_{};
    Eax eax_d_{};
    EaxActiveFxSlots eax_active_fx_slots_{};

    EaxSourceSendsDirtyFlags eax_sends_dirty_flags_{};
    EaxSourceSourceFilterDirtyFlags eax_source_dirty_filter_flags_{};
    EaxSourceSourceMiscDirtyFlags eax_source_dirty_misc_flags_{};


    [[noreturn]]
    static void eax_fail(
        const char* message);


    void eax_set_source_defaults() noexcept;
    void eax_set_active_fx_slots_defaults() noexcept;
    void eax_set_send_defaults(EAXSOURCEALLSENDPROPERTIES& eax_send) noexcept;
    void eax_set_sends_defaults() noexcept;
    void eax_set_speaker_levels_defaults() noexcept;
    void eax_set_defaults() noexcept;


    static float eax_calculate_dst_occlusion_mb(
        long src_occlusion_mb,
        float path_ratio,
        float lf_ratio) noexcept;

    EaxAlLowPassParam eax_create_direct_filter_param() const noexcept;

    EaxAlLowPassParam eax_create_room_filter_param(
        const ALeffectslot& fx_slot,
        const EAXSOURCEALLSENDPROPERTIES& send) const noexcept;

    void eax_set_fx_slots();

    void eax_initialize_fx_slots();

    void eax_update_direct_filter_internal();

    void eax_update_room_filters_internal();

    void eax_update_filters_internal();

    void eax_update_primary_fx_slot_id();


    void eax_defer_active_fx_slots(
        const EaxEaxCall& eax_call);


    static const char* eax_get_exclusion_name() noexcept;

    static const char* eax_get_exclusion_lf_ratio_name() noexcept;


    static const char* eax_get_occlusion_name() noexcept;

    static const char* eax_get_occlusion_lf_ratio_name() noexcept;

    static const char* eax_get_occlusion_direct_ratio_name() noexcept;

    static const char* eax_get_occlusion_room_ratio_name() noexcept;


    static void eax1_validate_reverb_mix(float reverb_mix);

    static void eax_validate_send_receiving_fx_slot_guid(
        const GUID& guidReceivingFXSlotID);

    static void eax_validate_send_send(
        long lSend);

    static void eax_validate_send_send_hf(
        long lSendHF);

    static void eax_validate_send_occlusion(
        long lOcclusion);

    static void eax_validate_send_occlusion_lf_ratio(
        float flOcclusionLFRatio);

    static void eax_validate_send_occlusion_room_ratio(
        float flOcclusionRoomRatio);

    static void eax_validate_send_occlusion_direct_ratio(
        float flOcclusionDirectRatio);

    static void eax_validate_send_exclusion(
        long lExclusion);

    static void eax_validate_send_exclusion_lf_ratio(
        float flExclusionLFRatio);

    static void eax_validate_send(
        const EAXSOURCESENDPROPERTIES& all);

    static void eax_validate_send_exclusion_all(
        const EAXSOURCEEXCLUSIONSENDPROPERTIES& all);

    static void eax_validate_send_occlusion_all(
        const EAXSOURCEOCCLUSIONSENDPROPERTIES& all);

    static void eax_validate_send_all(
        const EAXSOURCEALLSENDPROPERTIES& all);


    static EaxFxSlotIndexValue eax_get_send_index(
        const GUID& send_guid);


    void eax_defer_send_send(
        long lSend,
        EaxFxSlotIndexValue index);

    void eax_defer_send_send_hf(
        long lSendHF,
        EaxFxSlotIndexValue index);

    void eax_defer_send_occlusion(
        long lOcclusion,
        EaxFxSlotIndexValue index);

    void eax_defer_send_occlusion_lf_ratio(
        float flOcclusionLFRatio,
        EaxFxSlotIndexValue index);

    void eax_defer_send_occlusion_room_ratio(
        float flOcclusionRoomRatio,
        EaxFxSlotIndexValue index);

    void eax_defer_send_occlusion_direct_ratio(
        float flOcclusionDirectRatio,
        EaxFxSlotIndexValue index);

    void eax_defer_send_exclusion(
        long lExclusion,
        EaxFxSlotIndexValue index);

    void eax_defer_send_exclusion_lf_ratio(
        float flExclusionLFRatio,
        EaxFxSlotIndexValue index);

    void eax_defer_send(
        const EAXSOURCESENDPROPERTIES& all,
        EaxFxSlotIndexValue index);

    void eax_defer_send_exclusion_all(
        const EAXSOURCEEXCLUSIONSENDPROPERTIES& all,
        EaxFxSlotIndexValue index);

    void eax_defer_send_occlusion_all(
        const EAXSOURCEOCCLUSIONSENDPROPERTIES& all,
        EaxFxSlotIndexValue index);

    void eax_defer_send_all(
        const EAXSOURCEALLSENDPROPERTIES& all,
        EaxFxSlotIndexValue index);


    void eax_defer_send(
        const EaxEaxCall& eax_call);

    void eax_defer_send_exclusion_all(
        const EaxEaxCall& eax_call);

    void eax_defer_send_occlusion_all(
        const EaxEaxCall& eax_call);

    void eax_defer_send_all(
        const EaxEaxCall& eax_call);


    static void eax_validate_source_direct(
        long direct);

    static void eax_validate_source_direct_hf(
        long direct_hf);

    static void eax_validate_source_room(
        long room);

    static void eax_validate_source_room_hf(
        long room_hf);

    static void eax_validate_source_obstruction(
        long obstruction);

    static void eax_validate_source_obstruction_lf_ratio(
        float obstruction_lf_ratio);

    static void eax_validate_source_occlusion(
        long occlusion);

    static void eax_validate_source_occlusion_lf_ratio(
        float occlusion_lf_ratio);

    static void eax_validate_source_occlusion_room_ratio(
        float occlusion_room_ratio);

    static void eax_validate_source_occlusion_direct_ratio(
        float occlusion_direct_ratio);

    static void eax_validate_source_exclusion(
        long exclusion);

    static void eax_validate_source_exclusion_lf_ratio(
        float exclusion_lf_ratio);

    static void eax_validate_source_outside_volume_hf(
        long outside_volume_hf);

    static void eax_validate_source_doppler_factor(
        float doppler_factor);

    static void eax_validate_source_rolloff_factor(
        float rolloff_factor);

    static void eax_validate_source_room_rolloff_factor(
        float room_rolloff_factor);

    static void eax_validate_source_air_absorption_factor(
        float air_absorption_factor);

    static void eax_validate_source_flags(
        unsigned long flags,
        int eax_version);

    static void eax_validate_source_macro_fx_factor(
        float macro_fx_factor);

    static void eax_validate_source_2d_all(
        const EAXSOURCE2DPROPERTIES& all,
        int eax_version);

    static void eax_validate_source_obstruction_all(
        const EAXOBSTRUCTIONPROPERTIES& all);

    static void eax_validate_source_exclusion_all(
        const EAXEXCLUSIONPROPERTIES& all);

    static void eax_validate_source_occlusion_all(
        const EAXOCCLUSIONPROPERTIES& all);

    static void eax_validate_source_all(
        const EAX20BUFFERPROPERTIES& all,
        int eax_version);

    static void eax_validate_source_all(
        const EAX30SOURCEPROPERTIES& all,
        int eax_version);

    static void eax_validate_source_all(
        const EAX50SOURCEPROPERTIES& all,
        int eax_version);

    static void eax_validate_source_speaker_id(
        long speaker_id);

    static void eax_validate_source_speaker_level(
        long speaker_level);

    static void eax_validate_source_speaker_level_all(
        const EAXSPEAKERLEVELPROPERTIES& all);


    void eax_defer_source_direct(
        long lDirect);

    void eax_defer_source_direct_hf(
        long lDirectHF);

    void eax_defer_source_room(
        long lRoom);

    void eax_defer_source_room_hf(
        long lRoomHF);

    void eax_defer_source_obstruction(
        long lObstruction);

    void eax_defer_source_obstruction_lf_ratio(
        float flObstructionLFRatio);

    void eax_defer_source_occlusion(
        long lOcclusion);

    void eax_defer_source_occlusion_lf_ratio(
        float flOcclusionLFRatio);

    void eax_defer_source_occlusion_room_ratio(
        float flOcclusionRoomRatio);

    void eax_defer_source_occlusion_direct_ratio(
        float flOcclusionDirectRatio);

    void eax_defer_source_exclusion(
        long lExclusion);

    void eax_defer_source_exclusion_lf_ratio(
        float flExclusionLFRatio);

    void eax_defer_source_outside_volume_hf(
        long lOutsideVolumeHF);

    void eax_defer_source_doppler_factor(
        float flDopplerFactor);

    void eax_defer_source_rolloff_factor(
        float flRolloffFactor);

    void eax_defer_source_room_rolloff_factor(
        float flRoomRolloffFactor);

    void eax_defer_source_air_absorption_factor(
        float flAirAbsorptionFactor);

    void eax_defer_source_flags(
        unsigned long ulFlags);

    void eax_defer_source_macro_fx_factor(
        float flMacroFXFactor);

    void eax_defer_source_2d_all(
        const EAXSOURCE2DPROPERTIES& all);

    void eax_defer_source_obstruction_all(
        const EAXOBSTRUCTIONPROPERTIES& all);

    void eax_defer_source_exclusion_all(
        const EAXEXCLUSIONPROPERTIES& all);

    void eax_defer_source_occlusion_all(
        const EAXOCCLUSIONPROPERTIES& all);

    void eax_defer_source_all(
        const EAX20BUFFERPROPERTIES& all);

    void eax_defer_source_all(
        const EAX30SOURCEPROPERTIES& all);

    void eax_defer_source_all(
        const EAX50SOURCEPROPERTIES& all);

    void eax_defer_source_speaker_level_all(
        const EAXSPEAKERLEVELPROPERTIES& all);


    void eax_defer_source_direct(
        const EaxEaxCall& eax_call);

    void eax_defer_source_direct_hf(
        const EaxEaxCall& eax_call);

    void eax_defer_source_room(
        const EaxEaxCall& eax_call);

    void eax_defer_source_room_hf(
        const EaxEaxCall& eax_call);

    void eax_defer_source_obstruction(
        const EaxEaxCall& eax_call);

    void eax_defer_source_obstruction_lf_ratio(
        const EaxEaxCall& eax_call);

    void eax_defer_source_occlusion(
        const EaxEaxCall& eax_call);

    void eax_defer_source_occlusion_lf_ratio(
        const EaxEaxCall& eax_call);

    void eax_defer_source_occlusion_room_ratio(
        const EaxEaxCall& eax_call);

    void eax_defer_source_occlusion_direct_ratio(
        const EaxEaxCall& eax_call);

    void eax_defer_source_exclusion(
        const EaxEaxCall& eax_call);

    void eax_defer_source_exclusion_lf_ratio(
        const EaxEaxCall& eax_call);

    void eax_defer_source_outside_volume_hf(
        const EaxEaxCall& eax_call);

    void eax_defer_source_doppler_factor(
        const EaxEaxCall& eax_call);

    void eax_defer_source_rolloff_factor(
        const EaxEaxCall& eax_call);

    void eax_defer_source_room_rolloff_factor(
        const EaxEaxCall& eax_call);

    void eax_defer_source_air_absorption_factor(
        const EaxEaxCall& eax_call);

    void eax_defer_source_flags(
        const EaxEaxCall& eax_call);

    void eax_defer_source_macro_fx_factor(
        const EaxEaxCall& eax_call);

    void eax_defer_source_2d_all(
        const EaxEaxCall& eax_call);

    void eax_defer_source_obstruction_all(
        const EaxEaxCall& eax_call);

    void eax_defer_source_exclusion_all(
        const EaxEaxCall& eax_call);

    void eax_defer_source_occlusion_all(
        const EaxEaxCall& eax_call);

    void eax_defer_source_all(
        const EaxEaxCall& eax_call);

    void eax_defer_source_speaker_level_all(
        const EaxEaxCall& eax_call);


    void eax_set_outside_volume_hf();

    void eax_set_doppler_factor();

    void eax_set_rolloff_factor();

    void eax_set_room_rolloff_factor();

    void eax_set_air_absorption_factor();


    void eax_set_direct_hf_auto_flag();

    void eax_set_room_auto_flag();

    void eax_set_room_hf_auto_flag();

    void eax_set_flags();


    void eax_set_macro_fx_factor();

    void eax_set_speaker_levels();


    void eax1_set_efx();
    void eax1_set_reverb_mix(const EaxEaxCall& eax_call);
    void eax1_set(const EaxEaxCall& eax_call);

    void eax_apply_deferred();

    void eax_set(
        const EaxEaxCall& eax_call);


    static const GUID& eax_get_send_fx_slot_guid(
        int eax_version,
        EaxFxSlotIndexValue fx_slot_index);

    static void eax_copy_send(
        const EAXSOURCEALLSENDPROPERTIES& src_send,
        EAXSOURCESENDPROPERTIES& dst_send);

    static void eax_copy_send(
        const EAXSOURCEALLSENDPROPERTIES& src_send,
        EAXSOURCEALLSENDPROPERTIES& dst_send);

    static void eax_copy_send(
        const EAXSOURCEALLSENDPROPERTIES& src_send,
        EAXSOURCEOCCLUSIONSENDPROPERTIES& dst_send);

    static void eax_copy_send(
        const EAXSOURCEALLSENDPROPERTIES& src_send,
        EAXSOURCEEXCLUSIONSENDPROPERTIES& dst_send);

    template<
        typename TException,
        typename TSrcSend
    >
    void eax_api_get_send_properties(
        const EaxEaxCall& eax_call) const
    {
        const auto eax_version = eax_call.get_version();
        const auto dst_sends = eax_call.get_values<TException, TSrcSend>();
        const auto send_count = dst_sends.size();

        for (auto fx_slot_index = EaxFxSlotIndexValue{}; fx_slot_index < send_count; ++fx_slot_index)
        {
            auto& dst_send = dst_sends[fx_slot_index];
            const auto& src_send = eax_.sends[fx_slot_index];

            eax_copy_send(src_send, dst_send);

            dst_send.guidReceivingFXSlotID = eax_get_send_fx_slot_guid(eax_version, fx_slot_index);
        }
    }


    void eax1_get(const EaxEaxCall& eax_call);

    void eax_api_get_source_all_v2(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all_v3(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all_v5(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all_obstruction(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all_occlusion(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all_exclusion(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_active_fx_slot_id(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_all_2d(
        const EaxEaxCall& eax_call);

    void eax_api_get_source_speaker_level_all(
        const EaxEaxCall& eax_call);

    void eax_get(
        const EaxEaxCall& eax_call);


    // `alSource3i(source, AL_AUXILIARY_SEND_FILTER, ...)`
    void eax_set_al_source_send(ALeffectslot *slot, size_t sendidx,
        const EaxAlLowPassParam &filter);
#endif // ALSOFT_EAX
};

void UpdateAllSourceProps(ALCcontext *context);

#endif
