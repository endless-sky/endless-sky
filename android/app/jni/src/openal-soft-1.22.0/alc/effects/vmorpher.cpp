/**
 * This file is part of the OpenAL Soft cross platform audio library
 *
 * Copyright (C) 2019 by Anis A. Hireche
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

#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <iterator>

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

using uint = unsigned int;

#define MAX_UPDATE_SAMPLES 256
#define NUM_FORMANTS       4
#define NUM_FILTERS        2
#define Q_FACTOR           5.0f

#define VOWEL_A_INDEX      0
#define VOWEL_B_INDEX      1

#define WAVEFORM_FRACBITS  24
#define WAVEFORM_FRACONE   (1<<WAVEFORM_FRACBITS)
#define WAVEFORM_FRACMASK  (WAVEFORM_FRACONE-1)

inline float Sin(uint index)
{
    constexpr float scale{al::numbers::pi_v<float>*2.0f / WAVEFORM_FRACONE};
    return std::sin(static_cast<float>(index) * scale)*0.5f + 0.5f;
}

inline float Saw(uint index)
{ return static_cast<float>(index) / float{WAVEFORM_FRACONE}; }

inline float Triangle(uint index)
{ return std::fabs(static_cast<float>(index)*(2.0f/WAVEFORM_FRACONE) - 1.0f); }

inline float Half(uint) { return 0.5f; }

template<float (&func)(uint)>
void Oscillate(float *RESTRICT dst, uint index, const uint step, size_t todo)
{
    for(size_t i{0u};i < todo;i++)
    {
        index += step;
        index &= WAVEFORM_FRACMASK;
        dst[i] = func(index);
    }
}

struct FormantFilter
{
    float mCoeff{0.0f};
    float mGain{1.0f};
    float mS1{0.0f};
    float mS2{0.0f};

    FormantFilter() = default;
    FormantFilter(float f0norm, float gain)
      : mCoeff{std::tan(al::numbers::pi_v<float> * f0norm)}, mGain{gain}
    { }

    inline void process(const float *samplesIn, float *samplesOut, const size_t numInput)
    {
        /* A state variable filter from a topology-preserving transform.
         * Based on a talk given by Ivan Cohen: https://www.youtube.com/watch?v=esjHXGPyrhg
         */
        const float g{mCoeff};
        const float gain{mGain};
        const float h{1.0f / (1.0f + (g/Q_FACTOR) + (g*g))};
        float s1{mS1};
        float s2{mS2};

        for(size_t i{0u};i < numInput;i++)
        {
            const float H{(samplesIn[i] - (1.0f/Q_FACTOR + g)*s1 - s2)*h};
            const float B{g*H + s1};
            const float L{g*B + s2};

            s1 = g*H + B;
            s2 = g*B + L;

            // Apply peak and accumulate samples.
            samplesOut[i] += B * gain;
        }
        mS1 = s1;
        mS2 = s2;
    }

    inline void clear()
    {
        mS1 = 0.0f;
        mS2 = 0.0f;
    }
};


struct VmorpherState final : public EffectState {
    struct {
        /* Effect parameters */
        FormantFilter Formants[NUM_FILTERS][NUM_FORMANTS];

        /* Effect gains for each channel */
        float CurrentGains[MAX_OUTPUT_CHANNELS]{};
        float TargetGains[MAX_OUTPUT_CHANNELS]{};
    } mChans[MaxAmbiChannels];

    void (*mGetSamples)(float*RESTRICT, uint, const uint, size_t){};

    uint mIndex{0};
    uint mStep{1};

    /* Effects buffers */
    alignas(16) float mSampleBufferA[MAX_UPDATE_SAMPLES]{};
    alignas(16) float mSampleBufferB[MAX_UPDATE_SAMPLES]{};
    alignas(16) float mLfo[MAX_UPDATE_SAMPLES]{};

    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    static std::array<FormantFilter,4> getFiltersByPhoneme(VMorpherPhenome phoneme,
        float frequency, float pitch);

    DEF_NEWDEL(VmorpherState)
};

