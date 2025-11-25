/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
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

#include "alsa.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "albyte.h"
#include "alc/alconfig.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "core/device.h"
#include "core/helpers.h"
#include "core/logging.h"
#include "dynload.h"
#include "ringbuffer.h"
#include "threads.h"
#include "vector.h"

#include <alsa/asoundlib.h>


namespace {

constexpr char alsaDevice[] = "ALSA Default";


#ifdef HAVE_DYNLOAD
#define ALSA_FUNCS(MAGIC)                                                     \
    MAGIC(snd_strerror);                                                      \
    MAGIC(snd_pcm_open);                                                      \
    MAGIC(snd_pcm_close);                                                     \
    MAGIC(snd_pcm_nonblock);                                                  \
    MAGIC(snd_pcm_frames_to_bytes);                                           \
    MAGIC(snd_pcm_bytes_to_frames);                                           \
    MAGIC(snd_pcm_hw_params_malloc);                                          \
    MAGIC(snd_pcm_hw_params_free);                                            \
    MAGIC(snd_pcm_hw_params_any);                                             \
    MAGIC(snd_pcm_hw_params_current);                                         \
    MAGIC(snd_pcm_hw_params_get_access);                                      \
    MAGIC(snd_pcm_hw_params_get_buffer_size);                                 \
    MAGIC(snd_pcm_hw_params_get_buffer_time_min);                             \
    MAGIC(snd_pcm_hw_params_get_buffer_time_max);                             \
    MAGIC(snd_pcm_hw_params_get_channels);                                    \
    MAGIC(snd_pcm_hw_params_get_period_size);                                 \
    MAGIC(snd_pcm_hw_params_get_period_time_max);                             \
    MAGIC(snd_pcm_hw_params_get_period_time_min);                             \
    MAGIC(snd_pcm_hw_params_get_periods);                                     \
    MAGIC(snd_pcm_hw_params_set_access);                                      \
    MAGIC(snd_pcm_hw_params_set_buffer_size_min);                             \
    MAGIC(snd_pcm_hw_params_set_buffer_size_near);                            \
    MAGIC(snd_pcm_hw_params_set_buffer_time_near);                            \
    MAGIC(snd_pcm_hw_params_set_channels);                                    \
    MAGIC(snd_pcm_hw_params_set_channels_near);                               \
    MAGIC(snd_pcm_hw_params_set_format);                                      \
    MAGIC(snd_pcm_hw_params_set_period_time_near);                            \
    MAGIC(snd_pcm_hw_params_set_period_size_near);                            \
    MAGIC(snd_pcm_hw_params_set_periods_near);                                \
    MAGIC(snd_pcm_hw_params_set_rate_near);                                   \
    MAGIC(snd_pcm_hw_params_set_rate);                                        \
    MAGIC(snd_pcm_hw_params_set_rate_resample);                               \
    MAGIC(snd_pcm_hw_params_test_format);                                     \
    MAGIC(snd_pcm_hw_params_test_channels);                                   \
    MAGIC(snd_pcm_hw_params);                                                 \
    MAGIC(snd_pcm_sw_params);                                                 \
    MAGIC(snd_pcm_sw_params_current);                                         \
    MAGIC(snd_pcm_sw_params_free);                                            \
    MAGIC(snd_pcm_sw_params_malloc);                                          \
    MAGIC(snd_pcm_sw_params_set_avail_min);                                   \
    MAGIC(snd_pcm_sw_params_set_stop_threshold);                              \
    MAGIC(snd_pcm_prepare);                                                   \
    MAGIC(snd_pcm_start);                                                     \
    MAGIC(snd_pcm_resume);                                                    \
    MAGIC(snd_pcm_reset);                                                     \
    MAGIC(snd_pcm_wait);                                                      \
    MAGIC(snd_pcm_delay);                                                     \
    MAGIC(snd_pcm_state);                                                     \
    MAGIC(snd_pcm_avail_update);                                              \
    MAGIC(snd_pcm_mmap_begin);                                                \
    MAGIC(snd_pcm_mmap_commit);                                               \
    MAGIC(snd_pcm_readi);                                                     \
    MAGIC(snd_pcm_writei);                                                    \
    MAGIC(snd_pcm_drain);                                                     \
    MAGIC(snd_pcm_drop);                                                      \
    MAGIC(snd_pcm_recover);                                                   \
    MAGIC(snd_pcm_info_malloc);                                               \
    MAGIC(snd_pcm_info_free);                                                 \
    MAGIC(snd_pcm_info_set_device);                                           \
    MAGIC(snd_pcm_info_set_subdevice);                                        \
    MAGIC(snd_pcm_info_set_stream);                                           \
    MAGIC(snd_pcm_info_get_name);                                             \
    MAGIC(snd_ctl_pcm_next_device);                                           \
    MAGIC(snd_ctl_pcm_info);                                                  \
    MAGIC(snd_ctl_open);                                                      \
    MAGIC(snd_ctl_close);                                                     \
    MAGIC(snd_ctl_card_info_malloc);                                          \
    MAGIC(snd_ctl_card_info_free);                                            \
    MAGIC(snd_ctl_card_info);                                                 \
    MAGIC(snd_ctl_card_info_get_name);                                        \
    MAGIC(snd_ctl_card_info_get_id);                                          \
    MAGIC(snd_card_next);                                                     \
    MAGIC(snd_config_update_free_global)

static void *alsa_handle;
#define MAKE_FUNC(f) decltype(f) * p##f
ALSA_FUNCS(MAKE_FUNC);
#undef MAKE_FUNC

#ifndef IN_IDE_PARSER
#define snd_strerror psnd_strerror
#define snd_pcm_open psnd_pcm_open
#define snd_pcm_close psnd_pcm_close
#define snd_pcm_nonblock psnd_pcm_nonblock
#define snd_pcm_frames_to_bytes psnd_pcm_frames_to_bytes
#define snd_pcm_bytes_to_frames psnd_pcm_bytes_to_frames
#define snd_pcm_hw_params_malloc psnd_pcm_hw_params_malloc
#define snd_pcm_hw_params_free psnd_pcm_hw_params_free
#define snd_pcm_hw_params_any psnd_pcm_hw_params_any
#define snd_pcm_hw_params_current psnd_pcm_hw_params_current
#define snd_pcm_hw_params_set_access psnd_pcm_hw_params_set_access
#define snd_pcm_hw_params_set_format psnd_pcm_hw_params_set_format
#define snd_pcm_hw_params_set_channels psnd_pcm_hw_params_set_channels
#define snd_pcm_hw_params_set_channels_near psnd_pcm_hw_params_set_channels_near
#define snd_pcm_hw_params_set_periods_near psnd_pcm_hw_params_set_periods_near
#define snd_pcm_hw_params_set_rate_near psnd_pcm_hw_params_set_rate_near
#define snd_pcm_hw_params_set_rate psnd_pcm_hw_params_set_rate
#define snd_pcm_hw_params_set_rate_resample psnd_pcm_hw_params_set_rate_resample
#define snd_pcm_hw_params_set_buffer_time_near psnd_pcm_hw_params_set_buffer_time_near
#define snd_pcm_hw_params_set_period_time_near psnd_pcm_hw_params_set_period_time_near
#define snd_pcm_hw_params_set_buffer_size_near psnd_pcm_hw_params_set_buffer_size_near
#define snd_pcm_hw_params_set_period_size_near psnd_pcm_hw_params_set_period_size_near
#define snd_pcm_hw_params_set_buffer_size_min psnd_pcm_hw_params_set_buffer_size_min
#define snd_pcm_hw_params_get_buffer_time_min psnd_pcm_hw_params_get_buffer_time_min
#define snd_pcm_hw_params_get_buffer_time_max psnd_pcm_hw_params_get_buffer_time_max
#define snd_pcm_hw_params_get_period_time_min psnd_pcm_hw_params_get_period_time_min
#define snd_pcm_hw_params_get_period_time_max psnd_pcm_hw_params_get_period_time_max
#define snd_pcm_hw_params_get_buffer_size psnd_pcm_hw_params_get_buffer_size
#define snd_pcm_hw_params_get_period_size psnd_pcm_hw_params_get_period_size
#define snd_pcm_hw_params_get_access psnd_pcm_hw_params_get_access
#define snd_pcm_hw_params_get_periods psnd_pcm_hw_params_get_periods
#define snd_pcm_hw_params_get_channels psnd_pcm_hw_params_get_channels
#define snd_pcm_hw_params_test_format psnd_pcm_hw_params_test_format
#define snd_pcm_hw_params_test_channels psnd_pcm_hw_params_test_channels
#define snd_pcm_hw_params psnd_pcm_hw_params
#define snd_pcm_sw_params_malloc psnd_pcm_sw_params_malloc
#define snd_pcm_sw_params_current psnd_pcm_sw_params_current
#define snd_pcm_sw_params_set_avail_min psnd_pcm_sw_params_set_avail_min
#define snd_pcm_sw_params_set_stop_threshold psnd_pcm_sw_params_set_stop_threshold
#define snd_pcm_sw_params psnd_pcm_sw_params
#define snd_pcm_sw_params_free psnd_pcm_sw_params_free
#define snd_pcm_prepare psnd_pcm_prepare
#define snd_pcm_start psnd_pcm_start
#define snd_pcm_resume psnd_pcm_resume
#define snd_pcm_reset psnd_pcm_reset
#define snd_pcm_wait psnd_pcm_wait
#define snd_pcm_delay psnd_pcm_delay
#define snd_pcm_state psnd_pcm_state
#define snd_pcm_avail_update psnd_pcm_avail_update
#define snd_pcm_mmap_begin psnd_pcm_mmap_begin
#define snd_pcm_mmap_commit psnd_pcm_mmap_commit
#define snd_pcm_readi psnd_pcm_readi
#define snd_pcm_writei psnd_pcm_writei
#define snd_pcm_drain psnd_pcm_drain
#define snd_pcm_drop psnd_pcm_drop
#define snd_pcm_recover psnd_pcm_recover
#define snd_pcm_info_malloc psnd_pcm_info_malloc
#define snd_pcm_info_free psnd_pcm_info_free
#define snd_pcm_info_set_device psnd_pcm_info_set_device
#define snd_pcm_info_set_subdevice psnd_pcm_info_set_subdevice
#define snd_pcm_info_set_stream psnd_pcm_info_set_stream
#define snd_pcm_info_get_name psnd_pcm_info_get_name
#define snd_ctl_pcm_next_device psnd_ctl_pcm_next_device
#define snd_ctl_pcm_info psnd_ctl_pcm_info
#define snd_ctl_open psnd_ctl_open
#define snd_ctl_close psnd_ctl_close
#define snd_ctl_card_info_malloc psnd_ctl_card_info_malloc
#define snd_ctl_card_info_free psnd_ctl_card_info_free
#define snd_ctl_card_info psnd_ctl_card_info
#define snd_ctl_card_info_get_name psnd_ctl_card_info_get_name
#define snd_ctl_card_info_get_id psnd_ctl_card_info_get_id
#define snd_card_next psnd_card_next
#define snd_config_update_free_global psnd_config_update_free_global
#endif
#endif


struct HwParamsDeleter {
    void operator()(snd_pcm_hw_params_t *ptr) { snd_pcm_hw_params_free(ptr); }
};
using HwParamsPtr = std::unique_ptr<snd_pcm_hw_params_t,HwParamsDeleter>;
HwParamsPtr CreateHwParams()
{
    snd_pcm_hw_params_t *hp{};
    snd_pcm_hw_params_malloc(&hp);
    return HwParamsPtr{hp};
}

struct SwParamsDeleter {
    void operator()(snd_pcm_sw_params_t *ptr) { snd_pcm_sw_params_free(ptr); }
};
using SwParamsPtr = std::unique_ptr<snd_pcm_sw_params_t,SwParamsDeleter>;
SwParamsPtr CreateSwParams()
{
    snd_pcm_sw_params_t *sp{};
    snd_pcm_sw_params_malloc(&sp);
    return SwParamsPtr{sp};
}


struct DevMap {
    std::string name;
    std::string device_name;
};

al::vector<DevMap> PlaybackDevices;
al::vector<DevMap> CaptureDevices;


const char *prefix_name(snd_pcm_stream_t stream)
{
    assert(stream == SND_PCM_STREAM_PLAYBACK || stream == SND_PCM_STREAM_CAPTURE);
    return (stream==SND_PCM_STREAM_PLAYBACK) ? "device-prefix" : "capture-prefix";
}

al::vector<DevMap> probe_devices(snd_pcm_stream_t stream)
{
    al::vector<DevMap> devlist;

    snd_ctl_card_info_t *info;
    snd_ctl_card_info_malloc(&info);
    snd_pcm_info_t *pcminfo;
    snd_pcm_info_malloc(&pcminfo);

    auto defname = ConfigValueStr(nullptr, "alsa",
        (stream == SND_PCM_STREAM_PLAYBACK) ? "device" : "capture");
    devlist.emplace_back(DevMap{alsaDevice, defname ? defname->c_str() : "default"});

    if(auto customdevs = ConfigValueStr(nullptr, "alsa",
        (stream == SND_PCM_STREAM_PLAYBACK) ? "custom-devices" : "custom-captures"))
    {
        size_t nextpos{customdevs->find_first_not_of(';')};
        size_t curpos;
        while((curpos=nextpos) < customdevs->length())
        {
            nextpos = customdevs->find_first_of(';', curpos+1);

            size_t seppos{customdevs->find_first_of('=', curpos)};
            if(seppos == curpos || seppos >= nextpos)
            {
                std::string spec{customdevs->substr(curpos, nextpos-curpos)};
                ERR("Invalid ALSA device specification \"%s\"\n", spec.c_str());
            }
            else
            {
                devlist.emplace_back(DevMap{customdevs->substr(curpos, seppos-curpos),
                    customdevs->substr(seppos+1, nextpos-seppos-1)});
                const auto &entry = devlist.back();
                TRACE("Got device \"%s\", \"%s\"\n", entry.name.c_str(), entry.device_name.c_str());
            }

            if(nextpos < customdevs->length())
                nextpos = customdevs->find_first_not_of(';', nextpos+1);
        }
    }

    const std::string main_prefix{
        ConfigValueStr(nullptr, "alsa", prefix_name(stream)).value_or("plughw:")};

    int card{-1};
    int err{snd_card_next(&card)};
    for(;err >= 0 && card >= 0;err = snd_card_next(&card))
    {
        std::string name{"hw:" + std::to_string(card)};

        snd_ctl_t *handle;
        if((err=snd_ctl_open(&handle, name.c_str(), 0)) < 0)
        {
            ERR("control open (hw:%d): %s\n", card, snd_strerror(err));
            continue;
        }
        if((err=snd_ctl_card_info(handle, info)) < 0)
        {
            ERR("control hardware info (hw:%d): %s\n", card, snd_strerror(err));
            snd_ctl_close(handle);
            continue;
        }

        const char *cardname{snd_ctl_card_info_get_name(info)};
        const char *cardid{snd_ctl_card_info_get_id(info)};
        name = prefix_name(stream);
        name += '-';
        name += cardid;
        const std::string card_prefix{
            ConfigValueStr(nullptr, "alsa", name.c_str()).value_or(main_prefix)};

        int dev{-1};
        while(1)
        {
            if(snd_ctl_pcm_next_device(handle, &dev) < 0)
                ERR("snd_ctl_pcm_next_device failed\n");
            if(dev < 0) break;

            snd_pcm_info_set_device(pcminfo, static_cast<uint>(dev));
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, stream);
            if((err=snd_ctl_pcm_info(handle, pcminfo)) < 0)
            {
                if(err != -ENOENT)
                    ERR("control digital audio info (hw:%d): %s\n", card, snd_strerror(err));
                continue;
            }

            /* "prefix-cardid-dev" */
            name = prefix_name(stream);
            name += '-';
            name += cardid;
            name += '-';
            name += std::to_string(dev);
            const std::string device_prefix{
                ConfigValueStr(nullptr, "alsa", name.c_str()).value_or(card_prefix)};

            /* "CardName, PcmName (CARD=cardid,DEV=dev)" */
            name = cardname;
            name += ", ";
            name += snd_pcm_info_get_name(pcminfo);
            name += " (CARD=";
            name += cardid;
            name += ",DEV=";
            name += std::to_string(dev);
            name += ')';

            /* "devprefixCARD=cardid,DEV=dev" */
            std::string device{device_prefix};
            device += "CARD=";
            device += cardid;
            device += ",DEV=";
            device += std::to_string(dev);
            
            devlist.emplace_back(DevMap{std::move(name), std::move(device)});
            const auto &entry = devlist.back();
            TRACE("Got device \"%s\", \"%s\"\n", entry.name.c_str(), entry.device_name.c_str());
        }
        snd_ctl_close(handle);
    }
    if(err < 0)
        ERR("snd_card_next failed: %s\n", snd_strerror(err));

