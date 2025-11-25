#ifndef CORE_DEVICE_H
#define CORE_DEVICE_H

#include <stddef.h>

#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "almalloc.h"
#include "alspan.h"
#include "ambidefs.h"
#include "atomic.h"
#include "bufferline.h"
#include "devformat.h"
#include "filters/nfc.h"
#include "intrusive_ptr.h"
#include "mixer/hrtfdefs.h"
#include "opthelpers.h"
#include "resampler_limits.h"
#include "uhjfilter.h"
#include "vector.h"

class BFormatDec;
struct bs2b;
struct Compressor;
struct ContextBase;
struct DirectHrtfState;
struct HrtfStore;

using uint = unsigned int;


#define MIN_OUTPUT_RATE      8000
#define MAX_OUTPUT_RATE      192000
#define DEFAULT_OUTPUT_RATE  44100

#define DEFAULT_UPDATE_SIZE  882 /* 20ms */
#define DEFAULT_NUM_UPDATES  3


enum class DeviceType : unsigned char {
    Playback,
    Capture,
    Loopback
};


enum class RenderMode : unsigned char {
    Normal,
    Pairwise,
    Hrtf
};

enum class StereoEncoding : unsigned char {
    Basic,
    Uhj,
    Hrtf,

    Default = Basic
};


struct InputRemixMap {
    struct TargetMix { Channel channel; float mix; };

    Channel channel;
    std::array<TargetMix,2> targets;
};


/* Maximum delay in samples for speaker distance compensation. */
#define MAX_DELAY_LENGTH 1024

struct DistanceComp {
    struct ChanData {
        float Gain{1.0f};
        uint Length{0u}; /* Valid range is [0...MAX_DELAY_LENGTH). */
        float *Buffer{nullptr};
    };

    std::array<ChanData,MAX_OUTPUT_CHANNELS> mChannels;
    al::FlexArray<float,16> mSamples;

    DistanceComp(size_t count) : mSamples{count} { }

    static std::unique_ptr<DistanceComp> Create(size_t numsamples)
    { return std::unique_ptr<DistanceComp>{new(FamCount(numsamples)) DistanceComp{numsamples}}; }

    DEF_FAM_NEWDEL(DistanceComp, mSamples)
};


struct BFChannelConfig {
    float Scale;
    uint Index;
};


struct MixParams {
    /* Coefficient channel mapping for mixing to the buffer. */
    std::array<BFChannelConfig,MAX_OUTPUT_CHANNELS> AmbiMap{};

    al::span<FloatBufferLine> Buffer;
};

struct RealMixParams {
    al::span<const InputRemixMap> RemixMap;
    std::array<uint,MaxChannels> ChannelIndex{};

    al::span<FloatBufferLine> Buffer;
};

enum {
    // Frequency was requested by the app or config file
    FrequencyRequest,
    // Channel configuration was requested by the config file
    ChannelsRequest,
    // Sample type was requested by the config file
    SampleTypeRequest,

    // Specifies if the DSP is paused at user request
    DevicePaused,
    // Specifies if the device is currently running
    DeviceRunning,

    // Specifies if the output plays directly on/in ears (headphones, headset,
    // ear buds, etc).
    DirectEar,

    DeviceFlagsCount
};

struct DeviceBase {
    /* To avoid extraneous allocations, a 0-sized FlexArray<ContextBase*> is
     * defined globally as a sharable object.
     */
    static al::FlexArray<ContextBase*> sEmptyContextArray;

    std::atomic<bool> Connected{true};
    const DeviceType Type{};

    uint Frequency{};
    uint UpdateSize{};
    uint BufferSize{};

    DevFmtChannels FmtChans{};
    DevFmtType FmtType{};
    uint mAmbiOrder{0};
    float mXOverFreq{400.0f};
    /* For DevFmtAmbi* output only, specifies the channel order and
     * normalization.
     */
    DevAmbiLayout mAmbiLayout{DevAmbiLayout::Default};
    DevAmbiScaling mAmbiScale{DevAmbiScaling::Default};

    std::string DeviceName;

    // Device flags
    std::bitset<DeviceFlagsCount> Flags{};

    uint NumAuxSends{};

    /* Rendering mode. */
    RenderMode mRenderMode{RenderMode::Normal};

    /* The average speaker distance as determined by the ambdec configuration,
     * HRTF data set, or the NFC-HOA reference delay. Only used for NFC.
     */
    float AvgSpeakerDist{0.0f};

    /* The default NFC filter. Not used directly, but is pre-initialized with
     * the control distance from AvgSpeakerDist.
     */
    NfcFilter mNFCtrlFilter{};

    uint SamplesDone{0u};
    std::chrono::nanoseconds ClockBase{0};
    std::chrono::nanoseconds FixedLatency{0};

