
#include "config.h"

#include "uhjfilter.h"

#include <algorithm>
#include <iterator>

#include "alcomplex.h"
#include "alnumeric.h"
#include "opthelpers.h"
#include "phase_shifter.h"


namespace {

const PhaseShifterT<UhjFilterBase::sFilterDelay*2> PShift{};

} // namespace


/* Encoding UHJ from B-Format is done as:
 *
 * S = 0.9396926*W + 0.1855740*X
 * D = j(-0.3420201*W + 0.5098604*X) + 0.6554516*Y
 *
 * Left = (S + D)/2.0
 * Right = (S - D)/2.0
 * T = j(-0.1432*W + 0.6512*X) - 0.7071068*Y
 * Q = 0.9772*Z
 *
 * where j is a wide-band +90 degree phase shift. 3-channel UHJ excludes Q,
 * while 2-channel excludes Q and T.
 *
 * The phase shift is done using a linear FIR filter derived from an FFT'd
 * impulse with the desired shift.
 */

void UhjEncoder::encode(float *LeftOut, float *RightOut, const FloatBufferLine *InSamples,
    const size_t SamplesToDo)
{
    ASSUME(SamplesToDo > 0);

    float *RESTRICT left{al::assume_aligned<16>(LeftOut)};
    float *RESTRICT right{al::assume_aligned<16>(RightOut)};

    const float *RESTRICT winput{al::assume_aligned<16>(InSamples[0].data())};
    const float *RESTRICT xinput{al::assume_aligned<16>(InSamples[1].data())};
    const float *RESTRICT yinput{al::assume_aligned<16>(InSamples[2].data())};

    /* Combine the previously delayed S/D signal with the input. Include any
     * existing direct signal with it.
     */

    /* S = 0.9396926*W + 0.1855740*X */
    auto miditer = mS.begin() + sFilterDelay;
    std::transform(winput, winput+SamplesToDo, xinput, miditer,
        [](const float w, const float x) noexcept -> float
        { return 0.9396926f*w + 0.1855740f*x; });
    for(size_t i{0};i < SamplesToDo;++i,++miditer)
        *miditer += left[i] + right[i];

    /* D = 0.6554516*Y */
    auto sideiter = mD.begin() + sFilterDelay;
    std::transform(yinput, yinput+SamplesToDo, sideiter,
        [](const float y) noexcept -> float { return 0.6554516f*y; });
    for(size_t i{0};i < SamplesToDo;++i,++sideiter)
        *sideiter += left[i] - right[i];

    /* D += j(-0.3420201*W + 0.5098604*X) */
    auto tmpiter = std::copy(mWXHistory.cbegin(), mWXHistory.cend(), mTemp.begin());
    std::transform(winput, winput+SamplesToDo, xinput, tmpiter,
        [](const float w, const float x) noexcept -> float
        { return -0.3420201f*w + 0.5098604f*x; });
    std::copy_n(mTemp.cbegin()+SamplesToDo, mWXHistory.size(), mWXHistory.begin());
    PShift.processAccum({mD.data(), SamplesToDo}, mTemp.data());

    /* Left = (S + D)/2.0 */
    for(size_t i{0};i < SamplesToDo;i++)
        left[i] = (mS[i] + mD[i]) * 0.5f;
    /* Right = (S - D)/2.0 */
    for(size_t i{0};i < SamplesToDo;i++)
        right[i] = (mS[i] - mD[i]) * 0.5f;

    /* Copy the future samples to the front for next time. */
    std::copy(mS.cbegin()+SamplesToDo, mS.cbegin()+SamplesToDo+sFilterDelay, mS.begin());
    std::copy(mD.cbegin()+SamplesToDo, mD.cbegin()+SamplesToDo+sFilterDelay, mD.begin());
}


/* Decoding UHJ is done as:
 *
 * S = Left + Right
 * D = Left - Right
 *
 * W = 0.981532*S + 0.197484*j(0.828331*D + 0.767820*T)
 * X = 0.418496*S - j(0.828331*D + 0.767820*T)
 * Y = 0.795968*D - 0.676392*T + j(0.186633*S)
 * Z = 1.023332*Q
 *
 * where j is a +90 degree phase shift. 3-channel UHJ excludes Q, while 2-
 * channel excludes Q and T.
 */
