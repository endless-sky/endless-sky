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

#include "version.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "AL/efx.h"

#include "al/auxeffectslot.h"
#include "al/buffer.h"
#include "al/effect.h"
#include "al/filter.h"
#include "al/listener.h"
#include "al/source.h"
#include "albit.h"
#include "albyte.h"
#include "alconfig.h"
#include "almalloc.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "alspan.h"
#include "alstring.h"
#include "alu.h"
#include "atomic.h"
#include "context.h"
#include "core/ambidefs.h"
#include "core/bformatdec.h"
#include "core/bs2b.h"
#include "core/context.h"
#include "core/cpu_caps.h"
#include "core/devformat.h"
#include "core/device.h"
#include "core/effectslot.h"
#include "core/except.h"
#include "core/helpers.h"
#include "core/mastering.h"
#include "core/mixer/hrtfdefs.h"
#include "core/fpu_ctrl.h"
#include "core/front_stablizer.h"
#include "core/logging.h"
#include "core/uhjfilter.h"
#include "core/voice.h"
#include "core/voice_change.h"
#include "device.h"
#include "effects/base.h"
#include "inprogext.h"
#include "intrusive_ptr.h"
#include "opthelpers.h"
#include "strutils.h"
#include "threads.h"
#include "vector.h"

#include "backends/base.h"
#include "backends/null.h"
#include "backends/loopback.h"
#ifdef HAVE_PIPEWIRE
#include "backends/pipewire.h"
#endif
#ifdef HAVE_JACK
#include "backends/jack.h"
#endif
#ifdef HAVE_PULSEAUDIO
#include "backends/pulseaudio.h"
#endif
#ifdef HAVE_ALSA
#include "backends/alsa.h"
#endif
#ifdef HAVE_WASAPI
#include "backends/wasapi.h"
#endif
#ifdef HAVE_COREAUDIO
#include "backends/coreaudio.h"
#endif
#ifdef HAVE_OPENSL
#include "backends/opensl.h"
#endif
#ifdef HAVE_OBOE
#include "backends/oboe.h"
#endif
#ifdef HAVE_SOLARIS
#include "backends/solaris.h"
#endif
#ifdef HAVE_SNDIO
#include "backends/sndio.h"
#endif
#ifdef HAVE_OSS
#include "backends/oss.h"
#endif
#ifdef HAVE_DSOUND
#include "backends/dsound.h"
#endif
#ifdef HAVE_WINMM
#include "backends/winmm.h"
#endif
#ifdef HAVE_PORTAUDIO
#include "backends/portaudio.h"
#endif
#ifdef HAVE_SDL2
#include "backends/sdl2.h"
#endif
#ifdef HAVE_WAVE
#include "backends/wave.h"
#endif

#ifdef ALSOFT_EAX
#include "al/eax_globals.h"
#include "al/eax_x_ram.h"
#endif // ALSOFT_EAX


FILE *gLogFile{stderr};
#ifdef _DEBUG
LogLevel gLogLevel{LogLevel::Warning};
#else
LogLevel gLogLevel{LogLevel::Error};
#endif

/************************************************
 * Library initialization
 ************************************************/
#if defined(_WIN32) && !defined(AL_LIBTYPE_STATIC)
BOOL APIENTRY DllMain(HINSTANCE module, DWORD reason, LPVOID /*reserved*/)
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        /* Pin the DLL so we won't get unloaded until the process terminates */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<WCHAR*>(module), &module);
        break;
    }
    return TRUE;
}
#endif

