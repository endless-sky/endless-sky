/**
 * OpenAL cross platform audio library
 * Copyright (C) 2018 by Raul Herraiz.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <iterator>

#include "alc/effects/base.h"
#include "alcomplex.h"
#include "almalloc.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "alspan.h"
#include "core/bufferline.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/mixer.h"
#include "core/mixer/defs.h"
#include "intrusive_ptr.h"

struct ContextBase;


namespace {

using uint = unsigned int;
using complex_d = std::complex<double>;

#define STFT_SIZE      1024
#define STFT_HALF_SIZE (STFT_SIZE>>1)
#define OVERSAMP       (1<<2)

#define STFT_STEP    (STFT_SIZE / OVERSAMP)
#define FIFO_LATENCY (STFT_STEP * (OVERSAMP-1))

/* Define a Hann window, used to filter the STFT input and output. */
std::array<double,STFT_SIZE> InitHannWindow()
{
    std::array<double,STFT_SIZE> ret;
    /* Create lookup table of the Hann window for the desired size, i.e. STFT_SIZE */
    for(size_t i{0};i < STFT_SIZE>>1;i++)
    {
        constexpr double scale{al::numbers::pi / double{STFT_SIZE}};
        const double val{std::sin(static_cast<double>(i+1) * scale)};
        ret[i] = ret[STFT_SIZE-1-i] = val * val;
    }
    return ret;
}
alignas(16) const std::array<double,STFT_SIZE> HannWindow = InitHannWindow();


struct FrequencyBin {
    double Amplitude;
    double FreqBin;
};


struct PshifterState final : public EffectState {
    /* Effect parameters */
    size_t mCount;
    size_t mPos;
    uint mPitchShiftI;
    double mPitchShift;

    /* Effects buffers */
    std::array<double,STFT_SIZE> mFIFO;
    std::array<double,STFT_HALF_SIZE+1> mLastPhase;
    std::array<double,STFT_HALF_SIZE+1> mSumPhase;
    std::array<double,STFT_SIZE> mOutputAccum;

    std::array<complex_d,STFT_SIZE> mFftBuffer;

    std::array<FrequencyBin,STFT_HALF_SIZE+1> mAnalysisBuffer;
    std::array<FrequencyBin,STFT_HALF_SIZE+1> mSynthesisBuffer;

    alignas(16) FloatBufferLine mBufferOut;

    /* Effect gains for each output channel */
    float mCurrentGains[MAX_OUTPUT_CHANNELS];
    float mTargetGains[MAX_OUTPUT_CHANNELS];


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(PshifterState)
};

void PshifterState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    /* (Re-)initializing parameters and clear the buffers. */
    mCount       = 0;
    mPos         = FIFO_LATENCY;
    mPitchShiftI = MixerFracOne;
    mPitchShift  = 1.0;

    std::fill(mFIFO.begin(),            mFIFO.end(),            0.0);
    std::fill(mLastPhase.begin(),       mLastPhase.end(),       0.0);
    std::fill(mSumPhase.begin(),        mSumPhase.end(),        0.0);
    std::fill(mOutputAccum.begin(),     mOutputAccum.end(),     0.0);
    std::fill(mFftBuffer.begin(),       mFftBuffer.end(),       complex_d{});
    std::fill(mAnalysisBuffer.begin(),  mAnalysisBuffer.end(),  FrequencyBin{});
    std::fill(mSynthesisBuffer.begin(), mSynthesisBuffer.end(), FrequencyBin{});

    std::fill(std::begin(mCurrentGains), std::end(mCurrentGains), 0.0f);
    std::fill(std::begin(mTargetGains),  std::end(mTargetGains),  0.0f);
}

void PshifterState::update(const ContextBase*, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    const int tune{props->Pshifter.CoarseTune*100 + props->Pshifter.FineTune};
    const float pitch{std::pow(2.0f, static_cast<float>(tune) / 1200.0f)};
    mPitchShiftI = fastf2u(pitch*MixerFracOne);
    mPitchShift  = mPitchShiftI * double{1.0/MixerFracOne};

    const auto coeffs = CalcDirectionCoeffs({0.0f, 0.0f, -1.0f}, 0.0f);

    mOutTarget = target.Main->Buffer;
    ComputePanGains(target.Main, coeffs.data(), slot->Gain, mTargetGains);
}

