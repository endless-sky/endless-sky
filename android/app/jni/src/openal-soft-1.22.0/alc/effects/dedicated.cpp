/**
 * OpenAL cross platform audio library
 * Copyright (C) 2011 by Chris Robinson.
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
#include "alspan.h"
#include "core/bufferline.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/mixer.h"
#include "intrusive_ptr.h"

struct ContextBase;


namespace {

using uint = unsigned int;

struct DedicatedState final : public EffectState {
    float mCurrentGains[MAX_OUTPUT_CHANNELS];
    float mTargetGains[MAX_OUTPUT_CHANNELS];


    void deviceUpdate(const DeviceBase *device, const Buffer &buffer) override;
    void update(const ContextBase *context, const EffectSlot *slot, const EffectProps *props,
        const EffectTarget target) override;
    void process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn,
        const al::span<FloatBufferLine> samplesOut) override;

    DEF_NEWDEL(DedicatedState)
};

void DedicatedState::deviceUpdate(const DeviceBase*, const Buffer&)
{
    std::fill(std::begin(mCurrentGains), std::end(mCurrentGains), 0.0f);
}

void DedicatedState::update(const ContextBase*, const EffectSlot *slot,
    const EffectProps *props, const EffectTarget target)
{
    std::fill(std::begin(mTargetGains), std::end(mTargetGains), 0.0f);

    const float Gain{slot->Gain * props->Dedicated.Gain};

    if(slot->EffectType == EffectSlotType::DedicatedLFE)
    {
        const uint idx{!target.RealOut ? INVALID_CHANNEL_INDEX :
            GetChannelIdxByName(*target.RealOut, LFE)};
        if(idx != INVALID_CHANNEL_INDEX)
        {
            mOutTarget = target.RealOut->Buffer;
            mTargetGains[idx] = Gain;
        }
    }
    else if(slot->EffectType == EffectSlotType::DedicatedDialog)
    {
        /* Dialog goes to the front-center speaker if it exists, otherwise it
         * plays from the front-center location. */
        const uint idx{!target.RealOut ? INVALID_CHANNEL_INDEX :
            GetChannelIdxByName(*target.RealOut, FrontCenter)};
        if(idx != INVALID_CHANNEL_INDEX)
        {
            mOutTarget = target.RealOut->Buffer;
            mTargetGains[idx] = Gain;
        }
        else
        {
            const auto coeffs = CalcDirectionCoeffs({0.0f, 0.0f, -1.0f}, 0.0f);

            mOutTarget = target.Main->Buffer;
            ComputePanGains(target.Main, coeffs.data(), Gain, mTargetGains);
        }
    }
}

void DedicatedState::process(const size_t samplesToDo, const al::span<const FloatBufferLine> samplesIn, const al::span<FloatBufferLine> samplesOut)
{
    MixSamples({samplesIn[0].data(), samplesToDo}, samplesOut, mCurrentGains, mTargetGains,
        samplesToDo, 0);
}


struct DedicatedStateFactory final : public EffectStateFactory {
    al::intrusive_ptr<EffectState> create() override
    { return al::intrusive_ptr<EffectState>{new DedicatedState{}}; }
};

} // namespace

EffectStateFactory *DedicatedStateFactory_getFactory()
{
    static DedicatedStateFactory DedicatedFactory{};
    return &DedicatedFactory;
}