namespace {

using namespace std::placeholders;
using std::chrono::seconds;
using std::chrono::nanoseconds;

using voidp = void*;
using float2 = std::array<float,2>;


/************************************************
 * Backends
 ************************************************/
struct BackendInfo {
    const char *name;
    BackendFactory& (*getFactory)(void);
};

BackendInfo BackendList[] = {
#ifdef HAVE_PIPEWIRE
    { "pipewire", PipeWireBackendFactory::getFactory },
#endif
#ifdef HAVE_PULSEAUDIO
    { "pulse", PulseBackendFactory::getFactory },
#endif
#ifdef HAVE_WASAPI
    { "wasapi", WasapiBackendFactory::getFactory },
#endif
#ifdef HAVE_COREAUDIO
    { "core", CoreAudioBackendFactory::getFactory },
#endif
#ifdef HAVE_OBOE
    { "oboe", OboeBackendFactory::getFactory },
#endif
#ifdef HAVE_OPENSL
    { "opensl", OSLBackendFactory::getFactory },
#endif
#ifdef HAVE_SOLARIS
    { "solaris", SolarisBackendFactory::getFactory },
#endif
#ifdef HAVE_SNDIO
    { "sndio", SndIOBackendFactory::getFactory },
#endif
#ifdef HAVE_ALSA
    { "alsa", AlsaBackendFactory::getFactory },
#endif
#ifdef HAVE_OSS
    { "oss", OSSBackendFactory::getFactory },
#endif
#ifdef HAVE_JACK
    { "jack", JackBackendFactory::getFactory },
#endif
#ifdef HAVE_DSOUND
    { "dsound", DSoundBackendFactory::getFactory },
#endif
#ifdef HAVE_WINMM
    { "winmm", WinMMBackendFactory::getFactory },
#endif
#ifdef HAVE_PORTAUDIO
    { "port", PortBackendFactory::getFactory },
#endif
#ifdef HAVE_SDL2
    { "sdl2", SDL2BackendFactory::getFactory },
#endif

    { "null", NullBackendFactory::getFactory },
#ifdef HAVE_WAVE
    { "wave", WaveBackendFactory::getFactory },
#endif
};

BackendFactory *PlaybackFactory{};
BackendFactory *CaptureFactory{};


/************************************************
 * Functions, enums, and errors
 ************************************************/
#define DECL(x) { #x, reinterpret_cast<void*>(x) }
const struct {
    const char *funcName;
    void *address;
} alcFunctions[] = {
    DECL(alcCreateContext),
    DECL(alcMakeContextCurrent),
    DECL(alcProcessContext),
    DECL(alcSuspendContext),
    DECL(alcDestroyContext),
    DECL(alcGetCurrentContext),
    DECL(alcGetContextsDevice),
    DECL(alcOpenDevice),
    DECL(alcCloseDevice),
    DECL(alcGetError),
    DECL(alcIsExtensionPresent),
    DECL(alcGetProcAddress),
    DECL(alcGetEnumValue),
    DECL(alcGetString),
    DECL(alcGetIntegerv),
    DECL(alcCaptureOpenDevice),
    DECL(alcCaptureCloseDevice),
    DECL(alcCaptureStart),
    DECL(alcCaptureStop),
    DECL(alcCaptureSamples),

    DECL(alcSetThreadContext),
    DECL(alcGetThreadContext),

    DECL(alcLoopbackOpenDeviceSOFT),
    DECL(alcIsRenderFormatSupportedSOFT),
    DECL(alcRenderSamplesSOFT),

    DECL(alcDevicePauseSOFT),
    DECL(alcDeviceResumeSOFT),

    DECL(alcGetStringiSOFT),
    DECL(alcResetDeviceSOFT),

    DECL(alcGetInteger64vSOFT),

    DECL(alcReopenDeviceSOFT),

    DECL(alEnable),
    DECL(alDisable),
    DECL(alIsEnabled),
    DECL(alGetString),
    DECL(alGetBooleanv),
    DECL(alGetIntegerv),
    DECL(alGetFloatv),
    DECL(alGetDoublev),
    DECL(alGetBoolean),
    DECL(alGetInteger),
    DECL(alGetFloat),
    DECL(alGetDouble),
    DECL(alGetError),
    DECL(alIsExtensionPresent),
    DECL(alGetProcAddress),
    DECL(alGetEnumValue),
    DECL(alListenerf),
    DECL(alListener3f),
    DECL(alListenerfv),
    DECL(alListeneri),
    DECL(alListener3i),
    DECL(alListeneriv),
    DECL(alGetListenerf),
    DECL(alGetListener3f),
    DECL(alGetListenerfv),
    DECL(alGetListeneri),
    DECL(alGetListener3i),
    DECL(alGetListeneriv),
    DECL(alGenSources),
    DECL(alDeleteSources),
    DECL(alIsSource),
    DECL(alSourcef),
    DECL(alSource3f),
    DECL(alSourcefv),
    DECL(alSourcei),
    DECL(alSource3i),
    DECL(alSourceiv),
    DECL(alGetSourcef),
    DECL(alGetSource3f),
    DECL(alGetSourcefv),
    DECL(alGetSourcei),
    DECL(alGetSource3i),
    DECL(alGetSourceiv),
    DECL(alSourcePlayv),
    DECL(alSourceStopv),
    DECL(alSourceRewindv),
    DECL(alSourcePausev),
    DECL(alSourcePlay),
    DECL(alSourceStop),
    DECL(alSourceRewind),
    DECL(alSourcePause),
    DECL(alSourceQueueBuffers),
    DECL(alSourceUnqueueBuffers),
    DECL(alGenBuffers),
    DECL(alDeleteBuffers),
    DECL(alIsBuffer),
    DECL(alBufferData),
    DECL(alBufferf),
    DECL(alBuffer3f),
    DECL(alBufferfv),
    DECL(alBufferi),
    DECL(alBuffer3i),
    DECL(alBufferiv),
    DECL(alGetBufferf),
    DECL(alGetBuffer3f),
    DECL(alGetBufferfv),
    DECL(alGetBufferi),
    DECL(alGetBuffer3i),
    DECL(alGetBufferiv),
    DECL(alDopplerFactor),
    DECL(alDopplerVelocity),
    DECL(alSpeedOfSound),
    DECL(alDistanceModel),

    DECL(alGenFilters),
    DECL(alDeleteFilters),
    DECL(alIsFilter),
    DECL(alFilteri),
    DECL(alFilteriv),
    DECL(alFilterf),
    DECL(alFilterfv),
    DECL(alGetFilteri),
    DECL(alGetFilteriv),
    DECL(alGetFilterf),
    DECL(alGetFilterfv),
    DECL(alGenEffects),
    DECL(alDeleteEffects),
    DECL(alIsEffect),
    DECL(alEffecti),
    DECL(alEffectiv),
    DECL(alEffectf),
    DECL(alEffectfv),
    DECL(alGetEffecti),
    DECL(alGetEffectiv),
    DECL(alGetEffectf),
    DECL(alGetEffectfv),
    DECL(alGenAuxiliaryEffectSlots),
    DECL(alDeleteAuxiliaryEffectSlots),
    DECL(alIsAuxiliaryEffectSlot),
    DECL(alAuxiliaryEffectSloti),
    DECL(alAuxiliaryEffectSlotiv),
    DECL(alAuxiliaryEffectSlotf),
    DECL(alAuxiliaryEffectSlotfv),
    DECL(alGetAuxiliaryEffectSloti),
    DECL(alGetAuxiliaryEffectSlotiv),
    DECL(alGetAuxiliaryEffectSlotf),
    DECL(alGetAuxiliaryEffectSlotfv),

    DECL(alDeferUpdatesSOFT),
    DECL(alProcessUpdatesSOFT),

    DECL(alSourcedSOFT),
    DECL(alSource3dSOFT),
    DECL(alSourcedvSOFT),
    DECL(alGetSourcedSOFT),
    DECL(alGetSource3dSOFT),
    DECL(alGetSourcedvSOFT),
    DECL(alSourcei64SOFT),
    DECL(alSource3i64SOFT),
    DECL(alSourcei64vSOFT),
    DECL(alGetSourcei64SOFT),
    DECL(alGetSource3i64SOFT),
    DECL(alGetSourcei64vSOFT),

    DECL(alGetStringiSOFT),

    DECL(alBufferStorageSOFT),
    DECL(alMapBufferSOFT),
    DECL(alUnmapBufferSOFT),
    DECL(alFlushMappedBufferSOFT),

    DECL(alEventControlSOFT),
    DECL(alEventCallbackSOFT),
    DECL(alGetPointerSOFT),
    DECL(alGetPointervSOFT),

    DECL(alBufferCallbackSOFT),
    DECL(alGetBufferPtrSOFT),
    DECL(alGetBuffer3PtrSOFT),
    DECL(alGetBufferPtrvSOFT),

    DECL(alAuxiliaryEffectSlotPlaySOFT),
    DECL(alAuxiliaryEffectSlotPlayvSOFT),
    DECL(alAuxiliaryEffectSlotStopSOFT),
    DECL(alAuxiliaryEffectSlotStopvSOFT),
#ifdef ALSOFT_EAX
}, eaxFunctions[] = {
    DECL(EAXGet),
    DECL(EAXSet),
    DECL(EAXGetBufferMode),
    DECL(EAXSetBufferMode),
#endif
};
#undef DECL

#define DECL(x) { #x, (x) }
constexpr struct {
    const ALCchar *enumName;
    ALCenum value;
} alcEnumerations[] = {
    DECL(ALC_INVALID),
    DECL(ALC_FALSE),
    DECL(ALC_TRUE),

    DECL(ALC_MAJOR_VERSION),
    DECL(ALC_MINOR_VERSION),
    DECL(ALC_ATTRIBUTES_SIZE),
    DECL(ALC_ALL_ATTRIBUTES),
    DECL(ALC_DEFAULT_DEVICE_SPECIFIER),
    DECL(ALC_DEVICE_SPECIFIER),
    DECL(ALC_ALL_DEVICES_SPECIFIER),
    DECL(ALC_DEFAULT_ALL_DEVICES_SPECIFIER),
    DECL(ALC_EXTENSIONS),
    DECL(ALC_FREQUENCY),
    DECL(ALC_REFRESH),
    DECL(ALC_SYNC),
    DECL(ALC_MONO_SOURCES),
    DECL(ALC_STEREO_SOURCES),
    DECL(ALC_CAPTURE_DEVICE_SPECIFIER),
    DECL(ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER),
    DECL(ALC_CAPTURE_SAMPLES),
    DECL(ALC_CONNECTED),

    DECL(ALC_EFX_MAJOR_VERSION),
    DECL(ALC_EFX_MINOR_VERSION),
    DECL(ALC_MAX_AUXILIARY_SENDS),

    DECL(ALC_FORMAT_CHANNELS_SOFT),
    DECL(ALC_FORMAT_TYPE_SOFT),

    DECL(ALC_MONO_SOFT),
    DECL(ALC_STEREO_SOFT),
    DECL(ALC_QUAD_SOFT),
    DECL(ALC_5POINT1_SOFT),
    DECL(ALC_6POINT1_SOFT),
    DECL(ALC_7POINT1_SOFT),
    DECL(ALC_BFORMAT3D_SOFT),

    DECL(ALC_BYTE_SOFT),
    DECL(ALC_UNSIGNED_BYTE_SOFT),
    DECL(ALC_SHORT_SOFT),
    DECL(ALC_UNSIGNED_SHORT_SOFT),
    DECL(ALC_INT_SOFT),
    DECL(ALC_UNSIGNED_INT_SOFT),
    DECL(ALC_FLOAT_SOFT),

    DECL(ALC_HRTF_SOFT),
    DECL(ALC_DONT_CARE_SOFT),
    DECL(ALC_HRTF_STATUS_SOFT),
    DECL(ALC_HRTF_DISABLED_SOFT),
    DECL(ALC_HRTF_ENABLED_SOFT),
    DECL(ALC_HRTF_DENIED_SOFT),
    DECL(ALC_HRTF_REQUIRED_SOFT),
    DECL(ALC_HRTF_HEADPHONES_DETECTED_SOFT),
    DECL(ALC_HRTF_UNSUPPORTED_FORMAT_SOFT),
    DECL(ALC_NUM_HRTF_SPECIFIERS_SOFT),
    DECL(ALC_HRTF_SPECIFIER_SOFT),
    DECL(ALC_HRTF_ID_SOFT),

    DECL(ALC_AMBISONIC_LAYOUT_SOFT),
    DECL(ALC_AMBISONIC_SCALING_SOFT),
    DECL(ALC_AMBISONIC_ORDER_SOFT),
    DECL(ALC_ACN_SOFT),
    DECL(ALC_FUMA_SOFT),
    DECL(ALC_N3D_SOFT),
    DECL(ALC_SN3D_SOFT),

    DECL(ALC_OUTPUT_LIMITER_SOFT),

    DECL(ALC_OUTPUT_MODE_SOFT),
    DECL(ALC_ANY_SOFT),
    DECL(ALC_STEREO_BASIC_SOFT),
    DECL(ALC_STEREO_UHJ_SOFT),
    DECL(ALC_STEREO_HRTF_SOFT),
    DECL(ALC_SURROUND_5_1_SOFT),
    DECL(ALC_SURROUND_6_1_SOFT),
    DECL(ALC_SURROUND_7_1_SOFT),

    DECL(ALC_NO_ERROR),
    DECL(ALC_INVALID_DEVICE),
    DECL(ALC_INVALID_CONTEXT),
    DECL(ALC_INVALID_ENUM),
    DECL(ALC_INVALID_VALUE),
    DECL(ALC_OUT_OF_MEMORY),


    DECL(AL_INVALID),
    DECL(AL_NONE),
    DECL(AL_FALSE),
    DECL(AL_TRUE),

    DECL(AL_SOURCE_RELATIVE),
    DECL(AL_CONE_INNER_ANGLE),
    DECL(AL_CONE_OUTER_ANGLE),
    DECL(AL_PITCH),
    DECL(AL_POSITION),
    DECL(AL_DIRECTION),
    DECL(AL_VELOCITY),
    DECL(AL_LOOPING),
    DECL(AL_BUFFER),
    DECL(AL_GAIN),
    DECL(AL_MIN_GAIN),
    DECL(AL_MAX_GAIN),
    DECL(AL_ORIENTATION),
    DECL(AL_REFERENCE_DISTANCE),
    DECL(AL_ROLLOFF_FACTOR),
    DECL(AL_CONE_OUTER_GAIN),
    DECL(AL_MAX_DISTANCE),
    DECL(AL_SEC_OFFSET),
    DECL(AL_SAMPLE_OFFSET),
    DECL(AL_BYTE_OFFSET),
    DECL(AL_SOURCE_TYPE),
    DECL(AL_STATIC),
    DECL(AL_STREAMING),
    DECL(AL_UNDETERMINED),
    DECL(AL_METERS_PER_UNIT),
    DECL(AL_LOOP_POINTS_SOFT),
    DECL(AL_DIRECT_CHANNELS_SOFT),

    DECL(AL_DIRECT_FILTER),
    DECL(AL_AUXILIARY_SEND_FILTER),
    DECL(AL_AIR_ABSORPTION_FACTOR),
    DECL(AL_ROOM_ROLLOFF_FACTOR),
    DECL(AL_CONE_OUTER_GAINHF),
    DECL(AL_DIRECT_FILTER_GAINHF_AUTO),
    DECL(AL_AUXILIARY_SEND_FILTER_GAIN_AUTO),
    DECL(AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO),

    DECL(AL_SOURCE_STATE),
    DECL(AL_INITIAL),
    DECL(AL_PLAYING),
    DECL(AL_PAUSED),
    DECL(AL_STOPPED),

    DECL(AL_BUFFERS_QUEUED),
    DECL(AL_BUFFERS_PROCESSED),

    DECL(AL_FORMAT_MONO8),
    DECL(AL_FORMAT_MONO16),
    DECL(AL_FORMAT_MONO_FLOAT32),
    DECL(AL_FORMAT_MONO_DOUBLE_EXT),
    DECL(AL_FORMAT_STEREO8),
    DECL(AL_FORMAT_STEREO16),
    DECL(AL_FORMAT_STEREO_FLOAT32),
    DECL(AL_FORMAT_STEREO_DOUBLE_EXT),
    DECL(AL_FORMAT_MONO_IMA4),
    DECL(AL_FORMAT_STEREO_IMA4),
    DECL(AL_FORMAT_MONO_MSADPCM_SOFT),
    DECL(AL_FORMAT_STEREO_MSADPCM_SOFT),
    DECL(AL_FORMAT_QUAD8_LOKI),
    DECL(AL_FORMAT_QUAD16_LOKI),
    DECL(AL_FORMAT_QUAD8),
    DECL(AL_FORMAT_QUAD16),
    DECL(AL_FORMAT_QUAD32),
    DECL(AL_FORMAT_51CHN8),
    DECL(AL_FORMAT_51CHN16),
    DECL(AL_FORMAT_51CHN32),
    DECL(AL_FORMAT_61CHN8),
    DECL(AL_FORMAT_61CHN16),
    DECL(AL_FORMAT_61CHN32),
    DECL(AL_FORMAT_71CHN8),
    DECL(AL_FORMAT_71CHN16),
    DECL(AL_FORMAT_71CHN32),
    DECL(AL_FORMAT_REAR8),
    DECL(AL_FORMAT_REAR16),
    DECL(AL_FORMAT_REAR32),
    DECL(AL_FORMAT_MONO_MULAW),
    DECL(AL_FORMAT_MONO_MULAW_EXT),
    DECL(AL_FORMAT_STEREO_MULAW),
    DECL(AL_FORMAT_STEREO_MULAW_EXT),
    DECL(AL_FORMAT_QUAD_MULAW),
    DECL(AL_FORMAT_51CHN_MULAW),
    DECL(AL_FORMAT_61CHN_MULAW),
    DECL(AL_FORMAT_71CHN_MULAW),
    DECL(AL_FORMAT_REAR_MULAW),
    DECL(AL_FORMAT_MONO_ALAW_EXT),
    DECL(AL_FORMAT_STEREO_ALAW_EXT),

    DECL(AL_FORMAT_BFORMAT2D_8),
    DECL(AL_FORMAT_BFORMAT2D_16),
    DECL(AL_FORMAT_BFORMAT2D_FLOAT32),
    DECL(AL_FORMAT_BFORMAT2D_MULAW),
    DECL(AL_FORMAT_BFORMAT3D_8),
    DECL(AL_FORMAT_BFORMAT3D_16),
    DECL(AL_FORMAT_BFORMAT3D_FLOAT32),
    DECL(AL_FORMAT_BFORMAT3D_MULAW),

    DECL(AL_FREQUENCY),
    DECL(AL_BITS),
    DECL(AL_CHANNELS),
    DECL(AL_SIZE),
    DECL(AL_UNPACK_BLOCK_ALIGNMENT_SOFT),
    DECL(AL_PACK_BLOCK_ALIGNMENT_SOFT),

    DECL(AL_SOURCE_RADIUS),

    DECL(AL_STEREO_ANGLES),

    DECL(AL_UNUSED),
    DECL(AL_PENDING),
    DECL(AL_PROCESSED),

    DECL(AL_NO_ERROR),
    DECL(AL_INVALID_NAME),
    DECL(AL_INVALID_ENUM),
    DECL(AL_INVALID_VALUE),
    DECL(AL_INVALID_OPERATION),
    DECL(AL_OUT_OF_MEMORY),

    DECL(AL_VENDOR),
    DECL(AL_VERSION),
    DECL(AL_RENDERER),
    DECL(AL_EXTENSIONS),

    DECL(AL_DOPPLER_FACTOR),
    DECL(AL_DOPPLER_VELOCITY),
    DECL(AL_DISTANCE_MODEL),
    DECL(AL_SPEED_OF_SOUND),
    DECL(AL_SOURCE_DISTANCE_MODEL),
    DECL(AL_DEFERRED_UPDATES_SOFT),
    DECL(AL_GAIN_LIMIT_SOFT),

    DECL(AL_INVERSE_DISTANCE),
    DECL(AL_INVERSE_DISTANCE_CLAMPED),
    DECL(AL_LINEAR_DISTANCE),
    DECL(AL_LINEAR_DISTANCE_CLAMPED),
    DECL(AL_EXPONENT_DISTANCE),
    DECL(AL_EXPONENT_DISTANCE_CLAMPED),

    DECL(AL_FILTER_TYPE),
    DECL(AL_FILTER_NULL),
    DECL(AL_FILTER_LOWPASS),
    DECL(AL_FILTER_HIGHPASS),
    DECL(AL_FILTER_BANDPASS),

    DECL(AL_LOWPASS_GAIN),
    DECL(AL_LOWPASS_GAINHF),

    DECL(AL_HIGHPASS_GAIN),
    DECL(AL_HIGHPASS_GAINLF),

    DECL(AL_BANDPASS_GAIN),
    DECL(AL_BANDPASS_GAINHF),
    DECL(AL_BANDPASS_GAINLF),

    DECL(AL_EFFECT_TYPE),
    DECL(AL_EFFECT_NULL),
    DECL(AL_EFFECT_REVERB),
    DECL(AL_EFFECT_EAXREVERB),
    DECL(AL_EFFECT_CHORUS),
    DECL(AL_EFFECT_DISTORTION),
    DECL(AL_EFFECT_ECHO),
    DECL(AL_EFFECT_FLANGER),
    DECL(AL_EFFECT_PITCH_SHIFTER),
    DECL(AL_EFFECT_FREQUENCY_SHIFTER),
    DECL(AL_EFFECT_VOCAL_MORPHER),
    DECL(AL_EFFECT_RING_MODULATOR),
    DECL(AL_EFFECT_AUTOWAH),
    DECL(AL_EFFECT_COMPRESSOR),
    DECL(AL_EFFECT_EQUALIZER),
    DECL(AL_EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT),
    DECL(AL_EFFECT_DEDICATED_DIALOGUE),

    DECL(AL_EFFECTSLOT_EFFECT),
    DECL(AL_EFFECTSLOT_GAIN),
    DECL(AL_EFFECTSLOT_AUXILIARY_SEND_AUTO),
    DECL(AL_EFFECTSLOT_NULL),

    DECL(AL_EAXREVERB_DENSITY),
    DECL(AL_EAXREVERB_DIFFUSION),
    DECL(AL_EAXREVERB_GAIN),
    DECL(AL_EAXREVERB_GAINHF),
    DECL(AL_EAXREVERB_GAINLF),
    DECL(AL_EAXREVERB_DECAY_TIME),
    DECL(AL_EAXREVERB_DECAY_HFRATIO),
    DECL(AL_EAXREVERB_DECAY_LFRATIO),
    DECL(AL_EAXREVERB_REFLECTIONS_GAIN),
    DECL(AL_EAXREVERB_REFLECTIONS_DELAY),
    DECL(AL_EAXREVERB_REFLECTIONS_PAN),
    DECL(AL_EAXREVERB_LATE_REVERB_GAIN),
    DECL(AL_EAXREVERB_LATE_REVERB_DELAY),
    DECL(AL_EAXREVERB_LATE_REVERB_PAN),
    DECL(AL_EAXREVERB_ECHO_TIME),
    DECL(AL_EAXREVERB_ECHO_DEPTH),
    DECL(AL_EAXREVERB_MODULATION_TIME),
    DECL(AL_EAXREVERB_MODULATION_DEPTH),
    DECL(AL_EAXREVERB_AIR_ABSORPTION_GAINHF),
    DECL(AL_EAXREVERB_HFREFERENCE),
    DECL(AL_EAXREVERB_LFREFERENCE),
    DECL(AL_EAXREVERB_ROOM_ROLLOFF_FACTOR),
    DECL(AL_EAXREVERB_DECAY_HFLIMIT),

    DECL(AL_REVERB_DENSITY),
    DECL(AL_REVERB_DIFFUSION),
    DECL(AL_REVERB_GAIN),
    DECL(AL_REVERB_GAINHF),
    DECL(AL_REVERB_DECAY_TIME),
    DECL(AL_REVERB_DECAY_HFRATIO),
    DECL(AL_REVERB_REFLECTIONS_GAIN),
    DECL(AL_REVERB_REFLECTIONS_DELAY),
    DECL(AL_REVERB_LATE_REVERB_GAIN),
    DECL(AL_REVERB_LATE_REVERB_DELAY),
    DECL(AL_REVERB_AIR_ABSORPTION_GAINHF),
    DECL(AL_REVERB_ROOM_ROLLOFF_FACTOR),
    DECL(AL_REVERB_DECAY_HFLIMIT),

    DECL(AL_CHORUS_WAVEFORM),
    DECL(AL_CHORUS_PHASE),
    DECL(AL_CHORUS_RATE),
    DECL(AL_CHORUS_DEPTH),
    DECL(AL_CHORUS_FEEDBACK),
    DECL(AL_CHORUS_DELAY),

    DECL(AL_DISTORTION_EDGE),
    DECL(AL_DISTORTION_GAIN),
    DECL(AL_DISTORTION_LOWPASS_CUTOFF),
    DECL(AL_DISTORTION_EQCENTER),
    DECL(AL_DISTORTION_EQBANDWIDTH),

    DECL(AL_ECHO_DELAY),
    DECL(AL_ECHO_LRDELAY),
    DECL(AL_ECHO_DAMPING),
    DECL(AL_ECHO_FEEDBACK),
    DECL(AL_ECHO_SPREAD),

    DECL(AL_FLANGER_WAVEFORM),
    DECL(AL_FLANGER_PHASE),
    DECL(AL_FLANGER_RATE),
    DECL(AL_FLANGER_DEPTH),
    DECL(AL_FLANGER_FEEDBACK),
    DECL(AL_FLANGER_DELAY),

    DECL(AL_FREQUENCY_SHIFTER_FREQUENCY),
    DECL(AL_FREQUENCY_SHIFTER_LEFT_DIRECTION),
    DECL(AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION),

    DECL(AL_RING_MODULATOR_FREQUENCY),
    DECL(AL_RING_MODULATOR_HIGHPASS_CUTOFF),
    DECL(AL_RING_MODULATOR_WAVEFORM),

    DECL(AL_PITCH_SHIFTER_COARSE_TUNE),
    DECL(AL_PITCH_SHIFTER_FINE_TUNE),

    DECL(AL_COMPRESSOR_ONOFF),

    DECL(AL_EQUALIZER_LOW_GAIN),
    DECL(AL_EQUALIZER_LOW_CUTOFF),
    DECL(AL_EQUALIZER_MID1_GAIN),
    DECL(AL_EQUALIZER_MID1_CENTER),
    DECL(AL_EQUALIZER_MID1_WIDTH),
    DECL(AL_EQUALIZER_MID2_GAIN),
    DECL(AL_EQUALIZER_MID2_CENTER),
    DECL(AL_EQUALIZER_MID2_WIDTH),
    DECL(AL_EQUALIZER_HIGH_GAIN),
    DECL(AL_EQUALIZER_HIGH_CUTOFF),

    DECL(AL_DEDICATED_GAIN),

    DECL(AL_AUTOWAH_ATTACK_TIME),
    DECL(AL_AUTOWAH_RELEASE_TIME),
    DECL(AL_AUTOWAH_RESONANCE),
    DECL(AL_AUTOWAH_PEAK_GAIN),

    DECL(AL_VOCAL_MORPHER_PHONEMEA),
    DECL(AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING),
    DECL(AL_VOCAL_MORPHER_PHONEMEB),
    DECL(AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING),
    DECL(AL_VOCAL_MORPHER_WAVEFORM),
    DECL(AL_VOCAL_MORPHER_RATE),

    DECL(AL_EFFECTSLOT_TARGET_SOFT),

    DECL(AL_NUM_RESAMPLERS_SOFT),
    DECL(AL_DEFAULT_RESAMPLER_SOFT),
    DECL(AL_SOURCE_RESAMPLER_SOFT),
    DECL(AL_RESAMPLER_NAME_SOFT),

    DECL(AL_SOURCE_SPATIALIZE_SOFT),
    DECL(AL_AUTO_SOFT),

    DECL(AL_MAP_READ_BIT_SOFT),
    DECL(AL_MAP_WRITE_BIT_SOFT),
    DECL(AL_MAP_PERSISTENT_BIT_SOFT),
    DECL(AL_PRESERVE_DATA_BIT_SOFT),

    DECL(AL_EVENT_CALLBACK_FUNCTION_SOFT),
    DECL(AL_EVENT_CALLBACK_USER_PARAM_SOFT),
    DECL(AL_EVENT_TYPE_BUFFER_COMPLETED_SOFT),
    DECL(AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT),
    DECL(AL_EVENT_TYPE_DISCONNECTED_SOFT),

    DECL(AL_DROP_UNMATCHED_SOFT),
    DECL(AL_REMIX_UNMATCHED_SOFT),

    DECL(AL_AMBISONIC_LAYOUT_SOFT),
    DECL(AL_AMBISONIC_SCALING_SOFT),
    DECL(AL_FUMA_SOFT),
    DECL(AL_ACN_SOFT),
    DECL(AL_SN3D_SOFT),
    DECL(AL_N3D_SOFT),

    DECL(AL_BUFFER_CALLBACK_FUNCTION_SOFT),
    DECL(AL_BUFFER_CALLBACK_USER_PARAM_SOFT),

    DECL(AL_UNPACK_AMBISONIC_ORDER_SOFT),

    DECL(AL_EFFECT_CONVOLUTION_REVERB_SOFT),
    DECL(AL_EFFECTSLOT_STATE_SOFT),

    DECL(AL_FORMAT_UHJ2CHN8_SOFT),
    DECL(AL_FORMAT_UHJ2CHN16_SOFT),
    DECL(AL_FORMAT_UHJ2CHN_FLOAT32_SOFT),
    DECL(AL_FORMAT_UHJ3CHN8_SOFT),
    DECL(AL_FORMAT_UHJ3CHN16_SOFT),
    DECL(AL_FORMAT_UHJ3CHN_FLOAT32_SOFT),
    DECL(AL_FORMAT_UHJ4CHN8_SOFT),
    DECL(AL_FORMAT_UHJ4CHN16_SOFT),
    DECL(AL_FORMAT_UHJ4CHN_FLOAT32_SOFT),
    DECL(AL_STEREO_MODE_SOFT),
    DECL(AL_NORMAL_SOFT),
    DECL(AL_SUPER_STEREO_SOFT),
    DECL(AL_SUPER_STEREO_WIDTH_SOFT),

    DECL(AL_STOP_SOURCES_ON_DISCONNECT_SOFT),

#ifdef ALSOFT_EAX
}, eaxEnumerations[] = {
    DECL(AL_EAX_RAM_SIZE),
    DECL(AL_EAX_RAM_FREE),
    DECL(AL_STORAGE_AUTOMATIC),
    DECL(AL_STORAGE_HARDWARE),
    DECL(AL_STORAGE_ACCESSIBLE),
#endif // ALSOFT_EAX
};
#undef DECL

constexpr ALCchar alcNoError[] = "No Error";
constexpr ALCchar alcErrInvalidDevice[] = "Invalid Device";
constexpr ALCchar alcErrInvalidContext[] = "Invalid Context";
constexpr ALCchar alcErrInvalidEnum[] = "Invalid Enum";
constexpr ALCchar alcErrInvalidValue[] = "Invalid Value";
constexpr ALCchar alcErrOutOfMemory[] = "Out of Memory";


/************************************************
 * Global variables
 ************************************************/

/* Enumerated device names */
constexpr ALCchar alcDefaultName[] = "OpenAL Soft\0";

std::string alcAllDevicesList;
std::string alcCaptureDeviceList;

/* Default is always the first in the list */
std::string alcDefaultAllDevicesSpecifier;
std::string alcCaptureDefaultDeviceSpecifier;

std::atomic<ALCenum> LastNullDeviceError{ALC_NO_ERROR};

/* Flag to trap ALC device errors */
bool TrapALCError{false};

/* One-time configuration init control */
std::once_flag alc_config_once{};

/* Flag to specify if alcSuspendContext/alcProcessContext should defer/process
 * updates.
 */
bool SuspendDefers{true};

/* Initial seed for dithering. */
constexpr uint DitherRNGSeed{22222u};


/************************************************
 * ALC information
 ************************************************/
constexpr ALCchar alcNoDeviceExtList[] =
    "ALC_ENUMERATE_ALL_EXT "
    "ALC_ENUMERATION_EXT "
    "ALC_EXT_CAPTURE "
    "ALC_EXT_EFX "
    "ALC_EXT_thread_local_context "
    "ALC_SOFT_loopback "
    "ALC_SOFT_loopback_bformat "
    "ALC_SOFT_reopen_device";
constexpr ALCchar alcExtensionList[] =
    "ALC_ENUMERATE_ALL_EXT "
    "ALC_ENUMERATION_EXT "
    "ALC_EXT_CAPTURE "
    "ALC_EXT_DEDICATED "
    "ALC_EXT_disconnect "
    "ALC_EXT_EFX "
    "ALC_EXT_thread_local_context "
    "ALC_SOFT_device_clock "
    "ALC_SOFT_HRTF "
    "ALC_SOFT_loopback "
    "ALC_SOFT_loopback_bformat "
    "ALC_SOFT_output_limiter "
    "ALC_SOFT_output_mode "
    "ALC_SOFT_pause_device "
    "ALC_SOFT_reopen_device";
constexpr int alcMajorVersion{1};
constexpr int alcMinorVersion{1};

constexpr int alcEFXMajorVersion{1};
constexpr int alcEFXMinorVersion{0};


using DeviceRef = al::intrusive_ptr<ALCdevice>;


/************************************************
 * Device lists
 ************************************************/
al::vector<ALCdevice*> DeviceList;
al::vector<ALCcontext*> ContextList;

std::recursive_mutex ListLock;


void alc_initconfig(void)
{
    if(auto loglevel = al::getenv("ALSOFT_LOGLEVEL"))
    {
        long lvl = strtol(loglevel->c_str(), nullptr, 0);
        if(lvl >= static_cast<long>(LogLevel::Trace))
            gLogLevel = LogLevel::Trace;
        else if(lvl <= static_cast<long>(LogLevel::Disable))
            gLogLevel = LogLevel::Disable;
        else
            gLogLevel = static_cast<LogLevel>(lvl);
    }

#ifdef _WIN32
    if(const auto logfile = al::getenv(L"ALSOFT_LOGFILE"))
    {
        FILE *logf{_wfopen(logfile->c_str(), L"wt")};
        if(logf) gLogFile = logf;
        else
        {
            auto u8name = wstr_to_utf8(logfile->c_str());
            ERR("Failed to open log file '%s'\n", u8name.c_str());
        }
    }
#else
    if(const auto logfile = al::getenv("ALSOFT_LOGFILE"))
    {
        FILE *logf{fopen(logfile->c_str(), "wt")};
        if(logf) gLogFile = logf;
        else ERR("Failed to open log file '%s'\n", logfile->c_str());
    }
#endif

    TRACE("Initializing library v%s-%s %s\n", ALSOFT_VERSION, ALSOFT_GIT_COMMIT_HASH,
        ALSOFT_GIT_BRANCH);
    {
        std::string names;
        if(al::size(BackendList) < 1)
            names = "(none)";
        else
        {
            const al::span<const BackendInfo> infos{BackendList};
            names = infos[0].name;
            for(const auto &backend : infos.subspan<1>())
            {
                names += ", ";
                names += backend.name;
            }
        }
        TRACE("Supported backends: %s\n", names.c_str());
    }
    ReadALConfig();

    if(auto suspendmode = al::getenv("__ALSOFT_SUSPEND_CONTEXT"))
    {
        if(al::strcasecmp(suspendmode->c_str(), "ignore") == 0)
        {
            SuspendDefers = false;
            TRACE("Selected context suspend behavior, \"ignore\"\n");
        }
        else
            ERR("Unhandled context suspend behavior setting: \"%s\"\n", suspendmode->c_str());
    }

    int capfilter{0};
#if defined(HAVE_SSE4_1)
    capfilter |= CPU_CAP_SSE | CPU_CAP_SSE2 | CPU_CAP_SSE3 | CPU_CAP_SSE4_1;
#elif defined(HAVE_SSE3)
    capfilter |= CPU_CAP_SSE | CPU_CAP_SSE2 | CPU_CAP_SSE3;
#elif defined(HAVE_SSE2)
    capfilter |= CPU_CAP_SSE | CPU_CAP_SSE2;
#elif defined(HAVE_SSE)
    capfilter |= CPU_CAP_SSE;
#endif
#ifdef HAVE_NEON
    capfilter |= CPU_CAP_NEON;
#endif
    if(auto cpuopt = ConfigValueStr(nullptr, nullptr, "disable-cpu-exts"))
    {
        const char *str{cpuopt->c_str()};
        if(al::strcasecmp(str, "all") == 0)
            capfilter = 0;
        else
        {
            const char *next = str;
            do {
                str = next;
                while(isspace(str[0]))
                    str++;
                next = strchr(str, ',');

                if(!str[0] || str[0] == ',')
                    continue;

                size_t len{next ? static_cast<size_t>(next-str) : strlen(str)};
                while(len > 0 && isspace(str[len-1]))
                    len--;
                if(len == 3 && al::strncasecmp(str, "sse", len) == 0)
                    capfilter &= ~CPU_CAP_SSE;
                else if(len == 4 && al::strncasecmp(str, "sse2", len) == 0)
                    capfilter &= ~CPU_CAP_SSE2;
                else if(len == 4 && al::strncasecmp(str, "sse3", len) == 0)
                    capfilter &= ~CPU_CAP_SSE3;
                else if(len == 6 && al::strncasecmp(str, "sse4.1", len) == 0)
                    capfilter &= ~CPU_CAP_SSE4_1;
                else if(len == 4 && al::strncasecmp(str, "neon", len) == 0)
                    capfilter &= ~CPU_CAP_NEON;
                else
                    WARN("Invalid CPU extension \"%s\"\n", str);
            } while(next++);
        }
    }
    if(auto cpuopt = GetCPUInfo())
    {
        if(!cpuopt->mVendor.empty() || !cpuopt->mName.empty())
        {
            TRACE("Vendor ID: \"%s\"\n", cpuopt->mVendor.c_str());
            TRACE("Name: \"%s\"\n", cpuopt->mName.c_str());
        }
        const int caps{cpuopt->mCaps};
        TRACE("Extensions:%s%s%s%s%s%s\n",
            ((capfilter&CPU_CAP_SSE)    ? ((caps&CPU_CAP_SSE)    ? " +SSE"    : " -SSE")    : ""),
            ((capfilter&CPU_CAP_SSE2)   ? ((caps&CPU_CAP_SSE2)   ? " +SSE2"   : " -SSE2")   : ""),
            ((capfilter&CPU_CAP_SSE3)   ? ((caps&CPU_CAP_SSE3)   ? " +SSE3"   : " -SSE3")   : ""),
            ((capfilter&CPU_CAP_SSE4_1) ? ((caps&CPU_CAP_SSE4_1) ? " +SSE4.1" : " -SSE4.1") : ""),
            ((capfilter&CPU_CAP_NEON)   ? ((caps&CPU_CAP_NEON)   ? " +NEON"   : " -NEON")   : ""),
            ((!capfilter) ? " -none-" : ""));
        CPUCapFlags = caps & capfilter;
    }

    if(auto priopt = ConfigValueInt(nullptr, nullptr, "rt-prio"))
        RTPrioLevel = *priopt;
    if(auto limopt = ConfigValueBool(nullptr, nullptr, "rt-time-limit"))
        AllowRTTimeLimit = *limopt;

    CompatFlagBitset compatflags{};
    auto checkflag = [](const char *envname, const char *optname) -> bool
    {
        if(auto optval = al::getenv(envname))
        {
            if(al::strcasecmp(optval->c_str(), "true") == 0
                || strtol(optval->c_str(), nullptr, 0) == 1)
                return true;
            return false;
        }
        return GetConfigValueBool(nullptr, "game_compat", optname, false);
    };
    compatflags.set(CompatFlags::ReverseX, checkflag("__ALSOFT_REVERSE_X", "reverse-x"));
    compatflags.set(CompatFlags::ReverseY, checkflag("__ALSOFT_REVERSE_Y", "reverse-y"));
    compatflags.set(CompatFlags::ReverseZ, checkflag("__ALSOFT_REVERSE_Z", "reverse-z"));

    aluInit(compatflags);
    Voice::InitMixer(ConfigValueStr(nullptr, nullptr, "resampler"));

    auto traperr = al::getenv("ALSOFT_TRAP_ERROR");
    if(traperr && (al::strcasecmp(traperr->c_str(), "true") == 0
            || std::strtol(traperr->c_str(), nullptr, 0) == 1))
    {
        TrapALError  = true;
        TrapALCError = true;
    }
    else
    {
        traperr = al::getenv("ALSOFT_TRAP_AL_ERROR");
        if(traperr)
            TrapALError = al::strcasecmp(traperr->c_str(), "true") == 0
                || strtol(traperr->c_str(), nullptr, 0) == 1;
        else
            TrapALError = !!GetConfigValueBool(nullptr, nullptr, "trap-al-error", false);

        traperr = al::getenv("ALSOFT_TRAP_ALC_ERROR");
        if(traperr)
            TrapALCError = al::strcasecmp(traperr->c_str(), "true") == 0
                || strtol(traperr->c_str(), nullptr, 0) == 1;
        else
            TrapALCError = !!GetConfigValueBool(nullptr, nullptr, "trap-alc-error", false);
    }

    if(auto boostopt = ConfigValueFloat(nullptr, "reverb", "boost"))
    {
        const float valf{std::isfinite(*boostopt) ? clampf(*boostopt, -24.0f, 24.0f) : 0.0f};
        ReverbBoost *= std::pow(10.0f, valf / 20.0f);
    }

    auto BackendListEnd = std::end(BackendList);
    auto devopt = al::getenv("ALSOFT_DRIVERS");
    if(devopt || (devopt=ConfigValueStr(nullptr, nullptr, "drivers")))
    {
        auto backendlist_cur = std::begin(BackendList);

        bool endlist{true};
        const char *next{devopt->c_str()};
        do {
            const char *devs{next};
            while(isspace(devs[0]))
                devs++;
            next = strchr(devs, ',');

            const bool delitem{devs[0] == '-'};
            if(devs[0] == '-') devs++;

            if(!devs[0] || devs[0] == ',')
            {
                endlist = false;
                continue;
            }
            endlist = true;

            size_t len{next ? (static_cast<size_t>(next-devs)) : strlen(devs)};
            while(len > 0 && isspace(devs[len-1])) --len;
#ifdef HAVE_WASAPI
            /* HACK: For backwards compatibility, convert backend references of
             * mmdevapi to wasapi. This should eventually be removed.
             */
            if(len == 8 && strncmp(devs, "mmdevapi", len) == 0)
            {
                devs = "wasapi";
                len = 6;
            }
#endif

            auto find_backend = [devs,len](const BackendInfo &backend) -> bool
            { return len == strlen(backend.name) && strncmp(backend.name, devs, len) == 0; };
            auto this_backend = std::find_if(std::begin(BackendList), BackendListEnd,
                find_backend);

            if(this_backend == BackendListEnd)
                continue;

            if(delitem)
                BackendListEnd = std::move(this_backend+1, BackendListEnd, this_backend);
            else
                backendlist_cur = std::rotate(backendlist_cur, this_backend, this_backend+1);
        } while(next++);

        if(endlist)
            BackendListEnd = backendlist_cur;
    }

    auto init_backend = [](BackendInfo &backend) -> void
    {
        if(PlaybackFactory && CaptureFactory)
            return;

        BackendFactory &factory = backend.getFactory();
        if(!factory.init())
        {
            WARN("Failed to initialize backend \"%s\"\n", backend.name);
            return;
        }

        TRACE("Initialized backend \"%s\"\n", backend.name);
        if(!PlaybackFactory && factory.querySupport(BackendType::Playback))
        {
            PlaybackFactory = &factory;
            TRACE("Added \"%s\" for playback\n", backend.name);
        }
        if(!CaptureFactory && factory.querySupport(BackendType::Capture))
        {
            CaptureFactory = &factory;
            TRACE("Added \"%s\" for capture\n", backend.name);
        }
    };
    std::for_each(std::begin(BackendList), BackendListEnd, init_backend);

    LoopbackBackendFactory::getFactory().init();

    if(!PlaybackFactory)
        WARN("No playback backend available!\n");
    if(!CaptureFactory)
        WARN("No capture backend available!\n");

    if(auto exclopt = ConfigValueStr(nullptr, nullptr, "excludefx"))
    {
        const char *next{exclopt->c_str()};
        do {
            const char *str{next};
            next = strchr(str, ',');

            if(!str[0] || next == str)
                continue;

            size_t len{next ? static_cast<size_t>(next-str) : strlen(str)};
            for(const EffectList &effectitem : gEffectList)
            {
                if(len == strlen(effectitem.name) &&
                   strncmp(effectitem.name, str, len) == 0)
                    DisabledEffects[effectitem.type] = true;
            }
        } while(next++);
    }

    InitEffect(&ALCcontext::sDefaultEffect);
    auto defrevopt = al::getenv("ALSOFT_DEFAULT_REVERB");
    if(defrevopt || (defrevopt=ConfigValueStr(nullptr, nullptr, "default-reverb")))
        LoadReverbPreset(defrevopt->c_str(), &ALCcontext::sDefaultEffect);

#ifdef ALSOFT_EAX
    {
        static constexpr char eax_block_name[] = "eax";

        if(const auto eax_enable_opt = ConfigValueBool(nullptr, eax_block_name, "enable"))
        {
            eax_g_is_enabled = *eax_enable_opt;
            if(!eax_g_is_enabled)
                TRACE("%s\n", "EAX disabled by a configuration.");
        }
        else
            eax_g_is_enabled = true;

        if(eax_g_is_enabled && DisabledEffects[EAXREVERB_EFFECT])
        {
            eax_g_is_enabled = false;
            TRACE("%s\n", "EAX disabled because EAXReverb is disabled.");
        }
    }
#endif // ALSOFT_EAX
}
#define DO_INITCONFIG() std::call_once(alc_config_once, [](){alc_initconfig();})


/************************************************
 * Device enumeration
 ************************************************/
void ProbeAllDevicesList()
{
    DO_INITCONFIG();

    std::lock_guard<std::recursive_mutex> _{ListLock};
    if(!PlaybackFactory)
        decltype(alcAllDevicesList){}.swap(alcAllDevicesList);
    else
    {
        std::string names{PlaybackFactory->probe(BackendType::Playback)};
        if(names.empty()) names += '\0';
        names.swap(alcAllDevicesList);
    }
}
void ProbeCaptureDeviceList()
{
    DO_INITCONFIG();

    std::lock_guard<std::recursive_mutex> _{ListLock};
    if(!CaptureFactory)
        decltype(alcCaptureDeviceList){}.swap(alcCaptureDeviceList);
    else
    {
        std::string names{CaptureFactory->probe(BackendType::Capture)};
        if(names.empty()) names += '\0';
        names.swap(alcCaptureDeviceList);
    }
}


struct DevFmtPair { DevFmtChannels chans; DevFmtType type; };
al::optional<DevFmtPair> DecomposeDevFormat(ALenum format)
{
    static const struct {
        ALenum format;
        DevFmtChannels channels;
        DevFmtType type;
    } list[] = {
        { AL_FORMAT_MONO8,        DevFmtMono, DevFmtUByte },
        { AL_FORMAT_MONO16,       DevFmtMono, DevFmtShort },
        { AL_FORMAT_MONO_FLOAT32, DevFmtMono, DevFmtFloat },

        { AL_FORMAT_STEREO8,        DevFmtStereo, DevFmtUByte },
        { AL_FORMAT_STEREO16,       DevFmtStereo, DevFmtShort },
        { AL_FORMAT_STEREO_FLOAT32, DevFmtStereo, DevFmtFloat },

        { AL_FORMAT_QUAD8,  DevFmtQuad, DevFmtUByte },
        { AL_FORMAT_QUAD16, DevFmtQuad, DevFmtShort },
        { AL_FORMAT_QUAD32, DevFmtQuad, DevFmtFloat },

        { AL_FORMAT_51CHN8,  DevFmtX51, DevFmtUByte },
        { AL_FORMAT_51CHN16, DevFmtX51, DevFmtShort },
        { AL_FORMAT_51CHN32, DevFmtX51, DevFmtFloat },

        { AL_FORMAT_61CHN8,  DevFmtX61, DevFmtUByte },
        { AL_FORMAT_61CHN16, DevFmtX61, DevFmtShort },
        { AL_FORMAT_61CHN32, DevFmtX61, DevFmtFloat },

        { AL_FORMAT_71CHN8,  DevFmtX71, DevFmtUByte },
        { AL_FORMAT_71CHN16, DevFmtX71, DevFmtShort },
        { AL_FORMAT_71CHN32, DevFmtX71, DevFmtFloat },
    };

    for(const auto &item : list)
    {
        if(item.format == format)
            return al::make_optional(DevFmtPair{item.channels, item.type});
    }

    return al::nullopt;
}

al::optional<DevFmtType> DevFmtTypeFromEnum(ALCenum type)
{
    switch(type)
    {
    case ALC_BYTE_SOFT: return al::make_optional(DevFmtByte);
    case ALC_UNSIGNED_BYTE_SOFT: return al::make_optional(DevFmtUByte);
    case ALC_SHORT_SOFT: return al::make_optional(DevFmtShort);
    case ALC_UNSIGNED_SHORT_SOFT: return al::make_optional(DevFmtUShort);
    case ALC_INT_SOFT: return al::make_optional(DevFmtInt);
    case ALC_UNSIGNED_INT_SOFT: return al::make_optional(DevFmtUInt);
    case ALC_FLOAT_SOFT: return al::make_optional(DevFmtFloat);
    }
    WARN("Unsupported format type: 0x%04x\n", type);
    return al::nullopt;
}
ALCenum EnumFromDevFmt(DevFmtType type)
{
    switch(type)
    {
    case DevFmtByte: return ALC_BYTE_SOFT;
    case DevFmtUByte: return ALC_UNSIGNED_BYTE_SOFT;
    case DevFmtShort: return ALC_SHORT_SOFT;
    case DevFmtUShort: return ALC_UNSIGNED_SHORT_SOFT;
    case DevFmtInt: return ALC_INT_SOFT;
    case DevFmtUInt: return ALC_UNSIGNED_INT_SOFT;
    case DevFmtFloat: return ALC_FLOAT_SOFT;
    }
    throw std::runtime_error{"Invalid DevFmtType: "+std::to_string(int(type))};
}

al::optional<DevFmtChannels> DevFmtChannelsFromEnum(ALCenum channels)
{
    switch(channels)
    {
    case ALC_MONO_SOFT: return al::make_optional(DevFmtMono);
    case ALC_STEREO_SOFT: return al::make_optional(DevFmtStereo);
    case ALC_QUAD_SOFT: return al::make_optional(DevFmtQuad);
    case ALC_5POINT1_SOFT: return al::make_optional(DevFmtX51);
    case ALC_6POINT1_SOFT: return al::make_optional(DevFmtX61);
    case ALC_7POINT1_SOFT: return al::make_optional(DevFmtX71);
    case ALC_BFORMAT3D_SOFT: return al::make_optional(DevFmtAmbi3D);
    }
    WARN("Unsupported format channels: 0x%04x\n", channels);
    return al::nullopt;
}
ALCenum EnumFromDevFmt(DevFmtChannels channels)
{
    switch(channels)
    {
    case DevFmtMono: return ALC_MONO_SOFT;
    case DevFmtStereo: return ALC_STEREO_SOFT;
    case DevFmtQuad: return ALC_QUAD_SOFT;
    case DevFmtX51: return ALC_5POINT1_SOFT;
    case DevFmtX61: return ALC_6POINT1_SOFT;
    case DevFmtX71: return ALC_7POINT1_SOFT;
    case DevFmtAmbi3D: return ALC_BFORMAT3D_SOFT;
    }
    throw std::runtime_error{"Invalid DevFmtChannels: "+std::to_string(int(channels))};
}

al::optional<DevAmbiLayout> DevAmbiLayoutFromEnum(ALCenum layout)
{
    switch(layout)
    {
    case ALC_FUMA_SOFT: return al::make_optional(DevAmbiLayout::FuMa);
    case ALC_ACN_SOFT: return al::make_optional(DevAmbiLayout::ACN);
    }
    WARN("Unsupported ambisonic layout: 0x%04x\n", layout);
    return al::nullopt;
}
ALCenum EnumFromDevAmbi(DevAmbiLayout layout)
{
    switch(layout)
    {
    case DevAmbiLayout::FuMa: return ALC_FUMA_SOFT;
    case DevAmbiLayout::ACN: return ALC_ACN_SOFT;
    }
    throw std::runtime_error{"Invalid DevAmbiLayout: "+std::to_string(int(layout))};
}

al::optional<DevAmbiScaling> DevAmbiScalingFromEnum(ALCenum scaling)
{
    switch(scaling)
    {
    case ALC_FUMA_SOFT: return al::make_optional(DevAmbiScaling::FuMa);
    case ALC_SN3D_SOFT: return al::make_optional(DevAmbiScaling::SN3D);
    case ALC_N3D_SOFT: return al::make_optional(DevAmbiScaling::N3D);
    }
    WARN("Unsupported ambisonic scaling: 0x%04x\n", scaling);
    return al::nullopt;
}
ALCenum EnumFromDevAmbi(DevAmbiScaling scaling)
{
    switch(scaling)
    {
    case DevAmbiScaling::FuMa: return ALC_FUMA_SOFT;
    case DevAmbiScaling::SN3D: return ALC_SN3D_SOFT;
    case DevAmbiScaling::N3D: return ALC_N3D_SOFT;
    }
    throw std::runtime_error{"Invalid DevAmbiScaling: "+std::to_string(int(scaling))};
}


/* Downmixing channel arrays, to map the given format's missing channels to
 * existing ones. Based on Wine's DSound downmix values, which are based on
 * PulseAudio's.
 */
const std::array<InputRemixMap,6> StereoDownmix{{
    { FrontCenter, {{{FrontLeft, 0.5f},      {FrontRight, 0.5f}}} },
    { SideLeft,    {{{FrontLeft, 1.0f/9.0f}, {FrontRight, 0.0f}}} },
    { SideRight,   {{{FrontLeft, 0.0f},      {FrontRight, 1.0f/9.0f}}} },
    { BackLeft,    {{{FrontLeft, 1.0f/9.0f}, {FrontRight, 0.0f}}} },
    { BackRight,   {{{FrontLeft, 0.0f},      {FrontRight, 1.0f/9.0f}}} },
    { BackCenter,  {{{FrontLeft, 0.5f/9.0f}, {FrontRight, 0.5f/9.0f}}} },
}};
const std::array<InputRemixMap,4> QuadDownmix{{
    { FrontCenter, {{{FrontLeft,  0.5f}, {FrontRight, 0.5f}}} },
    { SideLeft,    {{{FrontLeft,  0.5f}, {BackLeft,   0.5f}}} },
    { SideRight,   {{{FrontRight, 0.5f}, {BackRight,  0.5f}}} },
    { BackCenter,  {{{BackLeft,   0.5f}, {BackRight,  0.5f}}} },
}};
const std::array<InputRemixMap,3> X51Downmix{{
    { BackLeft,   {{{SideLeft, 1.0f}, {SideRight, 0.0f}}} },
    { BackRight,  {{{SideLeft, 0.0f}, {SideRight, 1.0f}}} },
    { BackCenter, {{{SideLeft, 0.5f}, {SideRight, 0.5f}}} },
}};
const std::array<InputRemixMap,2> X61Downmix{{
    { BackLeft,  {{{BackCenter, 0.5f}, {SideLeft,  0.5f}}} },
    { BackRight, {{{BackCenter, 0.5f}, {SideRight, 0.5f}}} },
}};
const std::array<InputRemixMap,1> X71Downmix{{
    { BackCenter, {{{BackLeft, 0.5f}, {BackRight, 0.5f}}} },
}};


/** Stores the latest ALC device error. */
void alcSetError(ALCdevice *device, ALCenum errorCode)
{
    WARN("Error generated on device %p, code 0x%04x\n", voidp{device}, errorCode);
    if(TrapALCError)
    {
#ifdef _WIN32
        /* DebugBreak() will cause an exception if there is no debugger */
        if(IsDebuggerPresent())
            DebugBreak();
#elif defined(SIGTRAP)
        raise(SIGTRAP);
#endif
    }

    if(device)
        device->LastError.store(errorCode);
    else
        LastNullDeviceError.store(errorCode);
}


std::unique_ptr<Compressor> CreateDeviceLimiter(const ALCdevice *device, const float threshold)
{
    static constexpr bool AutoKnee{true};
    static constexpr bool AutoAttack{true};
    static constexpr bool AutoRelease{true};
    static constexpr bool AutoPostGain{true};
    static constexpr bool AutoDeclip{true};
    static constexpr float LookAheadTime{0.001f};
    static constexpr float HoldTime{0.002f};
    static constexpr float PreGainDb{0.0f};
    static constexpr float PostGainDb{0.0f};
    static constexpr float Ratio{std::numeric_limits<float>::infinity()};
    static constexpr float KneeDb{0.0f};
    static constexpr float AttackTime{0.02f};
    static constexpr float ReleaseTime{0.2f};

    return Compressor::Create(device->RealOut.Buffer.size(), static_cast<float>(device->Frequency),
        AutoKnee, AutoAttack, AutoRelease, AutoPostGain, AutoDeclip, LookAheadTime, HoldTime,
        PreGainDb, PostGainDb, threshold, Ratio, KneeDb, AttackTime, ReleaseTime);
}

/**
 * Updates the device's base clock time with however many samples have been
 * done. This is used so frequency changes on the device don't cause the time
 * to jump forward or back. Must not be called while the device is running/
 * mixing.
 */
static inline void UpdateClockBase(ALCdevice *device)
{
    IncrementRef(device->MixCount);
    device->ClockBase += nanoseconds{seconds{device->SamplesDone}} / device->Frequency;
    device->SamplesDone = 0;
    IncrementRef(device->MixCount);
}

/**
 * Updates device parameters according to the attribute list (caller is
 * responsible for holding the list lock).
 */
ALCenum UpdateDeviceParams(ALCdevice *device, const int *attrList)
{
    if((!attrList || !attrList[0]) && device->Type == DeviceType::Loopback)
    {
        WARN("Missing attributes for loopback device\n");
        return ALC_INVALID_VALUE;
    }

    al::optional<StereoEncoding> stereomode{};
    al::optional<bool> optlimit{};
    int hrtf_id{-1};

    // Check for attributes
    if(attrList && attrList[0])
    {
        uint numMono{device->NumMonoSources};
        uint numStereo{device->NumStereoSources};
        uint numSends{device->NumAuxSends};

        al::optional<DevFmtChannels> optchans;
        al::optional<DevFmtType> opttype;
        al::optional<DevAmbiLayout> optlayout;
        al::optional<DevAmbiScaling> optscale;
        al::optional<bool> opthrtf;

        ALenum outmode{ALC_ANY_SOFT};
        uint aorder{0u};
        uint freq{0u};

#define ATTRIBUTE(a) a: TRACE("%s = %d\n", #a, attrList[attrIdx + 1]);
        size_t attrIdx{0};
        while(attrList[attrIdx])
        {
            switch(attrList[attrIdx])
            {
            case ATTRIBUTE(ALC_FORMAT_CHANNELS_SOFT)
                optchans = DevFmtChannelsFromEnum(attrList[attrIdx + 1]);
                break;

            case ATTRIBUTE(ALC_FORMAT_TYPE_SOFT)
                opttype = DevFmtTypeFromEnum(attrList[attrIdx + 1]);
                break;

            case ATTRIBUTE(ALC_FREQUENCY)
                freq = static_cast<uint>(attrList[attrIdx + 1]);
                break;

            case ATTRIBUTE(ALC_AMBISONIC_LAYOUT_SOFT)
                optlayout = DevAmbiLayoutFromEnum(attrList[attrIdx + 1]);
                break;

            case ATTRIBUTE(ALC_AMBISONIC_SCALING_SOFT)
                optscale = DevAmbiScalingFromEnum(attrList[attrIdx + 1]);
                break;

            case ATTRIBUTE(ALC_AMBISONIC_ORDER_SOFT)
                aorder = static_cast<uint>(attrList[attrIdx + 1]);
                break;

            case ATTRIBUTE(ALC_MONO_SOURCES)
                numMono = static_cast<uint>(attrList[attrIdx + 1]);
                if(numMono > INT_MAX) numMono = 0;
                break;

            case ATTRIBUTE(ALC_STEREO_SOURCES)
                numStereo = static_cast<uint>(attrList[attrIdx + 1]);
                if(numStereo > INT_MAX) numStereo = 0;
                break;

            case ATTRIBUTE(ALC_MAX_AUXILIARY_SENDS)
                numSends = static_cast<uint>(attrList[attrIdx + 1]);
                if(numSends > INT_MAX) numSends = 0;
                else numSends = minu(numSends, MAX_SENDS);
                break;

            case ATTRIBUTE(ALC_HRTF_SOFT)
                if(attrList[attrIdx + 1] == ALC_FALSE)
                    opthrtf = false;
                else if(attrList[attrIdx + 1] == ALC_TRUE)
                    opthrtf = true;
                else if(attrList[attrIdx + 1] == ALC_DONT_CARE_SOFT)
                    opthrtf = al::nullopt;
                break;

            case ATTRIBUTE(ALC_HRTF_ID_SOFT)
                hrtf_id = attrList[attrIdx + 1];
                break;

            case ATTRIBUTE(ALC_OUTPUT_LIMITER_SOFT)
                if(attrList[attrIdx + 1] == ALC_FALSE)
                    optlimit = false;
                else if(attrList[attrIdx + 1] == ALC_TRUE)
                    optlimit = true;
                else if(attrList[attrIdx + 1] == ALC_DONT_CARE_SOFT)
                    optlimit = al::nullopt;
                break;

            case ATTRIBUTE(ALC_OUTPUT_MODE_SOFT)
                outmode = attrList[attrIdx + 1];
                break;

            default:
                TRACE("0x%04X = %d (0x%x)\n", attrList[attrIdx],
                    attrList[attrIdx + 1], attrList[attrIdx + 1]);
                break;
            }

            attrIdx += 2;
        }
#undef ATTRIBUTE

        const bool loopback{device->Type == DeviceType::Loopback};
        if(loopback)
        {
            if(!optchans || !opttype)
                return ALC_INVALID_VALUE;
            if(freq < MIN_OUTPUT_RATE || freq > MAX_OUTPUT_RATE)
                return ALC_INVALID_VALUE;
            if(*optchans == DevFmtAmbi3D)
            {
                if(!optlayout || !optscale)
                    return ALC_INVALID_VALUE;
                if(aorder < 1 || aorder > MaxAmbiOrder)
                    return ALC_INVALID_VALUE;
                if((*optlayout == DevAmbiLayout::FuMa || *optscale == DevAmbiScaling::FuMa)
                    && aorder > 3)
                    return ALC_INVALID_VALUE;
            }
        }

        /* If a context is already running on the device, stop playback so the
         * device attributes can be updated.
         */
        if(device->Flags.test(DeviceRunning))
            device->Backend->stop();
        device->Flags.reset(DeviceRunning);

        UpdateClockBase(device);

        /* Calculate the max number of sources, and split them between the mono
         * and stereo count given the requested number of stereo sources.
         */
        if(auto srcsopt = device->configValue<uint>(nullptr, "sources"))
        {
            if(*srcsopt <= 0) numMono = 256;
            else numMono = *srcsopt;
        }
        else
        {
            if(numMono > INT_MAX-numStereo)
                numMono = INT_MAX-numStereo;
            numMono = maxu(numMono+numStereo, 256);
        }
        numStereo = minu(numStereo, numMono);
        numMono -= numStereo;
        device->SourcesMax = numMono + numStereo;
        device->NumMonoSources = numMono;
        device->NumStereoSources = numStereo;

        if(auto sendsopt = device->configValue<int>(nullptr, "sends"))
            numSends = minu(numSends, static_cast<uint>(clampi(*sendsopt, 0, MAX_SENDS)));
        device->NumAuxSends = numSends;

        if(loopback)
        {
            device->Frequency = freq;
            device->FmtChans = *optchans;
            device->FmtType = *opttype;
            if(device->FmtChans == DevFmtAmbi3D)
            {
                device->mAmbiOrder = aorder;
                device->mAmbiLayout = *optlayout;
                device->mAmbiScale = *optscale;
            }
            else if(device->FmtChans == DevFmtStereo)
            {
                if(opthrtf)
                    stereomode = *opthrtf ? StereoEncoding::Hrtf : StereoEncoding::Default;

                if(outmode == ALC_STEREO_BASIC_SOFT)
                    stereomode = StereoEncoding::Basic;
                else if(outmode == ALC_STEREO_UHJ_SOFT)
                    stereomode = StereoEncoding::Uhj;
                else if(outmode == ALC_STEREO_HRTF_SOFT)
                    stereomode = StereoEncoding::Hrtf;
            }
            device->Flags.set(FrequencyRequest).set(ChannelsRequest).set(SampleTypeRequest);
        }
        else
        {
            device->Flags.reset(FrequencyRequest).reset(ChannelsRequest).reset(SampleTypeRequest);
            device->FmtType = DevFmtTypeDefault;
            device->FmtChans = DevFmtChannelsDefault;
            device->mAmbiOrder = 0;
            device->BufferSize = DEFAULT_UPDATE_SIZE * DEFAULT_NUM_UPDATES;
            device->UpdateSize = DEFAULT_UPDATE_SIZE;
            device->Frequency = DEFAULT_OUTPUT_RATE;

            freq = device->configValue<uint>(nullptr, "frequency").value_or(freq);
            if(freq > 0)
            {
                freq = clampu(freq, MIN_OUTPUT_RATE, MAX_OUTPUT_RATE);

                const double scale{static_cast<double>(freq) / device->Frequency};
                device->UpdateSize = static_cast<uint>(device->UpdateSize*scale + 0.5);
                device->BufferSize = static_cast<uint>(device->BufferSize*scale + 0.5);

                device->Frequency = freq;
                device->Flags.set(FrequencyRequest);
            }

            auto set_device_mode = [device](DevFmtChannels chans) noexcept
            {
                device->FmtChans = chans;
                device->Flags.set(ChannelsRequest);
            };
            if(opthrtf)
            {
                if(*opthrtf)
                {
                    set_device_mode(DevFmtStereo);
                    stereomode = StereoEncoding::Hrtf;
                }
                else
                    stereomode = StereoEncoding::Default;
            }

            using OutputMode = ALCdevice::OutputMode;
            switch(OutputMode(outmode))
            {
            case OutputMode::Any: break;
            case OutputMode::Mono: set_device_mode(DevFmtMono); break;
            case OutputMode::Stereo: set_device_mode(DevFmtStereo); break;
            case OutputMode::StereoBasic:
                set_device_mode(DevFmtStereo);
                stereomode = StereoEncoding::Basic;
                break;
            case OutputMode::Uhj2:
                set_device_mode(DevFmtStereo);
                stereomode = StereoEncoding::Uhj;
                break;
            case OutputMode::Hrtf:
                set_device_mode(DevFmtStereo);
                stereomode = StereoEncoding::Hrtf;
                break;
            case OutputMode::Quad: set_device_mode(DevFmtQuad); break;
            case OutputMode::X51: set_device_mode(DevFmtX51); break;
            case OutputMode::X61: set_device_mode(DevFmtX61); break;
            case OutputMode::X71: set_device_mode(DevFmtX71); break;
            }
        }
    }

    if(device->Flags.test(DeviceRunning))
        return ALC_NO_ERROR;

    device->AvgSpeakerDist = 0.0f;
    device->mNFCtrlFilter = NfcFilter{};
    device->mUhjEncoder = nullptr;
    device->AmbiDecoder = nullptr;
    device->Bs2b = nullptr;
    device->PostProcess = nullptr;

    device->Limiter = nullptr;
    device->ChannelDelays = nullptr;

    std::fill(std::begin(device->HrtfAccumData), std::end(device->HrtfAccumData), float2{});

    device->Dry.AmbiMap.fill(BFChannelConfig{});
    device->Dry.Buffer = {};
    std::fill(std::begin(device->NumChannelsPerOrder), std::end(device->NumChannelsPerOrder), 0u);
    device->RealOut.RemixMap = {};
    device->RealOut.ChannelIndex.fill(INVALID_CHANNEL_INDEX);
    device->RealOut.Buffer = {};
    device->MixBuffer.clear();
    device->MixBuffer.shrink_to_fit();

    UpdateClockBase(device);
    device->FixedLatency = nanoseconds::zero();

    device->DitherDepth = 0.0f;
    device->DitherSeed = DitherRNGSeed;

    device->mHrtfStatus = ALC_HRTF_DISABLED_SOFT;

    /*************************************************************************
     * Update device format request from the user configuration
     */
    if(device->Type != DeviceType::Loopback)
    {
        if(auto typeopt = device->configValue<std::string>(nullptr, "sample-type"))
        {
            static constexpr struct TypeMap {
                const char name[8];
                DevFmtType type;
            } typelist[] = {
                { "int8",    DevFmtByte   },
                { "uint8",   DevFmtUByte  },
                { "int16",   DevFmtShort  },
                { "uint16",  DevFmtUShort },
                { "int32",   DevFmtInt    },
                { "uint32",  DevFmtUInt   },
                { "float32", DevFmtFloat  },
            };

            const ALCchar *fmt{typeopt->c_str()};
            auto iter = std::find_if(std::begin(typelist), std::end(typelist),
                [fmt](const TypeMap &entry) -> bool
                { return al::strcasecmp(entry.name, fmt) == 0; });
            if(iter == std::end(typelist))
                ERR("Unsupported sample-type: %s\n", fmt);
            else
            {
                device->FmtType = iter->type;
                device->Flags.set(SampleTypeRequest);
            }
        }
        if(auto chanopt = device->configValue<std::string>(nullptr, "channels"))
        {
            static constexpr struct ChannelMap {
                const char name[16];
                DevFmtChannels chans;
                uint8_t order;
            } chanlist[] = {
                { "mono",       DevFmtMono,   0 },
                { "stereo",     DevFmtStereo, 0 },
                { "quad",       DevFmtQuad,   0 },
                { "surround51", DevFmtX51,    0 },
                { "surround61", DevFmtX61,    0 },
                { "surround71", DevFmtX71,    0 },
                { "surround51rear", DevFmtX51, 0 },
                { "ambi1", DevFmtAmbi3D, 1 },
                { "ambi2", DevFmtAmbi3D, 2 },
                { "ambi3", DevFmtAmbi3D, 3 },
            };

            const ALCchar *fmt{chanopt->c_str()};
            auto iter = std::find_if(std::begin(chanlist), std::end(chanlist),
                [fmt](const ChannelMap &entry) -> bool
                { return al::strcasecmp(entry.name, fmt) == 0; });
            if(iter == std::end(chanlist))
                ERR("Unsupported channels: %s\n", fmt);
            else
            {
                device->FmtChans = iter->chans;
                device->mAmbiOrder = iter->order;
                device->Flags.set(ChannelsRequest);
            }
        }
        if(auto ambiopt = device->configValue<std::string>(nullptr, "ambi-format"))
        {
            const ALCchar *fmt{ambiopt->c_str()};
            if(al::strcasecmp(fmt, "fuma") == 0)
            {
                if(device->mAmbiOrder > 3)
                    ERR("FuMa is incompatible with %d%s order ambisonics (up to 3rd order only)\n",
                        device->mAmbiOrder,
                        (((device->mAmbiOrder%100)/10) == 1) ? "th" :
                        ((device->mAmbiOrder%10) == 1) ? "st" :
                        ((device->mAmbiOrder%10) == 2) ? "nd" :
                        ((device->mAmbiOrder%10) == 3) ? "rd" : "th");
                else
                {
                    device->mAmbiLayout = DevAmbiLayout::FuMa;
                    device->mAmbiScale = DevAmbiScaling::FuMa;
                }
            }
            else if(al::strcasecmp(fmt, "acn+fuma") == 0)
            {
                if(device->mAmbiOrder > 3)
                    ERR("FuMa is incompatible with %d%s order ambisonics (up to 3rd order only)\n",
                        device->mAmbiOrder,
                        (((device->mAmbiOrder%100)/10) == 1) ? "th" :
                        ((device->mAmbiOrder%10) == 1) ? "st" :
                        ((device->mAmbiOrder%10) == 2) ? "nd" :
                        ((device->mAmbiOrder%10) == 3) ? "rd" : "th");
                else
                {
                    device->mAmbiLayout = DevAmbiLayout::ACN;
                    device->mAmbiScale = DevAmbiScaling::FuMa;
                }
            }
            else if(al::strcasecmp(fmt, "ambix") == 0 || al::strcasecmp(fmt, "acn+sn3d") == 0)
            {
                device->mAmbiLayout = DevAmbiLayout::ACN;
                device->mAmbiScale = DevAmbiScaling::SN3D;
            }
            else if(al::strcasecmp(fmt, "acn+n3d") == 0)
            {
                device->mAmbiLayout = DevAmbiLayout::ACN;
                device->mAmbiScale = DevAmbiScaling::N3D;
            }
            else
                ERR("Unsupported ambi-format: %s\n", fmt);
        }

        if(auto persizeopt = device->configValue<uint>(nullptr, "period_size"))
            device->UpdateSize = clampu(*persizeopt, 64, 8192);

        if(auto peropt = device->configValue<uint>(nullptr, "periods"))
            device->BufferSize = device->UpdateSize * clampu(*peropt, 2, 16);
        else
            device->BufferSize = maxu(device->BufferSize, device->UpdateSize*2);

        if(auto hrtfopt = device->configValue<std::string>(nullptr, "hrtf"))
        {
            const char *hrtf{hrtfopt->c_str()};
            if(al::strcasecmp(hrtf, "true") == 0)
            {
                stereomode = StereoEncoding::Hrtf;
                device->FmtChans = DevFmtStereo;
                device->Flags.set(ChannelsRequest);
            }
            else if(al::strcasecmp(hrtf, "false") == 0)
            {
                if(!stereomode || *stereomode == StereoEncoding::Hrtf)
                    stereomode = StereoEncoding::Default;
            }
            else if(al::strcasecmp(hrtf, "auto") != 0)
                ERR("Unexpected hrtf value: %s\n", hrtf);
        }
    }

    TRACE("Pre-reset: %s%s, %s%s, %s%uhz, %u / %u buffer\n",
        device->Flags.test(ChannelsRequest)?"*":"", DevFmtChannelsString(device->FmtChans),
        device->Flags.test(SampleTypeRequest)?"*":"", DevFmtTypeString(device->FmtType),
        device->Flags.test(FrequencyRequest)?"*":"", device->Frequency,
        device->UpdateSize, device->BufferSize);

    const uint oldFreq{device->Frequency};
    const DevFmtChannels oldChans{device->FmtChans};
    const DevFmtType oldType{device->FmtType};
    try {
        auto backend = device->Backend.get();
        if(!backend->reset())
            throw al::backend_exception{al::backend_error::DeviceError, "Device reset failure"};
    }
    catch(std::exception &e) {
        ERR("Device error: %s\n", e.what());
        device->handleDisconnect("%s", e.what());
        return ALC_INVALID_DEVICE;
    }

    if(device->FmtChans != oldChans && device->Flags.test(ChannelsRequest))
    {
        ERR("Failed to set %s, got %s instead\n", DevFmtChannelsString(oldChans),
            DevFmtChannelsString(device->FmtChans));
        device->Flags.reset(ChannelsRequest);
    }
    if(device->FmtType != oldType && device->Flags.test(SampleTypeRequest))
    {
        ERR("Failed to set %s, got %s instead\n", DevFmtTypeString(oldType),
            DevFmtTypeString(device->FmtType));
        device->Flags.reset(SampleTypeRequest);
    }
    if(device->Frequency != oldFreq && device->Flags.test(FrequencyRequest))
    {
        WARN("Failed to set %uhz, got %uhz instead\n", oldFreq, device->Frequency);
        device->Flags.reset(FrequencyRequest);
    }

    TRACE("Post-reset: %s, %s, %uhz, %u / %u buffer\n",
        DevFmtChannelsString(device->FmtChans), DevFmtTypeString(device->FmtType),
        device->Frequency, device->UpdateSize, device->BufferSize);

    if(device->Type != DeviceType::Loopback)
    {
        if(auto modeopt = device->configValue<std::string>(nullptr, "stereo-mode"))
        {
            const char *mode{modeopt->c_str()};
            if(al::strcasecmp(mode, "headphones") == 0)
                device->Flags.set(DirectEar);
            else if(al::strcasecmp(mode, "speakers") == 0)
                device->Flags.reset(DirectEar);
            else if(al::strcasecmp(mode, "auto") != 0)
                ERR("Unexpected stereo-mode: %s\n", mode);
        }

        if(auto encopt = device->configValue<std::string>(nullptr, "stereo-encoding"))
        {
            const char *mode{encopt->c_str()};
            if(al::strcasecmp(mode, "panpot") == 0)
                stereomode = al::make_optional(StereoEncoding::Basic);
            else if(al::strcasecmp(mode, "uhj") == 0)
                stereomode = al::make_optional(StereoEncoding::Uhj);
            else if(al::strcasecmp(mode, "hrtf") == 0)
                stereomode = al::make_optional(StereoEncoding::Hrtf);
            else
                ERR("Unexpected stereo-encoding: %s\n", mode);
        }
    }

    aluInitRenderer(device, hrtf_id, stereomode);

    TRACE("Max sources: %d (%d + %d), effect slots: %d, sends: %d\n",
        device->SourcesMax, device->NumMonoSources, device->NumStereoSources,
        device->AuxiliaryEffectSlotMax, device->NumAuxSends);

    switch(device->FmtChans)
    {
    case DevFmtMono: break;
    case DevFmtStereo:
        if(!device->mUhjEncoder)
            device->RealOut.RemixMap = StereoDownmix;
        break;
    case DevFmtQuad: device->RealOut.RemixMap = QuadDownmix; break;
    case DevFmtX51: device->RealOut.RemixMap = X51Downmix; break;
    case DevFmtX61: device->RealOut.RemixMap = X61Downmix; break;
    case DevFmtX71: device->RealOut.RemixMap = X71Downmix; break;
    case DevFmtAmbi3D: break;
    }

    nanoseconds::rep sample_delay{0};
    if(device->mUhjEncoder)
        sample_delay += UhjEncoder::sFilterDelay;
    if(auto *ambidec = device->AmbiDecoder.get())
    {
        if(ambidec->hasStablizer())
            sample_delay += FrontStablizer::DelayLength;
    }

    if(device->getConfigValueBool(nullptr, "dither", true))
    {
        int depth{device->configValue<int>(nullptr, "dither-depth").value_or(0)};
        if(depth <= 0)
        {
            switch(device->FmtType)
            {
            case DevFmtByte:
            case DevFmtUByte:
                depth = 8;
                break;
            case DevFmtShort:
            case DevFmtUShort:
                depth = 16;
                break;
            case DevFmtInt:
            case DevFmtUInt:
            case DevFmtFloat:
                break;
            }
        }

        if(depth > 0)
        {
            depth = clampi(depth, 2, 24);
            device->DitherDepth = std::pow(2.0f, static_cast<float>(depth-1));
        }
    }
    if(!(device->DitherDepth > 0.0f))
        TRACE("Dithering disabled\n");
    else
        TRACE("Dithering enabled (%d-bit, %g)\n", float2int(std::log2(device->DitherDepth)+0.5f)+1,
              device->DitherDepth);

    if(auto limopt = device->configValue<bool>(nullptr, "output-limiter"))
        optlimit = limopt;

    /* If the gain limiter is unset, use the limiter for integer-based output
     * (where samples must be clamped), and don't for floating-point (which can
     * take unclamped samples).
     */
    if(!optlimit)
    {
        switch(device->FmtType)
        {
        case DevFmtByte:
        case DevFmtUByte:
        case DevFmtShort:
        case DevFmtUShort:
        case DevFmtInt:
        case DevFmtUInt:
            optlimit = true;
            break;
        case DevFmtFloat:
            break;
        }
    }
    if(optlimit.value_or(false) == false)
        TRACE("Output limiter disabled\n");
    else
    {
        float thrshld{1.0f};
        switch(device->FmtType)
        {
        case DevFmtByte:
        case DevFmtUByte:
            thrshld = 127.0f / 128.0f;
            break;
        case DevFmtShort:
        case DevFmtUShort:
            thrshld = 32767.0f / 32768.0f;
            break;
        case DevFmtInt:
        case DevFmtUInt:
        case DevFmtFloat:
            break;
        }
        if(device->DitherDepth > 0.0f)
            thrshld -= 1.0f / device->DitherDepth;

        const float thrshld_dB{std::log10(thrshld) * 20.0f};
        auto limiter = CreateDeviceLimiter(device, thrshld_dB);

        sample_delay += limiter->getLookAhead();
        device->Limiter = std::move(limiter);
        TRACE("Output limiter enabled, %.4fdB limit\n", thrshld_dB);
    }

    /* Convert the sample delay from samples to nanosamples to nanoseconds. */
    device->FixedLatency += nanoseconds{seconds{sample_delay}} / device->Frequency;
    TRACE("Fixed device latency: %" PRId64 "ns\n", int64_t{device->FixedLatency.count()});

    FPUCtl mixer_mode{};
    for(ContextBase *ctxbase : *device->mContexts.load())
    {
        auto *context = static_cast<ALCcontext*>(ctxbase);

        auto GetEffectBuffer = [](ALbuffer *buffer) noexcept -> EffectState::Buffer
        {
            if(!buffer) return EffectState::Buffer{};
            return EffectState::Buffer{buffer, buffer->mData};
        };
        std::unique_lock<std::mutex> proplock{context->mPropLock};
        std::unique_lock<std::mutex> slotlock{context->mEffectSlotLock};

        /* Clear out unused wet buffers. */
        auto buffer_not_in_use = [](WetBufferPtr &wetbuffer) noexcept -> bool
        { return !wetbuffer->mInUse; };
        auto wetbuffer_iter = std::remove_if(context->mWetBuffers.begin(),
            context->mWetBuffers.end(), buffer_not_in_use);
        context->mWetBuffers.erase(wetbuffer_iter, context->mWetBuffers.end());

        if(ALeffectslot *slot{context->mDefaultSlot.get()})
        {
            aluInitEffectPanning(&slot->mSlot, context);

            EffectState *state{slot->Effect.State.get()};
            state->mOutTarget = device->Dry.Buffer;
            state->deviceUpdate(device, GetEffectBuffer(slot->Buffer));
            slot->updateProps(context);
        }

        if(EffectSlotArray *curarray{context->mActiveAuxSlots.load(std::memory_order_relaxed)})
            std::fill_n(curarray->end(), curarray->size(), nullptr);
        for(auto &sublist : context->mEffectSlotList)
        {
            uint64_t usemask{~sublist.FreeMask};
            while(usemask)
            {
                const int idx{al::countr_zero(usemask)};
                ALeffectslot *slot{sublist.EffectSlots + idx};
                usemask &= ~(1_u64 << idx);

                aluInitEffectPanning(&slot->mSlot, context);

                EffectState *state{slot->Effect.State.get()};
                state->mOutTarget = device->Dry.Buffer;
                state->deviceUpdate(device, GetEffectBuffer(slot->Buffer));
                slot->updateProps(context);
            }
        }
        slotlock.unlock();

        const uint num_sends{device->NumAuxSends};
        std::unique_lock<std::mutex> srclock{context->mSourceLock};
        for(auto &sublist : context->mSourceList)
        {
            uint64_t usemask{~sublist.FreeMask};
            while(usemask)
            {
                const int idx{al::countr_zero(usemask)};
                ALsource *source{sublist.Sources + idx};
                usemask &= ~(1_u64 << idx);

                auto clear_send = [](ALsource::SendData &send) -> void
                {
                    if(send.Slot)
                        DecrementRef(send.Slot->ref);
                    send.Slot = nullptr;
                    send.Gain = 1.0f;
                    send.GainHF = 1.0f;
                    send.HFReference = LOWPASSFREQREF;
                    send.GainLF = 1.0f;
                    send.LFReference = HIGHPASSFREQREF;
                };
                auto send_begin = source->Send.begin() + static_cast<ptrdiff_t>(num_sends);
                std::for_each(send_begin, source->Send.end(), clear_send);

                source->mPropsDirty = true;
            }
        }

        auto voicelist = context->getVoicesSpan();
        for(Voice *voice : voicelist)
        {
            /* Clear extraneous property set sends. */
            std::fill(std::begin(voice->mProps.Send)+num_sends, std::end(voice->mProps.Send),
                VoiceProps::SendData{});

            std::fill(voice->mSend.begin()+num_sends, voice->mSend.end(), Voice::TargetData{});
            for(auto &chandata : voice->mChans)
            {
                std::fill(chandata.mWetParams.begin()+num_sends, chandata.mWetParams.end(),
                    SendParams{});
            }

            if(VoicePropsItem *props{voice->mUpdate.exchange(nullptr, std::memory_order_relaxed)})
                AtomicReplaceHead(context->mFreeVoiceProps, props);

            /* Force the voice to stopped if it was stopping. */
            Voice::State vstate{Voice::Stopping};
            voice->mPlayState.compare_exchange_strong(vstate, Voice::Stopped,
                std::memory_order_acquire, std::memory_order_acquire);
            if(voice->mSourceID.load(std::memory_order_relaxed) == 0u)
                continue;

            voice->prepare(device);
        }
        /* Clear all voice props to let them get allocated again. */
        context->mVoicePropClusters.clear();
        context->mFreeVoiceProps.store(nullptr, std::memory_order_relaxed);
        srclock.unlock();

        context->mPropsDirty = false;
        UpdateContextProps(context);
        UpdateAllSourceProps(context);
    }
    mixer_mode.leave();

    if(!device->Flags.test(DevicePaused))
    {
        try {
            auto backend = device->Backend.get();
            backend->start();
            device->Flags.set(DeviceRunning);
        }
        catch(al::backend_exception& e) {
            ERR("%s\n", e.what());
            device->handleDisconnect("%s", e.what());
            return ALC_INVALID_DEVICE;
        }
        TRACE("Post-start: %s, %s, %uhz, %u / %u buffer\n",
            DevFmtChannelsString(device->FmtChans), DevFmtTypeString(device->FmtType),
            device->Frequency, device->UpdateSize, device->BufferSize);
    }

    return ALC_NO_ERROR;
}

/**
 * Updates device parameters as above, and also first clears the disconnected
 * status, if set.
 */
bool ResetDeviceParams(ALCdevice *device, const int *attrList)
{
    /* If the device was disconnected, reset it since we're opened anew. */
    if UNLIKELY(!device->Connected.load(std::memory_order_relaxed))
    {
        /* Make sure disconnection is finished before continuing on. */
        device->waitForMix();

        for(ContextBase *ctxbase : *device->mContexts.load(std::memory_order_acquire))
        {
            auto *ctx = static_cast<ALCcontext*>(ctxbase);
            if(!ctx->mStopVoicesOnDisconnect.load(std::memory_order_acquire))
                continue;

            /* Clear any pending voice changes and reallocate voices to get a
             * clean restart.
             */
            std::lock_guard<std::mutex> __{ctx->mSourceLock};
            auto *vchg = ctx->mCurrentVoiceChange.load(std::memory_order_acquire);
            while(auto *next = vchg->mNext.load(std::memory_order_acquire))
                vchg = next;
            ctx->mCurrentVoiceChange.store(vchg, std::memory_order_release);

            ctx->mVoicePropClusters.clear();
            ctx->mFreeVoiceProps.store(nullptr, std::memory_order_relaxed);

            ctx->mVoiceClusters.clear();
            ctx->allocVoices(std::max<size_t>(256,
                ctx->mActiveVoiceCount.load(std::memory_order_relaxed)));
        }

        device->Connected.store(true);
    }

    ALCenum err{UpdateDeviceParams(device, attrList)};
    if LIKELY(err == ALC_NO_ERROR) return ALC_TRUE;

    alcSetError(device, err);
    return ALC_FALSE;
}


/** Checks if the device handle is valid, and returns a new reference if so. */
DeviceRef VerifyDevice(ALCdevice *device)
{
    std::lock_guard<std::recursive_mutex> _{ListLock};
    auto iter = std::lower_bound(DeviceList.begin(), DeviceList.end(), device);
    if(iter != DeviceList.end() && *iter == device)
    {
        (*iter)->add_ref();
        return DeviceRef{*iter};
    }
    return nullptr;
}


/**
 * Checks if the given context is valid, returning a new reference to it if so.
 */
ContextRef VerifyContext(ALCcontext *context)
{
    std::lock_guard<std::recursive_mutex> _{ListLock};
    auto iter = std::lower_bound(ContextList.begin(), ContextList.end(), context);
    if(iter != ContextList.end() && *iter == context)
    {
        (*iter)->add_ref();
        return ContextRef{*iter};
    }
    return nullptr;
}

} // namespace

