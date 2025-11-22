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

#include "wave.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <thread>

#include "albit.h"
#include "albyte.h"
#include "alc/alconfig.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "core/device.h"
#include "core/helpers.h"
#include "core/logging.h"
#include "opthelpers.h"
#include "strutils.h"
#include "threads.h"
#include "vector.h"


namespace {

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

using ubyte = unsigned char;
using ushort = unsigned short;

constexpr char waveDevice[] = "Wave File Writer";

constexpr ubyte SUBTYPE_PCM[]{
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa,
    0x00, 0x38, 0x9b, 0x71
};
constexpr ubyte SUBTYPE_FLOAT[]{
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa,
    0x00, 0x38, 0x9b, 0x71
};

constexpr ubyte SUBTYPE_BFORMAT_PCM[]{
    0x01, 0x00, 0x00, 0x00, 0x21, 0x07, 0xd3, 0x11, 0x86, 0x44, 0xc8, 0xc1,
    0xca, 0x00, 0x00, 0x00
};

constexpr ubyte SUBTYPE_BFORMAT_FLOAT[]{
    0x03, 0x00, 0x00, 0x00, 0x21, 0x07, 0xd3, 0x11, 0x86, 0x44, 0xc8, 0xc1,
    0xca, 0x00, 0x00, 0x00
};

void fwrite16le(ushort val, FILE *f)
{
    ubyte data[2]{ static_cast<ubyte>(val&0xff), static_cast<ubyte>((val>>8)&0xff) };
    fwrite(data, 1, 2, f);
}

void fwrite32le(uint val, FILE *f)
{
    ubyte data[4]{ static_cast<ubyte>(val&0xff), static_cast<ubyte>((val>>8)&0xff),
        static_cast<ubyte>((val>>16)&0xff), static_cast<ubyte>((val>>24)&0xff) };
    fwrite(data, 1, 4, f);
}


struct WaveBackend final : public BackendBase {
    WaveBackend(DeviceBase *device) noexcept : BackendBase{device} { }
    ~WaveBackend() override;

    int mixerProc();

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;

    FILE *mFile{nullptr};
    long mDataStart{-1};

    al::vector<al::byte> mBuffer;

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(WaveBackend)
};

WaveBackend::~WaveBackend()
{
    if(mFile)
        fclose(mFile);
    mFile = nullptr;
}

int WaveBackend::mixerProc()
{
    const milliseconds restTime{mDevice->UpdateSize*1000/mDevice->Frequency / 2};

    althrd_setname(MIXER_THREAD_NAME);

    const size_t frameStep{mDevice->channelsFromFmt()};
    const size_t frameSize{mDevice->frameSizeFromFmt()};

    int64_t done{0};
    auto start = std::chrono::steady_clock::now();
    while(!mKillNow.load(std::memory_order_acquire)
        && mDevice->Connected.load(std::memory_order_acquire))
    {
        auto now = std::chrono::steady_clock::now();

        /* This converts from nanoseconds to nanosamples, then to samples. */
        int64_t avail{std::chrono::duration_cast<seconds>((now-start) *
            mDevice->Frequency).count()};
        if(avail-done < mDevice->UpdateSize)
        {
            std::this_thread::sleep_for(restTime);
            continue;
        }
        while(avail-done >= mDevice->UpdateSize)
        {
            mDevice->renderSamples(mBuffer.data(), mDevice->UpdateSize, frameStep);
            done += mDevice->UpdateSize;

            if(al::endian::native != al::endian::little)
            {
                const uint bytesize{mDevice->bytesFromFmt()};

                if(bytesize == 2)
                {
                    const size_t len{mBuffer.size() & ~size_t{1}};
                    for(size_t i{0};i < len;i+=2)
                        std::swap(mBuffer[i], mBuffer[i+1]);
                }
                else if(bytesize == 4)
                {
                    const size_t len{mBuffer.size() & ~size_t{3}};
                    for(size_t i{0};i < len;i+=4)
                    {
                        std::swap(mBuffer[i  ], mBuffer[i+3]);
                        std::swap(mBuffer[i+1], mBuffer[i+2]);
                    }
                }
            }

            const size_t fs{fwrite(mBuffer.data(), frameSize, mDevice->UpdateSize, mFile)};
            if(fs < mDevice->UpdateSize || ferror(mFile))
            {
                ERR("Error writing to file\n");
                mDevice->handleDisconnect("Failed to write playback samples");
                break;
            }
        }

        /* For every completed second, increment the start time and reduce the
         * samples done. This prevents the difference between the start time
         * and current time from growing too large, while maintaining the
         * correct number of samples to render.
         */
        if(done >= mDevice->Frequency)
        {
            seconds s{done/mDevice->Frequency};
            done %= mDevice->Frequency;
            start += s;
        }
    }

    return 0;
}

void WaveBackend::open(const char *name)
{
    auto fname = ConfigValueStr(nullptr, "wave", "file");
    if(!fname) throw al::backend_exception{al::backend_error::NoDevice,
        "No wave output filename"};

    if(!name)
        name = waveDevice;
    else if(strcmp(name, waveDevice) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};

    /* There's only one "device", so if it's already open, we're done. */
    if(mFile) return;

#ifdef _WIN32
    {
        std::wstring wname{utf8_to_wstr(fname->c_str())};
        mFile = _wfopen(wname.c_str(), L"wb");
    }
#else
    mFile = fopen(fname->c_str(), "wb");
#endif
    if(!mFile)
        throw al::backend_exception{al::backend_error::DeviceError, "Could not open file '%s': %s",
            fname->c_str(), strerror(errno)};

    mDevice->DeviceName = name;
}

bool WaveBackend::reset()
{
    uint channels{0}, bytes{0}, chanmask{0};
    bool isbformat{false};
    size_t val;

    fseek(mFile, 0, SEEK_SET);
    clearerr(mFile);

    if(GetConfigValueBool(nullptr, "wave", "bformat", 0))
    {
        mDevice->FmtChans = DevFmtAmbi3D;
        mDevice->mAmbiOrder = 1;
    }

    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        mDevice->FmtType = DevFmtUByte;
        break;
    case DevFmtUShort:
        mDevice->FmtType = DevFmtShort;
        break;
    case DevFmtUInt:
        mDevice->FmtType = DevFmtInt;
        break;
    case DevFmtUByte:
    case DevFmtShort:
    case DevFmtInt:
    case DevFmtFloat:
        break;
    }
    switch(mDevice->FmtChans)
    {
    case DevFmtMono:   chanmask = 0x04; break;
    case DevFmtStereo: chanmask = 0x01 | 0x02; break;
    case DevFmtQuad:   chanmask = 0x01 | 0x02 | 0x10 | 0x20; break;
    case DevFmtX51: chanmask = 0x01 | 0x02 | 0x04 | 0x08 | 0x200 | 0x400; break;
    case DevFmtX61: chanmask = 0x01 | 0x02 | 0x04 | 0x08 | 0x100 | 0x200 | 0x400; break;
    case DevFmtX71: chanmask = 0x01 | 0x02 | 0x04 | 0x08 | 0x010 | 0x020 | 0x200 | 0x400; break;
    case DevFmtAmbi3D:
        /* .amb output requires FuMa */
        mDevice->mAmbiOrder = minu(mDevice->mAmbiOrder, 3);
        mDevice->mAmbiLayout = DevAmbiLayout::FuMa;
        mDevice->mAmbiScale = DevAmbiScaling::FuMa;
        isbformat = true;
        chanmask = 0;
        break;
    }
    bytes = mDevice->bytesFromFmt();
    channels = mDevice->channelsFromFmt();

