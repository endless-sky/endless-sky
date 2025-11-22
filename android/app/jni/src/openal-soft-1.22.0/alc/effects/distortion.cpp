/**
 * OpenAL cross platform audio library
 * Copyright (C) 2013 by Mike Gorchak
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
#include <cstdlib>
#include <iterator>

#include "alc/effects/base.h"
#include "almalloc.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "alspan.h"
#include "core/bufferline.h"
#include "core/context.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/filters/biquad.h"
#include "core/mixer.h"
#include "core/mixer/defs.h"
#include "intrusive_ptr.h"


namespace {

struct DistortionState final : public EffectState {
    /* Effect gains for each channel */
    float mGain[MAX_OUTPUT_CHANNELS]{};

    /* Effect parameters */
    BiquadFilter mLowpass;
    BiquadFilter mBandpass;
    float mAttenuation{};
    float mEdgeCoeff{};

    float mBuffer[2][BufferLineSize]{};


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(DistortionState)
};

void DistortionState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    mLowpass.clear();
    mBandpass.clear();
}

void DistortionState::update(const ContextBase *context, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    const DeviceBase *device{context->mDevice};

    /* Store waveshaper edge settings. */
    const float edge{minf(std::sin(al::numbers::pi_v<float>*0.5f * props->Distortion.Edge),
        0.99f)};
    mEdgeCoeff = 2.0f * edge / (1.0f-edge);

    float cutoff{props->Distortion.LowpassCutoff};
    /* Bandwidth value is constant in octaves. */
    float bandwidth{(cutoff / 2.0f) / (cutoff * 0.67f)};
    /* Divide normalized frequency by the amount of oversampling done during
     * processing.
     */
    auto frequency = static_cast<float>(device->Frequency);
    mLowpass.setParamsFromBandwidth(BiquadType::LowPass, cutoff/frequency/4.0f, 1.0f, bandwidth);

    cutoff = props->Distortion.EQCenter;
    /* Convert bandwidth in Hz to octaves. */
    bandwidth = props->Distortion.EQBandwidth / (cutoff * 0.67f);
    mBandpass.setParamsFromBandwidth(BiquadType::BandPass, cutoff/frequency/4.0f, 1.0f, bandwidth);

    const auto coeffs = CalcDirectionCoeffs({0.0f, 0.0f, -1.0f}, 0.0f);

    mOutTarget = target.Main->Buffer;
    ComputePanGains(target.Main, coeffs.data(), slot->Gain*props->Distortion.Gain, mGain);
}

void DistortionState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    const float fc{mEdgeCoeff};
    for(size_t base{0u};base < samplesToDo;)
    {
        /* Perform 4x oversampling to avoid aliasing. Oversampling greatly
         * improves distortion quality and allows to implement lowpass and
         * bandpass filters using high frequencies, at which classic IIR
         * filters became unstable.
         */
        size_t todo{minz(BufferLineSize, (samplesToDo-base) * 4)};

        /* Fill oversample buffer using zero stuffing. Multiply the sample by
         * the amount of oversampling to maintain the signal's power.
         */
        for(size_t i{0u};i < todo;i++)
            mBuffer[0][i] = !(i&3) ? samplesIn[0][(i>>2)+base] * 4.0f : 0.0f;

        /* First step, do lowpass filtering of original signal. Additionally
         * perform buffer interpolation and lowpass cutoff for oversampling
         * (which is fortunately first step of distortion). So combine three
         * operations into the one.
         */
        mLowpass.process({mBuffer[0], todo}, mBuffer[1]);

        /* Second step, do distortion using waveshaper function to emulate
         * signal processing during tube overdriving. Three steps of
         * waveshaping are intended to modify waveform without boost/clipping/
         * attenuation process.
         */
        auto proc_sample = [fc](float smp) -> float
        {
            smp = (1.0f + fc) * smp/(1.0f + fc*std::abs(smp));
            smp = (1.0f + fc) * smp/(1.0f + fc*std::abs(smp)) * -1.0f;
            smp = (1.0f + fc) * smp/(1.0f + fc*std::abs(smp));
            return smp;
        };
        std::transform(std::begin(mBuffer[1]), std::begin(mBuffer[1])+todo, std::begin(mBuffer[0]),
            proc_sample);

        /* Third step, do bandpass filtering of distorted signal. */
        mBandpass.process({mBuffer[0], todo}, mBuffer[1]);

        todo >>= 2;
        const float *outgains{mGain};
        for(FloatBufferLine &output : samplesOut)
        {
            /* Fourth step, final, do attenuation and perform decimation,
             * storing only one sample out of four.
             */
            const float gain{*(outgains++)};
            if(!(std::fabs(gain) > GainSilenceThreshold))
                continue;

            for(size_t i{0u};i < todo;i++)
                output[base+i] += gain * mBuffer[1][i*4];
        }

        base += todo;
    }
}


struct DistortionStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new DistortionState{}}; }
};

} // namespace

EffectStateFactory *DistortionStateFactory_getFactory()
{
    static DistortionStateFactory DistortionFactory{};
    return &DistortionFactory;
}
