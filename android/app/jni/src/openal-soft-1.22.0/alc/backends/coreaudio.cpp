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

#include "coreaudio.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cmath>
#include <memory>
#include <string>

#include "alnumeric.h"
#include "core/converter.h"
#include "core/device.h"
#include "core/logging.h"
#include "ringbuffer.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>


namespace {

#if TARGET_OS_IOS || TARGET_OS_TV
#define CAN_ENUMERATE 0
#else
#define CAN_ENUMERATE 1
#endif


#if CAN_ENUMERATE
struct DeviceEntry {
    AudioDeviceID mId;
    std::string mName;
};

std::vector<DeviceEntry> PlaybackList;
std::vector<DeviceEntry> CaptureList;


OSStatus GetHwProperty(AudioHardwarePropertyID propId, UInt32 dataSize, void *propData)
{
    const AudioObjectPropertyAddress addr{propId, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster};
    return AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, nullptr, &dataSize,
        propData);
}

OSStatus GetHwPropertySize(AudioHardwarePropertyID propId, UInt32 *outSize)
{
    const AudioObjectPropertyAddress addr{propId, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster};
    return AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr, 0, nullptr, outSize);
}

OSStatus GetDevProperty(AudioDeviceID devId, AudioDevicePropertyID propId, bool isCapture,
    UInt32 elem, UInt32 dataSize, void *propData)
{
    static const AudioObjectPropertyScope scopes[2]{kAudioDevicePropertyScopeOutput,
        kAudioDevicePropertyScopeInput};
    const AudioObjectPropertyAddress addr{propId, scopes[isCapture], elem};
    return AudioObjectGetPropertyData(devId, &addr, 0, nullptr, &dataSize, propData);
}

OSStatus GetDevPropertySize(AudioDeviceID devId, AudioDevicePropertyID inPropertyID,
    bool isCapture, UInt32 elem, UInt32 *outSize)
{
    static const AudioObjectPropertyScope scopes[2]{kAudioDevicePropertyScopeOutput,
        kAudioDevicePropertyScopeInput};
    const AudioObjectPropertyAddress addr{inPropertyID, scopes[isCapture], elem};
    return AudioObjectGetPropertyDataSize(devId, &addr, 0, nullptr, outSize);
}


std::string GetDeviceName(AudioDeviceID devId)
{
    std::string devname;
    CFStringRef nameRef;

    /* Try to get the device name as a CFString, for Unicode name support. */
    OSStatus err{GetDevProperty(devId, kAudioDevicePropertyDeviceNameCFString, false, 0,
        sizeof(nameRef), &nameRef)};
    if(err == noErr)
    {
        const CFIndex propSize{CFStringGetMaximumSizeForEncoding(CFStringGetLength(nameRef),
            kCFStringEncodingUTF8)};
        devname.resize(static_cast<size_t>(propSize)+1, '\0');

        CFStringGetCString(nameRef, &devname[0], propSize+1, kCFStringEncodingUTF8);
        CFRelease(nameRef);
    }
    else
    {
        /* If that failed, just get the C string. Hopefully there's nothing bad
         * with this.
         */
        UInt32 propSize{};
        if(GetDevPropertySize(devId, kAudioDevicePropertyDeviceName, false, 0, &propSize))
            return devname;

        devname.resize(propSize+1, '\0');
        if(GetDevProperty(devId, kAudioDevicePropertyDeviceName, false, 0, propSize, &devname[0]))
        {
            devname.clear();
            return devname;
        }
    }

    /* Clear extraneous nul chars that may have been written with the name
     * string, and return it.
     */
    while(!devname.back())
        devname.pop_back();
    return devname;
}

