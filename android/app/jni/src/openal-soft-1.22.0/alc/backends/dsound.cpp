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

#include "dsound.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include <cguid.h>
#include <mmreg.h>
#ifndef _WAVEFORMATEXTENSIBLE_
#include <ks.h>
#include <ksmedia.h>
#endif

#include <atomic>
#include <cassert>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "alnumeric.h"
#include "comptr.h"
#include "core/device.h"
#include "core/helpers.h"
#include "core/logging.h"
#include "dynload.h"
#include "ringbuffer.h"
#include "strutils.h"
#include "threads.h"

/* MinGW-w64 needs this for some unknown reason now. */
using LPCWAVEFORMATEX = const WAVEFORMATEX*;
#include <dsound.h>


#ifndef DSSPEAKER_5POINT1
#   define DSSPEAKER_5POINT1          0x00000006
#endif
#ifndef DSSPEAKER_5POINT1_BACK
#   define DSSPEAKER_5POINT1_BACK     0x00000006
#endif
#ifndef DSSPEAKER_7POINT1
#   define DSSPEAKER_7POINT1          0x00000007
#endif
#ifndef DSSPEAKER_7POINT1_SURROUND
#   define DSSPEAKER_7POINT1_SURROUND 0x00000008
#endif
#ifndef DSSPEAKER_5POINT1_SURROUND
#   define DSSPEAKER_5POINT1_SURROUND 0x00000009
#endif


/* Some headers seem to define these as macros for __uuidof, which is annoying
 * since some headers don't declare them at all. Hopefully the ifdef is enough
 * to tell if they need to be declared.
 */
#ifndef KSDATAFORMAT_SUBTYPE_PCM
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_PCM, 0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
#endif
#ifndef KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
DEFINE_GUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
#endif

namespace {

#define DEVNAME_HEAD "OpenAL Soft on "


#ifdef HAVE_DYNLOAD
void *ds_handle;
HRESULT (WINAPI *pDirectSoundCreate)(const GUID *pcGuidDevice, IDirectSound **ppDS, IUnknown *pUnkOuter);
HRESULT (WINAPI *pDirectSoundEnumerateW)(LPDSENUMCALLBACKW pDSEnumCallback, void *pContext);
HRESULT (WINAPI *pDirectSoundCaptureCreate)(const GUID *pcGuidDevice, IDirectSoundCapture **ppDSC, IUnknown *pUnkOuter);
HRESULT (WINAPI *pDirectSoundCaptureEnumerateW)(LPDSENUMCALLBACKW pDSEnumCallback, void *pContext);

#ifndef IN_IDE_PARSER
#define DirectSoundCreate            pDirectSoundCreate
#define DirectSoundEnumerateW        pDirectSoundEnumerateW
#define DirectSoundCaptureCreate     pDirectSoundCaptureCreate
#define DirectSoundCaptureEnumerateW pDirectSoundCaptureEnumerateW
#endif
#endif


#define MONO SPEAKER_FRONT_CENTER
#define STEREO (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT)
#define QUAD (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define X5DOT1 (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)
#define X5DOT1REAR (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define X6DOT1 (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)
#define X7DOT1 (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)

#define MAX_UPDATES 128

struct DevMap {
    std::string name;
    GUID guid;

    template<typename T0, typename T1>
    DevMap(T0&& name_, T1&& guid_)
      : name{std::forward<T0>(name_)}, guid{std::forward<T1>(guid_)}
    { }
};

al::vector<DevMap> PlaybackDevices;
al::vector<DevMap> CaptureDevices;

bool checkName(const al::vector<DevMap> &list, const std::string &name)
{
    auto match_name = [&name](const DevMap &entry) -> bool
    { return entry.name == name; };
    return std::find_if(list.cbegin(), list.cend(), match_name) != list.cend();
}

BOOL CALLBACK DSoundEnumDevices(GUID *guid, const WCHAR *desc, const WCHAR*, void *data) noexcept
{
    if(!guid)
        return TRUE;

    auto& devices = *static_cast<al::vector<DevMap>*>(data);
    const std::string basename{DEVNAME_HEAD + wstr_to_utf8(desc)};

    int count{1};
    std::string newname{basename};
    while(checkName(devices, newname))
    {
        newname = basename;
        newname += " #";
        newname += std::to_string(++count);
    }
    devices.emplace_back(std::move(newname), *guid);
    const DevMap &newentry = devices.back();

    OLECHAR *guidstr{nullptr};
    HRESULT hr{StringFromCLSID(*guid, &guidstr)};
    if(SUCCEEDED(hr))
    {
        TRACE("Got device \"%s\", GUID \"%ls\"\n", newentry.name.c_str(), guidstr);
        CoTaskMemFree(guidstr);
    }

    return TRUE;
}


struct DSoundPlayback final : public BackendBase {
    DSoundPlayback(DeviceBase *device) noexcept : BackendBase{device} { }
    ~DSoundPlayback() override;

