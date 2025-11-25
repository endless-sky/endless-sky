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

#include "sndio.h"

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <thread>
#include <functional>

#include "alnumeric.h"
#include "core/device.h"
#include "core/helpers.h"
#include "core/logging.h"
#include "ringbuffer.h"
#include "threads.h"
#include "vector.h"

#include <sndio.h>


namespace {

static const char sndio_device[] = "SndIO Default";

struct SioPar : public sio_par {
    SioPar() { sio_initpar(this); }

    void clear() { sio_initpar(this); }
};

struct SndioPlayback final : public BackendBase {
    SndioPlayback(DeviceBase *device) noexcept : BackendBase{device} { }
    ~SndioPlayback() override;

    int mixerProc();

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;

    sio_hdl *mSndHandle{nullptr};
    uint mFrameStep{};

    al::vector<al::byte> mBuffer;

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(SndioPlayback)
};

SndioPlayback::~SndioPlayback()
{
    if(mSndHandle)
        sio_close(mSndHandle);
    mSndHandle = nullptr;
}

int SndioPlayback::mixerProc()
{
    const size_t frameStep{mFrameStep};
    const size_t frameSize{frameStep * mDevice->bytesFromFmt()};

    SetRTPriority();
    althrd_setname(MIXER_THREAD_NAME);

    while(!mKillNow.load(std::memory_order_acquire)
        && mDevice->Connected.load(std::memory_order_acquire))
    {
        al::span<al::byte> buffer{mBuffer};

        mDevice->renderSamples(buffer.data(), static_cast<uint>(buffer.size() / frameSize),
            frameStep);
        while(!buffer.empty() && !mKillNow.load(std::memory_order_acquire))
        {
            size_t wrote{sio_write(mSndHandle, buffer.data(), buffer.size())};
            if(wrote == 0)
            {
                ERR("sio_write failed\n");
                mDevice->handleDisconnect("Failed to write playback samples");
                break;
            }
            buffer = buffer.subspan(wrote);
        }
    }

    return 0;
}


void SndioPlayback::open(const char *name)
{
    if(!name)
        name = sndio_device;
    else if(strcmp(name, sndio_device) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};

    sio_hdl *sndHandle{sio_open(nullptr, SIO_PLAY, 0)};
    if(!sndHandle)
        throw al::backend_exception{al::backend_error::NoDevice, "Could not open backend device"};

    if(mSndHandle)
        sio_close(mSndHandle);
    mSndHandle = sndHandle;

    mDevice->DeviceName = name;
}

bool SndioPlayback::reset()
{
    SioPar par;

    auto tryfmt = mDevice->FmtType;
retry_params:
    switch(tryfmt)
    {
    case DevFmtByte:
        par.bits = 8;
        par.sig = 1;
        break;
    case DevFmtUByte:
        par.bits = 8;
        par.sig = 0;
        break;
    case DevFmtShort:
        par.bits = 16;
        par.sig = 1;
        break;
    case DevFmtUShort:
        par.bits = 16;
        par.sig = 0;
        break;
    case DevFmtFloat:
    case DevFmtInt:
        par.bits = 32;
        par.sig = 1;
        break;
    case DevFmtUInt:
        par.bits = 32;
        par.sig = 0;
        break;
    }
    par.bps = SIO_BPS(par.bits);
    par.le = SIO_LE_NATIVE;
    par.msb = 1;

    par.rate = mDevice->Frequency;
    par.pchan = mDevice->channelsFromFmt();

    par.round = mDevice->UpdateSize;
    par.appbufsz = mDevice->BufferSize - mDevice->UpdateSize;
    if(!par.appbufsz) par.appbufsz = mDevice->UpdateSize;

    try {
        if(!sio_setpar(mSndHandle, &par))
            throw al::backend_exception{al::backend_error::DeviceError,
                "Failed to set device parameters"};

        par.clear();
        if(!sio_getpar(mSndHandle, &par))
            throw al::backend_exception{al::backend_error::DeviceError,
                "Failed to get device parameters"};

        if(par.bps > 1 && par.le != SIO_LE_NATIVE)
            throw al::backend_exception{al::backend_error::DeviceError,
                "%s-endian samples not supported", par.le ? "Little" : "Big"};
        if(par.bits < par.bps*8 && !par.msb)
            throw al::backend_exception{al::backend_error::DeviceError,
                "MSB-padded samples not supported (%u of %u bits)", par.bits, par.bps*8};
        if(par.pchan < 1)
            throw al::backend_exception{al::backend_error::DeviceError,
                "No playback channels on device"};
    }
    catch(al::backend_exception &e) {
        if(tryfmt == DevFmtShort)
            throw;
        par.clear();
        tryfmt = DevFmtShort;
        goto retry_params;
    }

    if(par.bps == 1)
        mDevice->FmtType = (par.sig==1) ? DevFmtByte : DevFmtUByte;
    else if(par.bps == 2)
        mDevice->FmtType = (par.sig==1) ? DevFmtShort : DevFmtUShort;
    else if(par.bps == 4)
        mDevice->FmtType = (par.sig==1) ? DevFmtInt : DevFmtUInt;
    else
        throw al::backend_exception{al::backend_error::DeviceError,
            "Unhandled sample format: %s %u-bit", (par.sig?"signed":"unsigned"), par.bps*8};

    mFrameStep = par.pchan;
    if(par.pchan != mDevice->channelsFromFmt())
    {
        WARN("Got %u channel%s for %s\n", par.pchan, (par.pchan==1)?"":"s",
            DevFmtChannelsString(mDevice->FmtChans));
        if(par.pchan < 2) mDevice->FmtChans = DevFmtMono;
        else mDevice->FmtChans = DevFmtStereo;
    }
    mDevice->Frequency = par.rate;

    setDefaultChannelOrder();

    mDevice->UpdateSize = par.round;
    mDevice->BufferSize = par.bufsz + par.round;

    mBuffer.resize(mDevice->UpdateSize * par.pchan*par.bps);
    if(par.sig == 1)
        std::fill(mBuffer.begin(), mBuffer.end(), al::byte{});
    else if(par.bits == 8)
        std::fill_n(mBuffer.data(), mBuffer.size(), al::byte(0x80));
    else if(par.bits == 16)
        std::fill_n(reinterpret_cast<uint16_t*>(mBuffer.data()), mBuffer.size()/2, 0x8000);
    else if(par.bits == 32)
        std::fill_n(reinterpret_cast<uint32_t*>(mBuffer.data()), mBuffer.size()/4, 0x80000000u);

    return true;
}

void SndioPlayback::start()
{
    if(!sio_start(mSndHandle))
        throw al::backend_exception{al::backend_error::DeviceError, "Error starting playback"};

    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(&SndioPlayback::mixerProc), this};
    }
    catch(std::exception& e) {
        sio_stop(mSndHandle);
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start mixing thread: %s", e.what()};
    }
}