UInt32 GetDeviceChannelCount(AudioDeviceID devId, bool isCapture)
{
    UInt32 propSize{};
    auto err = GetDevPropertySize(devId, kAudioDevicePropertyStreamConfiguration, isCapture, 0,
        &propSize);
    if(err)
    {
        ERR("kAudioDevicePropertyStreamConfiguration size query failed: %u\n", err);
        return 0;
    }

    auto buflist_data = std::make_unique<char[]>(propSize);
    auto *buflist = reinterpret_cast<AudioBufferList*>(buflist_data.get());

    err = GetDevProperty(devId, kAudioDevicePropertyStreamConfiguration, isCapture, 0, propSize,
        buflist);
    if(err)
    {
        ERR("kAudioDevicePropertyStreamConfiguration query failed: %u\n", err);
        return 0;
    }

    UInt32 numChannels{0};
    for(size_t i{0};i < buflist->mNumberBuffers;++i)
        numChannels += buflist->mBuffers[i].mNumberChannels;

    return numChannels;
}


void EnumerateDevices(std::vector<DeviceEntry> &list, bool isCapture)
{
    UInt32 propSize{};
    if(auto err = GetHwPropertySize(kAudioHardwarePropertyDevices, &propSize))
    {
        ERR("Failed to get device list size: %u\n", err);
        return;
    }

    auto devIds = std::vector<AudioDeviceID>(propSize/sizeof(AudioDeviceID), kAudioDeviceUnknown);
    if(auto err = GetHwProperty(kAudioHardwarePropertyDevices, propSize, devIds.data()))
    {
        ERR("Failed to get device list: %u\n", err);
        return;
    }

    std::vector<DeviceEntry> newdevs;
    newdevs.reserve(devIds.size());

    AudioDeviceID defaultId{kAudioDeviceUnknown};
    GetHwProperty(isCapture ? kAudioHardwarePropertyDefaultInputDevice :
        kAudioHardwarePropertyDefaultOutputDevice, sizeof(defaultId), &defaultId);

    if(defaultId != kAudioDeviceUnknown)
    {
        newdevs.emplace_back(DeviceEntry{defaultId, GetDeviceName(defaultId)});
        const auto &entry = newdevs.back();
        TRACE("Got device: %s = ID %u\n", entry.mName.c_str(), entry.mId);
    }
    for(const AudioDeviceID devId : devIds)
    {
        if(devId == kAudioDeviceUnknown)
            continue;

        auto match_devid = [devId](const DeviceEntry &entry) noexcept -> bool
        { return entry.mId == devId; };
        auto match = std::find_if(newdevs.cbegin(), newdevs.cend(), match_devid);
        if(match != newdevs.cend()) continue;

        auto numChannels = GetDeviceChannelCount(devId, isCapture);
        if(numChannels > 0)
        {
            newdevs.emplace_back(DeviceEntry{devId, GetDeviceName(devId)});
            const auto &entry = newdevs.back();
            TRACE("Got device: %s = ID %u\n", entry.mName.c_str(), entry.mId);
        }
    }

    if(newdevs.size() > 1)
    {
        /* Rename entries that have matching names, by appending '#2', '#3',
         * etc, as needed.
         */
        for(auto curitem = newdevs.begin()+1;curitem != newdevs.end();++curitem)
        {
            auto check_match = [curitem](const DeviceEntry &entry) -> bool
            { return entry.mName == curitem->mName; };
            if(std::find_if(newdevs.begin(), curitem, check_match) != curitem)
            {
                std::string name{curitem->mName};
                size_t count{1};
                auto check_name = [&name](const DeviceEntry &entry) -> bool
                { return entry.mName == name; };
                do {
                    name = curitem->mName;
                    name += " #";
                    name += std::to_string(++count);
                } while(std::find_if(newdevs.begin(), curitem, check_name) != curitem);
                curitem->mName = std::move(name);
            }
        }
    }

    newdevs.shrink_to_fit();
    newdevs.swap(list);
}

#else

static constexpr char ca_device[] = "CoreAudio Default";
#endif


struct CoreAudioPlayback final : public BackendBase {
    CoreAudioPlayback(DeviceBase *device) noexcept : BackendBase{device} { }
    ~CoreAudioPlayback() override;