    int mixerProc();

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;

    ComPtr<IDirectSound>       mDS;
    ComPtr<IDirectSoundBuffer> mPrimaryBuffer;
    ComPtr<IDirectSoundBuffer> mBuffer;
    ComPtr<IDirectSoundNotify> mNotifies;
    HANDLE mNotifyEvent{nullptr};

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(DSoundPlayback)
};

DSoundPlayback::~DSoundPlayback()
{
    mNotifies = nullptr;
    mBuffer = nullptr;
    mPrimaryBuffer = nullptr;
    mDS = nullptr;

    if(mNotifyEvent)
        CloseHandle(mNotifyEvent);
    mNotifyEvent = nullptr;
}


FORCE_ALIGN int DSoundPlayback::mixerProc()
{
    SetRTPriority();
    althrd_setname(MIXER_THREAD_NAME);

    DSBCAPS DSBCaps{};
    DSBCaps.dwSize = sizeof(DSBCaps);
    HRESULT err{mBuffer->GetCaps(&DSBCaps)};
    if(FAILED(err))
    {
        ERR("Failed to get buffer caps: 0x%lx\n", err);
        mDevice->handleDisconnect("Failure retrieving playback buffer info: 0x%lx", err);
        return 1;
    }

    const size_t FrameStep{mDevice->channelsFromFmt()};
    uint FrameSize{mDevice->frameSizeFromFmt()};
    DWORD FragSize{mDevice->UpdateSize * FrameSize};

    bool Playing{false};
    DWORD LastCursor{0u};
    mBuffer->GetCurrentPosition(&LastCursor, nullptr);
    while(!mKillNow.load(std::memory_order_acquire)
        && mDevice->Connected.load(std::memory_order_acquire))
    {
        // Get current play cursor
        DWORD PlayCursor;
        mBuffer->GetCurrentPosition(&PlayCursor, nullptr);
        DWORD avail = (PlayCursor-LastCursor+DSBCaps.dwBufferBytes) % DSBCaps.dwBufferBytes;

        if(avail < FragSize)
        {
            if(!Playing)
            {
                err = mBuffer->Play(0, 0, DSBPLAY_LOOPING);
                if(FAILED(err))
                {
                    ERR("Failed to play buffer: 0x%lx\n", err);
                    mDevice->handleDisconnect("Failure starting playback: 0x%lx", err);
                    return 1;
                }
                Playing = true;
            }

            avail = WaitForSingleObjectEx(mNotifyEvent, 2000, FALSE);
            if(avail != WAIT_OBJECT_0)
                ERR("WaitForSingleObjectEx error: 0x%lx\n", avail);
            continue;
        }
        avail -= avail%FragSize;

        // Lock output buffer
        void *WritePtr1, *WritePtr2;
        DWORD WriteCnt1{0u},  WriteCnt2{0u};
        err = mBuffer->Lock(LastCursor, avail, &WritePtr1, &WriteCnt1, &WritePtr2, &WriteCnt2, 0);

        // If the buffer is lost, restore it and lock
        if(err == DSERR_BUFFERLOST)
        {
            WARN("Buffer lost, restoring...\n");
            err = mBuffer->Restore();
            if(SUCCEEDED(err))
            {
                Playing = false;
                LastCursor = 0;
                err = mBuffer->Lock(0, DSBCaps.dwBufferBytes, &WritePtr1, &WriteCnt1,
                                    &WritePtr2, &WriteCnt2, 0);
            }
        }

        if(SUCCEEDED(err))
        {
            mDevice->renderSamples(WritePtr1, WriteCnt1/FrameSize, FrameStep);
            if(WriteCnt2 > 0)
                mDevice->renderSamples(WritePtr2, WriteCnt2/FrameSize, FrameStep);

            mBuffer->Unlock(WritePtr1, WriteCnt1, WritePtr2, WriteCnt2);
        }
        else
        {
            ERR("Buffer lock error: %#lx\n", err);
            mDevice->handleDisconnect("Failed to lock output buffer: 0x%lx", err);
            return 1;
        }

        // Update old write cursor location
        LastCursor += WriteCnt1+WriteCnt2;
        LastCursor %= DSBCaps.dwBufferBytes;
    }

    return 0;
}

void DSoundPlayback::open(const char *name)
{
    HRESULT hr;
    if(PlaybackDevices.empty())
    {
        /* Initialize COM to prevent name truncation */
        HRESULT hrcom{CoInitialize(nullptr)};
        hr = DirectSoundEnumerateW(DSoundEnumDevices, &PlaybackDevices);
        if(FAILED(hr))
            ERR("Error enumerating DirectSound devices (0x%lx)!\n", hr);
        if(SUCCEEDED(hrcom))
            CoUninitialize();
    }

    const GUID *guid{nullptr};
    if(!name && !PlaybackDevices.empty())
    {
        name = PlaybackDevices[0].name.c_str();
        guid = &PlaybackDevices[0].guid;
    }
    else
    {
        auto iter = std::find_if(PlaybackDevices.cbegin(), PlaybackDevices.cend(),
            [name](const DevMap &entry) -> bool { return entry.name == name; });
        if(iter == PlaybackDevices.cend())
        {
            GUID id{};
            hr = CLSIDFromString(utf8_to_wstr(name).c_str(), &id);
            if(SUCCEEDED(hr))
                iter = std::find_if(PlaybackDevices.cbegin(), PlaybackDevices.cend(),
                    [&id](const DevMap &entry) -> bool { return entry.guid == id; });
            if(iter == PlaybackDevices.cend())
                throw al::backend_exception{al::backend_error::NoDevice,
                    "Device name \"%s\" not found", name};
        }
        guid = &iter->guid;
    }

    hr = DS_OK;
    if(!mNotifyEvent)
    {
        mNotifyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if(!mNotifyEvent) hr = E_FAIL;
    }

    //DirectSound Init code
    ComPtr<IDirectSound> ds;
    if(SUCCEEDED(hr))
        hr = DirectSoundCreate(guid, ds.getPtr(), nullptr);
    if(SUCCEEDED(hr))
        hr = ds->SetCooperativeLevel(GetForegroundWindow(), DSSCL_PRIORITY);
    if(FAILED(hr))
        throw al::backend_exception{al::backend_error::DeviceError, "Device init failed: 0x%08lx",
            hr};

    mNotifies = nullptr;
    mBuffer = nullptr;
    mPrimaryBuffer = nullptr;
    mDS = std::move(ds);

    mDevice->DeviceName = name;
}

bool DSoundPlayback::reset()
{
    mNotifies = nullptr;
    mBuffer = nullptr;
    mPrimaryBuffer = nullptr;

    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        mDevice->FmtType = DevFmtUByte;
        break;
    case DevFmtFloat:
        if(mDevice->Flags.test(SampleTypeRequest))
            break;
        /* fall-through */
    case DevFmtUShort:
        mDevice->FmtType = DevFmtShort;
        break;
    case DevFmtUInt:
        mDevice->FmtType = DevFmtInt;
        break;
    case DevFmtUByte:
    case DevFmtShort:
    case DevFmtInt:
        break;
    }

