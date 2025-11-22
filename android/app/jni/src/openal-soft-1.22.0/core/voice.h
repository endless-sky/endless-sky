#ifndef CORE_VOICE_H
#define CORE_VOICE_H

#include <array>
#include <atomic>
#include <bitset>
#include <memory>
#include <stddef.h>
#include <string>

#include "albyte.h"
#include "almalloc.h"
#include "aloptional.h"
#include "alspan.h"
#include "bufferline.h"
#include "buffer_storage.h"
#include "devformat.h"
#include "filters/biquad.h"
#include "filters/nfc.h"
#include "filters/splitter.h"
#include "mixer/defs.h"
#include "mixer/hrtfdefs.h"
#include "resampler_limits.h"
#include "uhjfilter.h"
#include "vector.h"

struct ContextBase;
struct DeviceBase;
struct EffectSlot;
enum class DistanceModel : unsigned char;

using uint = unsigned int;


#define MAX_SENDS  6


enum class SpatializeMode : unsigned char {
    Off,
    On,
    Auto
};

enum class DirectMode : unsigned char {
    Off,
    DropMismatch,
    RemixMismatch
};


/* Maximum number of extra source samples that may need to be loaded, for
 * resampling or conversion purposes.
 */
constexpr uint MaxPostVoiceLoad{MaxResamplerEdge + UhjDecoder::sFilterDelay};


enum {
    AF_None = 0,
    AF_LowPass = 1,
    AF_HighPass = 2,
    AF_BandPass = AF_LowPass | AF_HighPass
};


struct DirectParams {
    BiquadFilter LowPass;
    BiquadFilter HighPass;

    NfcFilter NFCtrlFilter;

    struct {
        HrtfFilter Old;
        HrtfFilter Target;
        alignas(16) std::array<float,HrtfHistoryLength> History;
    } Hrtf;

    struct {
        std::array<float,MAX_OUTPUT_CHANNELS> Current;
        std::array<float,MAX_OUTPUT_CHANNELS> Target;
    } Gains;
};

struct SendParams {
    BiquadFilter LowPass;
    BiquadFilter HighPass;

    struct {
        std::array<float,MAX_OUTPUT_CHANNELS> Current;
        std::array<float,MAX_OUTPUT_CHANNELS> Target;
    } Gains;
};


struct VoiceBufferItem {
    std::atomic<VoiceBufferItem*> mNext{nullptr};

    CallbackType mCallback{nullptr};
    void *mUserData{nullptr};

    uint mSampleLen{0u};
    uint mLoopStart{0u};
    uint mLoopEnd{0u};

    al::byte *mSamples{nullptr};
};


struct VoiceProps {
    float Pitch;
    float Gain;
    float OuterGain;
    float MinGain;
    float MaxGain;
    float InnerAngle;
    float OuterAngle;
    float RefDistance;
    float MaxDistance;
    float RolloffFactor;
    std::array<float,3> Position;
    std::array<float,3> Velocity;
    std::array<float,3> Direction;
    std::array<float,3> OrientAt;
    std::array<float,3> OrientUp;
    bool HeadRelative;
    DistanceModel mDistanceModel;
    Resampler mResampler;
    DirectMode DirectChannels;
    SpatializeMode mSpatializeMode;

    bool DryGainHFAuto;
    bool WetGainAuto;
    bool WetGainHFAuto;
    float OuterGainHF;

    float AirAbsorptionFactor;
    float RoomRolloffFactor;
    float DopplerFactor;

    std::array<float,2> StereoPan;

    float Radius;
    float EnhWidth;

    /** Direct filter and auxiliary send info. */
    struct {
        float Gain;
        float GainHF;
        float HFReference;
        float GainLF;
        float LFReference;
    } Direct;
    struct SendData {
        EffectSlot *Slot;
        float Gain;
        float GainHF;
        float HFReference;
        float GainLF;
        float LFReference;
    } Send[MAX_SENDS];
};

struct VoicePropsItem : public VoiceProps {
    std::atomic<VoicePropsItem*> next{nullptr};

    DEF_NEWDEL(VoicePropsItem)
};

enum : uint {
    VoiceIsStatic,
    VoiceIsCallback,
    VoiceIsAmbisonic,
    VoiceCallbackStopped,
    VoiceIsFading,
    VoiceHasHrtf,
    VoiceHasNfc,

    VoiceFlagCount
};

struct Voice {
    enum State {
        Stopped,
        Playing,
        Stopping,
        Pending
    };

    std::atomic<VoicePropsItem*> mUpdate{nullptr};

    VoiceProps mProps;

    std::atomic<uint> mSourceID{0u};
    std::atomic<State> mPlayState{Stopped};
    std::atomic<bool> mPendingChange{false};

    /**
     * Source offset in samples, relative to the currently playing buffer, NOT
     * the whole queue.
     */
    std::atomic<uint> mPosition;
    /** Fractional (fixed-point) offset to the next sample. */
    std::atomic<uint> mPositionFrac;

    /* Current buffer queue item being played. */
    std::atomic<VoiceBufferItem*> mCurrentBuffer;

    /* Buffer queue item to loop to at end of queue (will be NULL for non-
     * looping voices).
     */
    std::atomic<VoiceBufferItem*> mLoopBuffer;

    /* Properties for the attached buffer(s). */
    FmtChannels mFmtChannels;
    FmtType mFmtType;
    uint mFrequency;
    uint mFrameStep; /**< In steps of the sample type size. */
    uint mFrameSize; /**< In bytes. */
    AmbiLayout mAmbiLayout;
    AmbiScaling mAmbiScaling;
    uint mAmbiOrder;

    std::unique_ptr<UhjDecoder> mDecoder;
    UhjDecoder::DecoderFunc mDecoderFunc{};

    /** Current target parameters used for mixing. */
    uint mStep{0};

    ResamplerFunc mResampler;

    InterpState mResampleState;

    std::bitset<VoiceFlagCount> mFlags{};
    uint mNumCallbackSamples{0};

    struct TargetData {
        int FilterType;
        al::span<FloatBufferLine> Buffer;
    };
    TargetData mDirect;
    std::array<TargetData,MAX_SENDS> mSend;

    /* The first MaxResamplerPadding/2 elements are the sample history from the
     * previous mix, with an additional MaxResamplerPadding/2 elements that are
     * now current (which may be overwritten if the buffer data is still
     * available).
     */
    using HistoryLine = std::array<float,MaxResamplerPadding>;
    al::vector<HistoryLine,16> mPrevSamples{2};

    struct ChannelData {
        float mAmbiHFScale, mAmbiLFScale;
        BandSplitter mAmbiSplitter;

        DirectParams mDryParams;
        std::array<SendParams,MAX_SENDS> mWetParams;
    };
    al::vector<ChannelData> mChans{2};

    Voice() = default;
    ~Voice() = default;

    Voice(const Voice&) = delete;
    Voice& operator=(const Voice&) = delete;

    void mix(const State vstate, ContextBase *Context, const uint SamplesToDo);

    void prepare(DeviceBase *device);

    static void InitMixer(al::optional<std::string> resampler);

    DEF_NEWDEL(Voice)
};

extern Resampler ResamplerDefault;

#endif /* CORE_VOICE_H */
