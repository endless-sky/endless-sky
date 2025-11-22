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
#include <cstdlib>
#include <iterator>
#include <utility>

#include "alc/effects/base.h"
#include "almalloc.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "alspan.h"
#include "core/ambidefs.h"
#include "core/bufferline.h"
#include "core/context.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/mixer.h"
#include "intrusive_ptr.h"


namespace {

constexpr float GainScale{31621.0f};
constexpr float MinFreq{20.0f};
constexpr float MaxFreq{2500.0f};
constexpr float QFactor{5.0f};

struct AutowahState final : public EffectState {
    /* Effect parameters */
    float mAttackRate;
    float mReleaseRate;
    float mResonanceGain;
    float mPeakGain;
    float mFreqMinNorm;
    float mBandwidthNorm;
    float mEnvDelay;

    /* Filter components derived from the envelope. */
    struct {
        float cos_w0;
        float alpha;
    } mEnv[BufferLineSize];

    struct {
        /* Effect filters' history. */
        struct {
            float z1, z2;
        } Filter;

        /* Effect gains for each output channel */
        float CurrentGains[MAX_OUTPUT_CHANNELS];
        float TargetGains[MAX_OUTPUT_CHANNELS];
    } mChans[MaxAmbiChannels];

    /* Effects buffers */
    alignas(16) float mBufferOut[BufferLineSize];


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(AutowahState)
};

void AutowahState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    /* (Re-)initializing parameters and clear the buffers. */

    mAttackRate    = 1.0f;
    mReleaseRate   = 1.0f;
    mResonanceGain = 10.0f;
    mPeakGain      = 4.5f;
    mFreqMinNorm   = 4.5e-4f;
    mBandwidthNorm = 0.05f;
    mEnvDelay      = 0.0f;

    for(auto &e : mEnv)
    {
        e.cos_w0 = 0.0f;
        e.alpha = 0.0f;
    }

    for(auto &chan : mChans)
    {
        std::fill(std::begin(chan.CurrentGains), std::end(chan.CurrentGains), 0.0f);
        chan.Filter.z1 = 0.0f;
        chan.Filter.z2 = 0.0f;
    }
}

void AutowahState::update(const ContextBase *context, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    const DeviceBase *device{context->mDevice};
    const auto frequency = static_cast<float>(device->Frequency);

    const float ReleaseTime{clampf(props->Autowah.ReleaseTime, 0.001f, 1.0f)};

    mAttackRate    = std::exp(-1.0f / (props->Autowah.AttackTime*frequency));
    mReleaseRate   = std::exp(-1.0f / (ReleaseTime*frequency));
    /* 0-20dB Resonance Peak gain */
    mResonanceGain = std::sqrt(std::log10(props->Autowah.Resonance)*10.0f / 3.0f);
    mPeakGain      = 1.0f - std::log10(props->Autowah.PeakGain / GainScale);
    mFreqMinNorm   = MinFreq / frequency;
    mBandwidthNorm = (MaxFreq-MinFreq) / frequency;

    mOutTarget = target.Main->Buffer;
    auto set_gains = [slot,target](auto &chan, al::span<const float,MaxAmbiChannels> coeffs)
    { ComputePanGains(target.Main, coeffs.data(), slot->Gain, chan.TargetGains); };
    SetAmbiPanIdentity(std::begin(mChans), slot->Wet.Buffer.size(), set_gains);
}

void AutowahState::process(const size_t samplesToDo,
    const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    const float attack_rate{mAttackRate};
    const float release_rate{mReleaseRate};
    const float res_gain{mResonanceGain};
    const float peak_gain{mPeakGain};
    const float freq_min{mFreqMinNorm};
    const float bandwidth{mBandwidthNorm};

    float env_delay{mEnvDelay};
    for(size_t i{0u};i < samplesToDo;i++)
    {
        float w0, sample, a;

        /* Envelope follower described on the book: Audio Effects, Theory,
         * Implementation and Application.
         */
        sample = peak_gain * std::fabs(samplesIn[0][i]);
        a = (sample > env_delay) ? attack_rate : release_rate;
        env_delay = lerpf(sample, env_delay, a);

        /* Calculate the cos and alpha components for this sample's filter. */
        w0 = minf((bandwidth*env_delay + freq_min), 0.46f) * (al::numbers::pi_v<float>*2.0f);
        mEnv[i].cos_w0 = std::cos(w0);
        mEnv[i].alpha = std::sin(w0)/(2.0f * QFactor);
    }
    mEnvDelay = env_delay;

    auto chandata = std::addressof(mChans[0]);
    for(const auto &insamples : samplesIn)
    {
        /* This effectively inlines BiquadFilter_setParams for a peaking
         * filter and BiquadFilter_processC. The alpha and cosine components
         * for the filter coefficients were previously calculated with the
         * envelope. Because the filter changes for each sample, the
         * coefficients are transient and don't need to be held.
         */
        float z1{chandata->Filter.z1};
        float z2{chandata->Filter.z2};

        for(size_t i{0u};i < samplesToDo;i++)
        {
            const float alpha{mEnv[i].alpha};
            const float cos_w0{mEnv[i].cos_w0};
            float input, output;
            float a[3], b[3];

            b[0] =  1.0f + alpha*res_gain;
            b[1] = -2.0f * cos_w0;
            b[2] =  1.0f - alpha*res_gain;
            a[0] =  1.0f + alpha/res_gain;
            a[1] = -2.0f * cos_w0;
            a[2] =  1.0f - alpha/res_gain;

            input = insamples[i];
            output = input*(b[0]/a[0]) + z1;
            z1 = input*(b[1]/a[0]) - output*(a[1]/a[0]) + z2;
            z2 = input*(b[2]/a[0]) - output*(a[2]/a[0]);
            mBufferOut[i] = output;
        }
        chandata->Filter.z1 = z1;
        chandata->Filter.z2 = z2;

        /* Now, mix the processed sound data to the output. */
        MixSamples({mBufferOut, samplesToDo}, samplesOut, chandata->CurrentGains,
            chandata->TargetGains, samplesToDo, 0);
        ++chandata;
    }
}


struct AutowahStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new AutowahState{}}; }
};

} // namespace

EffectStateFactory *AutowahStateFactory_getFactory()
{
    static AutowahStateFactory AutowahFactory{};
    return &AutowahFactory;
}
