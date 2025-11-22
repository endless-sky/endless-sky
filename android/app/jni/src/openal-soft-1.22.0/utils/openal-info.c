/*
 * OpenAL Info Utility
 *
 * Copyright (c) 2010 by Chris Robinson <chris.kcat@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AL/alc.h"
#include "AL/al.h"
#include "AL/alext.h"

#include "win_main_utf8.h"

/* C doesn't allow casting between function and non-function pointer types, so
 * with C99 we need to use a union to reinterpret the pointer type. Pre-C99
 * still needs to use a normal cast and live with the warning (C++ is fine with
 * a regular reinterpret_cast).
 */
#if __STDC_VERSION__ >= 199901L
#define FUNCTION_CAST(T, ptr) (union{void *p; T f;}){ptr}.f
#else
#define FUNCTION_CAST(T, ptr) (T)(ptr)
#endif

#define MAX_WIDTH  80

static void printList(const char *list, char separator)
{
    size_t col = MAX_WIDTH, len;
    const char *indent = "    ";
    const char *next;

    if(!list || *list == '\0')
    {
        fprintf(stdout, "\n%s!!! none !!!\n", indent);
        return;
    }

    do {
        next = strchr(list, separator);
        if(next)
        {
            len = (size_t)(next-list);
            do {
                next++;
            } while(*next == separator);
        }
        else
            len = strlen(list);

        if(len + col + 2 >= MAX_WIDTH)
        {
            fprintf(stdout, "\n%s", indent);
            col = strlen(indent);
        }
        else
        {
            fputc(' ', stdout);
            col++;
        }

        len = fwrite(list, 1, len, stdout);
        col += len;

        if(!next || *next == '\0')
            break;
        fputc(',', stdout);
        col++;

        list = next;
    } while(1);
    fputc('\n', stdout);
}

static void printDeviceList(const char *list)
{
    if(!list || *list == '\0')
        printf("    !!! none !!!\n");
    else do {
        printf("    %s\n", list);
        list += strlen(list) + 1;
    } while(*list != '\0');
}


static ALenum checkALErrors(int linenum)
{
    ALenum err = alGetError();
    if(err != AL_NO_ERROR)
        printf("OpenAL Error: %s (0x%x), @ %d\n", alGetString(err), err, linenum);
    return err;
}
#define checkALErrors() checkALErrors(__LINE__)

static ALCenum checkALCErrors(ALCdevice *device, int linenum)
{
    ALCenum err = alcGetError(device);
    if(err != ALC_NO_ERROR)
        printf("ALC Error: %s (0x%x), @ %d\n", alcGetString(device, err), err, linenum);
    return err;
}
#define checkALCErrors(x) checkALCErrors((x),__LINE__)


static void printALCInfo(ALCdevice *device)
{
    ALCint major, minor;

    if(device)
    {
        const ALCchar *devname = NULL;
        printf("\n");
        if(alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE)
            devname = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
        if(checkALCErrors(device) != ALC_NO_ERROR || !devname)
            devname = alcGetString(device, ALC_DEVICE_SPECIFIER);
        printf("** Info for device \"%s\" **\n", devname);
    }
    alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &major);
    alcGetIntegerv(device, ALC_MINOR_VERSION, 1, &minor);
    if(checkALCErrors(device) == ALC_NO_ERROR)
        printf("ALC version: %d.%d\n", major, minor);
    if(device)
    {
        printf("ALC extensions:");
        printList(alcGetString(device, ALC_EXTENSIONS), ' ');
        checkALCErrors(device);
    }
}

static void printHRTFInfo(ALCdevice *device)
{
    LPALCGETSTRINGISOFT alcGetStringiSOFT;
    ALCint num_hrtfs;

    if(alcIsExtensionPresent(device, "ALC_SOFT_HRTF") == ALC_FALSE)
    {
        printf("HRTF extension not available\n");
        return;
    }

    alcGetStringiSOFT = FUNCTION_CAST(LPALCGETSTRINGISOFT,
        alcGetProcAddress(device, "alcGetStringiSOFT"));

    alcGetIntegerv(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtfs);
    if(!num_hrtfs)
        printf("No HRTFs found\n");
    else
    {
        ALCint i;
        printf("Available HRTFs:\n");
        for(i = 0;i < num_hrtfs;++i)
        {
            const ALCchar *name = alcGetStringiSOFT(device, ALC_HRTF_SPECIFIER_SOFT, i);
            printf("    %s\n", name);
        }
    }
    checkALCErrors(device);
}

