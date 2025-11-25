/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This is an OpenAL backend for Android using the native audio APIs based on
 * OpenSL ES 1.0.1. It is based on source code for the native-audio sample app
 * bundled with NDK.
 */

#include "config.h"

#include "opensl.h"

#include <stdlib.h>
#include <jni.h>

#include <new>
#include <array>
#include <cstring>
#include <thread>
#include <functional>

#include "albit.h"
#include "alnumeric.h"
#include "core/device.h"
#include "core/helpers.h"
#include "core/logging.h"
#include "opthelpers.h"
#include "ringbuffer.h"
#include "threads.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>


namespace {

/* Helper macros */
#define EXTRACT_VCALL_ARGS(...)  __VA_ARGS__))
#define VCALL(obj, func)  ((*(obj))->func((obj), EXTRACT_VCALL_ARGS
#define VCALL0(obj, func)  ((*(obj))->func((obj) EXTRACT_VCALL_ARGS


constexpr char opensl_device[] = "OpenSL";


constexpr SLuint32 GetChannelMask(DevFmtChannels chans) noexcept
{
    switch(chans)
    {
    case DevFmtMono: return SL_SPEAKER_FRONT_CENTER;
    case DevFmtStereo: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    case DevFmtQuad: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT |
        SL_SPEAKER_BACK_LEFT | SL_SPEAKER_BACK_RIGHT;
    case DevFmtX51: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT |
        SL_SPEAKER_FRONT_CENTER | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_SIDE_LEFT |
        SL_SPEAKER_SIDE_RIGHT;
    case DevFmtX61: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT |
        SL_SPEAKER_FRONT_CENTER | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_CENTER |
        SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT;
    case DevFmtX71: return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT |
        SL_SPEAKER_FRONT_CENTER | SL_SPEAKER_LOW_FREQUENCY | SL_SPEAKER_BACK_LEFT |
        SL_SPEAKER_BACK_RIGHT | SL_SPEAKER_SIDE_LEFT | SL_SPEAKER_SIDE_RIGHT;
    case DevFmtAmbi3D:
        break;
    }
    return 0;
}

#ifdef SL_ANDROID_DATAFORMAT_PCM_EX
constexpr SLuint32 GetTypeRepresentation(DevFmtType type) noexcept
{
    switch(type)
    {
    case DevFmtUByte:
    case DevFmtUShort:
    case DevFmtUInt:
        return SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT;
    case DevFmtByte:
    case DevFmtShort:
    case DevFmtInt:
        return SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT;
    case DevFmtFloat:
        return SL_ANDROID_PCM_REPRESENTATION_FLOAT;
    }
    return 0;
}
#endif

constexpr SLuint32 GetByteOrderEndianness() noexcept
{
    if(al::endian::native == al::endian::little)
        return SL_BYTEORDER_LITTLEENDIAN;
    return SL_BYTEORDER_BIGENDIAN;
}

const char *res_str(SLresult result) noexcept
{
    switch(result)
    {
    case SL_RESULT_SUCCESS: return "Success";
    case SL_RESULT_PRECONDITIONS_VIOLATED: return "Preconditions violated";
    case SL_RESULT_PARAMETER_INVALID: return "Parameter invalid";
    case SL_RESULT_MEMORY_FAILURE: return "Memory failure";
    case SL_RESULT_RESOURCE_ERROR: return "Resource error";
    case SL_RESULT_RESOURCE_LOST: return "Resource lost";
    case SL_RESULT_IO_ERROR: return "I/O error";
    case SL_RESULT_BUFFER_INSUFFICIENT: return "Buffer insufficient";
    case SL_RESULT_CONTENT_CORRUPTED: return "Content corrupted";
    case SL_RESULT_CONTENT_UNSUPPORTED: return "Content unsupported";
    case SL_RESULT_CONTENT_NOT_FOUND: return "Content not found";
    case SL_RESULT_PERMISSION_DENIED: return "Permission denied";
    case SL_RESULT_FEATURE_UNSUPPORTED: return "Feature unsupported";
    case SL_RESULT_INTERNAL_ERROR: return "Internal error";
    case SL_RESULT_UNKNOWN_ERROR: return "Unknown error";
    case SL_RESULT_OPERATION_ABORTED: return "Operation aborted";
    case SL_RESULT_CONTROL_LOST: return "Control lost";
#ifdef SL_RESULT_READONLY
    case SL_RESULT_READONLY: return "ReadOnly";
#endif
#ifdef SL_RESULT_ENGINEOPTION_UNSUPPORTED
    case SL_RESULT_ENGINEOPTION_UNSUPPORTED: return "Engine option unsupported";
#endif
#ifdef SL_RESULT_SOURCE_SINK_INCOMPATIBLE
    case SL_RESULT_SOURCE_SINK_INCOMPATIBLE: return "Source/Sink incompatible";
#endif
    }
    return "Unknown error code";
}

#define PRINTERR(x, s) do {                                                      \
    if UNLIKELY((x) != SL_RESULT_SUCCESS)                                        \
        ERR("%s: %s\n", (s), res_str((x)));                                      \
} while(0)