void PshifterState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    /* Pitch shifter engine based on the work of Stephan Bernsee.
     * http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
     */

    /* Cycle offset per update expected of each frequency bin (bin 0 is none,
     * bin 1 is x1, bin 2 is x2, etc).
     */
    constexpr double expected_cycles{al::numbers::pi*2.0 / OVERSAMP};

    for(size_t base{0u};base < samplesToDo;)
    {
        const size_t todo{minz(STFT_STEP-mCount, samplesToDo-base)};

        /* Retrieve the output samples from the FIFO and fill in the new input
         * samples.
         */
        auto fifo_iter = mFIFO.begin()+mPos + mCount;
        std::transform(fifo_iter, fifo_iter+todo, mBufferOut.begin()+base,
            [](double d) noexcept -> float { return static_cast<float>(d); });

        std::copy_n(samplesIn[0].begin()+base, todo, fifo_iter);
        mCount += todo;
        base += todo;

        /* Check whether FIFO buffer is filled with new samples. */
        if(mCount < STFT_STEP) break;
        mCount = 0;
        mPos = (mPos+STFT_STEP) & (mFIFO.size()-1);

        /* Time-domain signal windowing, store in FftBuffer, and apply a
         * forward FFT to get the frequency-domain signal.
         */
        for(size_t src{mPos}, k{0u};src < STFT_SIZE;++src,++k)
            mFftBuffer[k] = mFIFO[src] * HannWindow[k];
        for(size_t src{0u}, k{STFT_SIZE-mPos};src < mPos;++src,++k)
            mFftBuffer[k] = mFIFO[src] * HannWindow[k];
        forward_fft(mFftBuffer);

        /* Analyze the obtained data. Since the real FFT is symmetric, only
         * STFT_HALF_SIZE+1 samples are needed.
         */
        for(size_t k{0u};k < STFT_HALF_SIZE+1;k++)
        {
            const double amplitude{std::abs(mFftBuffer[k])};
            const double phase{std::arg(mFftBuffer[k])};

            /* Compute phase difference and subtract expected phase difference */
            double tmp{(phase - mLastPhase[k]) - static_cast<double>(k)*expected_cycles};

            /* Map delta phase into +/- Pi interval */
            int qpd{double2int(tmp / al::numbers::pi)};
            tmp -= al::numbers::pi * (qpd + (qpd%2));

            /* Get deviation from bin frequency from the +/- Pi interval */
            tmp /= expected_cycles;

            /* Compute the k-th partials' true frequency and store the
             * amplitude and frequency bin in the analysis buffer.
             */
            mAnalysisBuffer[k].Amplitude = amplitude;
            mAnalysisBuffer[k].FreqBin = static_cast<double>(k) + tmp;

            /* Store the actual phase[k] for the next frame. */
            mLastPhase[k] = phase;
        }

        /* Shift the frequency bins according to the pitch adjustment,
         * accumulating the amplitudes of overlapping frequency bins.
         */
        std::fill(mSynthesisBuffer.begin(), mSynthesisBuffer.end(), FrequencyBin{});
        const size_t bin_count{minz(STFT_HALF_SIZE+1,
            (((STFT_HALF_SIZE+1)<<MixerFracBits) - (MixerFracOne>>1) - 1)/mPitchShiftI + 1)};
        for(size_t k{0u};k < bin_count;k++)
        {
            const size_t j{(k*mPitchShiftI + (MixerFracOne>>1)) >> MixerFracBits};
            mSynthesisBuffer[j].Amplitude += mAnalysisBuffer[k].Amplitude;
            mSynthesisBuffer[j].FreqBin    = mAnalysisBuffer[k].FreqBin * mPitchShift;
        }

        /* Reconstruct the frequency-domain signal from the adjusted frequency
         * bins.
         */
        for(size_t k{0u};k < STFT_HALF_SIZE+1;k++)
        {
            /* Calculate actual delta phase and accumulate it to get bin phase */
            mSumPhase[k] += mSynthesisBuffer[k].FreqBin * expected_cycles;

            mFftBuffer[k] = std::polar(mSynthesisBuffer[k].Amplitude, mSumPhase[k]);
        }
        for(size_t k{STFT_HALF_SIZE+1};k < STFT_SIZE;++k)
            mFftBuffer[k] = std::conj(mFftBuffer[STFT_SIZE-k]);

        /* Apply an inverse FFT to get the time-domain siganl, and accumulate
         * for the output with windowing.
         */
        inverse_fft(mFftBuffer);
        for(size_t dst{mPos}, k{0u};dst < STFT_SIZE;++dst,++k)
            mOutputAccum[dst] += HannWindow[k]*mFftBuffer[k].real() * (4.0/OVERSAMP/STFT_SIZE);
        for(size_t dst{0u}, k{STFT_SIZE-mPos};dst < mPos;++dst,++k)
            mOutputAccum[dst] += HannWindow[k]*mFftBuffer[k].real() * (4.0/OVERSAMP/STFT_SIZE);

        /* Copy out the accumulated result, then clear for the next iteration. */
        std::copy_n(mOutputAccum.begin() + mPos, STFT_STEP, mFIFO.begin() + mPos);
        std::fill_n(mOutputAccum.begin() + mPos, STFT_STEP, 0.0);
    }

    /* Now, mix the processed sound data to the output. */
    MixSamples({mBufferOut.data(), samplesToDo}, samplesOut, mCurrentGains, mTargetGains,
        maxz(samplesToDo, 512), 0);
}


struct PshifterStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new PshifterState{}}; }
};

} // namespace

EffectStateFactory *PshifterStateFactory_getFactory()
{
    static PshifterStateFactory PshifterFactory{};
    return &PshifterFactory;
}