    OSStatus MixerProc(AudioUnitRenderActionFlags *ioActionFlags,
        const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
        AudioBufferList *ioData) noexcept;
    static OSStatus MixerProcC(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
        const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
        AudioBufferList *ioData) noexcept
    {
        return static_cast<CoreAudioPlayback*>(inRefCon)->MixerProc(ioActionFlags, inTimeStamp,
            inBusNumber, inNumberFrames, ioData);
    }

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;

    AudioUnit mAudioUnit{};

    uint mFrameSize{0u};
    AudioStreamBasicDescription mFormat{}; // This is the OpenAL format as a CoreAudio ASBD

    DEF_NEWDEL(CoreAudioPlayback)
};

CoreAudioPlayback::~CoreAudioPlayback()
{
    AudioUnitUninitialize(mAudioUnit);
    AudioComponentInstanceDispose(mAudioUnit);
}


OSStatus CoreAudioPlayback::MixerProc(AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32,
    UInt32, AudioBufferList *ioData) noexcept
{
    for(size_t i{0};i < ioData->mNumberBuffers;++i)
    {
        auto &buffer = ioData->mBuffers[i];
        mDevice->renderSamples(buffer.mData, buffer.mDataByteSize/mFrameSize,
            buffer.mNumberChannels);
    }
    return noErr;
}


void CoreAudioPlayback::open(const char *name)
{
#if CAN_ENUMERATE
    AudioDeviceID audioDevice{kAudioDeviceUnknown};
    if(!name)
        GetHwProperty(kAudioHardwarePropertyDefaultOutputDevice, sizeof(audioDevice),
            &audioDevice);
    else
    {
        if(PlaybackList.empty())
            EnumerateDevices(PlaybackList, false);

        auto find_name = [name](const DeviceEntry &entry) -> bool
        { return entry.mName == name; };
        auto devmatch = std::find_if(PlaybackList.cbegin(), PlaybackList.cend(), find_name);
        if(devmatch == PlaybackList.cend())
            throw al::backend_exception{al::backend_error::NoDevice,
                "Device name \"%s\" not found", name};

        audioDevice = devmatch->mId;
    }
#else
    if(!name)
        name = ca_device;
    else if(strcmp(name, ca_device) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};
#endif

    /* open the default output unit */
    AudioComponentDescription desc{};
    desc.componentType = kAudioUnitType_Output;
#if CAN_ENUMERATE
    desc.componentSubType = (audioDevice == kAudioDeviceUnknown) ?
        kAudioUnitSubType_DefaultOutput : kAudioUnitSubType_HALOutput;
#else
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp{AudioComponentFindNext(NULL, &desc)};
    if(comp == nullptr)
        throw al::backend_exception{al::backend_error::NoDevice, "Could not find audio component"};

    AudioUnit audioUnit{};
    OSStatus err{AudioComponentInstanceNew(comp, &audioUnit)};
    if(err != noErr)
        throw al::backend_exception{al::backend_error::NoDevice,
            "Could not create component instance: %u", err};

#if CAN_ENUMERATE
    if(audioDevice != kAudioDeviceUnknown)
        AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global, 0, &audioDevice, sizeof(AudioDeviceID));
#endif

    err = AudioUnitInitialize(audioUnit);
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not initialize audio unit: %u", err};

    /* WARNING: I don't know if "valid" audio unit values are guaranteed to be
     * non-0. If not, this logic is broken.
     */
    if(mAudioUnit)
    {
        AudioUnitUninitialize(mAudioUnit);
        AudioComponentInstanceDispose(mAudioUnit);
    }
    mAudioUnit = audioUnit;