struct OpenSLPlayback final : public BackendBase {
    OpenSLPlayback(DeviceBase *device) noexcept : BackendBase{device} { }
    ~OpenSLPlayback() override;

    void process(SLAndroidSimpleBufferQueueItf bq) noexcept;
    static void processC(SLAndroidSimpleBufferQueueItf bq, void *context) noexcept
    { static_cast<OpenSLPlayback*>(context)->process(bq); }

    int mixerProc();

    void open(const char *name) override;
    bool reset() override;
    void start() override;
    void stop() override;
    ClockLatency getClockLatency() override;

    /* engine interfaces */
    SLObjectItf mEngineObj{nullptr};
    SLEngineItf mEngine{nullptr};

    /* output mix interfaces */
    SLObjectItf mOutputMix{nullptr};

    /* buffer queue player interfaces */
    SLObjectItf mBufferQueueObj{nullptr};

    RingBufferPtr mRing{nullptr};
    al::semaphore mSem;

    std::mutex mMutex;

    uint mFrameSize{0};

    std::atomic<bool> mKillNow{true};
    std::thread mThread;

    DEF_NEWDEL(OpenSLPlayback)
};

OpenSLPlayback::~OpenSLPlayback()
{
    if(mBufferQueueObj)
        VCALL0(mBufferQueueObj,Destroy)();
    mBufferQueueObj = nullptr;

    if(mOutputMix)
        VCALL0(mOutputMix,Destroy)();
    mOutputMix = nullptr;

    if(mEngineObj)
        VCALL0(mEngineObj,Destroy)();
    mEngineObj = nullptr;
    mEngine = nullptr;
}


/* this callback handler is called every time a buffer finishes playing */
void OpenSLPlayback::process(SLAndroidSimpleBufferQueueItf) noexcept
{
    /* A note on the ringbuffer usage: The buffer queue seems to hold on to the
     * pointer passed to the Enqueue method, rather than copying the audio.
     * Consequently, the ringbuffer contains the audio that is currently queued
     * and waiting to play. This process() callback is called when a buffer is
     * finished, so we simply move the read pointer up to indicate the space is
     * available for writing again, and wake up the mixer thread to mix and
     * queue more audio.
     */
    mRing->readAdvance(1);

    mSem.post();
}

int OpenSLPlayback::mixerProc()
{
    SetRTPriority();
    althrd_setname(MIXER_THREAD_NAME);

    SLPlayItf player;
    SLAndroidSimpleBufferQueueItf bufferQueue;
    SLresult result{VCALL(mBufferQueueObj,GetInterface)(SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
        &bufferQueue)};
    PRINTERR(result, "bufferQueue->GetInterface SL_IID_ANDROIDSIMPLEBUFFERQUEUE");
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mBufferQueueObj,GetInterface)(SL_IID_PLAY, &player);
        PRINTERR(result, "bufferQueue->GetInterface SL_IID_PLAY");
    }

    const size_t frame_step{mDevice->channelsFromFmt()};

    if(SL_RESULT_SUCCESS != result)
        mDevice->handleDisconnect("Failed to get playback buffer: 0x%08x", result);

    while(SL_RESULT_SUCCESS == result && !mKillNow.load(std::memory_order_acquire)
        && mDevice->Connected.load(std::memory_order_acquire))
    {
        if(mRing->writeSpace() == 0)
        {
            SLuint32 state{0};

            result = VCALL(player,GetPlayState)(&state);
            PRINTERR(result, "player->GetPlayState");
            if(SL_RESULT_SUCCESS == result && state != SL_PLAYSTATE_PLAYING)
            {
                result = VCALL(player,SetPlayState)(SL_PLAYSTATE_PLAYING);
                PRINTERR(result, "player->SetPlayState");
            }
            if(SL_RESULT_SUCCESS != result)
            {
                mDevice->handleDisconnect("Failed to start playback: 0x%08x", result);
                break;
            }

            if(mRing->writeSpace() == 0)
            {
                mSem.wait();
                continue;
            }
        }

        std::unique_lock<std::mutex> dlock{mMutex};
        auto data = mRing->getWriteVector();
        mDevice->renderSamples(data.first.buf,
            static_cast<uint>(data.first.len)*mDevice->UpdateSize, frame_step);
        if(data.second.len > 0)
            mDevice->renderSamples(data.second.buf,
                static_cast<uint>(data.second.len)*mDevice->UpdateSize, frame_step);

        size_t todo{data.first.len + data.second.len};
        mRing->writeAdvance(todo);
        dlock.unlock();

        for(size_t i{0};i < todo;i++)
        {
            if(!data.first.len)
            {
                data.first = data.second;
                data.second.buf = nullptr;
                data.second.len = 0;
            }

            result = VCALL(bufferQueue,Enqueue)(data.first.buf, mDevice->UpdateSize*mFrameSize);
            PRINTERR(result, "bufferQueue->Enqueue");
            if(SL_RESULT_SUCCESS != result)
            {
                mDevice->handleDisconnect("Failed to queue audio: 0x%08x", result);
                break;
            }

            data.first.len--;
            data.first.buf += mDevice->UpdateSize*mFrameSize;
        }
    }

    return 0;
}


