
#include "config.h"

#include "base.h"

#include <algorithm>
#include <array>
#include <atomic>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmreg.h>

#include "albit.h"
#include "core/logging.h"
#include "aloptional.h"
#endif

#include "atomic.h"
#include "core/devformat.h"


bool BackendBase::reset()
{ throw al::backend_exception{al::backend_error::DeviceError, "Invalid BackendBase call"}; }

void BackendBase::captureSamples(al::byte*, uint)
{ }

uint BackendBase::availableSamples()
{ return 0; }

ClockLatency BackendBase::getClockLatency()
{
    ClockLatency ret;

    uint refcount;
    do {
        refcount = mDevice->waitForMix();
        ret.ClockTime = GetDeviceClockTime(mDevice);
        std::atomic_thread_fence(std::memory_order_acquire);
    } while(refcount != ReadRef(mDevice->MixCount));

    /* NOTE: The device will generally have about all but one periods filled at
     * any given time during playback. Without a more accurate measurement from
     * the output, this is an okay approximation.
     */
    ret.Latency = std::max(std::chrono::seconds{mDevice->BufferSize-mDevice->UpdateSize},
        std::chrono::seconds::zero());
    ret.Latency /= mDevice->Frequency;

    return ret;
}

void BackendBase::setDefaultWFXChannelOrder()
{
    mDevice->RealOut.ChannelIndex.fill(INVALID_CHANNEL_INDEX);

    switch(mDevice->FmtChans)
    {
    case DevFmtMono:
        mDevice->RealOut.ChannelIndex[FrontCenter] = 0;
        break;
    case DevFmtStereo:
        mDevice->RealOut.ChannelIndex[FrontLeft]  = 0;
        mDevice->RealOut.ChannelIndex[FrontRight] = 1;
        break;
    case DevFmtQuad:
        mDevice->RealOut.ChannelIndex[FrontLeft]  = 0;
        mDevice->RealOut.ChannelIndex[FrontRight] = 1;
        mDevice->RealOut.ChannelIndex[BackLeft]   = 2;
        mDevice->RealOut.ChannelIndex[BackRight]  = 3;
        break;
    case DevFmtX51:
        mDevice->RealOut.ChannelIndex[FrontLeft]   = 0;
        mDevice->RealOut.ChannelIndex[FrontRight]  = 1;
        mDevice->RealOut.ChannelIndex[FrontCenter] = 2;
        mDevice->RealOut.ChannelIndex[LFE]         = 3;
        mDevice->RealOut.ChannelIndex[SideLeft]    = 4;
        mDevice->RealOut.ChannelIndex[SideRight]   = 5;
        break;
    case DevFmtX61:
        mDevice->RealOut.ChannelIndex[FrontLeft]   = 0;
        mDevice->RealOut.ChannelIndex[FrontRight]  = 1;
        mDevice->RealOut.ChannelIndex[FrontCenter] = 2;
        mDevice->RealOut.ChannelIndex[LFE]         = 3;
        mDevice->RealOut.ChannelIndex[BackCenter]  = 4;
        mDevice->RealOut.ChannelIndex[SideLeft]    = 5;
        mDevice->RealOut.ChannelIndex[SideRight]   = 6;
        break;
    case DevFmtX71:
        mDevice->RealOut.ChannelIndex[FrontLeft]   = 0;
        mDevice->RealOut.ChannelIndex[FrontRight]  = 1;
        mDevice->RealOut.ChannelIndex[FrontCenter] = 2;
        mDevice->RealOut.ChannelIndex[LFE]         = 3;
        mDevice->RealOut.ChannelIndex[BackLeft]    = 4;
        mDevice->RealOut.ChannelIndex[BackRight]   = 5;
        mDevice->RealOut.ChannelIndex[SideLeft]    = 6;
        mDevice->RealOut.ChannelIndex[SideRight]   = 7;
        break;
    case DevFmtAmbi3D:
        break;
    }
}

