
#include "config.h"

#include "mastering.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <new>

#include "almalloc.h"
#include "alnumeric.h"
#include "alspan.h"
#include "opthelpers.h"


/* These structures assume BufferLineSize is a power of 2. */
static_assert((BufferLineSize & (BufferLineSize-1)) == 0, "BufferLineSize is not a power of 2");

struct SlidingHold {
    alignas(16) float mValues[BufferLineSize];
    uint mExpiries[BufferLineSize];
    uint mLowerIndex;
    uint mUpperIndex;
    uint mLength;
};


namespace {

using namespace std::placeholders;

/* This sliding hold follows the input level with an instant attack and a
 * fixed duration hold before an instant release to the next highest level.
 * It is a sliding window maximum (descending maxima) implementation based on
 * Richard Harter's ascending minima algorithm available at:
 *
 *   http://www.richardhartersworld.com/cri/2001/slidingmin.html
 */
float UpdateSlidingHold(SlidingHold *Hold, const uint i, const float in)
{
    static constexpr uint mask{BufferLineSize - 1};
    const uint length{Hold->mLength};
    float (&values)[BufferLineSize] = Hold->mValues;
    uint (&expiries)[BufferLineSize] = Hold->mExpiries;
    uint lowerIndex{Hold->mLowerIndex};
    uint upperIndex{Hold->mUpperIndex};

    if(i >= expiries[upperIndex])
        upperIndex = (upperIndex + 1) & mask;

    if(in >= values[upperIndex])
    {
        values[upperIndex] = in;
        expiries[upperIndex] = i + length;
        lowerIndex = upperIndex;
    }
    else
    {
        do {
            do {
                if(!(in >= values[lowerIndex]))
                    goto found_place;
            } while(lowerIndex--);
            lowerIndex = mask;
        } while(1);
    found_place:

        lowerIndex = (lowerIndex + 1) & mask;
        values[lowerIndex] = in;
        expiries[lowerIndex] = i + length;
    }

    Hold->mLowerIndex = lowerIndex;
    Hold->mUpperIndex = upperIndex;

    return values[upperIndex];
}

void ShiftSlidingHold(SlidingHold *Hold, const uint n)
{
    auto exp_begin = std::begin(Hold->mExpiries) + Hold->mUpperIndex;
    auto exp_last = std::begin(Hold->mExpiries) + Hold->mLowerIndex;
    if(exp_last-exp_begin < 0)
    {
        std::transform(exp_begin, std::end(Hold->mExpiries), exp_begin,
            std::bind(std::minus<>{}, _1, n));
        exp_begin = std::begin(Hold->mExpiries);
    }
    std::transform(exp_begin, exp_last+1, exp_begin, std::bind(std::minus<>{}, _1, n));
}


/* Multichannel compression is linked via the absolute maximum of all
 * channels.
 */
void LinkChannels(Compressor *Comp, const uint SamplesToDo, const FloatBufferLine *OutBuffer)
{
    const size_t numChans{Comp->mNumChans};

    ASSUME(SamplesToDo > 0);
    ASSUME(numChans > 0);

    auto side_begin = std::begin(Comp->mSideChain) + Comp->mLookAhead;
    std::fill(side_begin, side_begin+SamplesToDo, 0.0f);

    auto fill_max = [SamplesToDo,side_begin](const FloatBufferLine &input) -> void
    {
        const float *RESTRICT buffer{al::assume_aligned<16>(input.data())};
        auto max_abs = std::bind(maxf, _1, std::bind(static_cast<float(&)(float)>(std::fabs), _2));
        std::transform(side_begin, side_begin+SamplesToDo, buffer, side_begin, max_abs);
    };
    std::for_each(OutBuffer, OutBuffer+numChans, fill_max);
}

/* This calculates the squared crest factor of the control signal for the
 * basic automation of the attack/release times.  As suggested by the paper,
 * it uses an instantaneous squared peak detector and a squared RMS detector
 * both with 200ms release times.
 */
static void CrestDetector(Compressor *Comp, const uint SamplesToDo)
{
    const float a_crest{Comp->mCrestCoeff};
    float y2_peak{Comp->mLastPeakSq};
    float y2_rms{Comp->mLastRmsSq};

    ASSUME(SamplesToDo > 0);

    auto calc_crest = [&y2_rms,&y2_peak,a_crest](const float x_abs) noexcept -> float
    {
        const float x2{clampf(x_abs * x_abs, 0.000001f, 1000000.0f)};

        y2_peak = maxf(x2, lerpf(x2, y2_peak, a_crest));
        y2_rms = lerpf(x2, y2_rms, a_crest);
        return y2_peak / y2_rms;
    };
    auto side_begin = std::begin(Comp->mSideChain) + Comp->mLookAhead;
    std::transform(side_begin, side_begin+SamplesToDo, std::begin(Comp->mCrestFactor), calc_crest);

    Comp->mLastPeakSq = y2_peak;
    Comp->mLastRmsSq = y2_rms;
}

/* The side-chain starts with a simple peak detector (based on the absolute
 * value of the incoming signal) and performs most of its operations in the
 * log domain.
 */
void PeakDetector(Compressor *Comp, const uint SamplesToDo)
{
    ASSUME(SamplesToDo > 0);

    /* Clamp the minimum amplitude to near-zero and convert to logarithm. */
    auto side_begin = std::begin(Comp->mSideChain) + Comp->mLookAhead;
    std::transform(side_begin, side_begin+SamplesToDo, side_begin,
        [](const float s) -> float { return std::log(maxf(0.000001f, s)); });
}

/* An optional hold can be used to extend the peak detector so it can more
 * solidly detect fast transients.  This is best used when operating as a
 * limiter.
 */
void PeakHoldDetector(Compressor *Comp, const uint SamplesToDo)
{
    ASSUME(SamplesToDo > 0);

    SlidingHold *hold{Comp->mHold};
    uint i{0};
    auto detect_peak = [&i,hold](const float x_abs) -> float
    {
        const float x_G{std::log(maxf(0.000001f, x_abs))};
        return UpdateSlidingHold(hold, i++, x_G);
    };
    auto side_begin = std::begin(Comp->mSideChain) + Comp->mLookAhead;
    std::transform(side_begin, side_begin+SamplesToDo, side_begin, detect_peak);

    ShiftSlidingHold(hold, SamplesToDo);
}

/* This is the heart of the feed-forward compressor.  It operates in the log
 * domain (to better match human hearing) and can apply some basic automation
 * to knee width, attack/release times, make-up/post gain, and clipping
 * reduction.
 */
void GainCompressor(Compressor *Comp, const uint SamplesToDo)
{
    const bool autoKnee{Comp->mAuto.Knee};
    const bool autoAttack{Comp->mAuto.Attack};
    const bool autoRelease{Comp->mAuto.Release};
    const bool autoPostGain{Comp->mAuto.PostGain};
    const bool autoDeclip{Comp->mAuto.Declip};
    const uint lookAhead{Comp->mLookAhead};
    const float threshold{Comp->mThreshold};
    const float slope{Comp->mSlope};
    const float attack{Comp->mAttack};
    const float release{Comp->mRelease};
    const float c_est{Comp->mGainEstimate};
    const float a_adp{Comp->mAdaptCoeff};
    const float *crestFactor{Comp->mCrestFactor};
    float postGain{Comp->mPostGain};
    float knee{Comp->mKnee};
    float t_att{attack};
    float t_rel{release - attack};
    float a_att{std::exp(-1.0f / t_att)};
    float a_rel{std::exp(-1.0f / t_rel)};
    float y_1{Comp->mLastRelease};
    float y_L{Comp->mLastAttack};
    float c_dev{Comp->mLastGainDev};

    ASSUME(SamplesToDo > 0);

    for(float &sideChain : al::span<float>{Comp->mSideChain, SamplesToDo})
    {
        if(autoKnee)
            knee = maxf(0.0f, 2.5f * (c_dev + c_est));
        const float knee_h{0.5f * knee};

        /* This is the gain computer.  It applies a static compression curve
         * to the control signal.
         */
        const float x_over{std::addressof(sideChain)[lookAhead] - threshold};
        const float y_G{
            (x_over <= -knee_h) ? 0.0f :
            (std::fabs(x_over) < knee_h) ? (x_over + knee_h) * (x_over + knee_h) / (2.0f * knee) :
            x_over};

        const float y2_crest{*(crestFactor++)};
        if(autoAttack)
        {
            t_att = 2.0f*attack/y2_crest;
            a_att = std::exp(-1.0f / t_att);
        }
        if(autoRelease)
        {
            t_rel = 2.0f*release/y2_crest - t_att;
            a_rel = std::exp(-1.0f / t_rel);
        }

        /* Gain smoothing (ballistics) is done via a smooth decoupled peak
         * detector.  The attack time is subtracted from the release time
         * above to compensate for the chained operating mode.
         */
        const float x_L{-slope * y_G};
        y_1 = maxf(x_L, lerpf(x_L, y_1, a_rel));
        y_L = lerpf(y_1, y_L, a_att);

        /* Knee width and make-up gain automation make use of a smoothed
         * measurement of deviation between the control signal and estimate.
         * The estimate is also used to bias the measurement to hot-start its
         * average.
         */
        c_dev = lerpf(-(y_L+c_est), c_dev, a_adp);

        if(autoPostGain)
        {
            /* Clipping reduction is only viable when make-up gain is being
             * automated. It modifies the deviation to further attenuate the
             * control signal when clipping is detected. The adaptation time
             * is sufficiently long enough to suppress further clipping at the
             * same output level.
             */
            if(autoDeclip)
                c_dev = maxf(c_dev, sideChain - y_L - threshold - c_est);

            postGain = -(c_dev + c_est);
        }

        sideChain = std::exp(postGain - y_L);
    }

    Comp->mLastRelease = y_1;
    Comp->mLastAttack = y_L;
    Comp->mLastGainDev = c_dev;
}

/* Combined with the hold time, a look-ahead delay can improve handling of
 * fast transients by allowing the envelope time to converge prior to
 * reaching the offending impulse.  This is best used when operating as a
 * limiter.
 */
void SignalDelay(Compressor *Comp, const uint SamplesToDo, FloatBufferLine *OutBuffer)
{
    const size_t numChans{Comp->mNumChans};
    const uint lookAhead{Comp->mLookAhead};

    ASSUME(SamplesToDo > 0);
    ASSUME(numChans > 0);
    ASSUME(lookAhead > 0);

    for(size_t c{0};c < numChans;c++)
    {
        float *inout{al::assume_aligned<16>(OutBuffer[c].data())};
        float *delaybuf{al::assume_aligned<16>(Comp->mDelay[c].data())};

        auto inout_end = inout + SamplesToDo;
        if LIKELY(SamplesToDo >= lookAhead)
        {
            auto delay_end = std::rotate(inout, inout_end - lookAhead, inout_end);
            std::swap_ranges(inout, delay_end, delaybuf);
        }
        else
        {
            auto delay_start = std::swap_ranges(inout, inout_end, delaybuf);
            std::rotate(delaybuf, delay_start, delaybuf + lookAhead);
        }
    }
}

} // namespace