void OpenSLPlayback::open(const char *name)
{
    if(!name)
        name = opensl_device;
    else if(strcmp(name, opensl_device) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};

    /* There's only one device, so if it's already open, there's nothing to do. */
    if(mEngineObj) return;

    // create engine
    SLresult result{slCreateEngine(&mEngineObj, 0, nullptr, 0, nullptr, nullptr)};
    PRINTERR(result, "slCreateEngine");
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mEngineObj,Realize)(SL_BOOLEAN_FALSE);
        PRINTERR(result, "engine->Realize");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mEngineObj,GetInterface)(SL_IID_ENGINE, &mEngine);
        PRINTERR(result, "engine->GetInterface");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mEngine,CreateOutputMix)(&mOutputMix, 0, nullptr, nullptr);
        PRINTERR(result, "engine->CreateOutputMix");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mOutputMix,Realize)(SL_BOOLEAN_FALSE);
        PRINTERR(result, "outputMix->Realize");
    }

    if(SL_RESULT_SUCCESS != result)
    {
        if(mOutputMix)
            VCALL0(mOutputMix,Destroy)();
        mOutputMix = nullptr;

        if(mEngineObj)
            VCALL0(mEngineObj,Destroy)();
        mEngineObj = nullptr;
        mEngine = nullptr;

        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to initialize OpenSL device: 0x%08x", result};
    }

    mDevice->DeviceName = name;
}

