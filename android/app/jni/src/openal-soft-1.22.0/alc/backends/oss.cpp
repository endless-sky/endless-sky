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

#include "oss.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <new>
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
#include "ringbuffer.h"
#include "threads.h"
#include "vector.h"

#include <sys/soundcard.h>

/*
 * The OSS documentation talks about SOUND_MIXER_READ, but the header
 * only contains MIXER_READ. Play safe. Same for WRITE.
 */
#ifndef SOUND_MIXER_READ
#define SOUND_MIXER_READ MIXER_READ
#endif
#ifndef SOUND_MIXER_WRITE
#define SOUND_MIXER_WRITE MIXER_WRITE
#endif

#if defined(SOUND_VERSION) && (SOUND_VERSION < 0x040000)
#define ALC_OSS_COMPAT
#endif
#ifndef SNDCTL_AUDIOINFO
#define ALC_OSS_COMPAT
#endif

/*
 * FreeBSD strongly discourages the use of specific devices,
 * such as those returned in oss_audioinfo.devnode
 */
#ifdef __FreeBSD__
#define ALC_OSS_DEVNODE_TRUC
#endif

namespace {

constexpr char DefaultName[] = "OSS Default";
std::string DefaultPlayback{"/dev/dsp"};
std::string DefaultCapture{"/dev/dsp"};

struct DevMap {
    std::string name;
    std::string device_name;
};

al::vector<DevMap> PlaybackDevices;
al::vector<DevMap> CaptureDevices;


#ifdef ALC_OSS_COMPAT

#define DSP_CAP_OUTPUT 0x00020000
#define DSP_CAP_INPUT 0x00010000
void ALCossListPopulate(al::vector<DevMap> &devlist, int type)
{
    devlist.emplace_back(DevMap{DefaultName, (type==DSP_CAP_INPUT) ? DefaultCapture : DefaultPlayback});
}

#else

void ALCossListAppend(al::vector<DevMap> &list, al::span<const char> handle, al::span<const char> path)
{
#ifdef ALC_OSS_DEVNODE_TRUC
    for(size_t i{0};i < path.size();++i)
    {
        if(path[i] == '.' && handle.size() + i >= path.size())
        {
            const size_t hoffset{handle.size() + i - path.size()};
            if(strncmp(path.data() + i, handle.data() + hoffset, path.size() - i) == 0)
                handle = handle.first(hoffset);
            path = path.first(i);
        }
    }
#endif
    if(handle.empty())
        handle = path;

    std::string basename{handle.data(), handle.size()};
    std::string devname{path.data(), path.size()};

    auto match_devname = [&devname](const DevMap &entry) -> bool
    { return entry.device_name == devname; };
    if(std::find_if(list.cbegin(), list.cend(), match_devname) != list.cend())
        return;

    auto checkName = [&list](const std::string &name) -> bool
    {
        auto match_name = [&name](const DevMap &entry) -> bool { return entry.name == name; };
        return std::find_if(list.cbegin(), list.cend(), match_name) != list.cend();
    };
    int count{1};
    std::string newname{basename};
    while(checkName(newname))
    {
        newname = basename;
        newname += " #";
        newname += std::to_string(++count);
    }

    list.emplace_back(DevMap{std::move(newname), std::move(devname)});
    const DevMap &entry = list.back();

    TRACE("Got device \"%s\", \"%s\"\n", entry.name.c_str(), entry.device_name.c_str());
}

void ALCossListPopulate(al::vector<DevMap> &devlist, int type_flag)
{
    int fd{open("/dev/mixer", O_RDONLY)};
    if(fd < 0)
    {
        TRACE("Could not open /dev/mixer: %s\n", strerror(errno));
        goto done;
    }

    oss_sysinfo si;
    if(ioctl(fd, SNDCTL_SYSINFO, &si) == -1)
    {
        TRACE("SNDCTL_SYSINFO failed: %s\n", strerror(errno));
        goto done;
    }

    for(int i{0};i < si.numaudios;i++)
    {
        oss_audioinfo ai;
        ai.dev = i;
        if(ioctl(fd, SNDCTL_AUDIOINFO, &ai) == -1)
        {
            ERR("SNDCTL_AUDIOINFO (%d) failed: %s\n", i, strerror(errno));
            continue;
        }
        if(!(ai.caps&type_flag) || ai.devnode[0] == '\0')
            continue;

        al::span<const char> handle;
        if(ai.handle[0] != '\0')
            handle = {ai.handle, strnlen(ai.handle, sizeof(ai.handle))};
        else
            handle = {ai.name, strnlen(ai.name, sizeof(ai.name))};
        al::span<const char> devnode{ai.devnode, strnlen(ai.devnode, sizeof(ai.devnode))};

        ALCossListAppend(devlist, handle, devnode);
    }

done:
    if(fd >= 0)
        close(fd);
    fd = -1;

    const char *defdev{((type_flag==DSP_CAP_INPUT) ? DefaultCapture : DefaultPlayback).c_str()};
    auto iter = std::find_if(devlist.cbegin(), devlist.cend(),
        [defdev](const DevMap &entry) -> bool
        { return entry.device_name == defdev; }
    );
    if(iter == devlist.cend())
        devlist.insert(devlist.begin(), DevMap{DefaultName, defdev});
    else
    {
        DevMap entry{std::move(*iter)};
        devlist.erase(iter);
        devlist.insert(devlist.begin(), std::move(entry));
    }
    devlist.shrink_to_fit();
}

#endif

uint log2i(uint x)
{
    uint y{0};
    while(x > 1)
    {
        x >>= 1;
        y++;
    }
    return y;
}


struct OSSPlayback final : public BackendBase {
    OSSPlayback(DeviceBase *device) noexcept : BackendBase{device} { }
    ~OSSPlayback() override;

