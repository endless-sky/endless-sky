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
#include "core/context.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/mixer.h"
#include "core/mixer/defs.h"
#include "intrusive_ptr.h"


namespace {

using uint = unsigned int;
using complex_d = std::complex<double>;

#define HIL_SIZE 1024
#define OVERSAMP (1<<2)

#define HIL_STEP     (HIL_SIZE / OVERSAMP)
#define FIFO_LATENCY (HIL_STEP * (OVERSAMP-1))

/* Define a Hann window, used to filter the HIL input and output. */
std::array<double,HIL_SIZE> InitHannWindow()
{
    std::array<double,HIL_SIZE> ret;
    /* Create lookup table of the Hann window for the desired size, i.e. HIL_SIZE */
    for(size_t i{0};i < HIL_SIZE>>1;i++)
    {
        constexpr double scale{al::numbers::pi / double{HIL_SIZE}};
        const double val{std::sin(static_cast<double>(i+1) * scale)};
        ret[i] = ret[HIL_SIZE-1-i] = val * val;
    }
    return ret;
}
alignas(16) const std::array<double,HIL_SIZE> HannWindow = InitHannWindow();


struct FshifterState final : public EffectState {
    /* Effect parameters */
    size_t mCount{};
    size_t mPos{};
    uint mPhaseStep[2]{};
    uint mPhase[2]{};
    double mSign[2]{};

    /* Effects buffers */
    double mInFIFO[HIL_SIZE]{};
    complex_d mOutFIFO[HIL_STEP]{};
    complex_d mOutputAccum[HIL_SIZE]{};
    complex_d mAnalytic[HIL_SIZE]{};
    complex_d mOutdata[BufferLineSize]{};

    alignas(16) float mBufferOut[BufferLineSize]{};

    /* Effect gains for each output channel */
    struct {
        float Current[MAX_OUTPUT_CHANNELS]{};
        float Target[MAX_OUTPUT_CHANNELS]{};
    } mGains[2];


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(FshifterState)
};

void FshifterState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    /* (Re-)initializing parameters and clear the buffers. */
    mCount = 0;
    mPos = FIFO_LATENCY;

    std::fill(std::begin(mPhaseStep),   std::end(mPhaseStep),   0u);
    std::fill(std::begin(mPhase),       std::end(mPhase),       0u);
    std::fill(std::begin(mSign),        std::end(mSign),        1.0);
    std::fill(std::begin(mInFIFO),      std::end(mInFIFO),      0.0);
    std::fill(std::begin(mOutFIFO),     std::end(mOutFIFO),     complex_d{});
    std::fill(std::begin(mOutputAccum), std::end(mOutputAccum), complex_d{});
    std::fill(std::begin(mAnalytic),    std::end(mAnalytic),    complex_d{});

    for(auto &gain : mGains)
    {
        std::fill(std::begin(gain.Current), std::end(gain.Current), 0.0f);
        std::fill(std::begin(gain.Target), std::end(gain.Target), 0.0f);
    }
}

void FshifterState::update(const ContextBase *context, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    const DeviceBase *device{context->mDevice};

    const float step{props->Fshifter.Frequency / static_cast<float>(device->Frequency)};
    mPhaseStep[0] = mPhaseStep[1] = fastf2u(minf(step, 1.0f) * MixerFracOne);

    switch(props->Fshifter.LeftDirection)
    {
    case FShifterDirection::Down:
        mSign[0] = -1.0;
        break;
    case FShifterDirection::Up:
        mSign[0] = 1.0;
        break;
    case FShifterDirection::Off:
        mPhase[0]     = 0;
        mPhaseStep[0] = 0;
        break;
    }

    switch(props->Fshifter.RightDirection)
    {
    case FShifterDirection::Down:
        mSign[1] = -1.0;
        break;
    case FShifterDirection::Up:
        mSign[1] = 1.0;
        break;
    case FShifterDirection::Off:
        mPhase[1]     = 0;
        mPhaseStep[1] = 0;
        break;
    }

    const auto lcoeffs = CalcDirectionCoeffs({-1.0f, 0.0f, 0.0f}, 0.0f);
    const auto rcoeffs = CalcDirectionCoeffs({ 1.0f, 0.0f, 0.0f}, 0.0f);

    mOutTarget = target.Main->Buffer;
    ComputePanGains(target.Main, lcoeffs.data(), slot->Gain, mGains[0].Target);
    ComputePanGains(target.Main, rcoeffs.data(), slot->Gain, mGains[1].Target);
}

void FshifterState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    for(size_t base{0u};base < samplesToDo;)
    {
        size_t todo{minz(HIL_STEP-mCount, samplesToDo-base)};

        /* Fill FIFO buffer with samples data */
        const size_t pos{mPos};
        size_t count{mCount};
        do {
            mInFIFO[pos+count] = samplesIn[0][base];
            mOutdata[base] = mOutFIFO[count];
            ++base; ++count;
        } while(--todo);
        mCount = count;

        /* Check whether FIFO buffer is filled */
        if(mCount < HIL_STEP) break;
        mCount = 0;
        mPos = (mPos+HIL_STEP) & (HIL_SIZE-1);

        /* Real signal windowing and store in Analytic buffer */
        for(size_t src{mPos}, k{0u};src < HIL_SIZE;++src,++k)
            mAnalytic[k] = mInFIFO[src]*HannWindow[k];
        for(size_t src{0u}, k{HIL_SIZE-mPos};src < mPos;++src,++k)
            mAnalytic[k] = mInFIFO[src]*HannWindow[k];

        /* Processing signal by Discrete Hilbert Transform (analytical signal). */
        complex_hilbert(mAnalytic);

        /* Windowing and add to output accumulator */
        for(size_t dst{mPos}, k{0u};dst < HIL_SIZE;++dst,++k)
            mOutputAccum[dst] += 2.0/OVERSAMP*HannWindow[k]*mAnalytic[k];
        for(size_t dst{0u}, k{HIL_SIZE-mPos};dst < mPos;++dst,++k)
            mOutputAccum[dst] += 2.0/OVERSAMP*HannWindow[k]*mAnalytic[k];

        /* Copy out the accumulated result, then clear for the next iteration. */
        std::copy_n(mOutputAccum + mPos, HIL_STEP, mOutFIFO);
        std::fill_n(mOutputAccum + mPos, HIL_STEP, complex_d{});
    }

    /* Process frequency shifter using the analytic signal obtained. */
    float *RESTRICT BufferOut{mBufferOut};
    for(int c{0};c < 2;++c)
    {
        const uint phase_step{mPhaseStep[c]};
        uint phase_idx{mPhase[c]};
        for(size_t k{0};k < samplesToDo;++k)
        {
            const double phase{phase_idx * (al::numbers::pi*2.0 / MixerFracOne)};
            BufferOut[k] = static_cast<float>(mOutdata[k].real()*std::cos(phase) +
                mOutdata[k].imag()*std::sin(phase)*mSign[c]);

            phase_idx += phase_step;
            phase_idx &= MixerFracMask;
        }
        mPhase[c] = phase_idx;

        /* Now, mix the processed sound data to the output. */
        MixSamples({BufferOut, samplesToDo}, samplesOut, mGains[c].Current, mGains[c].Target,
            maxz(samplesToDo, 512), 0);
    }
}


struct FshifterStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new FshifterState{}}; }
};

} // namespace

EffectStateFactory *FshifterStateFactory_getFactory()
{
    static FshifterStateFactory FshifterFactory{};
    return &FshifterFactory;
}