    snd_pcm_info_free(pcminfo);
    snd_ctl_card_info_free(info);

    return devlist;
}


int verify_state(snd_pcm_t *handle)
{
    snd_pcm_state_t state{snd_pcm_state(handle)};

    int err;
    switch(state)
    {
        case SND_PCM_STATE_OPEN:
        case SND_PCM_STATE_SETUP:
        case SND_PCM_STATE_PREPARED:
        case SND_PCM_STATE_RUNNING:
        case SND_PCM_STATE_DRAINING:
        case SND_PCM_STATE_PAUSED:
            /* All Okay */
            break;

        case SND_PCM_STATE_XRUN:
            if((err=snd_pcm_recover(handle, -EPIPE, 1)) < 0)
                return err;
            break;
        case SND_PCM_STATE_SUSPENDED:
            if((err=snd_pcm_recover(handle, -ESTRPIPE, 1)) < 0)
                return err;
            break;
        case SND_PCM_STATE_DISCONNECTED:
            return -ENODEV;
    }

    return state;
}


struct AlsaPlayback final : public BackendBase {
    AlsaPlayback(DeviceBase *device) noexcept : BackendBase{device} { }
    ~AlsaPlayback() override;

    int mixerProc();
    int mixerNoMMapProc();

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;

    ClockLatency getClockLatency() override;