    WAVEFORMATEXTENSIBLE OutputType{};
    DWORD speakers;
    HRESULT hr{mDS->GetSpeakerConfig(&speakers)};
    if(SUCCEEDED(hr))
    {
        speakers = DSSPEAKER_CONFIG(speakers);
        if(!mDevice->Flags.test(ChannelsRequest))
        {
            if(speakers == DSSPEAKER_MONO)
                mDevice->FmtChans = DevFmtMono;
            else if(speakers == DSSPEAKER_STEREO || speakers == DSSPEAKER_HEADPHONE)
                mDevice->FmtChans = DevFmtStereo;
            else if(speakers == DSSPEAKER_QUAD)
                mDevice->FmtChans = DevFmtQuad;
            else if(speakers == DSSPEAKER_5POINT1_SURROUND || speakers == DSSPEAKER_5POINT1_BACK)
                mDevice->FmtChans = DevFmtX51;
            else if(speakers == DSSPEAKER_7POINT1 || speakers == DSSPEAKER_7POINT1_SURROUND)
                mDevice->FmtChans = DevFmtX71;
            else
                ERR("Unknown system speaker config: 0x%lx\n", speakers);
        }
        mDevice->Flags.set(DirectEar, (speakers == DSSPEAKER_HEADPHONE));

        switch(mDevice->FmtChans)
        {
        case DevFmtMono: OutputType.dwChannelMask = MONO; break;
        case DevFmtAmbi3D: mDevice->FmtChans = DevFmtStereo;
            /*fall-through*/
        case DevFmtStereo: OutputType.dwChannelMask = STEREO; break;
        case DevFmtQuad: OutputType.dwChannelMask = QUAD; break;
        case DevFmtX51: OutputType.dwChannelMask = X5DOT1; break;
        case DevFmtX61: OutputType.dwChannelMask = X6DOT1; break;
        case DevFmtX71: OutputType.dwChannelMask = X7DOT1; break;
        }

retry_open:
        hr = S_OK;
        OutputType.Format.wFormatTag = WAVE_FORMAT_PCM;
        OutputType.Format.nChannels = static_cast<WORD>(mDevice->channelsFromFmt());
        OutputType.Format.wBitsPerSample = static_cast<WORD>(mDevice->bytesFromFmt() * 8);
        OutputType.Format.nBlockAlign = static_cast<WORD>(OutputType.Format.nChannels *
            OutputType.Format.wBitsPerSample / 8);
        OutputType.Format.nSamplesPerSec = mDevice->Frequency;
        OutputType.Format.nAvgBytesPerSec = OutputType.Format.nSamplesPerSec *
            OutputType.Format.nBlockAlign;
        OutputType.Format.cbSize = 0;
    }