void UhjDecoder::decode(const al::span<float*> samples, const size_t samplesToDo,
    const size_t forwardSamples)
{
    ASSUME(samplesToDo > 0);

    {
        const float *RESTRICT left{al::assume_aligned<16>(samples[0])};
        const float *RESTRICT right{al::assume_aligned<16>(samples[1])};
        const float *RESTRICT t{al::assume_aligned<16>(samples[2])};

        /* S = Left + Right */
        for(size_t i{0};i < samplesToDo+sFilterDelay;++i)
            mS[i] = left[i] + right[i];

        /* D = Left - Right */
        for(size_t i{0};i < samplesToDo+sFilterDelay;++i)
            mD[i] = left[i] - right[i];

        /* T */
        for(size_t i{0};i < samplesToDo+sFilterDelay;++i)
            mT[i] = t[i];
    }

    float *RESTRICT woutput{al::assume_aligned<16>(samples[0])};
    float *RESTRICT xoutput{al::assume_aligned<16>(samples[1])};
    float *RESTRICT youtput{al::assume_aligned<16>(samples[2])};

    /* Precompute j(0.828331*D + 0.767820*T) and store in xoutput. */
    auto tmpiter = std::copy(mDTHistory.cbegin(), mDTHistory.cend(), mTemp.begin());
    std::transform(mD.cbegin(), mD.cbegin()+samplesToDo+sFilterDelay, mT.cbegin(), tmpiter,
        [](const float d, const float t) noexcept { return 0.828331f*d + 0.767820f*t; });
    std::copy_n(mTemp.cbegin()+forwardSamples, mDTHistory.size(), mDTHistory.begin());
    PShift.process({xoutput, samplesToDo}, mTemp.data());

    /* W = 0.981532*S + 0.197484*j(0.828331*D + 0.767820*T) */
    for(size_t i{0};i < samplesToDo;++i)
        woutput[i] = 0.981532f*mS[i] + 0.197484f*xoutput[i];
    /* X = 0.418496*S - j(0.828331*D + 0.767820*T) */
    for(size_t i{0};i < samplesToDo;++i)
        xoutput[i] = 0.418496f*mS[i] - xoutput[i];

    /* Precompute j*S and store in youtput. */
    tmpiter = std::copy(mSHistory.cbegin(), mSHistory.cend(), mTemp.begin());
    std::copy_n(mS.cbegin(), samplesToDo+sFilterDelay, tmpiter);
    std::copy_n(mTemp.cbegin()+forwardSamples, mSHistory.size(), mSHistory.begin());
    PShift.process({youtput, samplesToDo}, mTemp.data());

    /* Y = 0.795968*D - 0.676392*T + j(0.186633*S) */
    for(size_t i{0};i < samplesToDo;++i)
        youtput[i] = 0.795968f*mD[i] - 0.676392f*mT[i] + 0.186633f*youtput[i];

    if(samples.size() > 3)
    {
        float *RESTRICT zoutput{al::assume_aligned<16>(samples[3])};
        /* Z = 1.023332*Q */
        for(size_t i{0};i < samplesToDo;++i)
            zoutput[i] = 1.023332f*zoutput[i];
    }
}


/* Super Stereo processing is done as:
 *
 * S = Left + Right
 * D = Left - Right
 *
 * W = 0.6098637*S - 0.6896511*j*w*D
 * X = 0.8624776*S + 0.7626955*j*w*D
 * Y = 1.6822415*w*D - 0.2156194*j*S
 *
 * where j is a +90 degree phase shift. w is a variable control for the
 * resulting stereo width, with the range 0 <= w <= 0.7.
 */
void UhjDecoder::decodeStereo(const al::span<float*> samples, const size_t samplesToDo,
    const size_t forwardSamples)
{
    ASSUME(samplesToDo > 0);

    {
        const float *RESTRICT left{al::assume_aligned<16>(samples[0])};
        const float *RESTRICT right{al::assume_aligned<16>(samples[1])};

        for(size_t i{0};i < samplesToDo+sFilterDelay;++i)
            mS[i] = left[i] + right[i];

        /* Pre-apply the width factor to the difference signal D. Smoothly
         * interpolate when it changes.
         */
        const float wtarget{mWidthControl};
        const float wcurrent{unlikely(mCurrentWidth < 0.0f) ? wtarget : mCurrentWidth};
        if(likely(wtarget == wcurrent) || unlikely(forwardSamples == 0))
        {
            for(size_t i{0};i < samplesToDo+sFilterDelay;++i)
                mD[i] = (left[i] - right[i]) * wcurrent;
        }
        else
        {
            const float wstep{(wtarget - wcurrent) / static_cast<float>(forwardSamples)};
            float fi{0.0f};
            size_t i{0};
            for(;i < forwardSamples;++i)
            {
                mD[i] = (left[i] - right[i]) * (wcurrent + wstep*fi);
                fi += 1.0f;
            }
            for(;i < samplesToDo+sFilterDelay;++i)
                mD[i] = (left[i] - right[i]) * wtarget;
            mCurrentWidth = wtarget;
        }
    }

    float *RESTRICT woutput{al::assume_aligned<16>(samples[0])};
    float *RESTRICT xoutput{al::assume_aligned<16>(samples[1])};
    float *RESTRICT youtput{al::assume_aligned<16>(samples[2])};

    /* Precompute j*D and store in xoutput. */
    auto tmpiter = std::copy(mDTHistory.cbegin(), mDTHistory.cend(), mTemp.begin());
    std::copy_n(mD.cbegin(), samplesToDo+sFilterDelay, tmpiter);
    std::copy_n(mTemp.cbegin()+forwardSamples, mDTHistory.size(), mDTHistory.begin());
    PShift.process({xoutput, samplesToDo}, mTemp.data());

    /* W = 0.6098637*S - 0.6896511*j*w*D */
    for(size_t i{0};i < samplesToDo;++i)
        woutput[i] = 0.6098637f*mS[i] - 0.6896511f*xoutput[i];
    /* X = 0.8624776*S + 0.7626955*j*w*D */
    for(size_t i{0};i < samplesToDo;++i)
        xoutput[i] = 0.8624776f*mS[i] + 0.7626955f*xoutput[i];

    /* Precompute j*S and store in youtput. */
    tmpiter = std::copy(mSHistory.cbegin(), mSHistory.cend(), mTemp.begin());
    std::copy_n(mS.cbegin(), samplesToDo+sFilterDelay, tmpiter);
    std::copy_n(mTemp.cbegin()+forwardSamples, mSHistory.size(), mSHistory.begin());
    PShift.process({youtput, samplesToDo}, mTemp.data());

    /* Y = 1.6822415*w*D - 0.2156194*j*S */
    for(size_t i{0};i < samplesToDo;++i)
        youtput[i] = 1.6822415f*mD[i] - 0.2156194f*youtput[i];
}