    snd_pcm_t *mPcmHandle{nullptr};

    std::mutex mMutex;

    uint mFrameStep{};
    al::vector<al::byte> mBuffer;

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(AlsaPlayback)
};

AlsaPlayback::~AlsaPlayback()
{
    if(mPcmHandle)
        snd_pcm_close(mPcmHandle);
    mPcmHandle = nullptr;
}


int AlsaPlayback::mixerProc()
{
    SetRTPriority();
    althrd_setname(MIXER_THREAD_NAME);

    const snd_pcm_uframes_t update_size{mDevice->UpdateSize};
    const snd_pcm_uframes_t buffer_size{mDevice->BufferSize};
    while(!mKillNow.load(std::memory_order_acquire))
    {
        int state{verify_state(mPcmHandle)};
        if(state < 0)
        {
            ERR("Invalid state detected: %s\n", snd_strerror(state));
            mDevice->handleDisconnect("Bad state: %s", snd_strerror(state));
            break;
        }

        snd_pcm_sframes_t avails{snd_pcm_avail_update(mPcmHandle)};
        if(avails < 0)
        {
            ERR("available update failed: %s\n", snd_strerror(static_cast<int>(avails)));
            continue;
        }
        snd_pcm_uframes_t avail{static_cast<snd_pcm_uframes_t>(avails)};

        if(avail > buffer_size)
        {
            WARN("available samples exceeds the buffer size\n");
            snd_pcm_reset(mPcmHandle);
            continue;
        }

        // make sure there's frames to process
        if(avail < update_size)
        {
            if(state != SND_PCM_STATE_RUNNING)
            {
                int err{snd_pcm_start(mPcmHandle)};
                if(err < 0)
                {
                    ERR("start failed: %s\n", snd_strerror(err));
                    continue;
                }
            }
            if(snd_pcm_wait(mPcmHandle, 1000) == 0)
                ERR("Wait timeout... buffer size too low?\n");
            continue;
        }
        avail -= avail%update_size;

        // it is possible that contiguous areas are smaller, thus we use a loop
        std::lock_guard<std::mutex> _{mMutex};
        while(avail > 0)
        {
            snd_pcm_uframes_t frames{avail};

            const snd_pcm_channel_area_t *areas{};
            snd_pcm_uframes_t offset{};
            int err{snd_pcm_mmap_begin(mPcmHandle, &areas, &offset, &frames)};
            if(err < 0)
            {
                ERR("mmap begin error: %s\n", snd_strerror(err));
                break;
            }

            char *WritePtr{static_cast<char*>(areas->addr) + (offset * areas->step / 8)};
            mDevice->renderSamples(WritePtr, static_cast<uint>(frames), mFrameStep);

            snd_pcm_sframes_t commitres{snd_pcm_mmap_commit(mPcmHandle, offset, frames)};
            if(commitres < 0 || static_cast<snd_pcm_uframes_t>(commitres) != frames)
            {
                ERR("mmap commit error: %s\n",
                    snd_strerror(commitres >= 0 ? -EPIPE : static_cast<int>(commitres)));
                break;
            }

            avail -= frames;
        }
    }

    return 0;
}