    if(OutputType.Format.nChannels > 2 || mDevice->FmtType == DevFmtFloat)
    {
        OutputType.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        OutputType.Samples.wValidBitsPerSample = OutputType.Format.wBitsPerSample;
        OutputType.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        if(mDevice->FmtType == DevFmtFloat)
            OutputType.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        else
            OutputType.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        mPrimaryBuffer = nullptr;
    }
    else
    {
        if(SUCCEEDED(hr) && !mPrimaryBuffer)
        {
            DSBUFFERDESC DSBDescription{};
            DSBDescription.dwSize = sizeof(DSBDescription);
            DSBDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
            hr = mDS->CreateSoundBuffer(&DSBDescription, mPrimaryBuffer.getPtr(), nullptr);
        }
        if(SUCCEEDED(hr))
            hr = mPrimaryBuffer->SetFormat(&OutputType.Format);
    }

    if(SUCCEEDED(hr))
    {
        uint num_updates{mDevice->BufferSize / mDevice->UpdateSize};
        if(num_updates > MAX_UPDATES)
            num_updates = MAX_UPDATES;
        mDevice->BufferSize = mDevice->UpdateSize * num_updates;

        DSBUFFERDESC DSBDescription{};
        DSBDescription.dwSize = sizeof(DSBDescription);
        DSBDescription.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
            | DSBCAPS_GLOBALFOCUS;
        DSBDescription.dwBufferBytes = mDevice->BufferSize * OutputType.Format.nBlockAlign;
        DSBDescription.lpwfxFormat = &OutputType.Format;

        hr = mDS->CreateSoundBuffer(&DSBDescription, mBuffer.getPtr(), nullptr);
        if(FAILED(hr) && mDevice->FmtType == DevFmtFloat)
        {
            mDevice->FmtType = DevFmtShort;
            goto retry_open;
        }
    }