static void printModeInfo(ALCdevice *device)
{
    if(alcIsExtensionPresent(device, "ALC_SOFT_output_mode"))
    {
        const char *modename = "(error)";
        ALCenum mode = 0;

        alcGetIntegerv(device, ALC_OUTPUT_MODE_SOFT, 1, &mode);
        checkALCErrors(device);
        switch(mode)
        {
        case ALC_ANY_SOFT: modename = "Unknown / unspecified"; break;
        case ALC_MONO_SOFT: modename = "Mono"; break;
        case ALC_STEREO_SOFT: modename = "Stereo (unspecified encoding)"; break;
        case ALC_STEREO_BASIC_SOFT: modename = "Stereo (basic)"; break;
        case ALC_STEREO_UHJ_SOFT: modename = "Stereo (UHJ)"; break;
        case ALC_STEREO_HRTF_SOFT: modename = "Stereo (HRTF)"; break;
        case ALC_QUAD_SOFT: modename = "Quadraphonic"; break;
        case ALC_SURROUND_5_1_SOFT: modename = "5.1 Surround"; break;
        case ALC_SURROUND_6_1_SOFT: modename = "6.1 Surround"; break;
        case ALC_SURROUND_7_1_SOFT: modename = "7.1 Surround"; break;
        }
        printf("Output channel mode: %s\n", modename);
    }
    else
        printf("Output mode extension not available\n");
}

static void printALInfo(void)
{
    printf("OpenAL vendor string: %s\n", alGetString(AL_VENDOR));
    printf("OpenAL renderer string: %s\n", alGetString(AL_RENDERER));
    printf("OpenAL version string: %s\n", alGetString(AL_VERSION));
    printf("OpenAL extensions:");
    printList(alGetString(AL_EXTENSIONS), ' ');
    checkALErrors();
}

static void printResamplerInfo(void)
{
    LPALGETSTRINGISOFT alGetStringiSOFT;
    ALint num_resamplers;
    ALint def_resampler;

    if(!alIsExtensionPresent("AL_SOFT_source_resampler"))
    {
        printf("Resampler info not available\n");
        return;
    }

    alGetStringiSOFT = FUNCTION_CAST(LPALGETSTRINGISOFT, alGetProcAddress("alGetStringiSOFT"));

    num_resamplers = alGetInteger(AL_NUM_RESAMPLERS_SOFT);
    def_resampler = alGetInteger(AL_DEFAULT_RESAMPLER_SOFT);

    if(!num_resamplers)
        printf("!!! No resamplers found !!!\n");
    else
    {
        ALint i;
        printf("Available resamplers:\n");
        for(i = 0;i < num_resamplers;++i)
        {
            const ALchar *name = alGetStringiSOFT(AL_RESAMPLER_NAME_SOFT, i);
            printf("    %s%s\n", name, (i==def_resampler)?" *":"");
        }
    }
    checkALErrors();
}