#if CAN_ENUMERATE
    if(name)
        mDevice->DeviceName = name;
    else
    {
        UInt32 propSize{sizeof(audioDevice)};
        audioDevice = kAudioDeviceUnknown;
        AudioUnitGetProperty(audioUnit, kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global, 0, &audioDevice, &propSize);

        std::string devname{GetDeviceName(audioDevice)};
        if(!devname.empty()) mDevice->DeviceName = std::move(devname);
        else mDevice->DeviceName = "Unknown Device Name";
    }
#else
    mDevice->DeviceName = name;
#endif
}

bool CoreAudioPlayback::reset()
{
    OSStatus err{AudioUnitUninitialize(mAudioUnit)};
    if(err != noErr)
        ERR("-- AudioUnitUninitialize failed.\n");

    /* retrieve default output unit's properties (output side) */
    AudioStreamBasicDescription streamFormat{};
    UInt32 size{sizeof(streamFormat)};
    err = AudioUnitGetProperty(mAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output,
        0, &streamFormat, &size);
    if(err != noErr || size != sizeof(streamFormat))
    {
        ERR("AudioUnitGetProperty failed\n");
        return false;
    }

#if 0
    TRACE("Output streamFormat of default output unit -\n");
    TRACE("  streamFormat.mFramesPerPacket = %d\n", streamFormat.mFramesPerPacket);
    TRACE("  streamFormat.mChannelsPerFrame = %d\n", streamFormat.mChannelsPerFrame);
    TRACE("  streamFormat.mBitsPerChannel = %d\n", streamFormat.mBitsPerChannel);
    TRACE("  streamFormat.mBytesPerPacket = %d\n", streamFormat.mBytesPerPacket);
    TRACE("  streamFormat.mBytesPerFrame = %d\n", streamFormat.mBytesPerFrame);
    TRACE("  streamFormat.mSampleRate = %5.0f\n", streamFormat.mSampleRate);
#endif

    /* Use the sample rate from the output unit's current parameters, but reset
     * everything else.
     */
    if(mDevice->Frequency != streamFormat.mSampleRate)
    {
        mDevice->BufferSize = static_cast<uint>(uint64_t{mDevice->BufferSize} *
            streamFormat.mSampleRate / mDevice->Frequency);
        mDevice->Frequency = static_cast<uint>(streamFormat.mSampleRate);
    }

    /* FIXME: How to tell what channels are what in the output device, and how
     * to specify what we're giving? e.g. 6.0 vs 5.1
     */
    streamFormat.mChannelsPerFrame = mDevice->channelsFromFmt();

    streamFormat.mFramesPerPacket = 1;
    streamFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsPacked;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    switch(mDevice->FmtType)
    {
        case DevFmtUByte:
            mDevice->FmtType = DevFmtByte;
            /* fall-through */
        case DevFmtByte:
            streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            streamFormat.mBitsPerChannel = 8;
            break;
        case DevFmtUShort:
            mDevice->FmtType = DevFmtShort;
            /* fall-through */
        case DevFmtShort:
            streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            streamFormat.mBitsPerChannel = 16;
            break;
        case DevFmtUInt:
            mDevice->FmtType = DevFmtInt;
            /* fall-through */
        case DevFmtInt:
            streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            streamFormat.mBitsPerChannel = 32;
            break;
        case DevFmtFloat:
            streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
            streamFormat.mBitsPerChannel = 32;
            break;
    }
    streamFormat.mBytesPerFrame = streamFormat.mChannelsPerFrame*streamFormat.mBitsPerChannel/8;
    streamFormat.mBytesPerPacket = streamFormat.mBytesPerFrame*streamFormat.mFramesPerPacket;

    err = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
        0, &streamFormat, sizeof(streamFormat));
    if(err != noErr)
    {
        ERR("AudioUnitSetProperty failed\n");
        return false;
    }

    setDefaultWFXChannelOrder();

    /* setup callback */
    mFrameSize = mDevice->frameSizeFromFmt();
    AURenderCallbackStruct input{};
    input.inputProc = CoreAudioPlayback::MixerProcC;
    input.inputProcRefCon = this;

    err = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input, 0, &input, sizeof(AURenderCallbackStruct));
    if(err != noErr)
    {
        ERR("AudioUnitSetProperty failed\n");
        return false;
    }

    /* init the default audio unit... */
    err = AudioUnitInitialize(mAudioUnit);
    if(err != noErr)
    {
        ERR("AudioUnitInitialize failed\n");
        return false;
    }

    return true;
}