void SndioPlayback::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    if(!sio_stop(mSndHandle))
        ERR("Error stopping device\n");
}


/* TODO: This could be improved by avoiding the ring buffer and record thread,
 * counting the available samples with the sio_onmove callback and reading
 * directly from the device. However, this depends on reasonable support for
 * capture buffer sizes apps may request.
 */
struct SndioCapture final : public BackendBase {
    SndioCapture(DeviceBase *device) noexcept : BackendBase{device} { }
    ~SndioCapture() override;

    int recordProc();

    void open(const char *name) override;
    void start() override;
    void stop() override;
    void captureSamples(al::byte *buffer, uint samples) override;
    uint availableSamples() override;

    sio_hdl *mSndHandle{nullptr};

    RingBufferPtr mRing;

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(SndioCapture)
};

SndioCapture::~SndioCapture()
{
    if(mSndHandle)
        sio_close(mSndHandle);
    mSndHandle = nullptr;
}

int SndioCapture::recordProc()
{
    SetRTPriority();
    althrd_setname(RECORD_THREAD_NAME);

    const uint frameSize{mDevice->frameSizeFromFmt()};

    int nfds_pre{sio_nfds(mSndHandle)};
    if(nfds_pre <= 0)
    {
        mDevice->handleDisconnect("Incorrect return value from sio_nfds(): %d", nfds_pre);
        return 1;
    }

    auto fds = std::make_unique<pollfd[]>(static_cast<uint>(nfds_pre));

    while(!mKillNow.load(std::memory_order_acquire)
        && mDevice->Connected.load(std::memory_order_acquire))
    {
        /* Wait until there's some samples to read. */
        const int nfds{sio_pollfd(mSndHandle, fds.get(), POLLIN)};
        if(nfds <= 0)
        {
            mDevice->handleDisconnect("Failed to get polling fds: %d", nfds);
            break;
        }
        int pollres{::poll(fds.get(), static_cast<uint>(nfds), 2000)};
        if(pollres < 0)
        {
            if(errno == EINTR) continue;
            mDevice->handleDisconnect("Poll error: %s", strerror(errno));
            break;
        }
        if(pollres == 0)
            continue;

        const int revents{sio_revents(mSndHandle, fds.get())};
        if((revents&POLLHUP))
        {
            mDevice->handleDisconnect("Got POLLHUP from poll events");
            break;
        }
        if(!(revents&POLLIN))
            continue;

        auto data = mRing->getWriteVector();
        al::span<al::byte> buffer{data.first.buf, data.first.len*frameSize};
        while(!buffer.empty())
        {
            size_t got{sio_read(mSndHandle, buffer.data(), buffer.size())};
            if(got == 0) break;

            mRing->writeAdvance(got / frameSize);
            buffer = buffer.subspan(got);
            if(buffer.empty())
            {
                data = mRing->getWriteVector();
                buffer = {data.first.buf, data.first.len*frameSize};
            }
        }
        if(buffer.empty())
        {
            /* Got samples to read, but no place to store it. Drop it. */
            static char junk[4096];
            sio_read(mSndHandle, junk, sizeof(junk) - (sizeof(junk)%frameSize));
        }
    }

    return 0;
}