bool OpenSLPlayback::reset()
{
    SLresult result;

    if(mBufferQueueObj)
        VCALL0(mBufferQueueObj,Destroy)();
    mBufferQueueObj = nullptr;

    mRing = nullptr;

#if 0
    if(!mDevice->Flags.get<FrequencyRequest>())
    {
        /* FIXME: Disabled until I figure out how to get the Context needed for
         * the getSystemService call.
         */
        JNIEnv *env = Android_GetJNIEnv();
        jobject jctx = Android_GetContext();

        /* Get necessary stuff for using java.lang.Integer,
         * android.content.Context, and android.media.AudioManager.
         */
        jclass int_cls = JCALL(env,FindClass)("java/lang/Integer");
        jmethodID int_parseint = JCALL(env,GetStaticMethodID)(int_cls,
            "parseInt", "(Ljava/lang/String;)I"
        );
        TRACE("Integer: %p, parseInt: %p\n", int_cls, int_parseint);

        jclass ctx_cls = JCALL(env,FindClass)("android/content/Context");
        jfieldID ctx_audsvc = JCALL(env,GetStaticFieldID)(ctx_cls,
            "AUDIO_SERVICE", "Ljava/lang/String;"
        );
        jmethodID ctx_getSysSvc = JCALL(env,GetMethodID)(ctx_cls,
            "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;"
        );
        TRACE("Context: %p, AUDIO_SERVICE: %p, getSystemService: %p\n",
              ctx_cls, ctx_audsvc, ctx_getSysSvc);

        jclass audmgr_cls = JCALL(env,FindClass)("android/media/AudioManager");
        jfieldID audmgr_prop_out_srate = JCALL(env,GetStaticFieldID)(audmgr_cls,
            "PROPERTY_OUTPUT_SAMPLE_RATE", "Ljava/lang/String;"
        );
        jmethodID audmgr_getproperty = JCALL(env,GetMethodID)(audmgr_cls,
            "getProperty", "(Ljava/lang/String;)Ljava/lang/String;"
        );
        TRACE("AudioManager: %p, PROPERTY_OUTPUT_SAMPLE_RATE: %p, getProperty: %p\n",
              audmgr_cls, audmgr_prop_out_srate, audmgr_getproperty);

        const char *strchars;
        jstring strobj;

        /* Now make the calls. */
        //AudioManager audMgr = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        strobj = JCALL(env,GetStaticObjectField)(ctx_cls, ctx_audsvc);
        jobject audMgr = JCALL(env,CallObjectMethod)(jctx, ctx_getSysSvc, strobj);
        strchars = JCALL(env,GetStringUTFChars)(strobj, nullptr);
        TRACE("Context.getSystemService(%s) = %p\n", strchars, audMgr);
        JCALL(env,ReleaseStringUTFChars)(strobj, strchars);

        //String srateStr = audMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        strobj = JCALL(env,GetStaticObjectField)(audmgr_cls, audmgr_prop_out_srate);
        jstring srateStr = JCALL(env,CallObjectMethod)(audMgr, audmgr_getproperty, strobj);
        strchars = JCALL(env,GetStringUTFChars)(strobj, nullptr);
        TRACE("audMgr.getProperty(%s) = %p\n", strchars, srateStr);
        JCALL(env,ReleaseStringUTFChars)(strobj, strchars);

        //int sampleRate = Integer.parseInt(srateStr);
        sampleRate = JCALL(env,CallStaticIntMethod)(int_cls, int_parseint, srateStr);

        strchars = JCALL(env,GetStringUTFChars)(srateStr, nullptr);
        TRACE("Got system sample rate %uhz (%s)\n", sampleRate, strchars);
        JCALL(env,ReleaseStringUTFChars)(srateStr, strchars);

        if(!sampleRate) sampleRate = device->Frequency;
        else sampleRate = maxu(sampleRate, MIN_OUTPUT_RATE);
    }
#endif

    mDevice->FmtChans = DevFmtStereo;
    mDevice->FmtType = DevFmtShort;

    setDefaultWFXChannelOrder();
    mFrameSize = mDevice->frameSizeFromFmt();


    const std::array<SLInterfaceID,2> ids{{ SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION }};
    const std::array<SLboolean,2> reqs{{ SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE }};

    SLDataLocator_OutputMix loc_outmix{};
    loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    loc_outmix.outputMix = mOutputMix;

    SLDataSink audioSnk{};
    audioSnk.pLocator = &loc_outmix;
    audioSnk.pFormat = nullptr;

    SLDataLocator_AndroidSimpleBufferQueue loc_bufq{};
    loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bufq.numBuffers = mDevice->BufferSize / mDevice->UpdateSize;

    SLDataSource audioSrc{};
#ifdef SL_ANDROID_DATAFORMAT_PCM_EX
    SLAndroidDataFormat_PCM_EX format_pcm_ex{};
    format_pcm_ex.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
    format_pcm_ex.numChannels = mDevice->channelsFromFmt();
    format_pcm_ex.sampleRate = mDevice->Frequency * 1000;
    format_pcm_ex.bitsPerSample = mDevice->bytesFromFmt() * 8;
    format_pcm_ex.containerSize = format_pcm_ex.bitsPerSample;
    format_pcm_ex.channelMask = GetChannelMask(mDevice->FmtChans);
    format_pcm_ex.endianness = GetByteOrderEndianness();
    format_pcm_ex.representation = GetTypeRepresentation(mDevice->FmtType);

    audioSrc.pLocator = &loc_bufq;
    audioSrc.pFormat = &format_pcm_ex;

    result = VCALL(mEngine,CreateAudioPlayer)(&mBufferQueueObj, &audioSrc, &audioSnk, ids.size(),
        ids.data(), reqs.data());
    if(SL_RESULT_SUCCESS != result)
#endif
    {
        /* Alter sample type according to what SLDataFormat_PCM can support. */
        switch(mDevice->FmtType)
        {
        case DevFmtByte: mDevice->FmtType = DevFmtUByte; break;
        case DevFmtUInt: mDevice->FmtType = DevFmtInt; break;
        case DevFmtFloat:
        case DevFmtUShort: mDevice->FmtType = DevFmtShort; break;
        case DevFmtUByte:
        case DevFmtShort:
        case DevFmtInt:
            break;
        }

        SLDataFormat_PCM format_pcm{};
        format_pcm.formatType = SL_DATAFORMAT_PCM;
        format_pcm.numChannels = mDevice->channelsFromFmt();
        format_pcm.samplesPerSec = mDevice->Frequency * 1000;
        format_pcm.bitsPerSample = mDevice->bytesFromFmt() * 8;
        format_pcm.containerSize = format_pcm.bitsPerSample;
        format_pcm.channelMask = GetChannelMask(mDevice->FmtChans);
        format_pcm.endianness = GetByteOrderEndianness();

        audioSrc.pLocator = &loc_bufq;
        audioSrc.pFormat = &format_pcm;

        result = VCALL(mEngine,CreateAudioPlayer)(&mBufferQueueObj, &audioSrc, &audioSnk, ids.size(),
            ids.data(), reqs.data());
        PRINTERR(result, "engine->CreateAudioPlayer");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        /* Set the stream type to "media" (games, music, etc), if possible. */
        SLAndroidConfigurationItf config;
        result = VCALL(mBufferQueueObj,GetInterface)(SL_IID_ANDROIDCONFIGURATION, &config);
        PRINTERR(result, "bufferQueue->GetInterface SL_IID_ANDROIDCONFIGURATION");
        if(SL_RESULT_SUCCESS == result)
        {
            SLint32 streamType = SL_ANDROID_STREAM_MEDIA;
            result = VCALL(config,SetConfiguration)(SL_ANDROID_KEY_STREAM_TYPE, &streamType,
                sizeof(streamType));
            PRINTERR(result, "config->SetConfiguration");
        }

        /* Clear any error since this was optional. */
        result = SL_RESULT_SUCCESS;
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mBufferQueueObj,Realize)(SL_BOOLEAN_FALSE);
        PRINTERR(result, "bufferQueue->Realize");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        const uint num_updates{mDevice->BufferSize / mDevice->UpdateSize};
        mRing = RingBuffer::Create(num_updates, mFrameSize*mDevice->UpdateSize, true);
    }

    if(SL_RESULT_SUCCESS != result)
    {
        if(mBufferQueueObj)
            VCALL0(mBufferQueueObj,Destroy)();
        mBufferQueueObj = nullptr;

        return false;
    }

    return true;
}