/** Returns a new reference to the currently active context for this thread. */
ContextRef GetContextRef(void)
{
    ALCcontext *context{ALCcontext::getThreadContext()};
    if(context)
        context->add_ref();
    else
    {
        std::lock_guard<std::recursive_mutex> _{ListLock};
        context = ALCcontext::sGlobalContext.load(std::memory_order_acquire);
        if(context) context->add_ref();
    }
    return ContextRef{context};
}


/************************************************
 * Standard ALC functions
 ************************************************/

ALC_API ALCenum ALC_APIENTRY alcGetError(ALCdevice *device)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(dev) return dev->LastError.exchange(ALC_NO_ERROR);
    return LastNullDeviceError.exchange(ALC_NO_ERROR);
}
END_API_FUNC


ALC_API void ALC_APIENTRY alcSuspendContext(ALCcontext *context)
START_API_FUNC
{
    if(!SuspendDefers)
        return;

    ContextRef ctx{VerifyContext(context)};
    if(!ctx)
        alcSetError(nullptr, ALC_INVALID_CONTEXT);
    else
    {
        std::lock_guard<std::mutex> _{ctx->mPropLock};
        ctx->deferUpdates();
    }
}
END_API_FUNC

ALC_API void ALC_APIENTRY alcProcessContext(ALCcontext *context)
START_API_FUNC
{
    if(!SuspendDefers)
        return;

    ContextRef ctx{VerifyContext(context)};
    if(!ctx)
        alcSetError(nullptr, ALC_INVALID_CONTEXT);
    else
    {
        std::lock_guard<std::mutex> _{ctx->mPropLock};
        ctx->processUpdates();
    }
}
END_API_FUNC