static void printEFXInfo(ALCdevice *device)
{
    static LPALGENFILTERS palGenFilters;
    static LPALDELETEFILTERS palDeleteFilters;
    static LPALFILTERI palFilteri;
    static LPALGENEFFECTS palGenEffects;
    static LPALDELETEEFFECTS palDeleteEffects;
    static LPALEFFECTI palEffecti;

    static const ALint filters[] = {
        AL_FILTER_LOWPASS, AL_FILTER_HIGHPASS, AL_FILTER_BANDPASS,
        AL_FILTER_NULL
    };
    char filterNames[] = "Low-pass,High-pass,Band-pass,";
    static const ALint effects[] = {
        AL_EFFECT_EAXREVERB, AL_EFFECT_REVERB, AL_EFFECT_CHORUS,
        AL_EFFECT_DISTORTION, AL_EFFECT_ECHO, AL_EFFECT_FLANGER,
        AL_EFFECT_FREQUENCY_SHIFTER, AL_EFFECT_VOCAL_MORPHER,
        AL_EFFECT_PITCH_SHIFTER, AL_EFFECT_RING_MODULATOR,
        AL_EFFECT_AUTOWAH, AL_EFFECT_COMPRESSOR, AL_EFFECT_EQUALIZER,
        AL_EFFECT_NULL
    };
    static const ALint dedeffects[] = {
        AL_EFFECT_DEDICATED_DIALOGUE, AL_EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT,
        AL_EFFECT_NULL
    };
    char effectNames[] = "EAX Reverb,Reverb,Chorus,Distortion,Echo,Flanger,"
        "Frequency Shifter,Vocal Morpher,Pitch Shifter,Ring Modulator,Autowah,"
        "Compressor,Equalizer,Dedicated Dialog,Dedicated LFE,";
    ALCint major, minor, sends;
    ALuint object;
    char *current;
    int i;

    if(alcIsExtensionPresent(device, "ALC_EXT_EFX") == AL_FALSE)
    {
        printf("EFX not available\n");
        return;
    }

    palGenFilters = FUNCTION_CAST(LPALGENFILTERS, alGetProcAddress("alGenFilters"));
    palDeleteFilters = FUNCTION_CAST(LPALDELETEFILTERS, alGetProcAddress("alDeleteFilters"));
    palFilteri = FUNCTION_CAST(LPALFILTERI, alGetProcAddress("alFilteri"));
    palGenEffects = FUNCTION_CAST(LPALGENEFFECTS, alGetProcAddress("alGenEffects"));
    palDeleteEffects = FUNCTION_CAST(LPALDELETEEFFECTS, alGetProcAddress("alDeleteEffects"));
    palEffecti = FUNCTION_CAST(LPALEFFECTI, alGetProcAddress("alEffecti"));

    alcGetIntegerv(device, ALC_EFX_MAJOR_VERSION, 1, &major);
    alcGetIntegerv(device, ALC_EFX_MINOR_VERSION, 1, &minor);
    if(checkALCErrors(device) == ALC_NO_ERROR)
        printf("EFX version: %d.%d\n", major, minor);
    alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, &sends);
    if(checkALCErrors(device) == ALC_NO_ERROR)
        printf("Max auxiliary sends: %d\n", sends);

    palGenFilters(1, &object);
    checkALErrors();

    current = filterNames;
    for(i = 0;filters[i] != AL_FILTER_NULL;i++)
    {
        char *next = strchr(current, ',');
        assert(next != NULL);

        palFilteri(object, AL_FILTER_TYPE, filters[i]);
        if(alGetError() != AL_NO_ERROR)
            memmove(current, next+1, strlen(next));
        else
            current = next+1;
    }
    printf("Supported filters:");
    printList(filterNames, ',');

    palDeleteFilters(1, &object);
    palGenEffects(1, &object);
    checkALErrors();

    current = effectNames;
    for(i = 0;effects[i] != AL_EFFECT_NULL;i++)
    {
        char *next = strchr(current, ',');
        assert(next != NULL);

        palEffecti(object, AL_EFFECT_TYPE, effects[i]);
        if(alGetError() != AL_NO_ERROR)
            memmove(current, next+1, strlen(next));
        else
            current = next+1;
    }
    if(alcIsExtensionPresent(device, "ALC_EXT_DEDICATED"))
    {
        for(i = 0;dedeffects[i] != AL_EFFECT_NULL;i++)
        {
            char *next = strchr(current, ',');
            assert(next != NULL);

            palEffecti(object, AL_EFFECT_TYPE, dedeffects[i]);
            if(alGetError() != AL_NO_ERROR)
                memmove(current, next+1, strlen(next));
            else
                current = next+1;
        }
    }
    else
    {
        for(i = 0;dedeffects[i] != AL_EFFECT_NULL;i++)
        {
            char *next = strchr(current, ',');
            assert(next != NULL);
            memmove(current, next+1, strlen(next));
        }
    }
    printf("Supported effects:");
    printList(effectNames, ',');

    palDeleteEffects(1, &object);
    checkALErrors();
}

int main(int argc, char *argv[])
{
    ALCdevice *device;
    ALCcontext *context;

#ifdef _WIN32
    /* OpenAL Soft gives UTF-8 strings, so set the console to expect that. */
    SetConsoleOutputCP(CP_UTF8);
#endif

    if(argc > 1 && (strcmp(argv[1], "--help") == 0 ||
                    strcmp(argv[1], "-h") == 0))
    {
        printf("Usage: %s [playback device]\n", argv[0]);
        return 0;
    }

    printf("Available playback devices:\n");
    if(alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE)
        printDeviceList(alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER));
    else
        printDeviceList(alcGetString(NULL, ALC_DEVICE_SPECIFIER));
    printf("Available capture devices:\n");
    printDeviceList(alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER));

    if(alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE)
        printf("Default playback device: %s\n",
               alcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER));
    else
        printf("Default playback device: %s\n",
               alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER));
    printf("Default capture device: %s\n",
           alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER));

    printALCInfo(NULL);

    device = alcOpenDevice((argc>1) ? argv[1] : NULL);
    if(!device)
    {
        printf("\n!!! Failed to open %s !!!\n\n", ((argc>1) ? argv[1] : "default device"));
        return 1;
    }
    printALCInfo(device);
    printHRTFInfo(device);

    context = alcCreateContext(device, NULL);
    if(!context || alcMakeContextCurrent(context) == ALC_FALSE)
    {
        if(context)
            alcDestroyContext(context);
        alcCloseDevice(device);
        printf("\n!!! Failed to set a context !!!\n\n");
        return 1;
    }

    printModeInfo(device);
    printALInfo();
    printResamplerInfo();
    printEFXInfo(device);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return 0;
}