    int mixerProc();

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;

    int mFd{-1};

    al::vector<al::byte> mMixData;

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(OSSPlayback)
};

OSSPlayback::~OSSPlayback()
{
    if(mFd != -1)
        ::close(mFd);
    mFd = -1;
}


int OSSPlayback::mixerProc()
{
    SetRTPriority();
    althrd_setname(MIXER_THREAD_NAME);

    const size_t frame_step{mDevice->channelsFromFmt()};
    const size_t frame_size{mDevice->frameSizeFromFmt()};

    while(!mKillNow.load(std::memory_order_acquire)
        && mDevice->Connected.load(std::memory_order_acquire))
    {
        pollfd pollitem{};
        pollitem.fd = mFd;
        pollitem.events = POLLOUT;

        int pret{poll(&pollitem, 1, 1000)};
        if(pret < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            ERR("poll failed: %s\n", strerror(errno));
            mDevice->handleDisconnect("Failed waiting for playback buffer: %s", strerror(errno));
            break;
        }
        else if(pret == 0)
        {
            WARN("poll timeout\n");
            continue;
        }

        al::byte *write_ptr{mMixData.data()};
        size_t to_write{mMixData.size()};
        mDevice->renderSamples(write_ptr, static_cast<uint>(to_write/frame_size), frame_step);
        while(to_write > 0 && !mKillNow.load(std::memory_order_acquire))
        {
            ssize_t wrote{write(mFd, write_ptr, to_write)};
            if(wrote < 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                    continue;
                ERR("write failed: %s\n", strerror(errno));
                mDevice->handleDisconnect("Failed writing playback samples: %s", strerror(errno));
                break;
            }

            to_write -= static_cast<size_t>(wrote);
            write_ptr += wrote;
        }
    }

    return 0;
}


void OSSPlayback::open(const char *name)
{
    const char *devname{DefaultPlayback.c_str()};
    if(!name)
        name = DefaultName;
    else
    {
        if(PlaybackDevices.empty())
            ALCossListPopulate(PlaybackDevices, DSP_CAP_OUTPUT);

        auto iter = std::find_if(PlaybackDevices.cbegin(), PlaybackDevices.cend(),
            [&name](const DevMap &entry) -> bool
            { return entry.name == name; }
        );
        if(iter == PlaybackDevices.cend())
            throw al::backend_exception{al::backend_error::NoDevice,
                "Device name \"%s\" not found", name};
        devname = iter->device_name.c_str();
    }

    int fd{::open(devname, O_WRONLY)};
    if(fd == -1)
        throw al::backend_exception{al::backend_error::NoDevice, "Could not open %s: %s", devname,
            strerror(errno)};

    if(mFd != -1)
        ::close(mFd);
    mFd = fd;

    mDevice->DeviceName = name;
}

bool OSSPlayback::reset()
{
    int ossFormat{};
    switch(mDevice->FmtType)
    {
        case DevFmtByte:
            ossFormat = AFMT_S8;
            break;
        case DevFmtUByte:
            ossFormat = AFMT_U8;
            break;
        case DevFmtUShort:
        case DevFmtInt:
        case DevFmtUInt:
        case DevFmtFloat:
            mDevice->FmtType = DevFmtShort;
            /* fall-through */
        case DevFmtShort:
            ossFormat = AFMT_S16_NE;
            break;
    }

    uint periods{mDevice->BufferSize / mDevice->UpdateSize};
    uint numChannels{mDevice->channelsFromFmt()};
    uint ossSpeed{mDevice->Frequency};
    uint frameSize{numChannels * mDevice->bytesFromFmt()};
    /* According to the OSS spec, 16 bytes (log2(16)) is the minimum. */
    uint log2FragmentSize{maxu(log2i(mDevice->UpdateSize*frameSize), 4)};
    uint numFragmentsLogSize{(periods << 16) | log2FragmentSize};

    audio_buf_info info{};
    const char *err;
#define CHECKERR(func) if((func) < 0) {                                       \
    err = #func;                                                              \
    goto err;                                                                 \
}
    /* Don't fail if SETFRAGMENT fails. We can handle just about anything
     * that's reported back via GETOSPACE */
    ioctl(mFd, SNDCTL_DSP_SETFRAGMENT, &numFragmentsLogSize);
    CHECKERR(ioctl(mFd, SNDCTL_DSP_SETFMT, &ossFormat));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_CHANNELS, &numChannels));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_SPEED, &ossSpeed));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_GETOSPACE, &info));
    if(0)
    {
    err:
        ERR("%s failed: %s\n", err, strerror(errno));
        return false;
    }