void OpenSLPlayback::start()
{
    mRing->reset();

    SLAndroidSimpleBufferQueueItf bufferQueue;
    SLresult result{VCALL(mBufferQueueObj,GetInterface)(SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
        &bufferQueue)};
    PRINTERR(result, "bufferQueue->GetInterface");
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(bufferQueue,RegisterCallback)(&OpenSLPlayback::processC, this);
        PRINTERR(result, "bufferQueue->RegisterCallback");
    }
    if(SL_RESULT_SUCCESS != result)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to register callback: 0x%08x", result};

    try {
        mKillNow.store(false, std::memory_order_release);
        mThread = std::thread(std::mem_fn(&OpenSLPlayback::mixerProc), this);
    }
    catch(std::exception& e) {
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start mixing thread: %s", e.what()};
    }
}

void OpenSLPlayback::stop()
{
    if(mKillNow.exchange(true, std::memory_order_acq_rel) || !mThread.joinable())
        return;

    mSem.post();
    mThread.join();

    SLPlayItf player;
    SLresult result{VCALL(mBufferQueueObj,GetInterface)(SL_IID_PLAY, &player)};
    PRINTERR(result, "bufferQueue->GetInterface");
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(player,SetPlayState)(SL_PLAYSTATE_STOPPED);
        PRINTERR(result, "player->SetPlayState");
    }

    SLAndroidSimpleBufferQueueItf bufferQueue;
    result = VCALL(mBufferQueueObj,GetInterface)(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bufferQueue);
    PRINTERR(result, "bufferQueue->GetInterface");
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL0(bufferQueue,Clear)();
        PRINTERR(result, "bufferQueue->Clear");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(bufferQueue,RegisterCallback)(nullptr, nullptr);
        PRINTERR(result, "bufferQueue->RegisterCallback");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        SLAndroidSimpleBufferQueueState state;
        do {
            std::this_thread::yield();
            result = VCALL(bufferQueue,GetState)(&state);
        } while(SL_RESULT_SUCCESS == result && state.count > 0);
        PRINTERR(result, "bufferQueue->GetState");
    }
}

ClockLatency OpenSLPlayback::getClockLatency()
{
    ClockLatency ret;

    std::lock_guard<std::mutex> _{mMutex};
    ret.ClockTime = GetDeviceClockTime(mDevice);
    ret.Latency  = std::chrono::seconds{mRing->readSpace() * mDevice->UpdateSize};
    ret.Latency /= mDevice->Frequency;

    return ret;
}