std::array<FormantFilter,4> VmorpherState::getFiltersByPhoneme(VMorpherPhenome phoneme,
    float frequency, float pitch)
{
    /* Using soprano formant set of values to
     * better match mid-range frequency space.
     *
     * See: https://www.classes.cs.uchicago.edu/archive/1999/spring/CS295/Computing_Resources/Csound/CsManual3.48b1.HTML/Appendices/table3.html
     */
    switch(phoneme)
    {
    case VMorpherPhenome::A:
        return {{
            {( 800 * pitch) / frequency, 1.000000f}, /* std::pow(10.0f,   0 / 20.0f); */
            {(1150 * pitch) / frequency, 0.501187f}, /* std::pow(10.0f,  -6 / 20.0f); */
            {(2900 * pitch) / frequency, 0.025118f}, /* std::pow(10.0f, -32 / 20.0f); */
            {(3900 * pitch) / frequency, 0.100000f}  /* std::pow(10.0f, -20 / 20.0f); */
        }};
    case VMorpherPhenome::E:
        return {{
            {( 350 * pitch) / frequency, 1.000000f}, /* std::pow(10.0f,   0 / 20.0f); */
            {(2000 * pitch) / frequency, 0.100000f}, /* std::pow(10.0f, -20 / 20.0f); */
            {(2800 * pitch) / frequency, 0.177827f}, /* std::pow(10.0f, -15 / 20.0f); */
            {(3600 * pitch) / frequency, 0.009999f}  /* std::pow(10.0f, -40 / 20.0f); */
        }};
    case VMorpherPhenome::I:
        return {{
            {( 270 * pitch) / frequency, 1.000000f}, /* std::pow(10.0f,   0 / 20.0f); */
            {(2140 * pitch) / frequency, 0.251188f}, /* std::pow(10.0f, -12 / 20.0f); */
            {(2950 * pitch) / frequency, 0.050118f}, /* std::pow(10.0f, -26 / 20.0f); */
            {(3900 * pitch) / frequency, 0.050118f}  /* std::pow(10.0f, -26 / 20.0f); */
        }};
    case VMorpherPhenome::O:
        return {{
            {( 450 * pitch) / frequency, 1.000000f}, /* std::pow(10.0f,   0 / 20.0f); */
            {( 800 * pitch) / frequency, 0.281838f}, /* std::pow(10.0f, -11 / 20.0f); */
            {(2830 * pitch) / frequency, 0.079432f}, /* std::pow(10.0f, -22 / 20.0f); */
            {(3800 * pitch) / frequency, 0.079432f}  /* std::pow(10.0f, -22 / 20.0f); */
        }};
    case VMorpherPhenome::U:
        return {{
            {( 325 * pitch) / frequency, 1.000000f}, /* std::pow(10.0f,   0 / 20.0f); */
            {( 700 * pitch) / frequency, 0.158489f}, /* std::pow(10.0f, -16 / 20.0f); */
            {(2700 * pitch) / frequency, 0.017782f}, /* std::pow(10.0f, -35 / 20.0f); */
            {(3800 * pitch) / frequency, 0.009999f}  /* std::pow(10.0f, -40 / 20.0f); */
        }};
    default:
        break;
    }
    return {};
}


void VmorpherState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    for(auto &e : mChans)
    {
        std::for_each(std::begin(e.Formants[VOWEL_A_INDEX]), std::end(e.Formants[VOWEL_A_INDEX]),
            std::mem_fn(&FormantFilter::clear));
        std::for_each(std::begin(e.Formants[VOWEL_B_INDEX]), std::end(e.Formants[VOWEL_B_INDEX]),
            std::mem_fn(&FormantFilter::clear));
        std::fill(std::begin(e.CurrentGains), std::end(e.CurrentGains), 0.0f);
    }
}