#undef CHECKERR

    if(mDevice->channelsFromFmt() != numChannels)
    {
        ERR("Failed to set %s, got %d channels instead\n", DevFmtChannelsString(mDevice->FmtChans),
            numChannels);
        return false;
    }

    if(!((ossFormat == AFMT_S8 && mDevice->FmtType == DevFmtByte) ||
         (ossFormat == AFMT_U8 && mDevice->FmtType == DevFmtUByte) ||
         (ossFormat == AFMT_S16_NE && mDevice->FmtType == DevFmtShort)))
    {
        ERR("Failed to set %s samples, got OSS format %#x\n", DevFmtTypeString(mDevice->FmtType),
            ossFormat);
        return false;
    }

    mDevice->Frequency = ossSpeed;
    mDevice->UpdateSize = static_cast<uint>(info.fragsize) / frameSize;
    mDevice->BufferSize = static_cast<uint>(info.fragments) * mDevice->UpdateSize;

    setDefaultChannelOrder();

    mMixData.resize(mDevice->UpdateSize * mDevice->frameSizeFromFmt());

    return true;
}

void OSSPlayback::start()
{
    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(&OSSPlayback::mixerProc), this};
    }
    catch(std::exception& e) {
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start mixing thread: %s", e.what()};
    }
}

void OSSPlayback::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    if(ioctl(mFd, SNDCTL_DSP_RESET) != 0)
        ERR("Error resetting device: %s\n", strerror(errno));
}


struct OSScapture final : public BackendBase {
    OSScapture(DeviceBase *device) noexcept : BackendBase{device} { }
    ~OSScapture() override;

    int recordProc();

    void open(const char *name) override;
    void start() override;
    void stop() override;
    void captureSamples(al::byte *buffer, uint samples) override;
    uint availableSamples() override;

    int mFd{-1};

    RingBufferPtr mRing{nullptr};

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(OSScapture)
};

OSScapture::~OSScapture()
{
    if(mFd != -1)
        close(mFd);
    mFd = -1;
}


int OSScapture::recordProc()
{
    SetRTPriority();
    althrd_setname(RECORD_THREAD_NAME);

    const size_t frame_size{mDevice->frameSizeFromFmt()};
    while(!mKillNow.load(std::memory_order_acquire))
    {
        pollfd pollitem{};
        pollitem.fd = mFd;
        pollitem.events = POLLIN;

        int sret{poll(&pollitem, 1, 1000)};
        if(sret < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            ERR("poll failed: %s\n", strerror(errno));
            mDevice->handleDisconnect("Failed to check capture samples: %s", strerror(errno));
            break;
        }
        else if(sret == 0)
        {
            WARN("poll timeout\n");
            continue;
        }

        auto vec = mRing->getWriteVector();
        if(vec.first.len > 0)
        {
            ssize_t amt{read(mFd, vec.first.buf, vec.first.len*frame_size)};
            if(amt < 0)
            {
                ERR("read failed: %s\n", strerror(errno));
                mDevice->handleDisconnect("Failed reading capture samples: %s", strerror(errno));
                break;
            }
            mRing->writeAdvance(static_cast<size_t>(amt)/frame_size);
        }
    }

    return 0;
}