void BackendBase::setDefaultChannelOrder()
{
    mDevice->RealOut.ChannelIndex.fill(INVALID_CHANNEL_INDEX);

    switch(mDevice->FmtChans)
    {
    case DevFmtX51:
        mDevice->RealOut.ChannelIndex[FrontLeft]   = 0;
        mDevice->RealOut.ChannelIndex[FrontRight]  = 1;
        mDevice->RealOut.ChannelIndex[SideLeft]    = 2;
        mDevice->RealOut.ChannelIndex[SideRight]   = 3;
        mDevice->RealOut.ChannelIndex[FrontCenter] = 4;
        mDevice->RealOut.ChannelIndex[LFE]         = 5;
        return;
    case DevFmtX71:
        mDevice->RealOut.ChannelIndex[FrontLeft]   = 0;
        mDevice->RealOut.ChannelIndex[FrontRight]  = 1;
        mDevice->RealOut.ChannelIndex[BackLeft]    = 2;
        mDevice->RealOut.ChannelIndex[BackRight]   = 3;
        mDevice->RealOut.ChannelIndex[FrontCenter] = 4;
        mDevice->RealOut.ChannelIndex[LFE]         = 5;
        mDevice->RealOut.ChannelIndex[SideLeft]    = 6;
        mDevice->RealOut.ChannelIndex[SideRight]   = 7;
        return;

    /* Same as WFX order */
    case DevFmtMono:
    case DevFmtStereo:
    case DevFmtQuad:
    case DevFmtX61:
    case DevFmtAmbi3D:
        setDefaultWFXChannelOrder();
        break;
    }
}

#ifdef _WIN32
void BackendBase::setChannelOrderFromWFXMask(uint chanmask)
{
    static constexpr uint x51{SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
        | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT};
    static constexpr uint x51rear{SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
        | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT};
    /* Swap a 5.1 mask using the back channels for one with the sides. */
    if(chanmask == x51rear) chanmask = x51;

    auto get_channel = [](const DWORD chanbit) noexcept -> al::optional<Channel>
    {
        switch(chanbit)
        {
        case SPEAKER_FRONT_LEFT: return al::make_optional(FrontLeft);
        case SPEAKER_FRONT_RIGHT: return al::make_optional(FrontRight);
        case SPEAKER_FRONT_CENTER: return al::make_optional(FrontCenter);
        case SPEAKER_LOW_FREQUENCY: return al::make_optional(LFE);
        case SPEAKER_BACK_LEFT: return al::make_optional(BackLeft);
        case SPEAKER_BACK_RIGHT: return al::make_optional(BackRight);
        case SPEAKER_FRONT_LEFT_OF_CENTER: break;
        case SPEAKER_FRONT_RIGHT_OF_CENTER: break;
        case SPEAKER_BACK_CENTER: return al::make_optional(BackCenter);
        case SPEAKER_SIDE_LEFT: return al::make_optional(SideLeft);
        case SPEAKER_SIDE_RIGHT: return al::make_optional(SideRight);
        case SPEAKER_TOP_CENTER: return al::make_optional(TopCenter);
        case SPEAKER_TOP_FRONT_LEFT: return al::make_optional(TopFrontLeft);
        case SPEAKER_TOP_FRONT_CENTER: return al::make_optional(TopFrontCenter);
        case SPEAKER_TOP_FRONT_RIGHT: return al::make_optional(TopFrontRight);
        case SPEAKER_TOP_BACK_LEFT: return al::make_optional(TopBackLeft);
        case SPEAKER_TOP_BACK_CENTER: return al::make_optional(TopBackCenter);
        case SPEAKER_TOP_BACK_RIGHT: return al::make_optional(TopBackRight);
        }
        WARN("Unhandled WFX channel bit 0x%lx\n", chanbit);
        return al::nullopt;
    };

    const uint numchans{mDevice->channelsFromFmt()};
    uint idx{0};
    while(chanmask)
    {
        const int bit{al::countr_zero(chanmask)};
        const uint mask{1u << bit};
        chanmask &= ~mask;

        if(auto label = get_channel(mask))
        {
            mDevice->RealOut.ChannelIndex[*label] = idx;
            if(++idx == numchans) break;
        }
    }
}
#endif