int AlsaPlayback::mixerNoMMapProc()
{
    SetRTPriority();
    althrd_setname(MIXER_THREAD_NAME);

    const snd_pcm_uframes_t update_size{mDevice->UpdateSize};
    const snd_pcm_uframes_t buffer_size{mDevice->BufferSize};
    while(!mKillNow.load(std::memory_order_acquire))
    {
        int state{verify_state(mPcmHandle)};
        if(state < 0)
        {
            ERR("Invalid state detected: %s\n", snd_strerror(state));
            mDevice->handleDisconnect("Bad state: %s", snd_strerror(state));
            break;
        }

        snd_pcm_sframes_t avail{snd_pcm_avail_update(mPcmHandle)};
        if(avail < 0)
        {
            ERR("available update failed: %s\n", snd_strerror(static_cast<int>(avail)));
            continue;
        }

        if(static_cast<snd_pcm_uframes_t>(avail) > buffer_size)
        {
            WARN("available samples exceeds the buffer size\n");
            snd_pcm_reset(mPcmHandle);
            continue;
        }

        if(static_cast<snd_pcm_uframes_t>(avail) < update_size)
        {
            if(state != SND_PCM_STATE_RUNNING)
            {
                int err{snd_pcm_start(mPcmHandle)};
                if(err < 0)
                {
                    ERR("start failed: %s\n", snd_strerror(err));
                    continue;
                }
            }
            if(snd_pcm_wait(mPcmHandle, 1000) == 0)
                ERR("Wait timeout... buffer size too low?\n");
            continue;
        }

        al::byte *WritePtr{mBuffer.data()};
        avail = snd_pcm_bytes_to_frames(mPcmHandle, static_cast<ssize_t>(mBuffer.size()));
        std::lock_guard<std::mutex> _{mMutex};
        mDevice->renderSamples(WritePtr, static_cast<uint>(avail), mFrameStep);
        while(avail > 0)
        {
            snd_pcm_sframes_t ret{snd_pcm_writei(mPcmHandle, WritePtr,
                static_cast<snd_pcm_uframes_t>(avail))};
            switch(ret)
            {
            case -EAGAIN:
                continue;
#if ESTRPIPE != EPIPE
            case -ESTRPIPE:
#endif
            case -EPIPE:
            case -EINTR:
                ret = snd_pcm_recover(mPcmHandle, static_cast<int>(ret), 1);
                if(ret < 0)
                    avail = 0;
                break;
            default:
                if(ret >= 0)
                {
                    WritePtr += snd_pcm_frames_to_bytes(mPcmHandle, ret);
                    avail -= ret;
                }
                break;
            }
            if(ret < 0)
            {
                ret = snd_pcm_prepare(mPcmHandle);
                if(ret < 0) break;
            }
        }
    }

    return 0;
}