void VmorpherState::update(const ContextBase *context, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    const DeviceBase *device{context->mDevice};
    const float frequency{static_cast<float>(device->Frequency)};
    const float step{props->Vmorpher.Rate / frequency};
    mStep = fastf2u(clampf(step*WAVEFORM_FRACONE, 0.0f, float{WAVEFORM_FRACONE-1}));

    if(mStep == 0)
        mGetSamples = Oscillate<Half>;
    else if(props->Vmorpher.Waveform == VMorpherWaveform::Sinusoid)
        mGetSamples = Oscillate<Sin>;
    else if(props->Vmorpher.Waveform == VMorpherWaveform::Triangle)
        mGetSamples = Oscillate<Triangle>;
    else /*if(props->Vmorpher.Waveform == VMorpherWaveform::Sawtooth)*/
        mGetSamples = Oscillate<Saw>;

    const float pitchA{std::pow(2.0f,
        static_cast<float>(props->Vmorpher.PhonemeACoarseTuning) / 12.0f)};
    const float pitchB{std::pow(2.0f,
        static_cast<float>(props->Vmorpher.PhonemeBCoarseTuning) / 12.0f)};

    auto vowelA = getFiltersByPhoneme(props->Vmorpher.PhonemeA, frequency, pitchA);
    auto vowelB = getFiltersByPhoneme(props->Vmorpher.PhonemeB, frequency, pitchB);

    /* Copy the filter coefficients to the input channels. */
    for(size_t i{0u};i < slot->Wet.Buffer.size();++i)
    {
        std::copy(vowelA.begin(), vowelA.end(), std::begin(mChans[i].Formants[VOWEL_A_INDEX]));
        std::copy(vowelB.begin(), vowelB.end(), std::begin(mChans[i].Formants[VOWEL_B_INDEX]));
    }

    mOutTarget = target.Main->Buffer;
    auto set_gains = [slot,target](auto &chan, al::span<const float,MaxAmbiChannels> coeffs)
    { ComputePanGains(target.Main, coeffs.data(), slot->Gain, chan.TargetGains); };
    SetAmbiPanIdentity(std::begin(mChans), slot->Wet.Buffer.size(), set_gains);
}

void VmorpherState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    /* Following the EFX specification for a conformant implementation which describes
     * the effect as a pair of 4-band formant filters blended together using an LFO.
     */
    for(size_t base{0u};base < samplesToDo;)
    {
        const size_t td{minz(MAX_UPDATE_SAMPLES, samplesToDo-base)};

        mGetSamples(mLfo, mIndex, mStep, td);
        mIndex += static_cast<uint>(mStep * td);
        mIndex &= WAVEFORM_FRACMASK;

        auto chandata = std::begin(mChans);
        for(const auto &input : samplesIn)
        {
            auto& vowelA = chandata->Formants[VOWEL_A_INDEX];
            auto& vowelB = chandata->Formants[VOWEL_B_INDEX];

            /* Process first vowel. */
            std::fill_n(std::begin(mSampleBufferA), td, 0.0f);
            vowelA[0].process(&input[base], mSampleBufferA, td);
            vowelA[1].process(&input[base], mSampleBufferA, td);
            vowelA[2].process(&input[base], mSampleBufferA, td);
            vowelA[3].process(&input[base], mSampleBufferA, td);

            /* Process second vowel. */
            std::fill_n(std::begin(mSampleBufferB), td, 0.0f);
            vowelB[0].process(&input[base], mSampleBufferB, td);
            vowelB[1].process(&input[base], mSampleBufferB, td);
            vowelB[2].process(&input[base], mSampleBufferB, td);
            vowelB[3].process(&input[base], mSampleBufferB, td);

            alignas(16) float blended[MAX_UPDATE_SAMPLES];
            for(size_t i{0u};i < td;i++)
                blended[i] = lerpf(mSampleBufferA[i], mSampleBufferB[i], mLfo[i]);

            /* Now, mix the processed sound data to the output. */
            MixSamples({blended, td}, samplesOut, chandata->CurrentGains, chandata->TargetGains,
                samplesToDo-base, base);
            ++chandata;
        }

        base += td;
    }
}


struct VmorpherStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new VmorpherState{}}; }
};

} // namespace

EffectStateFactory *VmorpherStateFactory_getFactory()
{
    static VmorpherStateFactory VmorpherFactory{};
    return &VmorpherFactory;
}
