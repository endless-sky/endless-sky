/**
 * This file is part of the OpenAL Soft cross platform audio library
 *
 * Copyright (C) 2013 by Anis A. Hireche
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Spherical-Harmonic-Transform nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <array>
#include <cstdlib>
#include <iterator>
#include <utility>

#include "alc/effects/base.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "alspan.h"
#include "core/ambidefs.h"
#include "core/bufferline.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/mixer.h"
#include "core/mixer/defs.h"
#include "intrusive_ptr.h"

struct ContextBase;


namespace {

#define AMP_ENVELOPE_MIN  0.5f
#define AMP_ENVELOPE_MAX  2.0f

#define ATTACK_TIME  0.1f /* 100ms to rise from min to max */
#define RELEASE_TIME 0.2f /* 200ms to drop from max to min */


struct CompressorState final : public EffectState {
    /* Effect gains for each channel */
    float mGain[MaxAmbiChannels][MAX_OUTPUT_CHANNELS]{};

    /* Effect parameters */
    bool mEnabled{true};
    float mAttackMult{1.0f};
    float mReleaseMult{1.0f};
    float mEnvFollower{1.0f};


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(CompressorState)
};

void CompressorState::deviceUpdate(const DeviceBase *device, const Buffer&)
{
    /* Number of samples to do a full attack and release (non-integer sample
     * counts are okay).
     */
    const float attackCount{static_cast<float>(device->Frequency) * ATTACK_TIME};
    const float releaseCount{static_cast<float>(device->Frequency) * RELEASE_TIME};

    /* Calculate per-sample multipliers to attack and release at the desired
     * rates.
     */
    mAttackMult  = std::pow(AMP_ENVELOPE_MAX/AMP_ENVELOPE_MIN, 1.0f/attackCount);
    mReleaseMult = std::pow(AMP_ENVELOPE_MIN/AMP_ENVELOPE_MAX, 1.0f/releaseCount);
}

void CompressorState::update(const ContextBase*, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    mEnabled = props->Compressor.OnOff;

    mOutTarget = target.Main->Buffer;
    auto set_gains = [slot,target](auto &gains, al::span<const float,MaxAmbiChannels> coeffs)
    { ComputePanGains(target.Main, coeffs.data(), slot->Gain, gains); };
    SetAmbiPanIdentity(std::begin(mGain), slot->Wet.Buffer.size(), set_gains);
}

void CompressorState::process(const size_t samplesToDo,
    const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    for(size_t base{0u};base < samplesToDo;)
    {
        float gains[256];
        const size_t td{minz(256, samplesToDo-base)};

        /* Generate the per-sample gains from the signal envelope. */
        float env{mEnvFollower};
        if(mEnabled)
        {
            for(size_t i{0u};i < td;++i)
            {
                /* Clamp the absolute amplitude to the defined envelope limits,
                 * then attack or release the envelope to reach it.
                 */
                const float amplitude{clampf(std::fabs(samplesIn[0][base+i]), AMP_ENVELOPE_MIN,
                    AMP_ENVELOPE_MAX)};
                if(amplitude > env)
                    env = minf(env*mAttackMult, amplitude);
                else if(amplitude < env)
                    env = maxf(env*mReleaseMult, amplitude);

                /* Apply the reciprocal of the envelope to normalize the volume
                 * (compress the dynamic range).
                 */
                gains[i] = 1.0f / env;
            }
        }
        else
        {
            /* Same as above, except the amplitude is forced to 1. This helps
             * ensure smooth gain changes when the compressor is turned on and
             * off.
             */
            for(size_t i{0u};i < td;++i)
            {
                const float amplitude{1.0f};
                if(amplitude > env)
                    env = minf(env*mAttackMult, amplitude);
                else if(amplitude < env)
                    env = maxf(env*mReleaseMult, amplitude);

                gains[i] = 1.0f / env;
            }
        }
        mEnvFollower = env;

        /* Now compress the signal amplitude to output. */
        auto changains = std::addressof(mGain[0]);
        for(const auto &input : samplesIn)
        {
            const float *outgains{*(changains++)};
            for(FloatBufferLine &output : samplesOut)
            {
                const float gain{*(outgains++)};
                if(!(std::fabs(gain) > GainSilenceThreshold))
                    continue;

                for(size_t i{0u};i < td;i++)
                    output[base+i] += input[base+i] * gains[i] * gain;
            }
        }

        base += td;
    }
}


struct CompressorStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new CompressorState{}}; }
};

} // namespace

EffectStateFactory *CompressorStateFactory_getFactory()
{
    static CompressorStateFactory CompressorFactory{};
    return &CompressorFactory;
}