void AlsaPlayback::open(const char *name)
{
    al::optional<std::string> driveropt;
    const char *driver{"default"};
    if(name)
    {
        if(PlaybackDevices.empty())
            PlaybackDevices = probe_devices(SND_PCM_STREAM_PLAYBACK);

        auto iter = std::find_if(PlaybackDevices.cbegin(), PlaybackDevices.cend(),
            [name](const DevMap &entry) -> bool { return entry.name == name; });
        if(iter == PlaybackDevices.cend())
            throw al::backend_exception{al::backend_error::NoDevice,
                "Device name \"%s\" not found", name};
        driver = iter->device_name.c_str();
    }
    else
    {
        name = alsaDevice;
        if(bool{driveropt = ConfigValueStr(nullptr, "alsa", "device")})
            driver = driveropt->c_str();
    }
    TRACE("Opening device \"%s\"\n", driver);

    snd_pcm_t *pcmHandle{};
    int err{snd_pcm_open(&pcmHandle, driver, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)};
    if(err < 0)
        throw al::backend_exception{al::backend_error::NoDevice,
            "Could not open ALSA device \"%s\"", driver};
    if(mPcmHandle)
        snd_pcm_close(mPcmHandle);
    mPcmHandle = pcmHandle;

    /* Free alsa's global config tree. Otherwise valgrind reports a ton of leaks. */
    snd_config_update_free_global();

    mDevice->DeviceName = name;
}

bool AlsaPlayback::reset()
{
    snd_pcm_format_t format{SND_PCM_FORMAT_UNKNOWN};
    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        format = SND_PCM_FORMAT_S8;
        break;
    case DevFmtUByte:
        format = SND_PCM_FORMAT_U8;
        break;
    case DevFmtShort:
        format = SND_PCM_FORMAT_S16;
        break;
    case DevFmtUShort:
        format = SND_PCM_FORMAT_U16;
        break;
    case DevFmtInt:
        format = SND_PCM_FORMAT_S32;
        break;
    case DevFmtUInt:
        format = SND_PCM_FORMAT_U32;
        break;
    case DevFmtFloat:
        format = SND_PCM_FORMAT_FLOAT;
        break;
    }

    bool allowmmap{!!GetConfigValueBool(mDevice->DeviceName.c_str(), "alsa", "mmap", 1)};
    uint periodLen{static_cast<uint>(mDevice->UpdateSize * 1000000_u64 / mDevice->Frequency)};
    uint bufferLen{static_cast<uint>(mDevice->BufferSize * 1000000_u64 / mDevice->Frequency)};
    uint rate{mDevice->Frequency};

    int err{};
    HwParamsPtr hp{CreateHwParams()};
#define CHECK(x) do {                                                         \
    if((err=(x)) < 0)                                                         \
        throw al::backend_exception{al::backend_error::DeviceError, #x " failed: %s", \
            snd_strerror(err)};                                               \
} while(0)
    CHECK(snd_pcm_hw_params_any(mPcmHandle, hp.get()));
    /* set interleaved access */
    if(!allowmmap
        || snd_pcm_hw_params_set_access(mPcmHandle, hp.get(), SND_PCM_ACCESS_MMAP_INTERLEAVED) < 0)
    {
        /* No mmap */
        CHECK(snd_pcm_hw_params_set_access(mPcmHandle, hp.get(), SND_PCM_ACCESS_RW_INTERLEAVED));
    }
    /* test and set format (implicitly sets sample bits) */
    if(snd_pcm_hw_params_test_format(mPcmHandle, hp.get(), format) < 0)
    {
        static const struct {
            snd_pcm_format_t format;
            DevFmtType fmttype;
        } formatlist[] = {
            { SND_PCM_FORMAT_FLOAT, DevFmtFloat  },
            { SND_PCM_FORMAT_S32,   DevFmtInt    },
            { SND_PCM_FORMAT_U32,   DevFmtUInt   },
            { SND_PCM_FORMAT_S16,   DevFmtShort  },
            { SND_PCM_FORMAT_U16,   DevFmtUShort },
            { SND_PCM_FORMAT_S8,    DevFmtByte   },
            { SND_PCM_FORMAT_U8,    DevFmtUByte  },
        };

        for(const auto &fmt : formatlist)
        {
            format = fmt.format;
            if(snd_pcm_hw_params_test_format(mPcmHandle, hp.get(), format) >= 0)
            {
                mDevice->FmtType = fmt.fmttype;
                break;
            }
        }
    }
    CHECK(snd_pcm_hw_params_set_format(mPcmHandle, hp.get(), format));
    /* set channels (implicitly sets frame bits) */
    if(snd_pcm_hw_params_set_channels(mPcmHandle, hp.get(), mDevice->channelsFromFmt()) < 0)
    {
        uint numchans{2u};
        CHECK(snd_pcm_hw_params_set_channels_near(mPcmHandle, hp.get(), &numchans));
        if(numchans < 1)
            throw al::backend_exception{al::backend_error::DeviceError, "Got 0 device channels"};
        if(numchans == 1) mDevice->FmtChans = DevFmtMono;
        else mDevice->FmtChans = DevFmtStereo;
    }
    /* set rate (implicitly constrains period/buffer parameters) */
    if(!GetConfigValueBool(mDevice->DeviceName.c_str(), "alsa", "allow-resampler", 0)
        || !mDevice->Flags.test(FrequencyRequest))
    {
        if(snd_pcm_hw_params_set_rate_resample(mPcmHandle, hp.get(), 0) < 0)
            WARN("Failed to disable ALSA resampler\n");
    }
    else if(snd_pcm_hw_params_set_rate_resample(mPcmHandle, hp.get(), 1) < 0)
        WARN("Failed to enable ALSA resampler\n");
    CHECK(snd_pcm_hw_params_set_rate_near(mPcmHandle, hp.get(), &rate, nullptr));
    /* set period time (implicitly constrains period/buffer parameters) */
    if((err=snd_pcm_hw_params_set_period_time_near(mPcmHandle, hp.get(), &periodLen, nullptr)) < 0)
        ERR("snd_pcm_hw_params_set_period_time_near failed: %s\n", snd_strerror(err));
    /* set buffer time (implicitly sets buffer size/bytes/time and period size/bytes) */
    if((err=snd_pcm_hw_params_set_buffer_time_near(mPcmHandle, hp.get(), &bufferLen, nullptr)) < 0)
        ERR("snd_pcm_hw_params_set_buffer_time_near failed: %s\n", snd_strerror(err));
    /* install and prepare hardware configuration */
    CHECK(snd_pcm_hw_params(mPcmHandle, hp.get()));

    /* retrieve configuration info */
    snd_pcm_uframes_t periodSizeInFrames{};
    snd_pcm_uframes_t bufferSizeInFrames{};
    snd_pcm_access_t access{};

    CHECK(snd_pcm_hw_params_get_access(hp.get(), &access));
    CHECK(snd_pcm_hw_params_get_period_size(hp.get(), &periodSizeInFrames, nullptr));
    CHECK(snd_pcm_hw_params_get_buffer_size(hp.get(), &bufferSizeInFrames));
    CHECK(snd_pcm_hw_params_get_channels(hp.get(), &mFrameStep));
    hp = nullptr;

    SwParamsPtr sp{CreateSwParams()};
    CHECK(snd_pcm_sw_params_current(mPcmHandle, sp.get()));
    CHECK(snd_pcm_sw_params_set_avail_min(mPcmHandle, sp.get(), periodSizeInFrames));
    CHECK(snd_pcm_sw_params_set_stop_threshold(mPcmHandle, sp.get(), bufferSizeInFrames));
    CHECK(snd_pcm_sw_params(mPcmHandle, sp.get()));