ALC_API const ALCchar* ALC_APIENTRY alcGetString(ALCdevice *Device, ALCenum param)
START_API_FUNC
{
    const ALCchar *value{nullptr};

    switch(param)
    {
    case ALC_NO_ERROR:
        value = alcNoError;
        break;

    case ALC_INVALID_ENUM:
        value = alcErrInvalidEnum;
        break;

    case ALC_INVALID_VALUE:
        value = alcErrInvalidValue;
        break;

    case ALC_INVALID_DEVICE:
        value = alcErrInvalidDevice;
        break;

    case ALC_INVALID_CONTEXT:
        value = alcErrInvalidContext;
        break;

    case ALC_OUT_OF_MEMORY:
        value = alcErrOutOfMemory;
        break;

    case ALC_DEVICE_SPECIFIER:
        value = alcDefaultName;
        break;

    case ALC_ALL_DEVICES_SPECIFIER:
        if(DeviceRef dev{VerifyDevice(Device)})
        {
            if(dev->Type == DeviceType::Capture)
                alcSetError(dev.get(), ALC_INVALID_ENUM);
            else if(dev->Type == DeviceType::Loopback)
                value = alcDefaultName;
            else
            {
                std::lock_guard<std::mutex> _{dev->StateLock};
                value = dev->DeviceName.c_str();
            }
        }
        else
        {
            ProbeAllDevicesList();
            value = alcAllDevicesList.c_str();
        }
        break;

    case ALC_CAPTURE_DEVICE_SPECIFIER:
        if(DeviceRef dev{VerifyDevice(Device)})
        {
            if(dev->Type != DeviceType::Capture)
                alcSetError(dev.get(), ALC_INVALID_ENUM);
            else
            {
                std::lock_guard<std::mutex> _{dev->StateLock};
                value = dev->DeviceName.c_str();
            }
        }
        else
        {
            ProbeCaptureDeviceList();
            value = alcCaptureDeviceList.c_str();
        }
        break;

    /* Default devices are always first in the list */
    case ALC_DEFAULT_DEVICE_SPECIFIER:
        value = alcDefaultName;
        break;

    case ALC_DEFAULT_ALL_DEVICES_SPECIFIER:
        if(alcAllDevicesList.empty())
            ProbeAllDevicesList();

        /* Copy first entry as default. */
        alcDefaultAllDevicesSpecifier = alcAllDevicesList.c_str();
        value = alcDefaultAllDevicesSpecifier.c_str();
        break;

    case ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER:
        if(alcCaptureDeviceList.empty())
            ProbeCaptureDeviceList();

        /* Copy first entry as default. */
        alcCaptureDefaultDeviceSpecifier = alcCaptureDeviceList.c_str();
        value = alcCaptureDefaultDeviceSpecifier.c_str();
        break;

    case ALC_EXTENSIONS:
        if(VerifyDevice(Device))
            value = alcExtensionList;
        else
            value = alcNoDeviceExtList;
        break;

    case ALC_HRTF_SPECIFIER_SOFT:
        if(DeviceRef dev{VerifyDevice(Device)})
        {
            std::lock_guard<std::mutex> _{dev->StateLock};
            value = (dev->mHrtf ? dev->mHrtfName.c_str() : "");
        }
        else
            alcSetError(nullptr, ALC_INVALID_DEVICE);
        break;

    default:
        alcSetError(VerifyDevice(Device).get(), ALC_INVALID_ENUM);
        break;
    }

    return value;
}
END_API_FUNC


