#ifndef CORE_MASTERING_H
#define CORE_MASTERING_H

#include <memory>

#include "almalloc.h"
#include "bufferline.h"

struct SlidingHold;

using uint = unsigned int;


/* General topology and basic automation was based on the following paper:
 *
 *   D. Giannoulis, M. Massberg and J. D. Reiss,
 *   "Parameter Automation in a Dynamic Range Compressor,"
 *   Journal of the Audio Engineering Society, v61 (10), Oct. 2013
 *
 * Available (along with supplemental reading) at:
 *
 *   http://c4dm.eecs.qmul.ac.uk/audioengineering/compressors/
 */
struct Compressor {
    size_t mNumChans{0u};

    struct {
        bool Knee : 1;
        bool Attack : 1;
        bool Release : 1;
        bool PostGain : 1;
        bool Declip : 1;
    } mAuto{};

    uint mLookAhead{0};

    float mPreGain{0.0f};
    float mPostGain{0.0f};

    float mThreshold{0.0f};
    float mSlope{0.0f};
    float mKnee{0.0f};

    float mAttack{0.0f};
    float mRelease{0.0f};

    alignas(16) float mSideChain[2*BufferLineSize]{};
    alignas(16) float mCrestFactor[BufferLineSize]{};

    SlidingHold *mHold{nullptr};
    FloatBufferLine *mDelay{nullptr};

    float mCrestCoeff{0.0f};
    float mGainEstimate{0.0f};
    float mAdaptCoeff{0.0f};

    float mLastPeakSq{0.0f};
    float mLastRmsSq{0.0f};
    float mLastRelease{0.0f};
    float mLastAttack{0.0f};
    float mLastGainDev{0.0f};


    ~Compressor();
    void process(const uint SamplesToDo, FloatBufferLine *OutBuffer);
    int getLookAhead() const noexcept { return static_cast<int>(mLookAhead); }

    DEF_PLACE_NEWDEL()

    /**
     * The compressor is initialized with the following settings:
     *
     * \param NumChans      Number of channels to process.
     * \param SampleRate    Sample rate to process.
     * \param AutoKnee      Whether to automate the knee width parameter.
     * \param AutoAttack    Whether to automate the attack time parameter.
     * \param AutoRelease   Whether to automate the release time parameter.
     * \param AutoPostGain  Whether to automate the make-up (post) gain
     *        parameter.
     * \param AutoDeclip    Whether to automate clipping reduction. Ignored
     *        when not automating make-up gain.
     * \param LookAheadTime Look-ahead time (in seconds).
     * \param HoldTime      Peak hold-time (in seconds).
     * \param PreGainDb     Gain applied before detection (in dB).
     * \param PostGainDb    Make-up gain applied after compression (in dB).
     * \param ThresholdDb   Triggering threshold (in dB).
     * \param Ratio         Compression ratio (x:1). Set to INFINIFTY for true
     *        limiting. Ignored when automating knee width.
     * \param KneeDb        Knee width (in dB). Ignored when automating knee
     *        width.
     * \param AttackTime    Attack time (in seconds). Acts as a maximum when
     *        automating attack time.
     * \param ReleaseTime   Release time (in seconds). Acts as a maximum when
     *        automating release time.
     */
    static std::unique_ptr<Compressor> Create(const size_t NumChans, const float SampleRate,
        const bool AutoKnee, const bool AutoAttack, const bool AutoRelease,
        const bool AutoPostGain, const bool AutoDeclip, const float LookAheadTime,
        const float HoldTime, const float PreGainDb, const float PostGainDb,
        const float ThresholdDb, const float Ratio, const float KneeDb, const float AttackTime,
        const float ReleaseTime);
};
using CompressorPtr = std::unique_ptr<Compressor>;

#endif /* CORE_MASTERING_H */