#undef CHECK
    sp = nullptr;

    mDevice->BufferSize = static_cast<uint>(bufferSizeInFrames);
    mDevice->UpdateSize = static_cast<uint>(periodSizeInFrames);
    mDevice->Frequency = rate;

    setDefaultChannelOrder();

    return true;
}

void AlsaPlayback::start()
{
    int err{};
    snd_pcm_access_t access{};
    HwParamsPtr hp{CreateHwParams()};
#define CHECK(x) do {                                                         \
    if((err=(x)) < 0)                                                         \
        throw al::backend_exception{al::backend_error::DeviceError, #x " failed: %s", \
            snd_strerror(err)};                                               \
} while(0)
    CHECK(snd_pcm_hw_params_current(mPcmHandle, hp.get()));
    /* retrieve configuration info */
    CHECK(snd_pcm_hw_params_get_access(hp.get(), &access));
    hp = nullptr;

    int (AlsaPlayback::*thread_func)(){};
    if(access == SND_PCM_ACCESS_RW_INTERLEAVED)
    {
        auto datalen = snd_pcm_frames_to_bytes(mPcmHandle, mDevice->UpdateSize);
        mBuffer.resize(static_cast<size_t>(datalen));
        thread_func = &AlsaPlayback::mixerNoMMapProc;
    }
    else
    {
        CHECK(snd_pcm_prepare(mPcmHandle));
        thread_func = &AlsaPlayback::mixerProc;
    }
#undef CHECK

    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(thread_func), this};
    }
    catch(std::exception& e) {
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start mixing thread: %s", e.what()};
    }
}

void AlsaPlayback::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    mBuffer.clear();
    int err{snd_pcm_drop(mPcmHandle)};
    if(err < 0)
        ERR("snd_pcm_drop failed: %s\n", snd_strerror(err));
}

ClockLatency AlsaPlayback::getClockLatency()
{
    ClockLatency ret;

    std::lock_guard<std::mutex> _{mMutex};
    ret.ClockTime = GetDeviceClockTime(mDevice);
    snd_pcm_sframes_t delay{};
    int err{snd_pcm_delay(mPcmHandle, &delay)};
    if(err < 0)
    {
        ERR("Failed to get pcm delay: %s\n", snd_strerror(err));
        delay = 0;
    }
    ret.Latency  = std::chrono::seconds{std::max<snd_pcm_sframes_t>(0, delay)};
    ret.Latency /= mDevice->Frequency;

    return ret;
}


struct AlsaCapture final : public BackendBase {
    AlsaCapture(DeviceBase *device) noexcept : BackendBase{device} { }
    ~AlsaCapture() override;

    void open(const char *name) override;
    void start() override;
    void stop() override;
    void captureSamples(al::byte *buffer, uint samples) override;
    uint availableSamples() override;
    ClockLatency getClockLatency() override;

    snd_pcm_t *mPcmHandle{nullptr};

    al::vector<al::byte> mBuffer;

    bool mDoCapture{false};
    RingBufferPtr mRing{nullptr};

    snd_pcm_sframes_t mLastAvail{0};

    DEF_NEWDEL(AlsaCapture)
};

AlsaCapture::~AlsaCapture()
{
    if(mPcmHandle)
        snd_pcm_close(mPcmHandle);
    mPcmHandle = nullptr;
}


void AlsaCapture::open(const char *name)
{
    al::optional<std::string> driveropt;
    const char *driver{"default"};
    if(name)
    {
        if(CaptureDevices.empty())
            CaptureDevices = probe_devices(SND_PCM_STREAM_CAPTURE);

        auto iter = std::find_if(CaptureDevices.cbegin(), CaptureDevices.cend(),
            [name](const DevMap &entry) -> bool { return entry.name == name; });
        if(iter == CaptureDevices.cend())
            throw al::backend_exception{al::backend_error::NoDevice,
                "Device name \"%s\" not found", name};
        driver = iter->device_name.c_str();
    }
    else
    {
        name = alsaDevice;
        if(bool{driveropt = ConfigValueStr(nullptr, "alsa", "capture")})
            driver = driveropt->c_str();
    }

    TRACE("Opening device \"%s\"\n", driver);
    int err{snd_pcm_open(&mPcmHandle, driver, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)};
    if(err < 0)
        throw al::backend_exception{al::backend_error::NoDevice,
            "Could not open ALSA device \"%s\"", driver};

    /* Free alsa's global config tree. Otherwise valgrind reports a ton of leaks. */
    snd_config_update_free_global();

    snd_pcm_format_t format{SND_PCM_FORMAT_UNKNOWN};
    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        format = SND_PCM_FORMAT_S8;
        break;
    case DevFmtUByte:
        format = SND_PCM_FORMAT_U8;
        break;
    case DevFmtShort:
        format = SND_PCM_FORMAT_S16;
        break;
    case DevFmtUShort:
        format = SND_PCM_FORMAT_U16;
        break;
    case DevFmtInt:
        format = SND_PCM_FORMAT_S32;
        break;
    case DevFmtUInt:
        format = SND_PCM_FORMAT_U32;
        break;
    case DevFmtFloat:
        format = SND_PCM_FORMAT_FLOAT;
        break;
    }

    snd_pcm_uframes_t bufferSizeInFrames{maxu(mDevice->BufferSize, 100*mDevice->Frequency/1000)};
    snd_pcm_uframes_t periodSizeInFrames{minu(mDevice->BufferSize, 25*mDevice->Frequency/1000)};

    bool needring{false};
    HwParamsPtr hp{CreateHwParams()};