void CoreAudioPlayback::start()
{
    const OSStatus err{AudioOutputUnitStart(mAudioUnit)};
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "AudioOutputUnitStart failed: %d", err};
}

void CoreAudioPlayback::stop()
{
    OSStatus err{AudioOutputUnitStop(mAudioUnit)};
    if(err != noErr)
        ERR("AudioOutputUnitStop failed\n");
}


struct CoreAudioCapture final : public BackendBase {
    CoreAudioCapture(DeviceBase *device) noexcept : BackendBase{device} { }
    ~CoreAudioCapture() override;

    OSStatus RecordProc(AudioUnitRenderActionFlags *ioActionFlags,
        const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
        UInt32 inNumberFrames, AudioBufferList *ioData) noexcept;
    static OSStatus RecordProcC(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
        const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
        AudioBufferList *ioData) noexcept
    {
        return static_cast<CoreAudioCapture*>(inRefCon)->RecordProc(ioActionFlags, inTimeStamp,
            inBusNumber, inNumberFrames, ioData);
    }

    void open(const char *name) override;
    void start() override;
    void stop() override;
    void captureSamples(al::byte *buffer, uint samples) override;
    uint availableSamples() override;

    AudioUnit mAudioUnit{0};

    uint mFrameSize{0u};
    AudioStreamBasicDescription mFormat{};  // This is the OpenAL format as a CoreAudio ASBD

    SampleConverterPtr mConverter;

    RingBufferPtr mRing{nullptr};

    DEF_NEWDEL(CoreAudioCapture)
};

CoreAudioCapture::~CoreAudioCapture()
{
    if(mAudioUnit)
        AudioComponentInstanceDispose(mAudioUnit);
    mAudioUnit = 0;
}


OSStatus CoreAudioCapture::RecordProc(AudioUnitRenderActionFlags*,
    const AudioTimeStamp *inTimeStamp, UInt32, UInt32 inNumberFrames,
    AudioBufferList*) noexcept
{
    AudioUnitRenderActionFlags flags = 0;
    union {
        al::byte _[sizeof(AudioBufferList) + sizeof(AudioBuffer)*2];
        AudioBufferList list;
    } audiobuf{};

    auto rec_vec = mRing->getWriteVector();
    inNumberFrames = static_cast<UInt32>(minz(inNumberFrames,
        rec_vec.first.len+rec_vec.second.len));

    // Fill the ringbuffer's two segments with data from the input device
    if(rec_vec.first.len >= inNumberFrames)
    {
        audiobuf.list.mNumberBuffers = 1;
        audiobuf.list.mBuffers[0].mNumberChannels = mFormat.mChannelsPerFrame;
        audiobuf.list.mBuffers[0].mData = rec_vec.first.buf;
        audiobuf.list.mBuffers[0].mDataByteSize = inNumberFrames * mFormat.mBytesPerFrame;
    }
    else
    {
        const auto remaining = static_cast<uint>(inNumberFrames - rec_vec.first.len);
        audiobuf.list.mNumberBuffers = 2;
        audiobuf.list.mBuffers[0].mNumberChannels = mFormat.mChannelsPerFrame;
        audiobuf.list.mBuffers[0].mData = rec_vec.first.buf;
        audiobuf.list.mBuffers[0].mDataByteSize = static_cast<UInt32>(rec_vec.first.len) *
            mFormat.mBytesPerFrame;
        audiobuf.list.mBuffers[1].mNumberChannels = mFormat.mChannelsPerFrame;
        audiobuf.list.mBuffers[1].mData = rec_vec.second.buf;
        audiobuf.list.mBuffers[1].mDataByteSize = remaining * mFormat.mBytesPerFrame;
    }
    OSStatus err{AudioUnitRender(mAudioUnit, &flags, inTimeStamp, audiobuf.list.mNumberBuffers,
        inNumberFrames, &audiobuf.list)};
    if(err != noErr)
    {
        ERR("AudioUnitRender error: %d\n", err);
        return err;
    }

    mRing->writeAdvance(inNumberFrames);
    return noErr;
}