static size_t GetIntegerv(ALCdevice *device, ALCenum param, const al::span<int> values)
{
    size_t i;

    if(values.empty())
    {
        alcSetError(device, ALC_INVALID_VALUE);
        return 0;
    }

    if(!device)
    {
        switch(param)
        {
        case ALC_MAJOR_VERSION:
            values[0] = alcMajorVersion;
            return 1;
        case ALC_MINOR_VERSION:
            values[0] = alcMinorVersion;
            return 1;

        case ALC_EFX_MAJOR_VERSION:
            values[0] = alcEFXMajorVersion;
            return 1;
        case ALC_EFX_MINOR_VERSION:
            values[0] = alcEFXMinorVersion;
            return 1;
        case ALC_MAX_AUXILIARY_SENDS:
            values[0] = MAX_SENDS;
            return 1;

        case ALC_ATTRIBUTES_SIZE:
        case ALC_ALL_ATTRIBUTES:
        case ALC_FREQUENCY:
        case ALC_REFRESH:
        case ALC_SYNC:
        case ALC_MONO_SOURCES:
        case ALC_STEREO_SOURCES:
        case ALC_CAPTURE_SAMPLES:
        case ALC_FORMAT_CHANNELS_SOFT:
        case ALC_FORMAT_TYPE_SOFT:
        case ALC_AMBISONIC_LAYOUT_SOFT:
        case ALC_AMBISONIC_SCALING_SOFT:
        case ALC_AMBISONIC_ORDER_SOFT:
        case ALC_MAX_AMBISONIC_ORDER_SOFT:
            alcSetError(nullptr, ALC_INVALID_DEVICE);
            return 0;

        default:
            alcSetError(nullptr, ALC_INVALID_ENUM);
        }
        return 0;
    }

    std::lock_guard<std::mutex> _{device->StateLock};
    if(device->Type == DeviceType::Capture)
    {
        static constexpr int MaxCaptureAttributes{9};
        switch(param)
        {
        case ALC_ATTRIBUTES_SIZE:
            values[0] = MaxCaptureAttributes;
            return 1;
        case ALC_ALL_ATTRIBUTES:
            i = 0;
            if(values.size() < MaxCaptureAttributes)
                alcSetError(device, ALC_INVALID_VALUE);
            else
            {
                values[i++] = ALC_MAJOR_VERSION;
                values[i++] = alcMajorVersion;
                values[i++] = ALC_MINOR_VERSION;
                values[i++] = alcMinorVersion;
                values[i++] = ALC_CAPTURE_SAMPLES;
                values[i++] = static_cast<int>(device->Backend->availableSamples());
                values[i++] = ALC_CONNECTED;
                values[i++] = device->Connected.load(std::memory_order_relaxed);
                values[i++] = 0;
                assert(i == MaxCaptureAttributes);
            }
            return i;

        case ALC_MAJOR_VERSION:
            values[0] = alcMajorVersion;
            return 1;
        case ALC_MINOR_VERSION:
            values[0] = alcMinorVersion;
            return 1;

        case ALC_CAPTURE_SAMPLES:
            values[0] = static_cast<int>(device->Backend->availableSamples());
            return 1;

        case ALC_CONNECTED:
            values[0] = device->Connected.load(std::memory_order_acquire);
            return 1;

        default:
            alcSetError(device, ALC_INVALID_ENUM);
        }
        return 0;
    }

    /* render device */
    auto NumAttrsForDevice = [](ALCdevice *aldev) noexcept
    {
        if(aldev->Type == DeviceType::Loopback && aldev->FmtChans == DevFmtAmbi3D)
            return 37;
        return 31;
    };
    switch(param)
    {
    case ALC_ATTRIBUTES_SIZE:
        values[0] = NumAttrsForDevice(device);
        return 1;

    case ALC_ALL_ATTRIBUTES:
        i = 0;
        if(values.size() < static_cast<size_t>(NumAttrsForDevice(device)))
            alcSetError(device, ALC_INVALID_VALUE);
        else
        {
            values[i++] = ALC_MAJOR_VERSION;
            values[i++] = alcMajorVersion;
            values[i++] = ALC_MINOR_VERSION;
            values[i++] = alcMinorVersion;
            values[i++] = ALC_EFX_MAJOR_VERSION;
            values[i++] = alcEFXMajorVersion;
            values[i++] = ALC_EFX_MINOR_VERSION;
            values[i++] = alcEFXMinorVersion;

            values[i++] = ALC_FREQUENCY;
            values[i++] = static_cast<int>(device->Frequency);
            if(device->Type != DeviceType::Loopback)
            {
                values[i++] = ALC_REFRESH;
                values[i++] = static_cast<int>(device->Frequency / device->UpdateSize);

                values[i++] = ALC_SYNC;
                values[i++] = ALC_FALSE;
            }
            else
            {
                if(device->FmtChans == DevFmtAmbi3D)
                {
                    values[i++] = ALC_AMBISONIC_LAYOUT_SOFT;
                    values[i++] = EnumFromDevAmbi(device->mAmbiLayout);

                    values[i++] = ALC_AMBISONIC_SCALING_SOFT;
                    values[i++] = EnumFromDevAmbi(device->mAmbiScale);

                    values[i++] = ALC_AMBISONIC_ORDER_SOFT;
                    values[i++] = static_cast<int>(device->mAmbiOrder);
                }

                values[i++] = ALC_FORMAT_CHANNELS_SOFT;
                values[i++] = EnumFromDevFmt(device->FmtChans);

                values[i++] = ALC_FORMAT_TYPE_SOFT;
                values[i++] = EnumFromDevFmt(device->FmtType);
            }

            values[i++] = ALC_MONO_SOURCES;
            values[i++] = static_cast<int>(device->NumMonoSources);

            values[i++] = ALC_STEREO_SOURCES;
            values[i++] = static_cast<int>(device->NumStereoSources);

            values[i++] = ALC_MAX_AUXILIARY_SENDS;
            values[i++] = static_cast<int>(device->NumAuxSends);

            values[i++] = ALC_HRTF_SOFT;
            values[i++] = (device->mHrtf ? ALC_TRUE : ALC_FALSE);

            values[i++] = ALC_HRTF_STATUS_SOFT;
            values[i++] = device->mHrtfStatus;

            values[i++] = ALC_OUTPUT_LIMITER_SOFT;
            values[i++] = device->Limiter ? ALC_TRUE : ALC_FALSE;

            values[i++] = ALC_MAX_AMBISONIC_ORDER_SOFT;
            values[i++] = MaxAmbiOrder;

            values[i++] = ALC_OUTPUT_MODE_SOFT;
            values[i++] = static_cast<ALCenum>(device->getOutputMode1());

            values[i++] = 0;
        }
        return i;

    case ALC_MAJOR_VERSION:
        values[0] = alcMajorVersion;
        return 1;

    case ALC_MINOR_VERSION:
        values[0] = alcMinorVersion;
        return 1;

    case ALC_EFX_MAJOR_VERSION:
        values[0] = alcEFXMajorVersion;
        return 1;

    case ALC_EFX_MINOR_VERSION:
        values[0] = alcEFXMinorVersion;
        return 1;

    case ALC_FREQUENCY:
        values[0] = static_cast<int>(device->Frequency);
        return 1;

    case ALC_REFRESH:
        if(device->Type == DeviceType::Loopback)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = static_cast<int>(device->Frequency / device->UpdateSize);
        return 1;

    case ALC_SYNC:
        if(device->Type == DeviceType::Loopback)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = ALC_FALSE;
        return 1;

    case ALC_FORMAT_CHANNELS_SOFT:
        if(device->Type != DeviceType::Loopback)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = EnumFromDevFmt(device->FmtChans);
        return 1;

    case ALC_FORMAT_TYPE_SOFT:
        if(device->Type != DeviceType::Loopback)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = EnumFromDevFmt(device->FmtType);
        return 1;

    case ALC_AMBISONIC_LAYOUT_SOFT:
        if(device->Type != DeviceType::Loopback || device->FmtChans != DevFmtAmbi3D)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = EnumFromDevAmbi(device->mAmbiLayout);
        return 1;

    case ALC_AMBISONIC_SCALING_SOFT:
        if(device->Type != DeviceType::Loopback || device->FmtChans != DevFmtAmbi3D)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = EnumFromDevAmbi(device->mAmbiScale);
        return 1;

    case ALC_AMBISONIC_ORDER_SOFT:
        if(device->Type != DeviceType::Loopback || device->FmtChans != DevFmtAmbi3D)
        {
            alcSetError(device, ALC_INVALID_DEVICE);
            return 0;
        }
        values[0] = static_cast<int>(device->mAmbiOrder);
        return 1;

    case ALC_MONO_SOURCES:
        values[0] = static_cast<int>(device->NumMonoSources);
        return 1;

    case ALC_STEREO_SOURCES:
        values[0] = static_cast<int>(device->NumStereoSources);
        return 1;

    case ALC_MAX_AUXILIARY_SENDS:
        values[0] = static_cast<int>(device->NumAuxSends);
        return 1;

    case ALC_CONNECTED:
        values[0] = device->Connected.load(std::memory_order_acquire);
        return 1;

    case ALC_HRTF_SOFT:
        values[0] = (device->mHrtf ? ALC_TRUE : ALC_FALSE);
        return 1;

    case ALC_HRTF_STATUS_SOFT:
        values[0] = device->mHrtfStatus;
        return 1;

    case ALC_NUM_HRTF_SPECIFIERS_SOFT:
        device->enumerateHrtfs();
        values[0] = static_cast<int>(minz(device->mHrtfList.size(),
            std::numeric_limits<int>::max()));
        return 1;

    case ALC_OUTPUT_LIMITER_SOFT:
        values[0] = device->Limiter ? ALC_TRUE : ALC_FALSE;
        return 1;

    case ALC_MAX_AMBISONIC_ORDER_SOFT:
        values[0] = MaxAmbiOrder;
        return 1;

    case ALC_OUTPUT_MODE_SOFT:
        values[0] = static_cast<ALCenum>(device->getOutputMode1());
        return 1;

    default:
        alcSetError(device, ALC_INVALID_ENUM);
    }
    return 0;
}