#define CHECK(x) do {                                                         \
    if((err=(x)) < 0)                                                         \
        throw al::backend_exception{al::backend_error::DeviceError, #x " failed: %s", \
            snd_strerror(err)};                                               \
} while(0)
    CHECK(snd_pcm_hw_params_any(mPcmHandle, hp.get()));
    /* set interleaved access */
    CHECK(snd_pcm_hw_params_set_access(mPcmHandle, hp.get(), SND_PCM_ACCESS_RW_INTERLEAVED));
    /* set format (implicitly sets sample bits) */
    CHECK(snd_pcm_hw_params_set_format(mPcmHandle, hp.get(), format));
    /* set channels (implicitly sets frame bits) */
    CHECK(snd_pcm_hw_params_set_channels(mPcmHandle, hp.get(), mDevice->channelsFromFmt()));
    /* set rate (implicitly constrains period/buffer parameters) */
    CHECK(snd_pcm_hw_params_set_rate(mPcmHandle, hp.get(), mDevice->Frequency, 0));
    /* set buffer size in frame units (implicitly sets period size/bytes/time and buffer time/bytes) */
    if(snd_pcm_hw_params_set_buffer_size_min(mPcmHandle, hp.get(), &bufferSizeInFrames) < 0)
    {
        TRACE("Buffer too large, using intermediate ring buffer\n");
        needring = true;
        CHECK(snd_pcm_hw_params_set_buffer_size_near(mPcmHandle, hp.get(), &bufferSizeInFrames));
    }
    /* set buffer size in frame units (implicitly sets period size/bytes/time and buffer time/bytes) */
    CHECK(snd_pcm_hw_params_set_period_size_near(mPcmHandle, hp.get(), &periodSizeInFrames, nullptr));
    /* install and prepare hardware configuration */
    CHECK(snd_pcm_hw_params(mPcmHandle, hp.get()));
    /* retrieve configuration info */
    CHECK(snd_pcm_hw_params_get_period_size(hp.get(), &periodSizeInFrames, nullptr));
#undef CHECK
    hp = nullptr;

    if(needring)
        mRing = RingBuffer::Create(mDevice->BufferSize, mDevice->frameSizeFromFmt(), false);

    mDevice->DeviceName = name;
}


void AlsaCapture::start()
{
    int err{snd_pcm_prepare(mPcmHandle)};
    if(err < 0)
        throw al::backend_exception{al::backend_error::DeviceError, "snd_pcm_prepare failed: %s",
            snd_strerror(err)};

    err = snd_pcm_start(mPcmHandle);
    if(err < 0)
        throw al::backend_exception{al::backend_error::DeviceError, "snd_pcm_start failed: %s",
            snd_strerror(err)};

    mDoCapture = true;
}

void AlsaCapture::stop()
{
    /* OpenAL requires access to unread audio after stopping, but ALSA's
     * snd_pcm_drain is unreliable and snd_pcm_drop drops it. Capture what's
     * available now so it'll be available later after the drop.
     */
    uint avail{availableSamples()};
    if(!mRing && avail > 0)
    {
        /* The ring buffer implicitly captures when checking availability.
         * Direct access needs to explicitly capture it into temp storage.
         */
        auto temp = al::vector<al::byte>(
            static_cast<size_t>(snd_pcm_frames_to_bytes(mPcmHandle, avail)));
        captureSamples(temp.data(), avail);
        mBuffer = std::move(temp);
    }
    int err{snd_pcm_drop(mPcmHandle)};
    if(err < 0)
        ERR("drop failed: %s\n", snd_strerror(err));
    mDoCapture = false;
}

void AlsaCapture::captureSamples(al::byte *buffer, uint samples)
{
    if(mRing)
    {
        mRing->read(buffer, samples);
        return;
    }

    mLastAvail -= samples;
    while(mDevice->Connected.load(std::memory_order_acquire) && samples > 0)
    {
        snd_pcm_sframes_t amt{0};

        if(!mBuffer.empty())
        {
            /* First get any data stored from the last stop */
            amt = snd_pcm_bytes_to_frames(mPcmHandle, static_cast<ssize_t>(mBuffer.size()));
            if(static_cast<snd_pcm_uframes_t>(amt) > samples) amt = samples;

            amt = snd_pcm_frames_to_bytes(mPcmHandle, amt);
            std::copy_n(mBuffer.begin(), amt, buffer);

            mBuffer.erase(mBuffer.begin(), mBuffer.begin()+amt);
            amt = snd_pcm_bytes_to_frames(mPcmHandle, amt);
        }
        else if(mDoCapture)
            amt = snd_pcm_readi(mPcmHandle, buffer, samples);
        if(amt < 0)
        {
            ERR("read error: %s\n", snd_strerror(static_cast<int>(amt)));

            if(amt == -EAGAIN)
                continue;
            if((amt=snd_pcm_recover(mPcmHandle, static_cast<int>(amt), 1)) >= 0)
            {
                amt = snd_pcm_start(mPcmHandle);
                if(amt >= 0)
                    amt = snd_pcm_avail_update(mPcmHandle);
            }
            if(amt < 0)
            {
                const char *err{snd_strerror(static_cast<int>(amt))};
                ERR("restore error: %s\n", err);
                mDevice->handleDisconnect("Capture recovery failure: %s", err);
                break;
            }
            /* If the amount available is less than what's asked, we lost it
             * during recovery. So just give silence instead. */
            if(static_cast<snd_pcm_uframes_t>(amt) < samples)
                break;
            continue;
        }

        buffer = buffer + amt;
        samples -= static_cast<uint>(amt);
    }
    if(samples > 0)
        std::fill_n(buffer, snd_pcm_frames_to_bytes(mPcmHandle, samples),
            al::byte((mDevice->FmtType == DevFmtUByte) ? 0x80 : 0));
}