std::unique_ptr<Compressor> Compressor::Create(const size_t NumChans, const float SampleRate,
    const bool AutoKnee, const bool AutoAttack, const bool AutoRelease, const bool AutoPostGain,
    const bool AutoDeclip, const float LookAheadTime, const float HoldTime, const float PreGainDb,
    const float PostGainDb, const float ThresholdDb, const float Ratio, const float KneeDb,
    const float AttackTime, const float ReleaseTime)
{
    const auto lookAhead = static_cast<uint>(
        clampf(std::round(LookAheadTime*SampleRate), 0.0f, BufferLineSize-1));
    const auto hold = static_cast<uint>(
        clampf(std::round(HoldTime*SampleRate), 0.0f, BufferLineSize-1));

    size_t size{sizeof(Compressor)};
    if(lookAhead > 0)
    {
        size += sizeof(*Compressor::mDelay) * NumChans;
        /* The sliding hold implementation doesn't handle a length of 1. A 1-
         * sample hold is useless anyway, it would only ever give back what was
         * just given to it.
         */
        if(hold > 1)
            size += sizeof(*Compressor::mHold);
    }

    auto Comp = CompressorPtr{al::construct_at(static_cast<Compressor*>(al_calloc(16, size)))};
    Comp->mNumChans = NumChans;
    Comp->mAuto.Knee = AutoKnee;
    Comp->mAuto.Attack = AutoAttack;
    Comp->mAuto.Release = AutoRelease;
    Comp->mAuto.PostGain = AutoPostGain;
    Comp->mAuto.Declip = AutoPostGain && AutoDeclip;
    Comp->mLookAhead = lookAhead;
    Comp->mPreGain = std::pow(10.0f, PreGainDb / 20.0f);
    Comp->mPostGain = PostGainDb * std::log(10.0f) / 20.0f;
    Comp->mThreshold = ThresholdDb * std::log(10.0f) / 20.0f;
    Comp->mSlope = 1.0f / maxf(1.0f, Ratio) - 1.0f;
    Comp->mKnee = maxf(0.0f, KneeDb * std::log(10.0f) / 20.0f);
    Comp->mAttack = maxf(1.0f, AttackTime * SampleRate);
    Comp->mRelease = maxf(1.0f, ReleaseTime * SampleRate);

    /* Knee width automation actually treats the compressor as a limiter. By
     * varying the knee width, it can effectively be seen as applying
     * compression over a wide range of ratios.
     */
    if(AutoKnee)
        Comp->mSlope = -1.0f;

    if(lookAhead > 0)
    {
        if(hold > 1)
        {
            Comp->mHold = al::construct_at(reinterpret_cast<SlidingHold*>(Comp.get() + 1));
            Comp->mHold->mValues[0] = -std::numeric_limits<float>::infinity();
            Comp->mHold->mExpiries[0] = hold;
            Comp->mHold->mLength = hold;
            Comp->mDelay = reinterpret_cast<FloatBufferLine*>(Comp->mHold + 1);
        }
        else
            Comp->mDelay = reinterpret_cast<FloatBufferLine*>(Comp.get() + 1);
        std::uninitialized_fill_n(Comp->mDelay, NumChans, FloatBufferLine{});
    }

    Comp->mCrestCoeff = std::exp(-1.0f / (0.200f * SampleRate)); // 200ms
    Comp->mGainEstimate = Comp->mThreshold * -0.5f * Comp->mSlope;
    Comp->mAdaptCoeff = std::exp(-1.0f / (2.0f * SampleRate)); // 2s

    return Comp;
}

