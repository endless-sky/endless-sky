#ifndef CORE_CONTEXT_H
#define CORE_CONTEXT_H

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>

#include "almalloc.h"
#include "alspan.h"
#include "atomic.h"
#include "bufferline.h"
#include "threads.h"
#include "vecmat.h"
#include "vector.h"

struct DeviceBase;
struct EffectSlot;
struct EffectSlotProps;
struct RingBuffer;
struct Voice;
struct VoiceChange;
struct VoicePropsItem;

using uint = unsigned int;


constexpr float SpeedOfSoundMetersPerSec{343.3f};

constexpr float AirAbsorbGainHF{0.99426f}; /* -0.05dB */

enum class DistanceModel : unsigned char {
    Disable,
    Inverse, InverseClamped,
    Linear, LinearClamped,
    Exponent, ExponentClamped,

    Default = InverseClamped
};


struct WetBuffer {
    bool mInUse;
    al::FlexArray<FloatBufferLine, 16> mBuffer;

    WetBuffer(size_t count) : mBuffer{count} { }

    DEF_FAM_NEWDEL(WetBuffer, mBuffer)
};
using WetBufferPtr = std::unique_ptr<WetBuffer>;


struct ContextProps {
    std::array<float,3> Position;
    std::array<float,3> Velocity;
    std::array<float,3> OrientAt;
    std::array<float,3> OrientUp;
    float Gain;
    float MetersPerUnit;
    float AirAbsorptionGainHF;

    float DopplerFactor;
    float DopplerVelocity;
    float SpeedOfSound;
    bool SourceDistanceModel;
    DistanceModel mDistanceModel;

    std::atomic<ContextProps*> next;

    DEF_NEWDEL(ContextProps)
};

struct ContextParams {
    /* Pointer to the most recent property values that are awaiting an update. */
    std::atomic<ContextProps*> ContextUpdate{nullptr};

    alu::Vector Position{};
    alu::Matrix Matrix{alu::Matrix::Identity()};
    alu::Vector Velocity{};

    float Gain{1.0f};
    float MetersPerUnit{1.0f};
    float AirAbsorptionGainHF{AirAbsorbGainHF};

    float DopplerFactor{1.0f};
    float SpeedOfSound{SpeedOfSoundMetersPerSec}; /* in units per sec! */

    bool SourceDistanceModel{false};
    DistanceModel mDistanceModel{};
};

struct ContextBase {
    DeviceBase *const mDevice;

    /* Counter for the pre-mixing updates, in 31.1 fixed point (lowest bit
     * indicates if updates are currently happening).
     */
    RefCount mUpdateCount{0u};
    std::atomic<bool> mHoldUpdates{false};
    std::atomic<bool> mStopVoicesOnDisconnect{true};

    float mGainBoost{1.0f};

    /* Linked lists of unused property containers, free to use for future
     * updates.
     */
    std::atomic<ContextProps*> mFreeContextProps{nullptr};
    std::atomic<VoicePropsItem*> mFreeVoiceProps{nullptr};
    std::atomic<EffectSlotProps*> mFreeEffectslotProps{nullptr};

    /* The voice change tail is the beginning of the "free" elements, up to and
     * *excluding* the current. If tail==current, there's no free elements and
     * new ones need to be allocated. The current voice change is the element
     * last processed, and any after are pending.
     */
    VoiceChange *mVoiceChangeTail{};
    std::atomic<VoiceChange*> mCurrentVoiceChange{};

    void allocVoiceChanges();
    void allocVoiceProps();


    ContextParams mParams;

    using VoiceArray = al::FlexArray<Voice*>;
    std::atomic<VoiceArray*> mVoices{};
    std::atomic<size_t> mActiveVoiceCount{};

    void allocVoices(size_t addcount);
    al::span<Voice*> getVoicesSpan() const noexcept
    {
        return {mVoices.load(std::memory_order_relaxed)->data(),
            mActiveVoiceCount.load(std::memory_order_relaxed)};
    }
    al::span<Voice*> getVoicesSpanAcquired() const noexcept
    {
        return {mVoices.load(std::memory_order_acquire)->data(),
            mActiveVoiceCount.load(std::memory_order_acquire)};
    }


    using EffectSlotArray = al::FlexArray<EffectSlot*>;
    std::atomic<EffectSlotArray*> mActiveAuxSlots{nullptr};

    std::thread mEventThread;
    al::semaphore mEventSem;
    std::unique_ptr<RingBuffer> mAsyncEvents;
    std::atomic<uint> mEnabledEvts{0u};

    /* Asynchronous voice change actions are processed as a linked list of
     * VoiceChange objects by the mixer, which is atomically appended to.
     * However, to avoid allocating each object individually, they're allocated
     * in clusters that are stored in a vector for easy automatic cleanup.
     */
    using VoiceChangeCluster = std::unique_ptr<VoiceChange[]>;
    al::vector<VoiceChangeCluster> mVoiceChangeClusters;

    using VoiceCluster = std::unique_ptr<Voice[]>;
    al::vector<VoiceCluster> mVoiceClusters;

    using VoicePropsCluster = std::unique_ptr<VoicePropsItem[]>;
    al::vector<VoicePropsCluster> mVoicePropClusters;


    ContextBase(DeviceBase *device);
    ContextBase(const ContextBase&) = delete;
    ContextBase& operator=(const ContextBase&) = delete;
    ~ContextBase();
};

#endif /* CORE_CONTEXT_H */