uint AlsaCapture::availableSamples()
{
    snd_pcm_sframes_t avail{0};
    if(mDevice->Connected.load(std::memory_order_acquire) && mDoCapture)
        avail = snd_pcm_avail_update(mPcmHandle);
    if(avail < 0)
    {
        ERR("avail update failed: %s\n", snd_strerror(static_cast<int>(avail)));

        if((avail=snd_pcm_recover(mPcmHandle, static_cast<int>(avail), 1)) >= 0)
        {
            if(mDoCapture)
                avail = snd_pcm_start(mPcmHandle);
            if(avail >= 0)
                avail = snd_pcm_avail_update(mPcmHandle);
        }
        if(avail < 0)
        {
            const char *err{snd_strerror(static_cast<int>(avail))};
            ERR("restore error: %s\n", err);
            mDevice->handleDisconnect("Capture recovery failure: %s", err);
        }
    }

    if(!mRing)
    {
        if(avail < 0) avail = 0;
        avail += snd_pcm_bytes_to_frames(mPcmHandle, static_cast<ssize_t>(mBuffer.size()));
        if(avail > mLastAvail) mLastAvail = avail;
        return static_cast<uint>(mLastAvail);
    }

    while(avail > 0)
    {
        auto vec = mRing->getWriteVector();
        if(vec.first.len == 0) break;

        snd_pcm_sframes_t amt{std::min(static_cast<snd_pcm_sframes_t>(vec.first.len), avail)};
        amt = snd_pcm_readi(mPcmHandle, vec.first.buf, static_cast<snd_pcm_uframes_t>(amt));
        if(amt < 0)
        {
            ERR("read error: %s\n", snd_strerror(static_cast<int>(amt)));

            if(amt == -EAGAIN)
                continue;
            if((amt=snd_pcm_recover(mPcmHandle, static_cast<int>(amt), 1)) >= 0)
            {
                if(mDoCapture)
                    amt = snd_pcm_start(mPcmHandle);
                if(amt >= 0)
                    amt = snd_pcm_avail_update(mPcmHandle);
            }
            if(amt < 0)
            {
                const char *err{snd_strerror(static_cast<int>(amt))};
                ERR("restore error: %s\n", err);
                mDevice->handleDisconnect("Capture recovery failure: %s", err);
                break;
            }
            avail = amt;
            continue;
        }

        mRing->writeAdvance(static_cast<snd_pcm_uframes_t>(amt));
        avail -= amt;
    }

    return static_cast<uint>(mRing->readSpace());
}

ClockLatency AlsaCapture::getClockLatency()
{
    ClockLatency ret;

    ret.ClockTime = GetDeviceClockTime(mDevice);
    snd_pcm_sframes_t delay{};
    int err{snd_pcm_delay(mPcmHandle, &delay)};
    if(err < 0)
    {
        ERR("Failed to get pcm delay: %s\n", snd_strerror(err));
        delay = 0;
    }
    ret.Latency  = std::chrono::seconds{std::max<snd_pcm_sframes_t>(0, delay)};
    ret.Latency /= mDevice->Frequency;

    return ret;
}

} // namespace


bool AlsaBackendFactory::init()
{
    bool error{false};

#ifdef HAVE_DYNLOAD
    if(!alsa_handle)
    {
        std::string missing_funcs;

        alsa_handle = LoadLib("libasound.so.2");
        if(!alsa_handle)
        {
            WARN("Failed to load %s\n", "libasound.so.2");
            return false;
        }

        error = false;
#define LOAD_FUNC(f) do {                                                     \
    p##f = reinterpret_cast<decltype(p##f)>(GetSymbol(alsa_handle, #f));      \
    if(p##f == nullptr) {                                                     \
        error = true;                                                         \
        missing_funcs += "\n" #f;                                             \
    }                                                                         \
} while(0)
        ALSA_FUNCS(LOAD_FUNC);
#undef LOAD_FUNC

        if(error)
        {
            WARN("Missing expected functions:%s\n", missing_funcs.c_str());
            CloseLib(alsa_handle);
            alsa_handle = nullptr;
        }
    }
#endif

    return !error;
}

bool AlsaBackendFactory::querySupport(BackendType type)
{ return (type == BackendType::Playback || type == BackendType::Capture); }

std::string AlsaBackendFactory::probe(BackendType type)
{
    std::string outnames;

    auto add_device = [&outnames](const DevMap &entry) -> void
    {
        /* +1 to also append the null char (to ensure a null-separated list and
         * double-null terminated list).
         */
        outnames.append(entry.name.c_str(), entry.name.length()+1);
    };
    switch(type)
    {
    case BackendType::Playback:
        PlaybackDevices = probe_devices(SND_PCM_STREAM_PLAYBACK);
        std::for_each(PlaybackDevices.cbegin(), PlaybackDevices.cend(), add_device);
        break;

    case BackendType::Capture:
        CaptureDevices = probe_devices(SND_PCM_STREAM_CAPTURE);
        std::for_each(CaptureDevices.cbegin(), CaptureDevices.cend(), add_device);
        break;
    }

    return outnames;
}

BackendPtr AlsaBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new AlsaPlayback{device}};
    if(type == BackendType::Capture)
        return BackendPtr{new AlsaCapture{device}};
    return nullptr;
}

BackendFactory &AlsaBackendFactory::getFactory()
{
    static AlsaBackendFactory factory{};
    return factory;
}