Compressor::~Compressor()
{
    if(mHold)
        al::destroy_at(mHold);
    mHold = nullptr;
    if(mDelay)
        al::destroy_n(mDelay, mNumChans);
    mDelay = nullptr;
}


void Compressor::process(const uint SamplesToDo, FloatBufferLine *OutBuffer)
{
    const size_t numChans{mNumChans};

    ASSUME(SamplesToDo > 0);
    ASSUME(numChans > 0);

    const float preGain{mPreGain};
    if(preGain != 1.0f)
    {
        auto apply_gain = [SamplesToDo,preGain](FloatBufferLine &input) noexcept -> void
        {
            float *buffer{al::assume_aligned<16>(input.data())};
            std::transform(buffer, buffer+SamplesToDo, buffer,
                std::bind(std::multiplies<float>{}, _1, preGain));
        };
        std::for_each(OutBuffer, OutBuffer+numChans, apply_gain);
    }

    LinkChannels(this, SamplesToDo, OutBuffer);

    if(mAuto.Attack || mAuto.Release)
        CrestDetector(this, SamplesToDo);

    if(mHold)
        PeakHoldDetector(this, SamplesToDo);
    else
        PeakDetector(this, SamplesToDo);

    GainCompressor(this, SamplesToDo);

    if(mDelay)
        SignalDelay(this, SamplesToDo, OutBuffer);

    const float (&sideChain)[BufferLineSize*2] = mSideChain;
    auto apply_comp = [SamplesToDo,&sideChain](FloatBufferLine &input) noexcept -> void
    {
        float *buffer{al::assume_aligned<16>(input.data())};
        const float *gains{al::assume_aligned<16>(&sideChain[0])};
        std::transform(gains, gains+SamplesToDo, buffer, buffer,
            std::bind(std::multiplies<float>{}, _1, _2));
    };
    std::for_each(OutBuffer, OutBuffer+numChans, apply_comp);

    auto side_begin = std::begin(mSideChain) + SamplesToDo;
    std::copy(side_begin, side_begin+mLookAhead, std::begin(mSideChain));
}