ALC_API void ALC_APIENTRY alcGetIntegerv(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(size <= 0 || values == nullptr)
        alcSetError(dev.get(), ALC_INVALID_VALUE);
    else
        GetIntegerv(dev.get(), param, {values, static_cast<uint>(size)});
}
END_API_FUNC

ALC_API void ALC_APIENTRY alcGetInteger64vSOFT(ALCdevice *device, ALCenum pname, ALCsizei size, ALCint64SOFT *values)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(size <= 0 || values == nullptr)
    {
        alcSetError(dev.get(), ALC_INVALID_VALUE);
        return;
    }
    if(!dev || dev->Type == DeviceType::Capture)
    {
        auto ivals = al::vector<int>(static_cast<uint>(size));
        if(size_t got{GetIntegerv(dev.get(), pname, ivals)})
            std::copy_n(ivals.begin(), got, values);
        return;
    }
    /* render device */
    auto NumAttrsForDevice = [](ALCdevice *aldev) noexcept
    {
        if(aldev->Type == DeviceType::Loopback && aldev->FmtChans == DevFmtAmbi3D)
            return 41;
        return 35;
    };
    std::lock_guard<std::mutex> _{dev->StateLock};
    switch(pname)
    {
    case ALC_ATTRIBUTES_SIZE:
        *values = NumAttrsForDevice(dev.get());
        break;

    case ALC_ALL_ATTRIBUTES:
        if(size < NumAttrsForDevice(dev.get()))
            alcSetError(dev.get(), ALC_INVALID_VALUE);
        else
        {
            size_t i{0};
            values[i++] = ALC_FREQUENCY;
            values[i++] = dev->Frequency;

            if(dev->Type != DeviceType::Loopback)
            {
                values[i++] = ALC_REFRESH;
                values[i++] = dev->Frequency / dev->UpdateSize;

                values[i++] = ALC_SYNC;
                values[i++] = ALC_FALSE;
            }
            else
            {
                values[i++] = ALC_FORMAT_CHANNELS_SOFT;
                values[i++] = EnumFromDevFmt(dev->FmtChans);

                values[i++] = ALC_FORMAT_TYPE_SOFT;
                values[i++] = EnumFromDevFmt(dev->FmtType);

                if(dev->FmtChans == DevFmtAmbi3D)
                {
                    values[i++] = ALC_AMBISONIC_LAYOUT_SOFT;
                    values[i++] = EnumFromDevAmbi(dev->mAmbiLayout);

                    values[i++] = ALC_AMBISONIC_SCALING_SOFT;
                    values[i++] = EnumFromDevAmbi(dev->mAmbiScale);

                    values[i++] = ALC_AMBISONIC_ORDER_SOFT;
                    values[i++] = dev->mAmbiOrder;
                }
            }

            values[i++] = ALC_MONO_SOURCES;
            values[i++] = dev->NumMonoSources;

            values[i++] = ALC_STEREO_SOURCES;
            values[i++] = dev->NumStereoSources;

            values[i++] = ALC_MAX_AUXILIARY_SENDS;
            values[i++] = dev->NumAuxSends;

            values[i++] = ALC_HRTF_SOFT;
            values[i++] = (dev->mHrtf ? ALC_TRUE : ALC_FALSE);

            values[i++] = ALC_HRTF_STATUS_SOFT;
            values[i++] = dev->mHrtfStatus;

            values[i++] = ALC_OUTPUT_LIMITER_SOFT;
            values[i++] = dev->Limiter ? ALC_TRUE : ALC_FALSE;

            ClockLatency clock{GetClockLatency(dev.get(), dev->Backend.get())};
            values[i++] = ALC_DEVICE_CLOCK_SOFT;
            values[i++] = clock.ClockTime.count();

            values[i++] = ALC_DEVICE_LATENCY_SOFT;
            values[i++] = clock.Latency.count();

            values[i++] = ALC_OUTPUT_MODE_SOFT;
            values[i++] = static_cast<ALCenum>(device->getOutputMode1());

            values[i++] = 0;
        }
        break;

    case ALC_DEVICE_CLOCK_SOFT:
        {
            uint samplecount, refcount;
            nanoseconds basecount;
            do {
                refcount = dev->waitForMix();
                basecount = dev->ClockBase;
                samplecount = dev->SamplesDone;
            } while(refcount != ReadRef(dev->MixCount));
            basecount += nanoseconds{seconds{samplecount}} / dev->Frequency;
            *values = basecount.count();
        }
        break;

    case ALC_DEVICE_LATENCY_SOFT:
        *values = GetClockLatency(dev.get(), dev->Backend.get()).Latency.count();
        break;

    case ALC_DEVICE_CLOCK_LATENCY_SOFT:
        if(size < 2)
            alcSetError(dev.get(), ALC_INVALID_VALUE);
        else
        {
            ClockLatency clock{GetClockLatency(dev.get(), dev->Backend.get())};
            values[0] = clock.ClockTime.count();
            values[1] = clock.Latency.count();
        }
        break;

    default:
        auto ivals = al::vector<int>(static_cast<uint>(size));
        if(size_t got{GetIntegerv(dev.get(), pname, ivals)})
            std::copy_n(ivals.begin(), got, values);
        break;
    }
}
END_API_FUNC