struct OpenSLCapture final : public BackendBase {
    OpenSLCapture(DeviceBase *device) noexcept : BackendBase{device} { }
    ~OpenSLCapture() override;

    void process(SLAndroidSimpleBufferQueueItf bq) noexcept;
    static void processC(SLAndroidSimpleBufferQueueItf bq, void *context) noexcept
    { static_cast<OpenSLCapture*>(context)->process(bq); }

    void open(const char *name) override;
    void start() override;
    void stop() override;
    void captureSamples(al::byte *buffer, uint samples) override;
    uint availableSamples() override;

    /* engine interfaces */
    SLObjectItf mEngineObj{nullptr};
    SLEngineItf mEngine;

    /* recording interfaces */
    SLObjectItf mRecordObj{nullptr};

    RingBufferPtr mRing{nullptr};
    uint mSplOffset{0u};

    uint mFrameSize{0};

    DEF_NEWDEL(OpenSLCapture)
};

OpenSLCapture::~OpenSLCapture()
{
    if(mRecordObj)
        VCALL0(mRecordObj,Destroy)();
    mRecordObj = nullptr;

    if(mEngineObj)
        VCALL0(mEngineObj,Destroy)();
    mEngineObj = nullptr;
    mEngine = nullptr;
}


void OpenSLCapture::process(SLAndroidSimpleBufferQueueItf) noexcept
{
    /* A new chunk has been written into the ring buffer, advance it. */
    mRing->writeAdvance(1);
}