    if(SUCCEEDED(hr))
    {
        void *ptr;
        hr = mBuffer->QueryInterface(IID_IDirectSoundNotify, &ptr);
        if(SUCCEEDED(hr))
        {
            mNotifies = ComPtr<IDirectSoundNotify>{static_cast<IDirectSoundNotify*>(ptr)};

            uint num_updates{mDevice->BufferSize / mDevice->UpdateSize};
            assert(num_updates <= MAX_UPDATES);

            std::array<DSBPOSITIONNOTIFY,MAX_UPDATES> nots;
            for(uint i{0};i < num_updates;++i)
            {
                nots[i].dwOffset = i * mDevice->UpdateSize * OutputType.Format.nBlockAlign;
                nots[i].hEventNotify = mNotifyEvent;
            }
            if(mNotifies->SetNotificationPositions(num_updates, nots.data()) != DS_OK)
                hr = E_FAIL;
        }
    }

    if(FAILED(hr))
    {
        mNotifies = nullptr;
        mBuffer = nullptr;
        mPrimaryBuffer = nullptr;
        return false;
    }

    ResetEvent(mNotifyEvent);
    setChannelOrderFromWFXMask(OutputType.dwChannelMask);

    return true;
}

void DSoundPlayback::start()
{
    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread{std::mem_fn(&DSoundPlayback::mixerProc), this};
    }
    catch(std::exception& e) {
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start mixing thread: %s", e.what()};
    }
}

void DSoundPlayback::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;
    mThread.join();

    mBuffer->Stop();
}


struct DSoundCapture final : public BackendBase {
    DSoundCapture(DeviceBase *device) noexcept : BackendBase{device} { }
    ~DSoundCapture() override;

    void open(const char *name) override;
    void start() override;
    void stop() override;
    void captureSamples(al::byte *buffer, uint samples) override;
    uint availableSamples() override;

    ComPtr<IDirectSoundCapture> mDSC;
    ComPtr<IDirectSoundCaptureBuffer> mDSCbuffer;
    DWORD mBufferBytes{0u};
    DWORD mCursor{0u};

    RingBufferPtr mRing;

    DEF_NEWDEL(DSoundCapture)
};

DSoundCapture::~DSoundCapture()
{
    if(mDSCbuffer)
    {
        mDSCbuffer->Stop();
        mDSCbuffer = nullptr;
    }
    mDSC = nullptr;
}