void OSScapture::open(const char *name)
{
    const char *devname{DefaultCapture.c_str()};
    if(!name)
        name = DefaultName;
    else
    {
        if(CaptureDevices.empty())
            ALCossListPopulate(CaptureDevices, DSP_CAP_INPUT);

        auto iter = std::find_if(CaptureDevices.cbegin(), CaptureDevices.cend(),
            [&name](const DevMap &entry) -> bool
            { return entry.name == name; }
        );
        if(iter == CaptureDevices.cend())
            throw al::backend_exception{al::backend_error::NoDevice,
                "Device name \"%s\" not found", name};
        devname = iter->device_name.c_str();
    }

    mFd = ::open(devname, O_RDONLY);
    if(mFd == -1)
        throw al::backend_exception{al::backend_error::NoDevice, "Could not open %s: %s", devname,
            strerror(errno)};

    int ossFormat{};
    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        ossFormat = AFMT_S8;
        break;
    case DevFmtUByte:
        ossFormat = AFMT_U8;
        break;
    case DevFmtShort:
        ossFormat = AFMT_S16_NE;
        break;
    case DevFmtUShort:
    case DevFmtInt:
    case DevFmtUInt:
    case DevFmtFloat:
        throw al::backend_exception{al::backend_error::DeviceError,
            "%s capture samples not supported", DevFmtTypeString(mDevice->FmtType)};
    }

    uint periods{4};
    uint numChannels{mDevice->channelsFromFmt()};
    uint frameSize{numChannels * mDevice->bytesFromFmt()};
    uint ossSpeed{mDevice->Frequency};
    /* according to the OSS spec, 16 bytes are the minimum */
    uint log2FragmentSize{maxu(log2i(mDevice->BufferSize * frameSize / periods), 4)};
    uint numFragmentsLogSize{(periods << 16) | log2FragmentSize};

    audio_buf_info info{};
#define CHECKERR(func) if((func) < 0) {                                       \
    throw al::backend_exception{al::backend_error::DeviceError, #func " failed: %s", \
        strerror(errno)};                                                     \
}
    CHECKERR(ioctl(mFd, SNDCTL_DSP_SETFRAGMENT, &numFragmentsLogSize));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_SETFMT, &ossFormat));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_CHANNELS, &numChannels));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_SPEED, &ossSpeed));
    CHECKERR(ioctl(mFd, SNDCTL_DSP_GETISPACE, &info));
#undef CHECKERR

    if(mDevice->channelsFromFmt() != numChannels)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to set %s, got %d channels instead", DevFmtChannelsString(mDevice->FmtChans),
            numChannels};

    if(!((ossFormat == AFMT_S8 && mDevice->FmtType == DevFmtByte)
        || (ossFormat == AFMT_U8 && mDevice->FmtType == DevFmtUByte)
        || (ossFormat == AFMT_S16_NE && mDevice->FmtType == DevFmtShort)))
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to set %s samples, got OSS format %#x", DevFmtTypeString(mDevice->FmtType),
            ossFormat};

    mRing = RingBuffer::Create(mDevice->BufferSize, frameSize, false);

    mDevice->DeviceName = name;
}

void OSScapture::start()
{
    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(&OSScapture::recordProc), this};
    }
    catch(std::exception& e) {
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start recording thread: %s", e.what()};
    }
}

void OSScapture::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    if(ioctl(mFd, SNDCTL_DSP_RESET) != 0)
        ERR("Error resetting device: %s\n", strerror(errno));
}

void OSScapture::captureSamples(al::byte *buffer, uint samples)
{ mRing->read(buffer, samples); }

uint OSScapture::availableSamples()
{ return static_cast<uint>(mRing->readSpace()); }

} // namespace


BackendFactory &OSSBackendFactory::getFactory()
{
    static OSSBackendFactory factory{};
    return factory;
}

bool OSSBackendFactory::init()
{
    if(auto devopt = ConfigValueStr(nullptr, "oss", "device"))
        DefaultPlayback = std::move(*devopt);
    if(auto capopt = ConfigValueStr(nullptr, "oss", "capture"))
        DefaultCapture = std::move(*capopt);

    return true;
}

bool OSSBackendFactory::querySupport(BackendType type)
{ return (type == BackendType::Playback || type == BackendType::Capture); }

std::string OSSBackendFactory::probe(BackendType type)
{
    std::string outnames;

    auto add_device = [&outnames](const DevMap &entry) -> void
    {
        struct stat buf;
        if(stat(entry.device_name.c_str(), &buf) == 0)
        {
            /* Includes null char. */
            outnames.append(entry.name.c_str(), entry.name.length()+1);
        }
    };

    switch(type)
    {
    case BackendType::Playback:
        PlaybackDevices.clear();
        ALCossListPopulate(PlaybackDevices, DSP_CAP_OUTPUT);
        std::for_each(PlaybackDevices.cbegin(), PlaybackDevices.cend(), add_device);
        break;

    case BackendType::Capture:
        CaptureDevices.clear();
        ALCossListPopulate(CaptureDevices, DSP_CAP_INPUT);
        std::for_each(CaptureDevices.cbegin(), CaptureDevices.cend(), add_device);
        break;
    }

    return outnames;
}

BackendPtr OSSBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new OSSPlayback{device}};
    if(type == BackendType::Capture)
        return BackendPtr{new OSScapture{device}};
    return nullptr;
}