    rewind(mFile);

    fputs("RIFF", mFile);
    fwrite32le(0xFFFFFFFF, mFile); // 'RIFF' header len; filled in at close

    fputs("WAVE", mFile);

    fputs("fmt ", mFile);
    fwrite32le(40, mFile); // 'fmt ' header len; 40 bytes for EXTENSIBLE

    // 16-bit val, format type id (extensible: 0xFFFE)
    fwrite16le(0xFFFE, mFile);
    // 16-bit val, channel count
    fwrite16le(static_cast<ushort>(channels), mFile);
    // 32-bit val, frequency
    fwrite32le(mDevice->Frequency, mFile);
    // 32-bit val, bytes per second
    fwrite32le(mDevice->Frequency * channels * bytes, mFile);
    // 16-bit val, frame size
    fwrite16le(static_cast<ushort>(channels * bytes), mFile);
    // 16-bit val, bits per sample
    fwrite16le(static_cast<ushort>(bytes * 8), mFile);
    // 16-bit val, extra byte count
    fwrite16le(22, mFile);
    // 16-bit val, valid bits per sample
    fwrite16le(static_cast<ushort>(bytes * 8), mFile);
    // 32-bit val, channel mask
    fwrite32le(chanmask, mFile);
    // 16 byte GUID, sub-type format
    val = fwrite((mDevice->FmtType == DevFmtFloat) ?
        (isbformat ? SUBTYPE_BFORMAT_FLOAT : SUBTYPE_FLOAT) :
        (isbformat ? SUBTYPE_BFORMAT_PCM : SUBTYPE_PCM), 1, 16, mFile);
    (void)val;

    fputs("data", mFile);
    fwrite32le(0xFFFFFFFF, mFile); // 'data' header len; filled in at close

    if(ferror(mFile))
    {
        ERR("Error writing header: %s\n", strerror(errno));
        return false;
    }
    mDataStart = ftell(mFile);

    setDefaultWFXChannelOrder();

    const uint bufsize{mDevice->frameSizeFromFmt() * mDevice->UpdateSize};
    mBuffer.resize(bufsize);

    return true;
}

void WaveBackend::start()
{
    if(mDataStart > 0 && fseek(mFile, 0, SEEK_END) != 0)
        WARN("Failed to seek on output file\n");
    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(&WaveBackend::mixerProc), this};
    }
    catch(std::exception& e) {
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start mixing thread: %s", e.what()};
    }
}

void WaveBackend::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    if(mDataStart > 0)
    {
        long size{ftell(mFile)};
        if(size > 0)
        {
            long dataLen{size - mDataStart};
            if(fseek(mFile, 4, SEEK_SET) == 0)
                fwrite32le(static_cast<uint>(size-8), mFile); // 'WAVE' header len
            if(fseek(mFile, mDataStart-4, SEEK_SET) == 0)
                fwrite32le(static_cast<uint>(dataLen), mFile); // 'data' header len
        }
    }
}

} // namespace


bool WaveBackendFactory::init()
{ return true; }

bool WaveBackendFactory::querySupport(BackendType type)
{ return type == BackendType::Playback; }

std::string WaveBackendFactory::probe(BackendType type)
{
    std::string outnames;
    switch(type)
    {
    case BackendType::Playback:
        /* Includes null char. */
        outnames.append(waveDevice, sizeof(waveDevice));
        break;
    case BackendType::Capture:
        break;
    }
    return outnames;
}

BackendPtr WaveBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new WaveBackend{device}};
    return nullptr;
}

BackendFactory &WaveBackendFactory::getFactory()
{
    static WaveBackendFactory factory{};
    return factory;
}