void CoreAudioCapture::open(const char *name)
{
#if CAN_ENUMERATE
    AudioDeviceID audioDevice{kAudioDeviceUnknown};
    if(!name)
        GetHwProperty(kAudioHardwarePropertyDefaultInputDevice, sizeof(audioDevice),
            &audioDevice);
    else
    {
        if(CaptureList.empty())
            EnumerateDevices(CaptureList, true);

        auto find_name = [name](const DeviceEntry &entry) -> bool
        { return entry.mName == name; };
        auto devmatch = std::find_if(CaptureList.cbegin(), CaptureList.cend(), find_name);
        if(devmatch == CaptureList.cend())
            throw al::backend_exception{al::backend_error::NoDevice,
                "Device name \"%s\" not found", name};

        audioDevice = devmatch->mId;
    }
#else
    if(!name)
        name = ca_device;
    else if(strcmp(name, ca_device) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};
#endif

    AudioComponentDescription desc{};
    desc.componentType = kAudioUnitType_Output;
#if CAN_ENUMERATE
    desc.componentSubType = (audioDevice == kAudioDeviceUnknown) ?
        kAudioUnitSubType_DefaultOutput : kAudioUnitSubType_HALOutput;
#else
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    // Search for component with given description
    AudioComponent comp{AudioComponentFindNext(NULL, &desc)};
    if(comp == NULL)
        throw al::backend_exception{al::backend_error::NoDevice, "Could not find audio component"};

    // Open the component
    OSStatus err{AudioComponentInstanceNew(comp, &mAudioUnit)};
    if(err != noErr)
        throw al::backend_exception{al::backend_error::NoDevice,
            "Could not create component instance: %u", err};

#if CAN_ENUMERATE
    if(audioDevice != kAudioDeviceUnknown)
        AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global, 0, &audioDevice, sizeof(AudioDeviceID));