void DSoundCapture::open(const char *name)
{
    HRESULT hr;
    if(CaptureDevices.empty())
    {
        /* Initialize COM to prevent name truncation */
        HRESULT hrcom{CoInitialize(nullptr)};
        hr = DirectSoundCaptureEnumerateW(DSoundEnumDevices, &CaptureDevices);
        if(FAILED(hr))
            ERR("Error enumerating DirectSound devices (0x%lx)!\n", hr);
        if(SUCCEEDED(hrcom))
            CoUninitialize();
    }

    const GUID *guid{nullptr};
    if(!name && !CaptureDevices.empty())
    {
        name = CaptureDevices[0].name.c_str();
        guid = &CaptureDevices[0].guid;
    }
    else
    {
        auto iter = std::find_if(CaptureDevices.cbegin(), CaptureDevices.cend(),
            [name](const DevMap &entry) -> bool { return entry.name == name; });
        if(iter == CaptureDevices.cend())
        {
            GUID id{};
            hr = CLSIDFromString(utf8_to_wstr(name).c_str(), &id);
            if(SUCCEEDED(hr))
                iter = std::find_if(CaptureDevices.cbegin(), CaptureDevices.cend(),
                    [&id](const DevMap &entry) -> bool { return entry.guid == id; });
            if(iter == CaptureDevices.cend())
                throw al::backend_exception{al::backend_error::NoDevice,
                    "Device name \"%s\" not found", name};
        }
        guid = &iter->guid;
    }

    switch(mDevice->FmtType)
    {
    case DevFmtByte:
    case DevFmtUShort:
    case DevFmtUInt:
        WARN("%s capture samples not supported\n", DevFmtTypeString(mDevice->FmtType));
        throw al::backend_exception{al::backend_error::DeviceError,
            "%s capture samples not supported", DevFmtTypeString(mDevice->FmtType)};

    case DevFmtUByte:
    case DevFmtShort:
    case DevFmtInt:
    case DevFmtFloat:
        break;
    }

    WAVEFORMATEXTENSIBLE InputType{};
    switch(mDevice->FmtChans)
    {
    case DevFmtMono: InputType.dwChannelMask = MONO; break;
    case DevFmtStereo: InputType.dwChannelMask = STEREO; break;
    case DevFmtQuad: InputType.dwChannelMask = QUAD; break;
    case DevFmtX51: InputType.dwChannelMask = X5DOT1; break;
    case DevFmtX61: InputType.dwChannelMask = X6DOT1; break;
    case DevFmtX71: InputType.dwChannelMask = X7DOT1; break;
    case DevFmtAmbi3D:
        WARN("%s capture not supported\n", DevFmtChannelsString(mDevice->FmtChans));
        throw al::backend_exception{al::backend_error::DeviceError, "%s capture not supported",
            DevFmtChannelsString(mDevice->FmtChans)};
    }

    InputType.Format.wFormatTag = WAVE_FORMAT_PCM;
    InputType.Format.nChannels = static_cast<WORD>(mDevice->channelsFromFmt());
    InputType.Format.wBitsPerSample = static_cast<WORD>(mDevice->bytesFromFmt() * 8);
    InputType.Format.nBlockAlign = static_cast<WORD>(InputType.Format.nChannels *
        InputType.Format.wBitsPerSample / 8);
    InputType.Format.nSamplesPerSec = mDevice->Frequency;
    InputType.Format.nAvgBytesPerSec = InputType.Format.nSamplesPerSec *
        InputType.Format.nBlockAlign;
    InputType.Format.cbSize = 0;
    InputType.Samples.wValidBitsPerSample = InputType.Format.wBitsPerSample;
    if(mDevice->FmtType == DevFmtFloat)
        InputType.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    else
        InputType.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    if(InputType.Format.nChannels > 2 || mDevice->FmtType == DevFmtFloat)
    {
        InputType.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        InputType.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    }

    uint samples{mDevice->BufferSize};
    samples = maxu(samples, 100 * mDevice->Frequency / 1000);

    DSCBUFFERDESC DSCBDescription{};
    DSCBDescription.dwSize = sizeof(DSCBDescription);
    DSCBDescription.dwFlags = 0;
    DSCBDescription.dwBufferBytes = samples * InputType.Format.nBlockAlign;
    DSCBDescription.lpwfxFormat = &InputType.Format;

    //DirectSoundCapture Init code
    hr = DirectSoundCaptureCreate(guid, mDSC.getPtr(), nullptr);
    if(SUCCEEDED(hr))
        mDSC->CreateCaptureBuffer(&DSCBDescription, mDSCbuffer.getPtr(), nullptr);
    if(SUCCEEDED(hr))
         mRing = RingBuffer::Create(mDevice->BufferSize, InputType.Format.nBlockAlign, false);

    if(FAILED(hr))
    {
        mRing = nullptr;
        mDSCbuffer = nullptr;
        mDSC = nullptr;

        throw al::backend_exception{al::backend_error::DeviceError, "Device init failed: 0x%08lx",
            hr};
    }

    mBufferBytes = DSCBDescription.dwBufferBytes;
    setChannelOrderFromWFXMask(InputType.dwChannelMask);

    mDevice->DeviceName = name;
}

void DSoundCapture::start()
{
    const HRESULT hr{mDSCbuffer->Start(DSCBSTART_LOOPING)};
    if(FAILED(hr))
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failure starting capture: 0x%lx", hr};
}

void DSoundCapture::stop()
{
    HRESULT hr{mDSCbuffer->Stop()};
    if(FAILED(hr))
    {
        ERR("stop failed: 0x%08lx\n", hr);
        mDevice->handleDisconnect("Failure stopping capture: 0x%lx", hr);
    }
}

void DSoundCapture::captureSamples(al::byte *buffer, uint samples)
{ mRing->read(buffer, samples); }