void SndioCapture::open(const char *name)
{
    if(!name)
        name = sndio_device;
    else if(strcmp(name, sndio_device) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};

    mSndHandle = sio_open(nullptr, SIO_REC, true);
    if(mSndHandle == nullptr)
        throw al::backend_exception{al::backend_error::NoDevice, "Could not open backend device"};

    SioPar par;
    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        par.bits = 8;
        par.sig = 1;
        break;
    case DevFmtUByte:
        par.bits = 8;
        par.sig = 0;
        break;
    case DevFmtShort:
        par.bits = 16;
        par.sig = 1;
        break;
    case DevFmtUShort:
        par.bits = 16;
        par.sig = 0;
        break;
    case DevFmtInt:
        par.bits = 32;
        par.sig = 1;
        break;
    case DevFmtUInt:
        par.bits = 32;
        par.sig = 0;
        break;
    case DevFmtFloat:
        throw al::backend_exception{al::backend_error::DeviceError,
            "%s capture samples not supported", DevFmtTypeString(mDevice->FmtType)};
    }
    par.bps = SIO_BPS(par.bits);
    par.le = SIO_LE_NATIVE;
    par.msb = 1;
    par.rchan = mDevice->channelsFromFmt();
    par.rate = mDevice->Frequency;

    par.appbufsz = maxu(mDevice->BufferSize, mDevice->Frequency/10);
    par.round = minu(par.appbufsz/2, mDevice->Frequency/40);

    if(!sio_setpar(mSndHandle, &par) || !sio_getpar(mSndHandle, &par))
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to set device praameters"};

    if(par.bps > 1 && par.le != SIO_LE_NATIVE)
        throw al::backend_exception{al::backend_error::DeviceError,
            "%s-endian samples not supported", par.le ? "Little" : "Big"};
    if(par.bits < par.bps*8 && !par.msb)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Padded samples not supported (got %u of %u bits)", par.bits, par.bps*8};

    auto match_fmt = [](DevFmtType fmttype, const sio_par &p) -> bool
    {
        return (fmttype == DevFmtByte && p.bps == 1 && p.sig != 0)
            || (fmttype == DevFmtUByte && p.bps == 1 && p.sig == 0)
            || (fmttype == DevFmtShort && p.bps == 2 && p.sig != 0)
            || (fmttype == DevFmtUShort && p.bps == 2 && p.sig == 0)
            || (fmttype == DevFmtInt && p.bps == 4 && p.sig != 0)
            || (fmttype == DevFmtUInt && p.bps == 4 && p.sig == 0);
    };
    if(!match_fmt(mDevice->FmtType, par) || mDevice->channelsFromFmt() != par.rchan
        || mDevice->Frequency != par.rate)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to set format %s %s %uhz, got %c%u %u-channel %uhz instead",
            DevFmtTypeString(mDevice->FmtType), DevFmtChannelsString(mDevice->FmtChans),
            mDevice->Frequency, par.sig?'s':'u', par.bps*8, par.rchan, par.rate};

    mRing = RingBuffer::Create(mDevice->BufferSize, par.bps*par.rchan, false);
    mDevice->BufferSize = static_cast<uint>(mRing->writeSpace());
    mDevice->UpdateSize = par.round;

    setDefaultChannelOrder();

    mDevice->DeviceName = name;
}

void SndioCapture::start()
{
    if(!sio_start(mSndHandle))
        throw al::backend_exception{al::backend_error::DeviceError, "Error starting capture"};

    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(&SndioCapture::recordProc), this};
    }
    catch(std::exception& e) {
        sio_stop(mSndHandle);
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start capture thread: %s", e.what()};
    }
}

void SndioCapture::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    if(!sio_stop(mSndHandle))
        ERR("Error stopping device\n");
}

void SndioCapture::captureSamples(al::byte *buffer, uint samples)
{ mRing->read(buffer, samples); }

uint SndioCapture::availableSamples()
{ return static_cast<uint>(mRing->readSpace()); }

} // namespace

BackendFactory &SndIOBackendFactory::getFactory()
{
    static SndIOBackendFactory factory{};
    return factory;
}

bool SndIOBackendFactory::init()
{ return true; }

bool SndIOBackendFactory::querySupport(BackendType type)
{ return (type == BackendType::Playback || type == BackendType::Capture); }

std::string SndIOBackendFactory::probe(BackendType type)
{
    std::string outnames;
    switch(type)
    {
    case BackendType::Playback:
    case BackendType::Capture:
        /* Includes null char. */
        outnames.append(sndio_device, sizeof(sndio_device));
        break;
    }
    return outnames;
}

BackendPtr SndIOBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new SndioPlayback{device}};
    if(type == BackendType::Capture)
        return BackendPtr{new SndioCapture{device}};
    return nullptr;
}