ALC_API ALCboolean ALC_APIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCchar *extName)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!extName)
        alcSetError(dev.get(), ALC_INVALID_VALUE);
    else
    {
        size_t len = strlen(extName);
        const char *ptr = (dev ? alcExtensionList : alcNoDeviceExtList);
        while(ptr && *ptr)
        {
            if(al::strncasecmp(ptr, extName, len) == 0 && (ptr[len] == '\0' || isspace(ptr[len])))
                return ALC_TRUE;

            if((ptr=strchr(ptr, ' ')) != nullptr)
            {
                do {
                    ++ptr;
                } while(isspace(*ptr));
            }
        }
    }
    return ALC_FALSE;
}
END_API_FUNC


ALC_API ALCvoid* ALC_APIENTRY alcGetProcAddress(ALCdevice *device, const ALCchar *funcName)
START_API_FUNC
{
    if(!funcName)
    {
        DeviceRef dev{VerifyDevice(device)};
        alcSetError(dev.get(), ALC_INVALID_VALUE);
        return nullptr;
    }
#ifdef ALSOFT_EAX
    if(eax_g_is_enabled)
    {
        for(const auto &func : eaxFunctions)
        {
            if(strcmp(func.funcName, funcName) == 0)
                return func.address;
        }
    }
#endif
    for(const auto &func : alcFunctions)
    {
        if(strcmp(func.funcName, funcName) == 0)
            return func.address;
    }
    return nullptr;
}
END_API_FUNC


ALC_API ALCenum ALC_APIENTRY alcGetEnumValue(ALCdevice *device, const ALCchar *enumName)
START_API_FUNC
{
    if(!enumName)
    {
        DeviceRef dev{VerifyDevice(device)};
        alcSetError(dev.get(), ALC_INVALID_VALUE);
        return 0;
    }
#ifdef ALSOFT_EAX
    if(eax_g_is_enabled)
    {
        for(const auto &enm : eaxEnumerations)
        {
            if(strcmp(enm.enumName, enumName) == 0)
                return enm.value;
        }
    }
#endif
    for(const auto &enm : alcEnumerations)
    {
        if(strcmp(enm.enumName, enumName) == 0)
            return enm.value;
    }

    return 0;
}
END_API_FUNC


ALC_API ALCcontext* ALC_APIENTRY alcCreateContext(ALCdevice *device, const ALCint *attrList)
START_API_FUNC
{
    /* Explicitly hold the list lock while taking the StateLock in case the
     * device is asynchronously destroyed, to ensure this new context is
     * properly cleaned up after being made.
     */
    std::unique_lock<std::recursive_mutex> listlock{ListLock};
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type == DeviceType::Capture || !dev->Connected.load(std::memory_order_relaxed))
    {
        listlock.unlock();
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return nullptr;
    }
    std::unique_lock<std::mutex> statelock{dev->StateLock};
    listlock.unlock();

    dev->LastError.store(ALC_NO_ERROR);

    ALCenum err{UpdateDeviceParams(dev.get(), attrList)};
    if(err != ALC_NO_ERROR)
    {
        alcSetError(dev.get(), err);
        return nullptr;
    }

    ContextRef context{new ALCcontext{dev}};
    context->init();

    if(auto volopt = dev->configValue<float>(nullptr, "volume-adjust"))
    {
        const float valf{*volopt};
        if(!std::isfinite(valf))
            ERR("volume-adjust must be finite: %f\n", valf);
        else
        {
            const float db{clampf(valf, -24.0f, 24.0f)};
            if(db != valf)
                WARN("volume-adjust clamped: %f, range: +/-%f\n", valf, 24.0f);
            context->mGainBoost = std::pow(10.0f, db/20.0f);
            TRACE("volume-adjust gain: %f\n", context->mGainBoost);
        }
    }

    {
        using ContextArray = al::FlexArray<ContextBase*>;

        /* Allocate a new context array, which holds 1 more than the current/
         * old array.
         */
        auto *oldarray = device->mContexts.load();
        const size_t newcount{oldarray->size()+1};
        std::unique_ptr<ContextArray> newarray{ContextArray::Create(newcount)};

        /* Copy the current/old context handles to the new array, appending the
         * new context.
         */
        auto iter = std::copy(oldarray->begin(), oldarray->end(), newarray->begin());
        *iter = context.get();

        /* Store the new context array in the device. Wait for any current mix
         * to finish before deleting the old array.
         */
        dev->mContexts.store(newarray.release());
        if(oldarray != &DeviceBase::sEmptyContextArray)
        {
            dev->waitForMix();
            delete oldarray;
        }
    }
    statelock.unlock();

    {
        std::lock_guard<std::recursive_mutex> _{ListLock};
        auto iter = std::lower_bound(ContextList.cbegin(), ContextList.cend(), context.get());
        ContextList.emplace(iter, context.get());
    }

    if(ALeffectslot *slot{context->mDefaultSlot.get()})
    {
        ALenum sloterr{slot->initEffect(ALCcontext::sDefaultEffect.type,
            ALCcontext::sDefaultEffect.Props, context.get())};
        if(sloterr == AL_NO_ERROR)
            slot->updateProps(context.get());
        else
            ERR("Failed to initialize the default effect\n");
    }

    TRACE("Created context %p\n", voidp{context.get()});
    return context.release();
}
END_API_FUNC

ALC_API void ALC_APIENTRY alcDestroyContext(ALCcontext *context)
START_API_FUNC
{
    std::unique_lock<std::recursive_mutex> listlock{ListLock};
    auto iter = std::lower_bound(ContextList.begin(), ContextList.end(), context);
    if(iter == ContextList.end() || *iter != context)
    {
        listlock.unlock();
        alcSetError(nullptr, ALC_INVALID_CONTEXT);
        return;
    }

    /* Hold a reference to this context so it remains valid until the ListLock
     * is released.
     */
    ContextRef ctx{*iter};
    ContextList.erase(iter);

    ALCdevice *Device{ctx->mALDevice.get()};

    std::lock_guard<std::mutex> _{Device->StateLock};
    if(!ctx->deinit() && Device->Flags.test(DeviceRunning))
    {
        Device->Backend->stop();
        Device->Flags.reset(DeviceRunning);
    }
}
END_API_FUNC


ALC_API ALCcontext* ALC_APIENTRY alcGetCurrentContext(void)
START_API_FUNC
{
    ALCcontext *Context{ALCcontext::getThreadContext()};
    if(!Context) Context = ALCcontext::sGlobalContext.load();
    return Context;
}
END_API_FUNC

/** Returns the currently active thread-local context. */
ALC_API ALCcontext* ALC_APIENTRY alcGetThreadContext(void)
START_API_FUNC
{ return ALCcontext::getThreadContext(); }
END_API_FUNC

ALC_API ALCboolean ALC_APIENTRY alcMakeContextCurrent(ALCcontext *context)
START_API_FUNC
{
    /* context must be valid or nullptr */
    ContextRef ctx;
    if(context)
    {
        ctx = VerifyContext(context);
        if(!ctx)
        {
            alcSetError(nullptr, ALC_INVALID_CONTEXT);
            return ALC_FALSE;
        }
    }
    /* Release this reference (if any) to store it in the GlobalContext
     * pointer. Take ownership of the reference (if any) that was previously
     * stored there.
     */
    ctx = ContextRef{ALCcontext::sGlobalContext.exchange(ctx.release())};

    /* Reset (decrement) the previous global reference by replacing it with the
     * thread-local context. Take ownership of the thread-local context
     * reference (if any), clearing the storage to null.
     */
    ctx = ContextRef{ALCcontext::getThreadContext()};
    if(ctx) ALCcontext::setThreadContext(nullptr);
    /* Reset (decrement) the previous thread-local reference. */

    return ALC_TRUE;
}
END_API_FUNC

/** Makes the given context the active context for the current thread. */
ALC_API ALCboolean ALC_APIENTRY alcSetThreadContext(ALCcontext *context)
START_API_FUNC
{
    /* context must be valid or nullptr */
    ContextRef ctx;
    if(context)
    {
        ctx = VerifyContext(context);
        if(!ctx)
        {
            alcSetError(nullptr, ALC_INVALID_CONTEXT);
            return ALC_FALSE;
        }
    }
    /* context's reference count is already incremented */
    ContextRef old{ALCcontext::getThreadContext()};
    ALCcontext::setThreadContext(ctx.release());

    return ALC_TRUE;
}
END_API_FUNC


ALC_API ALCdevice* ALC_APIENTRY alcGetContextsDevice(ALCcontext *Context)
START_API_FUNC
{
    ContextRef ctx{VerifyContext(Context)};
    if(!ctx)
    {
        alcSetError(nullptr, ALC_INVALID_CONTEXT);
        return nullptr;
    }
    return ctx->mALDevice.get();
}
END_API_FUNC


ALC_API ALCdevice* ALC_APIENTRY alcOpenDevice(const ALCchar *deviceName)
START_API_FUNC
{
    DO_INITCONFIG();

    if(!PlaybackFactory)
    {
        alcSetError(nullptr, ALC_INVALID_VALUE);
        return nullptr;
    }

    if(deviceName)
    {
        if(!deviceName[0] || al::strcasecmp(deviceName, alcDefaultName) == 0
#ifdef _WIN32
            /* Some old Windows apps hardcode these expecting OpenAL to use a
             * specific audio API, even when they're not enumerated. Creative's
             * router effectively ignores them too.
             */
            || al::strcasecmp(deviceName, "DirectSound3D") == 0
            || al::strcasecmp(deviceName, "DirectSound") == 0
            || al::strcasecmp(deviceName, "MMSYSTEM") == 0
#endif
            /* Some old Linux apps hardcode configuration strings that were
             * supported by the OpenAL SI. We can't really do anything useful
             * with them, so just ignore.
             */
            || (deviceName[0] == '\'' && deviceName[1] == '(')
            || al::strcasecmp(deviceName, "openal-soft") == 0)
            deviceName = nullptr;
    }

    DeviceRef device{new ALCdevice{DeviceType::Playback}};

    /* Set output format */
    device->FmtChans = DevFmtChannelsDefault;
    device->FmtType = DevFmtTypeDefault;
    device->Frequency = DEFAULT_OUTPUT_RATE;
    device->UpdateSize = DEFAULT_UPDATE_SIZE;
    device->BufferSize = DEFAULT_UPDATE_SIZE * DEFAULT_NUM_UPDATES;

    device->SourcesMax = 256;
    device->AuxiliaryEffectSlotMax = 64;
    device->NumAuxSends = DEFAULT_SENDS;
#ifdef ALSOFT_EAX
    if(eax_g_is_enabled)
        device->NumAuxSends = EAX_MAX_FXSLOTS;
#endif // ALSOFT_EAX

    try {
        auto backend = PlaybackFactory->createBackend(device.get(), BackendType::Playback);
        std::lock_guard<std::recursive_mutex> _{ListLock};
        backend->open(deviceName);
        device->Backend = std::move(backend);
    }
    catch(al::backend_exception &e) {
        WARN("Failed to open playback device: %s\n", e.what());
        alcSetError(nullptr, (e.errorCode() == al::backend_error::OutOfMemory)
            ? ALC_OUT_OF_MEMORY : ALC_INVALID_VALUE);
        return nullptr;
    }

    if(uint freq{device->configValue<uint>(nullptr, "frequency").value_or(0u)})
    {
        if(freq < MIN_OUTPUT_RATE || freq > MAX_OUTPUT_RATE)
        {
            const uint newfreq{clampu(freq, MIN_OUTPUT_RATE, MAX_OUTPUT_RATE)};
            ERR("%uhz request clamped to %uhz\n", freq, newfreq);
            freq = newfreq;
        }
        const double scale{static_cast<double>(freq) / device->Frequency};
        device->UpdateSize = static_cast<uint>(device->UpdateSize*scale + 0.5);
        device->BufferSize = static_cast<uint>(device->BufferSize*scale + 0.5);
        device->Frequency = freq;
        device->Flags.set(FrequencyRequest);
    }

    if(auto srcsmax = device->configValue<uint>(nullptr, "sources").value_or(0))
        device->SourcesMax = srcsmax;

    if(auto slotsmax = device->configValue<uint>(nullptr, "slots").value_or(0))
        device->AuxiliaryEffectSlotMax = minu(slotsmax, INT_MAX);

    if(auto sendsopt = device->configValue<int>(nullptr, "sends"))
        device->NumAuxSends = minu(DEFAULT_SENDS,
            static_cast<uint>(clampi(*sendsopt, 0, MAX_SENDS)));

    device->NumStereoSources = 1;
    device->NumMonoSources = device->SourcesMax - device->NumStereoSources;

    {
        std::lock_guard<std::recursive_mutex> _{ListLock};
        auto iter = std::lower_bound(DeviceList.cbegin(), DeviceList.cend(), device.get());
        DeviceList.emplace(iter, device.get());
    }

    TRACE("Created device %p, \"%s\"\n", voidp{device.get()}, device->DeviceName.c_str());
    return device.release();
}
END_API_FUNC