#endif

    // Turn off AudioUnit output
    UInt32 enableIO{0};
    err = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_EnableIO,
        kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not disable audio unit output property: %u", err};

    // Turn on AudioUnit input
    enableIO = 1;
    err = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_EnableIO,
        kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not enable audio unit input property: %u", err};

    // set capture callback
    AURenderCallbackStruct input{};
    input.inputProc = CoreAudioCapture::RecordProcC;
    input.inputProcRefCon = this;

    err = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_SetInputCallback,
        kAudioUnitScope_Global, 0, &input, sizeof(AURenderCallbackStruct));
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not set capture callback: %u", err};

    // Disable buffer allocation for capture
    UInt32 flag{0};
    err = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_ShouldAllocateBuffer,
        kAudioUnitScope_Output, 1, &flag, sizeof(flag));
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not disable buffer allocation property: %u", err};

    // Initialize the device
    err = AudioUnitInitialize(mAudioUnit);
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not initialize audio unit: %u", err};

    // Get the hardware format
    AudioStreamBasicDescription hardwareFormat{};
    UInt32 propertySize{sizeof(hardwareFormat)};
    err = AudioUnitGetProperty(mAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
        1, &hardwareFormat, &propertySize);
    if(err != noErr || propertySize != sizeof(hardwareFormat))
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not get input format: %u", err};

    // Set up the requested format description
    AudioStreamBasicDescription requestedFormat{};
    switch(mDevice->FmtType)
    {
    case DevFmtByte:
        requestedFormat.mBitsPerChannel = 8;
        requestedFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        break;
    case DevFmtUByte:
        requestedFormat.mBitsPerChannel = 8;
        requestedFormat.mFormatFlags = kAudioFormatFlagIsPacked;
        break;
    case DevFmtShort:
        requestedFormat.mBitsPerChannel = 16;
        requestedFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger
            | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
        break;
    case DevFmtUShort:
        requestedFormat.mBitsPerChannel = 16;
        requestedFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
        break;
    case DevFmtInt:
        requestedFormat.mBitsPerChannel = 32;
        requestedFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger
            | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
        break;
    case DevFmtUInt:
        requestedFormat.mBitsPerChannel = 32;
        requestedFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
        break;
    case DevFmtFloat:
        requestedFormat.mBitsPerChannel = 32;
        requestedFormat.mFormatFlags = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagsNativeEndian
            | kAudioFormatFlagIsPacked;
        break;
    }

    switch(mDevice->FmtChans)
    {
    case DevFmtMono:
        requestedFormat.mChannelsPerFrame = 1;
        break;
    case DevFmtStereo:
        requestedFormat.mChannelsPerFrame = 2;
        break;

    case DevFmtQuad:
    case DevFmtX51:
    case DevFmtX61:
    case DevFmtX71:
    case DevFmtAmbi3D:
        throw al::backend_exception{al::backend_error::DeviceError, "%s not supported",
            DevFmtChannelsString(mDevice->FmtChans)};
    }

    requestedFormat.mBytesPerFrame = requestedFormat.mChannelsPerFrame * requestedFormat.mBitsPerChannel / 8;
    requestedFormat.mBytesPerPacket = requestedFormat.mBytesPerFrame;
    requestedFormat.mSampleRate = mDevice->Frequency;
    requestedFormat.mFormatID = kAudioFormatLinearPCM;
    requestedFormat.mReserved = 0;
    requestedFormat.mFramesPerPacket = 1;

    // save requested format description for later use
    mFormat = requestedFormat;
    mFrameSize = mDevice->frameSizeFromFmt();

    // Use intermediate format for sample rate conversion (outputFormat)
    // Set sample rate to the same as hardware for resampling later
    AudioStreamBasicDescription outputFormat{requestedFormat};
    outputFormat.mSampleRate = hardwareFormat.mSampleRate;

    // The output format should be the requested format, but using the hardware sample rate
    // This is because the AudioUnit will automatically scale other properties, except for sample rate
    err = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output,
        1, &outputFormat, sizeof(outputFormat));
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not set input format: %u", err};

    /* Calculate the minimum AudioUnit output format frame count for the pre-
     * conversion ring buffer. Ensure at least 100ms for the total buffer.
     */
    double srateScale{double{outputFormat.mSampleRate} / mDevice->Frequency};
    auto FrameCount64 = maxu64(static_cast<uint64_t>(std::ceil(mDevice->BufferSize*srateScale)),
        static_cast<UInt32>(outputFormat.mSampleRate)/10);
    FrameCount64 += MaxResamplerPadding;
    if(FrameCount64 > std::numeric_limits<int32_t>::max())
        throw al::backend_exception{al::backend_error::DeviceError,
            "Calculated frame count is too large: %" PRIu64, FrameCount64};

    UInt32 outputFrameCount{};
    propertySize = sizeof(outputFrameCount);
    err = AudioUnitGetProperty(mAudioUnit, kAudioUnitProperty_MaximumFramesPerSlice,
        kAudioUnitScope_Global, 0, &outputFrameCount, &propertySize);
    if(err != noErr || propertySize != sizeof(outputFrameCount))
        throw al::backend_exception{al::backend_error::DeviceError,
            "Could not get input frame count: %u", err};

    outputFrameCount = static_cast<UInt32>(maxu64(outputFrameCount, FrameCount64));
    mRing = RingBuffer::Create(outputFrameCount, mFrameSize, false);

    /* Set up sample converter if needed */
    if(outputFormat.mSampleRate != mDevice->Frequency)
        mConverter = CreateSampleConverter(mDevice->FmtType, mDevice->FmtType,
            mFormat.mChannelsPerFrame, static_cast<uint>(hardwareFormat.mSampleRate),
            mDevice->Frequency, Resampler::FastBSinc24);