void OpenSLCapture::open(const char* name)
{
    if(!name)
        name = opensl_device;
    else if(strcmp(name, opensl_device) != 0)
        throw al::backend_exception{al::backend_error::NoDevice, "Device name \"%s\" not found",
            name};

    SLresult result{slCreateEngine(&mEngineObj, 0, nullptr, 0, nullptr, nullptr)};
    PRINTERR(result, "slCreateEngine");
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mEngineObj,Realize)(SL_BOOLEAN_FALSE);
        PRINTERR(result, "engine->Realize");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mEngineObj,GetInterface)(SL_IID_ENGINE, &mEngine);
        PRINTERR(result, "engine->GetInterface");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        mFrameSize = mDevice->frameSizeFromFmt();
        /* Ensure the total length is at least 100ms */
        uint length{maxu(mDevice->BufferSize, mDevice->Frequency/10)};
        /* Ensure the per-chunk length is at least 10ms, and no more than 50ms. */
        uint update_len{clampu(mDevice->BufferSize/3, mDevice->Frequency/100,
            mDevice->Frequency/100*5)};
        uint num_updates{(length+update_len-1) / update_len};

        mRing = RingBuffer::Create(num_updates, update_len*mFrameSize, false);

        mDevice->UpdateSize = update_len;
        mDevice->BufferSize = static_cast<uint>(mRing->writeSpace() * update_len);
    }
    if(SL_RESULT_SUCCESS == result)
    {
        const std::array<SLInterfaceID,2> ids{{ SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION }};
        const std::array<SLboolean,2> reqs{{ SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE }};

        SLDataLocator_IODevice loc_dev{};
        loc_dev.locatorType = SL_DATALOCATOR_IODEVICE;
        loc_dev.deviceType = SL_IODEVICE_AUDIOINPUT;
        loc_dev.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
        loc_dev.device = nullptr;

        SLDataSource audioSrc{};
        audioSrc.pLocator = &loc_dev;
        audioSrc.pFormat = nullptr;

        SLDataLocator_AndroidSimpleBufferQueue loc_bq{};
        loc_bq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
        loc_bq.numBuffers = mDevice->BufferSize / mDevice->UpdateSize;

        SLDataSink audioSnk{};
#ifdef SL_ANDROID_DATAFORMAT_PCM_EX
        SLAndroidDataFormat_PCM_EX format_pcm_ex{};
        format_pcm_ex.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
        format_pcm_ex.numChannels = mDevice->channelsFromFmt();
        format_pcm_ex.sampleRate = mDevice->Frequency * 1000;
        format_pcm_ex.bitsPerSample = mDevice->bytesFromFmt() * 8;
        format_pcm_ex.containerSize = format_pcm_ex.bitsPerSample;
        format_pcm_ex.channelMask = GetChannelMask(mDevice->FmtChans);
        format_pcm_ex.endianness = GetByteOrderEndianness();
        format_pcm_ex.representation = GetTypeRepresentation(mDevice->FmtType);

        audioSnk.pLocator = &loc_bq;
        audioSnk.pFormat = &format_pcm_ex;
        result = VCALL(mEngine,CreateAudioRecorder)(&mRecordObj, &audioSrc, &audioSnk,
            ids.size(), ids.data(), reqs.data());
        if(SL_RESULT_SUCCESS != result)
#endif
        {
            /* Fallback to SLDataFormat_PCM only if it supports the desired
             * sample type.
             */
            if(mDevice->FmtType == DevFmtUByte || mDevice->FmtType == DevFmtShort
                || mDevice->FmtType == DevFmtInt)
            {
                SLDataFormat_PCM format_pcm{};
                format_pcm.formatType = SL_DATAFORMAT_PCM;
                format_pcm.numChannels = mDevice->channelsFromFmt();
                format_pcm.samplesPerSec = mDevice->Frequency * 1000;
                format_pcm.bitsPerSample = mDevice->bytesFromFmt() * 8;
                format_pcm.containerSize = format_pcm.bitsPerSample;
                format_pcm.channelMask = GetChannelMask(mDevice->FmtChans);
                format_pcm.endianness = GetByteOrderEndianness();

                audioSnk.pLocator = &loc_bq;
                audioSnk.pFormat = &format_pcm;
                result = VCALL(mEngine,CreateAudioRecorder)(&mRecordObj, &audioSrc, &audioSnk,
                    ids.size(), ids.data(), reqs.data());
            }
            PRINTERR(result, "engine->CreateAudioRecorder");
        }
    }
    if(SL_RESULT_SUCCESS == result)
    {
        /* Set the record preset to "generic", if possible. */
        SLAndroidConfigurationItf config;
        result = VCALL(mRecordObj,GetInterface)(SL_IID_ANDROIDCONFIGURATION, &config);
        PRINTERR(result, "recordObj->GetInterface SL_IID_ANDROIDCONFIGURATION");
        if(SL_RESULT_SUCCESS == result)
        {
            SLuint32 preset = SL_ANDROID_RECORDING_PRESET_GENERIC;
            result = VCALL(config,SetConfiguration)(SL_ANDROID_KEY_RECORDING_PRESET, &preset,
                sizeof(preset));
            PRINTERR(result, "config->SetConfiguration");
        }

        /* Clear any error since this was optional. */
        result = SL_RESULT_SUCCESS;
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mRecordObj,Realize)(SL_BOOLEAN_FALSE);
        PRINTERR(result, "recordObj->Realize");
    }

    SLAndroidSimpleBufferQueueItf bufferQueue;
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(mRecordObj,GetInterface)(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bufferQueue);
        PRINTERR(result, "recordObj->GetInterface");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(bufferQueue,RegisterCallback)(&OpenSLCapture::processC, this);
        PRINTERR(result, "bufferQueue->RegisterCallback");
    }
    if(SL_RESULT_SUCCESS == result)
    {
        const uint chunk_size{mDevice->UpdateSize * mFrameSize};
        const auto silence = (mDevice->FmtType == DevFmtUByte) ? al::byte{0x80} : al::byte{0};

        auto data = mRing->getWriteVector();
        std::fill_n(data.first.buf, data.first.len*chunk_size, silence);
        std::fill_n(data.second.buf, data.second.len*chunk_size, silence);
        for(size_t i{0u};i < data.first.len && SL_RESULT_SUCCESS == result;i++)
        {
            result = VCALL(bufferQueue,Enqueue)(data.first.buf + chunk_size*i, chunk_size);
            PRINTERR(result, "bufferQueue->Enqueue");
        }
        for(size_t i{0u};i < data.second.len && SL_RESULT_SUCCESS == result;i++)
        {
            result = VCALL(bufferQueue,Enqueue)(data.second.buf + chunk_size*i, chunk_size);
            PRINTERR(result, "bufferQueue->Enqueue");
        }
    }

    if(SL_RESULT_SUCCESS != result)
    {
        if(mRecordObj)
            VCALL0(mRecordObj,Destroy)();
        mRecordObj = nullptr;

        if(mEngineObj)
            VCALL0(mEngineObj,Destroy)();
        mEngineObj = nullptr;
        mEngine = nullptr;

        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to initialize OpenSL device: 0x%08x", result};
    }

    mDevice->DeviceName = name;
}