ALC_API ALCboolean ALC_APIENTRY alcCloseDevice(ALCdevice *device)
START_API_FUNC
{
    std::unique_lock<std::recursive_mutex> listlock{ListLock};
    auto iter = std::lower_bound(DeviceList.begin(), DeviceList.end(), device);
    if(iter == DeviceList.end() || *iter != device)
    {
        alcSetError(nullptr, ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }
    if((*iter)->Type == DeviceType::Capture)
    {
        alcSetError(*iter, ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }

    /* Erase the device, and any remaining contexts left on it, from their
     * respective lists.
     */
    DeviceRef dev{*iter};
    DeviceList.erase(iter);

    std::unique_lock<std::mutex> statelock{dev->StateLock};
    al::vector<ContextRef> orphanctxs;
    for(ContextBase *ctx : *dev->mContexts.load())
    {
        auto ctxiter = std::lower_bound(ContextList.begin(), ContextList.end(), ctx);
        if(ctxiter != ContextList.end() && *ctxiter == ctx)
        {
            orphanctxs.emplace_back(ContextRef{*ctxiter});
            ContextList.erase(ctxiter);
        }
    }
    listlock.unlock();

    for(ContextRef &context : orphanctxs)
    {
        WARN("Releasing orphaned context %p\n", voidp{context.get()});
        context->deinit();
    }
    orphanctxs.clear();

    if(dev->Flags.test(DeviceRunning))
        dev->Backend->stop();
    dev->Flags.reset(DeviceRunning);

    return ALC_TRUE;
}
END_API_FUNC


/************************************************
 * ALC capture functions
 ************************************************/
ALC_API ALCdevice* ALC_APIENTRY alcCaptureOpenDevice(const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei samples)
START_API_FUNC
{
    DO_INITCONFIG();

    if(!CaptureFactory)
    {
        alcSetError(nullptr, ALC_INVALID_VALUE);
        return nullptr;
    }

    if(samples <= 0)
    {
        alcSetError(nullptr, ALC_INVALID_VALUE);
        return nullptr;
    }

    if(deviceName)
    {
        if(!deviceName[0] || al::strcasecmp(deviceName, alcDefaultName) == 0
            || al::strcasecmp(deviceName, "openal-soft") == 0)
            deviceName = nullptr;
    }

    DeviceRef device{new ALCdevice{DeviceType::Capture}};

    auto decompfmt = DecomposeDevFormat(format);
    if(!decompfmt)
    {
        alcSetError(nullptr, ALC_INVALID_ENUM);
        return nullptr;
    }

    device->Frequency = frequency;
    device->FmtChans = decompfmt->chans;
    device->FmtType = decompfmt->type;
    device->Flags.set(FrequencyRequest);
    device->Flags.set(ChannelsRequest);
    device->Flags.set(SampleTypeRequest);

    device->UpdateSize = static_cast<uint>(samples);
    device->BufferSize = static_cast<uint>(samples);

    try {
        TRACE("Capture format: %s, %s, %uhz, %u / %u buffer\n",
            DevFmtChannelsString(device->FmtChans), DevFmtTypeString(device->FmtType),
            device->Frequency, device->UpdateSize, device->BufferSize);

        auto backend = CaptureFactory->createBackend(device.get(), BackendType::Capture);
        std::lock_guard<std::recursive_mutex> _{ListLock};
        backend->open(deviceName);
        device->Backend = std::move(backend);
    }
    catch(al::backend_exception &e) {
        WARN("Failed to open capture device: %s\n", e.what());
        alcSetError(nullptr, (e.errorCode() == al::backend_error::OutOfMemory)
            ? ALC_OUT_OF_MEMORY : ALC_INVALID_VALUE);
        return nullptr;
    }

    {
        std::lock_guard<std::recursive_mutex> _{ListLock};
        auto iter = std::lower_bound(DeviceList.cbegin(), DeviceList.cend(), device.get());
        DeviceList.emplace(iter, device.get());
    }

    TRACE("Created capture device %p, \"%s\"\n", voidp{device.get()}, device->DeviceName.c_str());
    return device.release();
}
END_API_FUNC

ALC_API ALCboolean ALC_APIENTRY alcCaptureCloseDevice(ALCdevice *device)
START_API_FUNC
{
    std::unique_lock<std::recursive_mutex> listlock{ListLock};
    auto iter = std::lower_bound(DeviceList.begin(), DeviceList.end(), device);
    if(iter == DeviceList.end() || *iter != device)
    {
        alcSetError(nullptr, ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }
    if((*iter)->Type != DeviceType::Capture)
    {
        alcSetError(*iter, ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }

    DeviceRef dev{*iter};
    DeviceList.erase(iter);
    listlock.unlock();

    std::lock_guard<std::mutex> _{dev->StateLock};
    if(dev->Flags.test(DeviceRunning))
        dev->Backend->stop();
    dev->Flags.reset(DeviceRunning);

    return ALC_TRUE;
}
END_API_FUNC

ALC_API void ALC_APIENTRY alcCaptureStart(ALCdevice *device)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Capture)
    {
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return;
    }

    std::lock_guard<std::mutex> _{dev->StateLock};
    if(!dev->Connected.load(std::memory_order_acquire))
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
    else if(!dev->Flags.test(DeviceRunning))
    {
        try {
            auto backend = dev->Backend.get();
            backend->start();
            dev->Flags.set(DeviceRunning);
        }
        catch(al::backend_exception& e) {
            ERR("%s\n", e.what());
            dev->handleDisconnect("%s", e.what());
            alcSetError(dev.get(), ALC_INVALID_DEVICE);
        }
    }
}
END_API_FUNC

ALC_API void ALC_APIENTRY alcCaptureStop(ALCdevice *device)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Capture)
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
    else
    {
        std::lock_guard<std::mutex> _{dev->StateLock};
        if(dev->Flags.test(DeviceRunning))
            dev->Backend->stop();
        dev->Flags.reset(DeviceRunning);
    }
}
END_API_FUNC

ALC_API void ALC_APIENTRY alcCaptureSamples(ALCdevice *device, ALCvoid *buffer, ALCsizei samples)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Capture)
    {
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return;
    }

    if(samples < 0 || (samples > 0 && buffer == nullptr))
    {
        alcSetError(dev.get(), ALC_INVALID_VALUE);
        return;
    }
    if(samples < 1)
        return;

    std::lock_guard<std::mutex> _{dev->StateLock};
    BackendBase *backend{dev->Backend.get()};

    const auto usamples = static_cast<uint>(samples);
    if(usamples > backend->availableSamples())
    {
        alcSetError(dev.get(), ALC_INVALID_VALUE);
        return;
    }

    backend->captureSamples(static_cast<al::byte*>(buffer), usamples);
}
END_API_FUNC


/************************************************
 * ALC loopback functions
 ************************************************/

/** Open a loopback device, for manual rendering. */
ALC_API ALCdevice* ALC_APIENTRY alcLoopbackOpenDeviceSOFT(const ALCchar *deviceName)
START_API_FUNC
{
    DO_INITCONFIG();

    /* Make sure the device name, if specified, is us. */
    if(deviceName && strcmp(deviceName, alcDefaultName) != 0)
    {
        alcSetError(nullptr, ALC_INVALID_VALUE);
        return nullptr;
    }

    DeviceRef device{new ALCdevice{DeviceType::Loopback}};

    device->SourcesMax = 256;
    device->AuxiliaryEffectSlotMax = 64;
    device->NumAuxSends = DEFAULT_SENDS;

    //Set output format
    device->BufferSize = 0;
    device->UpdateSize = 0;

    device->Frequency = DEFAULT_OUTPUT_RATE;
    device->FmtChans = DevFmtChannelsDefault;
    device->FmtType = DevFmtTypeDefault;

    if(auto srcsmax = ConfigValueUInt(nullptr, nullptr, "sources").value_or(0))
        device->SourcesMax = srcsmax;

    if(auto slotsmax = ConfigValueUInt(nullptr, nullptr, "slots").value_or(0))
        device->AuxiliaryEffectSlotMax = minu(slotsmax, INT_MAX);

    if(auto sendsopt = ConfigValueInt(nullptr, nullptr, "sends"))
        device->NumAuxSends = minu(DEFAULT_SENDS,
            static_cast<uint>(clampi(*sendsopt, 0, MAX_SENDS)));

    device->NumStereoSources = 1;
    device->NumMonoSources = device->SourcesMax - device->NumStereoSources;

    try {
        auto backend = LoopbackBackendFactory::getFactory().createBackend(device.get(),
            BackendType::Playback);
        backend->open("Loopback");
        device->Backend = std::move(backend);
    }
    catch(al::backend_exception &e) {
        WARN("Failed to open loopback device: %s\n", e.what());
        alcSetError(nullptr, (e.errorCode() == al::backend_error::OutOfMemory)
            ? ALC_OUT_OF_MEMORY : ALC_INVALID_VALUE);
        return nullptr;
    }

    {
        std::lock_guard<std::recursive_mutex> _{ListLock};
        auto iter = std::lower_bound(DeviceList.cbegin(), DeviceList.cend(), device.get());
        DeviceList.emplace(iter, device.get());
    }

    TRACE("Created loopback device %p\n", voidp{device.get()});
    return device.release();
}
END_API_FUNC

/**
 * Determines if the loopback device supports the given format for rendering.
 */
ALC_API ALCboolean ALC_APIENTRY alcIsRenderFormatSupportedSOFT(ALCdevice *device, ALCsizei freq, ALCenum channels, ALCenum type)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Loopback)
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
    else if(freq <= 0)
        alcSetError(dev.get(), ALC_INVALID_VALUE);
    else
    {
        if(DevFmtTypeFromEnum(type).has_value() && DevFmtChannelsFromEnum(channels).has_value()
            && freq >= MIN_OUTPUT_RATE && freq <= MAX_OUTPUT_RATE)
            return ALC_TRUE;
    }

    return ALC_FALSE;
}
END_API_FUNC

/**
 * Renders some samples into a buffer, using the format last set by the
 * attributes given to alcCreateContext.
 */
FORCE_ALIGN ALC_API void ALC_APIENTRY alcRenderSamplesSOFT(ALCdevice *device, ALCvoid *buffer, ALCsizei samples)
START_API_FUNC
{
    if(!device || device->Type != DeviceType::Loopback)
        alcSetError(device, ALC_INVALID_DEVICE);
    else if(samples < 0 || (samples > 0 && buffer == nullptr))
        alcSetError(device, ALC_INVALID_VALUE);
    else
        device->renderSamples(buffer, static_cast<uint>(samples), device->channelsFromFmt());
}
END_API_FUNC


/************************************************
 * ALC DSP pause/resume functions
 ************************************************/

/** Pause the DSP to stop audio processing. */
ALC_API void ALC_APIENTRY alcDevicePauseSOFT(ALCdevice *device)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Playback)
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
    else
    {
        std::lock_guard<std::mutex> _{dev->StateLock};
        if(dev->Flags.test(DeviceRunning))
            dev->Backend->stop();
        dev->Flags.reset(DeviceRunning);
        dev->Flags.set(DevicePaused);
    }
}
END_API_FUNC

/** Resume the DSP to restart audio processing. */
ALC_API void ALC_APIENTRY alcDeviceResumeSOFT(ALCdevice *device)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Playback)
    {
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return;
    }

    std::lock_guard<std::mutex> _{dev->StateLock};
    if(!dev->Flags.test(DevicePaused))
        return;
    dev->Flags.reset(DevicePaused);
    if(dev->mContexts.load()->empty())
        return;

    try {
        auto backend = dev->Backend.get();
        backend->start();
        dev->Flags.set(DeviceRunning);
    }
    catch(al::backend_exception& e) {
        ERR("%s\n", e.what());
        dev->handleDisconnect("%s", e.what());
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return;
    }
    TRACE("Post-resume: %s, %s, %uhz, %u / %u buffer\n",
        DevFmtChannelsString(device->FmtChans), DevFmtTypeString(device->FmtType),
        device->Frequency, device->UpdateSize, device->BufferSize);
}
END_API_FUNC


/************************************************
 * ALC HRTF functions
 ************************************************/

/** Gets a string parameter at the given index. */
ALC_API const ALCchar* ALC_APIENTRY alcGetStringiSOFT(ALCdevice *device, ALCenum paramName, ALCsizei index)
START_API_FUNC
{
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type == DeviceType::Capture)
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
    else switch(paramName)
    {
        case ALC_HRTF_SPECIFIER_SOFT:
            if(index >= 0 && static_cast<uint>(index) < dev->mHrtfList.size())
                return dev->mHrtfList[static_cast<uint>(index)].c_str();
            alcSetError(dev.get(), ALC_INVALID_VALUE);
            break;

        default:
            alcSetError(dev.get(), ALC_INVALID_ENUM);
            break;
    }

    return nullptr;
}
END_API_FUNC

/** Resets the given device output, using the specified attribute list. */
ALC_API ALCboolean ALC_APIENTRY alcResetDeviceSOFT(ALCdevice *device, const ALCint *attribs)
START_API_FUNC
{
    std::unique_lock<std::recursive_mutex> listlock{ListLock};
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type == DeviceType::Capture)
    {
        listlock.unlock();
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }
    std::lock_guard<std::mutex> _{dev->StateLock};
    listlock.unlock();

    /* Force the backend to stop mixing first since we're resetting. Also reset
     * the connected state so lost devices can attempt recover.
     */
    if(dev->Flags.test(DeviceRunning))
        dev->Backend->stop();
    dev->Flags.reset(DeviceRunning);

    return ResetDeviceParams(dev.get(), attribs) ? ALC_TRUE : ALC_FALSE;
}
END_API_FUNC


/************************************************
 * ALC device reopen functions
 ************************************************/

/** Reopens the given device output, using the specified name and attribute list. */
FORCE_ALIGN ALCboolean ALC_APIENTRY alcReopenDeviceSOFT(ALCdevice *device,
    const ALCchar *deviceName, const ALCint *attribs)
START_API_FUNC
{
    if(deviceName)
    {
        if(!deviceName[0] || al::strcasecmp(deviceName, alcDefaultName) == 0)
            deviceName = nullptr;
    }

    std::unique_lock<std::recursive_mutex> listlock{ListLock};
    DeviceRef dev{VerifyDevice(device)};
    if(!dev || dev->Type != DeviceType::Playback)
    {
        listlock.unlock();
        alcSetError(dev.get(), ALC_INVALID_DEVICE);
        return ALC_FALSE;
    }
    std::lock_guard<std::mutex> _{dev->StateLock};

    /* Force the backend to stop mixing first since we're reopening. */
    if(dev->Flags.test(DeviceRunning))
    {
        auto backend = dev->Backend.get();
        backend->stop();
        dev->Flags.reset(DeviceRunning);
    }

    BackendPtr newbackend;
    try {
        newbackend = PlaybackFactory->createBackend(dev.get(), BackendType::Playback);
        newbackend->open(deviceName);
    }
    catch(al::backend_exception &e) {
        listlock.unlock();
        newbackend = nullptr;

        WARN("Failed to reopen playback device: %s\n", e.what());
        alcSetError(dev.get(), (e.errorCode() == al::backend_error::OutOfMemory)
            ? ALC_OUT_OF_MEMORY : ALC_INVALID_VALUE);

        /* If the device is connected, not paused, and has contexts, ensure it
         * continues playing.
         */
        if(dev->Connected.load(std::memory_order_relaxed) && !dev->Flags.test(DevicePaused)
            && !dev->mContexts.load(std::memory_order_relaxed)->empty())
        {
            try {
                auto backend = dev->Backend.get();
                backend->start();
                dev->Flags.set(DeviceRunning);
            }
            catch(al::backend_exception &be) {
                ERR("%s\n", be.what());
                dev->handleDisconnect("%s", be.what());
            }
        }
        return ALC_FALSE;
    }
    listlock.unlock();
    dev->Backend = std::move(newbackend);
    TRACE("Reopened device %p, \"%s\"\n", voidp{dev.get()}, dev->DeviceName.c_str());

    /* Always return true even if resetting fails. It shouldn't fail, but this
     * is primarily to avoid confusion by the app seeing the function return
     * false while the device is on the new output anyway. We could try to
     * restore the old backend if this fails, but the configuration would be
     * changed with the new backend and would need to be reset again with the
     * old one, and the provided attributes may not be appropriate or desirable
     * for the old device.
     *
     * In this way, we essentially act as if the function succeeded, but
     * immediately disconnects following it.
     */
    ResetDeviceParams(dev.get(), attribs);
    return ALC_TRUE;
}
END_API_FUNC