#if CAN_ENUMERATE
    if(name)
        mDevice->DeviceName = name;
    else
    {
        UInt32 propSize{sizeof(audioDevice)};
        audioDevice = kAudioDeviceUnknown;
        AudioUnitGetProperty(mAudioUnit, kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global, 0, &audioDevice, &propSize);

        std::string devname{GetDeviceName(audioDevice)};
        if(!devname.empty()) mDevice->DeviceName = std::move(devname);
        else mDevice->DeviceName = "Unknown Device Name";
    }
#else
    mDevice->DeviceName = name;
#endif
}


void CoreAudioCapture::start()
{
    OSStatus err{AudioOutputUnitStart(mAudioUnit)};
    if(err != noErr)
        throw al::backend_exception{al::backend_error::DeviceError,
            "AudioOutputUnitStart failed: %d", err};
}

void CoreAudioCapture::stop()
{
    OSStatus err{AudioOutputUnitStop(mAudioUnit)};
    if(err != noErr)
        ERR("AudioOutputUnitStop failed\n");
}

void CoreAudioCapture::captureSamples(al::byte *buffer, uint samples)
{
    if(!mConverter)
    {
        mRing->read(buffer, samples);
        return;
    }

    auto rec_vec = mRing->getReadVector();
    const void *src0{rec_vec.first.buf};
    auto src0len = static_cast<uint>(rec_vec.first.len);
    uint got{mConverter->convert(&src0, &src0len, buffer, samples)};
    size_t total_read{rec_vec.first.len - src0len};
    if(got < samples && !src0len && rec_vec.second.len > 0)
    {
        const void *src1{rec_vec.second.buf};
        auto src1len = static_cast<uint>(rec_vec.second.len);
        got += mConverter->convert(&src1, &src1len, buffer + got*mFrameSize, samples-got);
        total_read += rec_vec.second.len - src1len;
    }

    mRing->readAdvance(total_read);
}

uint CoreAudioCapture::availableSamples()
{
    if(!mConverter) return static_cast<uint>(mRing->readSpace());
    return mConverter->availableOut(static_cast<uint>(mRing->readSpace()));
}

} // namespace

BackendFactory &CoreAudioBackendFactory::getFactory()
{
    static CoreAudioBackendFactory factory{};
    return factory;
}

bool CoreAudioBackendFactory::init() { return true; }

bool CoreAudioBackendFactory::querySupport(BackendType type)
{ return type == BackendType::Playback || type == BackendType::Capture; }

std::string CoreAudioBackendFactory::probe(BackendType type)
{
    std::string outnames;
#if CAN_ENUMERATE
    auto append_name = [&outnames](const DeviceEntry &entry) -> void
    {
        /* Includes null char. */
        outnames.append(entry.mName.c_str(), entry.mName.length()+1);
    };
    switch(type)
    {
    case BackendType::Playback:
        EnumerateDevices(PlaybackList, false);
        std::for_each(PlaybackList.cbegin(), PlaybackList.cend(), append_name);
        break;
    case BackendType::Capture:
        EnumerateDevices(CaptureList, true);
        std::for_each(CaptureList.cbegin(), CaptureList.cend(), append_name);
        break;
    }

#else

    switch(type)
    {
    case BackendType::Playback:
    case BackendType::Capture:
        /* Includes null char. */
        outnames.append(ca_device, sizeof(ca_device));
        break;
    }
#endif
    return outnames;
}

BackendPtr CoreAudioBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new CoreAudioPlayback{device}};
    if(type == BackendType::Capture)
        return BackendPtr{new CoreAudioCapture{device}};
    return nullptr;
}