void OpenSLCapture::start()
{
    SLRecordItf record;
    SLresult result{VCALL(mRecordObj,GetInterface)(SL_IID_RECORD, &record)};
    PRINTERR(result, "recordObj->GetInterface");

    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(record,SetRecordState)(SL_RECORDSTATE_RECORDING);
        PRINTERR(result, "record->SetRecordState");
    }
    if(SL_RESULT_SUCCESS != result)
        throw al::backend_exception{al::backend_error::DeviceError,
            "Failed to start capture: 0x%08x", result};
}

void OpenSLCapture::stop()
{
    SLRecordItf record;
    SLresult result{VCALL(mRecordObj,GetInterface)(SL_IID_RECORD, &record)};
    PRINTERR(result, "recordObj->GetInterface");

    if(SL_RESULT_SUCCESS == result)
    {
        result = VCALL(record,SetRecordState)(SL_RECORDSTATE_PAUSED);
        PRINTERR(result, "record->SetRecordState");
    }
}

void OpenSLCapture::captureSamples(al::byte *buffer, uint samples)
{
    const uint update_size{mDevice->UpdateSize};
    const uint chunk_size{update_size * mFrameSize};
    const auto silence = (mDevice->FmtType == DevFmtUByte) ? al::byte{0x80} : al::byte{0};

    /* Read the desired samples from the ring buffer then advance its read
     * pointer.
     */
    size_t adv_count{0};
    auto rdata = mRing->getReadVector();
    for(uint i{0};i < samples;)
    {
        const uint rem{minu(samples - i, update_size - mSplOffset)};
        std::copy_n(rdata.first.buf + mSplOffset*size_t{mFrameSize}, rem*size_t{mFrameSize},
            buffer + i*size_t{mFrameSize});

        mSplOffset += rem;
        if(mSplOffset == update_size)
        {
            /* Finished a chunk, reset the offset and advance the read pointer. */
            mSplOffset = 0;

            ++adv_count;
            rdata.first.len -= 1;
            if(!rdata.first.len)
                rdata.first = rdata.second;
            else
                rdata.first.buf += chunk_size;
        }

        i += rem;
    }
    mRing->readAdvance(adv_count);

    SLAndroidSimpleBufferQueueItf bufferQueue{};
    if LIKELY(mDevice->Connected.load(std::memory_order_acquire))
    {
        const SLresult result{VCALL(mRecordObj,GetInterface)(SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
            &bufferQueue)};
        PRINTERR(result, "recordObj->GetInterface");
        if UNLIKELY(SL_RESULT_SUCCESS != result)
        {
            mDevice->handleDisconnect("Failed to get capture buffer queue: 0x%08x", result);
            bufferQueue = nullptr;
        }
    }

    if LIKELY(bufferQueue)
    {
        SLresult result{SL_RESULT_SUCCESS};
        auto wdata = mRing->getWriteVector();
        std::fill_n(wdata.first.buf, wdata.first.len*chunk_size, silence);
        for(size_t i{0u};i < wdata.first.len && SL_RESULT_SUCCESS == result;i++)
        {
            result = VCALL(bufferQueue,Enqueue)(wdata.first.buf + chunk_size*i, chunk_size);
            PRINTERR(result, "bufferQueue->Enqueue");
        }
        if(wdata.second.len > 0)
        {
            std::fill_n(wdata.second.buf, wdata.second.len*chunk_size, silence);
            for(size_t i{0u};i < wdata.second.len && SL_RESULT_SUCCESS == result;i++)
            {
                result = VCALL(bufferQueue,Enqueue)(wdata.second.buf + chunk_size*i, chunk_size);
                PRINTERR(result, "bufferQueue->Enqueue");
            }
        }
    }
}

uint OpenSLCapture::availableSamples()
{ return static_cast<uint>(mRing->readSpace()*mDevice->UpdateSize - mSplOffset); }

} // namespace

bool OSLBackendFactory::init() { return true; }

bool OSLBackendFactory::querySupport(BackendType type)
{ return (type == BackendType::Playback || type == BackendType::Capture); }

std::string OSLBackendFactory::probe(BackendType type)
{
    std::string outnames;
    switch(type)
    {
    case BackendType::Playback:
    case BackendType::Capture:
        /* Includes null char. */
        outnames.append(opensl_device, sizeof(opensl_device));
        break;
    }
    return outnames;
}

BackendPtr OSLBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if(type == BackendType::Playback)
        return BackendPtr{new OpenSLPlayback{device}};
    if(type == BackendType::Capture)
        return BackendPtr{new OpenSLCapture{device}};
    return nullptr;
}

BackendFactory &OSLBackendFactory::getFactory()
{
    static OSLBackendFactory factory{};
    return factory;
}
