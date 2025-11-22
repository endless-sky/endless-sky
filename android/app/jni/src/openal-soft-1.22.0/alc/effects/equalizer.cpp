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
#include <functional>
#include <iterator>
#include <utility>

#include "alc/effects/base.h"
#include "almalloc.h"
#include "alspan.h"
#include "core/ambidefs.h"
#include "core/bufferline.h"
#include "core/context.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/filters/biquad.h"
#include "core/mixer.h"
#include "intrusive_ptr.h"


namespace {

/*  The document  "Effects Extension Guide.pdf"  says that low and high  *
 *  frequencies are cutoff frequencies. This is not fully correct, they  *
 *  are corner frequencies for low and high shelf filters. If they were  *
 *  just cutoff frequencies, there would be no need in cutoff frequency  *
 *  gains, which are present.  Documentation for  "Creative Proteus X2"  *
 *  software describes  4-band equalizer functionality in a much better  *
 *  way.  This equalizer seems  to be a predecessor  of  OpenAL  4-band  *
 *  equalizer.  With low and high  shelf filters  we are able to cutoff  *
 *  frequencies below and/or above corner frequencies using attenuation  *
 *  gains (below 1.0) and amplify all low and/or high frequencies using  *
 *  gains above 1.0.                                                     *
 *                                                                       *
 *     Low-shelf       Low Mid Band      High Mid Band     High-shelf    *
 *      corner            center             center          corner      *
 *     frequency        frequency          frequency       frequency     *
 *    50Hz..800Hz     200Hz..3000Hz      1000Hz..8000Hz  4000Hz..16000Hz *
 *                                                                       *
 *          |               |                  |               |         *
 *          |               |                  |               |         *
 *   B -----+            /--+--\            /--+--\            +-----    *
 *   O      |\          |   |   |          |   |   |          /|         *
 *   O      | \        -    |    -        -    |    -        / |         *
 *   S +    |  \      |     |     |      |     |     |      /  |         *
 *   T      |   |    |      |      |    |      |      |    |   |         *
 * ---------+---------------+------------------+---------------+-------- *
 *   C      |   |    |      |      |    |      |      |    |   |         *
 *   U -    |  /      |     |     |      |     |     |      \  |         *
 *   T      | /        -    |    -        -    |    -        \ |         *
 *   O      |/          |   |   |          |   |   |          \|         *
 *   F -----+            \--+--/            \--+--/            +-----    *
 *   F      |               |                  |               |         *
 *          |               |                  |               |         *
 *                                                                       *
 * Gains vary from 0.126 up to 7.943, which means from -18dB attenuation *
 * up to +18dB amplification. Band width varies from 0.01 up to 1.0 in   *
 * octaves for two mid bands.                                            *
 *                                                                       *
 * Implementation is based on the "Cookbook formulae for audio EQ biquad *
 * filter coefficients" by Robert Bristow-Johnson                        *
 * http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt                   */


struct EqualizerState final : public EffectState {
    struct {
        /* Effect parameters */
        BiquadFilter filter[4];

        /* Effect gains for each channel */
        float CurrentGains[MAX_OUTPUT_CHANNELS]{};
        float TargetGains[MAX_OUTPUT_CHANNELS]{};
    } mChans[MaxAmbiChannels];

    FloatBufferLine mSampleBuffer{};


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(EqualizerState)
};

void EqualizerState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    for(auto &e : mChans)
    {
        std::for_each(std::begin(e.filter), std::end(e.filter), std::mem_fn(&BiquadFilter::clear));
        std::fill(std::begin(e.CurrentGains), std::end(e.CurrentGains), 0.0f);
    }
}

void EqualizerState::update(const ContextBase *context, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    const DeviceBase *device{context->mDevice};
    auto frequency = static_cast<float>(device->Frequency);
    float gain, f0norm;

    /* Calculate coefficients for the each type of filter. Note that the shelf
     * and peaking filters' gain is for the centerpoint of the transition band,
     * while the effect property gains are for the shelf/peak itself. So the
     * property gains need their dB halved (sqrt of linear gain) for the
     * shelf/peak to reach the provided gain.
     */
    gain = std::sqrt(props->Equalizer.LowGain);
    f0norm = props->Equalizer.LowCutoff / frequency;
    mChans[0].filter[0].setParamsFromSlope(BiquadType::LowShelf, f0norm, gain, 0.75f);

    gain = std::sqrt(props->Equalizer.Mid1Gain);
    f0norm = props->Equalizer.Mid1Center / frequency;
    mChans[0].filter[1].setParamsFromBandwidth(BiquadType::Peaking, f0norm, gain,
        props->Equalizer.Mid1Width);

    gain = std::sqrt(props->Equalizer.Mid2Gain);
    f0norm = props->Equalizer.Mid2Center / frequency;
    mChans[0].filter[2].setParamsFromBandwidth(BiquadType::Peaking, f0norm, gain,
        props->Equalizer.Mid2Width);

    gain = std::sqrt(props->Equalizer.HighGain);
    f0norm = props->Equalizer.HighCutoff / frequency;
    mChans[0].filter[3].setParamsFromSlope(BiquadType::HighShelf, f0norm, gain, 0.75f);

    /* Copy the filter coefficients for the other input channels. */
    for(size_t i{1u};i < slot->Wet.Buffer.size();++i)
    {
        mChans[i].filter[0].copyParamsFrom(mChans[0].filter[0]);
        mChans[i].filter[1].copyParamsFrom(mChans[0].filter[1]);
        mChans[i].filter[2].copyParamsFrom(mChans[0].filter[2]);
        mChans[i].filter[3].copyParamsFrom(mChans[0].filter[3]);
    }

    mOutTarget = target.Main->Buffer;
    auto set_gains = [slot,target](auto &chan, al::span<const float,MaxAmbiChannels> coeffs)
    { ComputePanGains(target.Main, coeffs.data(), slot->Gain, chan.TargetGains); };
    SetAmbiPanIdentity(std::begin(mChans), slot->Wet.Buffer.size(), set_gains);
}

void EqualizerState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    const al::span<float> buffer{mSampleBuffer.data(), samplesToDo};
    auto chan = std::addressof(mChans[0]);
    for(const auto &input : samplesIn)
    {
        const al::span<const float> inbuf{input.data(), samplesToDo};
        DualBiquad{chan->filter[0], chan->filter[1]}.process(inbuf, buffer.begin());
        DualBiquad{chan->filter[2], chan->filter[3]}.process(buffer, buffer.begin());

        MixSamples(buffer, samplesOut, chan->CurrentGains, chan->TargetGains, samplesToDo, 0u);
        ++chan;
    }
}


struct EqualizerStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new EqualizerState{}}; }
};

} // namespace

EffectStateFactory *EqualizerStateFactory_getFactory()
{
    static EqualizerStateFactory EqualizerFactory{};
    return &EqualizerFactory;
}