    /* Temp storage used for mixer processing. */
    static constexpr size_t MixerLineSize{BufferLineSize + MaxResamplerPadding +
        UhjDecoder::sFilterDelay};
    static constexpr size_t MixerChannelsMax{16};
    using MixerBufferLine = std::array<float,MixerLineSize>;
    alignas(16) std::array<MixerBufferLine,MixerChannelsMax> mSampleData;

    alignas(16) float ResampledData[BufferLineSize];
    alignas(16) float FilteredData[BufferLineSize];
    union {
        alignas(16) float HrtfSourceData[BufferLineSize + HrtfHistoryLength];
        alignas(16) float NfcSampleData[BufferLineSize];
    };

    /* Persistent storage for HRTF mixing. */
    alignas(16) float2 HrtfAccumData[BufferLineSize + HrirLength];

    /* Mixing buffer used by the Dry mix and Real output. */
    al::vector<FloatBufferLine, 16> MixBuffer;

    /* The "dry" path corresponds to the main output. */
    MixParams Dry;
    uint NumChannelsPerOrder[MaxAmbiOrder+1]{};

    /* "Real" output, which will be written to the device buffer. May alias the
     * dry buffer.
     */
    RealMixParams RealOut;

    /* HRTF state and info */
    std::unique_ptr<DirectHrtfState> mHrtfState;
    al::intrusive_ptr<HrtfStore> mHrtf;
    uint mIrSize{0};

    /* Ambisonic-to-UHJ encoder */
    std::unique_ptr<UhjEncoder> mUhjEncoder;

    /* Ambisonic decoder for speakers */
    std::unique_ptr<BFormatDec> AmbiDecoder;

    /* Stereo-to-binaural filter */
    std::unique_ptr<bs2b> Bs2b;

    using PostProc = void(DeviceBase::*)(const size_t SamplesToDo);
    PostProc PostProcess{nullptr};

    std::unique_ptr<Compressor> Limiter;

    /* Delay buffers used to compensate for speaker distances. */
    std::unique_ptr<DistanceComp> ChannelDelays;

    /* Dithering control. */
    float DitherDepth{0.0f};
    uint DitherSeed{0u};

    /* Running count of the mixer invocations, in 31.1 fixed point. This
     * actually increments *twice* when mixing, first at the start and then at
     * the end, so the bottom bit indicates if the device is currently mixing
     * and the upper bits indicates how many mixes have been done.
     */
    RefCount MixCount{0u};

    // Contexts created on this device
    std::atomic<al::FlexArray<ContextBase*>*> mContexts{nullptr};


    DeviceBase(DeviceType type);
    DeviceBase(const DeviceBase&) = delete;
    DeviceBase& operator=(const DeviceBase&) = delete;
    ~DeviceBase();

    uint bytesFromFmt() const noexcept { return BytesFromDevFmt(FmtType); }
    uint channelsFromFmt() const noexcept { return ChannelsFromDevFmt(FmtChans, mAmbiOrder); }
    uint frameSizeFromFmt() const noexcept { return bytesFromFmt() * channelsFromFmt(); }

    uint waitForMix() const noexcept
    {
        uint refcount;
        while((refcount=MixCount.load(std::memory_order_acquire))&1) {
        }
        return refcount;
    }

    void ProcessHrtf(const size_t SamplesToDo);
    void ProcessAmbiDec(const size_t SamplesToDo);
    void ProcessAmbiDecStablized(const size_t SamplesToDo);
    void ProcessUhj(const size_t SamplesToDo);
    void ProcessBs2b(const size_t SamplesToDo);

    inline void postProcess(const size_t SamplesToDo)
    { if LIKELY(PostProcess) (this->*PostProcess)(SamplesToDo); }

    void renderSamples(const al::span<float*> outBuffers, const uint numSamples);
    void renderSamples(void *outBuffer, const uint numSamples, const size_t frameStep);

    /* Caller must lock the device state, and the mixer must not be running. */
#ifdef __USE_MINGW_ANSI_STDIO
    [[gnu::format(gnu_printf,2,3)]]
#else
    [[gnu::format(printf,2,3)]]
#endif
    void handleDisconnect(const char *msg, ...);

    DISABLE_ALLOC()

private:
    uint renderSamples(const uint numSamples);
};


/* Must be less than 15 characters (16 including terminating null) for
 * compatibility with pthread_setname_np limitations. */
#define MIXER_THREAD_NAME "alsoft-mixer"

#define RECORD_THREAD_NAME "alsoft-record"


/**
 * Returns the index for the given channel name (e.g. FrontCenter), or
 * INVALID_CHANNEL_INDEX if it doesn't exist.
 */
inline uint GetChannelIdxByName(const RealMixParams &real, Channel chan) noexcept
{ return real.ChannelIndex[chan]; }
#define INVALID_CHANNEL_INDEX ~0u

#endif /* CORE_DEVICE_H */