uint DSoundCapture::availableSamples()
{
    if(!mDevice->Connected.load(std::memory_order_acquire))
        return static_cast<uint>(mRing->readSpace());

    const uint FrameSize{mDevice->frameSizeFromFmt()};
    const DWORD BufferBytes{mBufferBytes};
    const DWORD LastCursor{mCursor};

    DWORD ReadCursor{};
    void *ReadPtr1{}, *ReadPtr2{};
    DWORD ReadCnt1{},  ReadCnt2{};
    HRESULT hr{mDSCbuffer->GetCurrentPosition(nullptr, &ReadCursor)};
    if(SUCCEEDED(hr))
    {
        const DWORD NumBytes{(BufferBytes+ReadCursor-LastCursor) % BufferBytes};
        if(!NumBytes) return static_cast<uint>(mRing->readSpace());
        hr = mDSCbuffer->Lock(LastCursor, NumBytes, &ReadPtr1, &ReadCnt1, &ReadPtr2, &ReadCnt2, 0);
    }
    if(SUCCEEDED(hr))
    {
        mRing->write(ReadPtr1, ReadCnt1/FrameSize);
        if(ReadPtr2 != nullptr && ReadCnt2 > 0)
            mRing->write(ReadPtr2, ReadCnt2/FrameSize);
        hr = mDSCbuffer->Unlock(ReadPtr1, ReadCnt1, ReadPtr2, ReadCnt2);
        mCursor = ReadCursor;
    }

    if(FAILED(hr))
    {
        ERR("update failed: 0x%08lx\n", hr);
        mDevice->handleDisconnect("Failure retrieving capture data: 0x%lx", hr);
    }

    return static_cast<uint>(mRing->readSpace());
}

} // namespace


BackendFactory &DSoundBackendFactory::getFactory()
{
    static DSoundBackendFactory factory{};
    return factory;
}

bool DSoundBackendFactory::init()
{
#ifdef HAVE_DYNLOAD
    if(!ds_handle)
    {
        ds_handle = LoadLib("dsound.dll");
        if(!ds_handle)
        {
            ERR("Failed to load dsound.dll\n");
            return false;
        }

#define LOAD_FUNC(f) do {                                                     \
    p##f = reinterpret_cast<decltype(p##f)>(GetSymbol(ds_handle, #f));        \
    if(!p##f)                                                                 \
    {                                                                         \
        CloseLib(ds_handle);                                                  \
        ds_handle = nullptr;                                                  \
        return false;                                                         \
    }                                                                         \
} while(0)
        LOAD_FUNC(DirectSoundCreate);
        LOAD_FUNC(DirectSoundEnumerateW);
        LOAD_FUNC(DirectSoundCaptureCreate);
        LOAD_FUNC(DirectSoundCaptureEnumerateW);
#undef LOAD_FUNC
    }
#endif
    return true;
}

bool DSoundBackendFactory::querySupport(BackendType type)
{ return (type == BackendType::Playback || type == BackendType::Capture); }

std::string DSoundBackendFactory::probe(BackendType type)
{
    std::string outnames;
    auto add_device = [&outnames](const DevMap &entry) -> void
    {
        /* +1 to also append the null char (to ensure a null-separated list and
         * double-null terminated list).
         */
        outnames.append(entry.name.c_str(), entry.name.length()+1);
    };

    /* Initialize COM to prevent name truncation */
    HRESULT hr;
    HRESULT hrcom{CoInitialize(nullptr)};
    switch(type)
    {
    case BackendType::Playback:
        PlaybackDevices.clear();
        hr = DirectSoundEnumerateW(DSoundEnumDevices, &PlaybackDevices);
        if(FAILED(hr))
            ERR("Error enumerating DirectSound playback devices (0x%lx)!\n", hr);
        std::for_each(PlaybackDevices.cbegin(), PlaybackDevices.cend(), add_device);
        break;

    case BackendType::Capture:
        CaptureDevices.clear();
        hr = DirectSoundCaptureEnumerateW(DSoundEnumDevices, &CaptureDevices);
        if(FAILED(hr))
            ERR("Error enumerating DirectSound capture devices (0x%lx)!\n", hr);
        std::for_each(CaptureDevices.cbegin(), CaptureDevices.cend(), add_device);
        break;
    }
    if(SUCCEEDED(hrcom))
        CoUninitialize();

    return outnames;
}

BackendPtr DSoundBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new DSoundPlayback{device}};
    if(type == BackendType::Capture)
        return BackendPtr{new DSoundCapture{device}};
    return nullptr;
}
