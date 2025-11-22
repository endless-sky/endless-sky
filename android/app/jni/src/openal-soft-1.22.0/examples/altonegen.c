/*
 * OpenAL Tone Generator Test
 *
 * Copyright (c) 2015 by Chris Robinson <chris.kcat@gmail.com>
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

/* This file contains a test for generating waveforms and plays them for a
 * given length of time. Intended to inspect the behavior of the mixer by
 * checking the output with a spectrum analyzer and oscilloscope.
 *
 * TODO: This would actually be nicer as a GUI app with buttons to start and
 * stop individual waveforms, include additional whitenoise and pinknoise
 * generators, and have the ability to hook up EFX filters and effects.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "common/alhelpers.h"

#include "win_main_utf8.h"

#ifndef M_PI
#define M_PI    (3.14159265358979323846)
#endif

enum WaveType {
    WT_Sine,
    WT_Square,
    WT_Sawtooth,
    WT_Triangle,
    WT_Impulse,
    WT_WhiteNoise,
};

static const char *GetWaveTypeName(enum WaveType type)
{
    switch(type)
    {
        case WT_Sine: return "sine";
        case WT_Square: return "square";
        case WT_Sawtooth: return "sawtooth";
        case WT_Triangle: return "triangle";
        case WT_Impulse: return "impulse";
        case WT_WhiteNoise: return "noise";
    }
    return "(unknown)";
}

static inline ALuint dither_rng(ALuint *seed)
{
    *seed = (*seed * 96314165) + 907633515;
    return *seed;
}

static void ApplySin(ALfloat *data, ALdouble g, ALuint srate, ALuint freq)
{
    ALdouble smps_per_cycle = (ALdouble)srate / freq;
    ALuint i;
    for(i = 0;i < srate;i++)
    {
        ALdouble ival;
        data[i] += (ALfloat)(sin(modf(i/smps_per_cycle, &ival) * 2.0*M_PI) * g);
    }
}

/* Generates waveforms using additive synthesis. Each waveform is constructed
 * by summing one or more sine waves, up to (and excluding) nyquist.
 */
static ALuint CreateWave(enum WaveType type, ALuint freq, ALuint srate, ALfloat gain)
{
    ALuint seed = 22222;
    ALuint data_size;
    ALfloat *data;
    ALuint buffer;
    ALenum err;
    ALuint i;

    data_size = (ALuint)(srate * sizeof(ALfloat));
    data = calloc(1, data_size);
    switch(type)
    {
        case WT_Sine:
            ApplySin(data, 1.0, srate, freq);
            break;
        case WT_Square:
            for(i = 1;freq*i < srate/2;i+=2)
                ApplySin(data, 4.0/M_PI * 1.0/i, srate, freq*i);
            break;
        case WT_Sawtooth:
            for(i = 1;freq*i < srate/2;i++)
                ApplySin(data, 2.0/M_PI * ((i&1)*2 - 1.0) / i, srate, freq*i);
            break;
        case WT_Triangle:
            for(i = 1;freq*i < srate/2;i+=2)
                ApplySin(data, 8.0/(M_PI*M_PI) * (1.0 - (i&2)) / (i*i), srate, freq*i);
            break;
        case WT_Impulse:
            /* NOTE: Impulse isn't handled using additive synthesis, and is
             * instead just a non-0 sample at a given rate. This can still be
             * useful to test (other than resampling, the ALSOFT_DEFAULT_REVERB
             * environment variable can prove useful here to test the reverb
             * response).
             */
            for(i = 0;i < srate;i++)
                data[i] = (i%(srate/freq)) ? 0.0f : 1.0f;
            break;
        case WT_WhiteNoise:
            /* NOTE: WhiteNoise is just uniform set of uncorrelated values, and
             * is not influenced by the waveform frequency.
             */
            for(i = 0;i < srate;i++)
            {
                ALuint rng0 = dither_rng(&seed);
                ALuint rng1 = dither_rng(&seed);
                data[i] = (ALfloat)(rng0*(1.0/UINT_MAX) - rng1*(1.0/UINT_MAX));
            }
            break;
    }

    if(gain != 1.0f)
    {
        for(i = 0;i < srate;i++)
            data[i] *= gain;
    }

    /* Buffer the audio data into a new buffer object. */
    buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, AL_FORMAT_MONO_FLOAT32, data, (ALsizei)data_size, (ALsizei)srate);
    free(data);

    /* Check if an error occured, and clean up if so. */
    err = alGetError();
    if(err != AL_NO_ERROR)
    {
        fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
        if(alIsBuffer(buffer))
            alDeleteBuffers(1, &buffer);
        return 0;
    }

    return buffer;
}


int main(int argc, char *argv[])
{
    enum WaveType wavetype = WT_Sine;
    const char *appname = argv[0];
    ALuint source, buffer;
    ALint last_pos, num_loops;
    ALint max_loops = 4;
    ALint srate = -1;
    ALint tone_freq = 1000;
    ALCint dev_rate;
    ALenum state;
    ALfloat gain = 1.0f;
    int i;

    argv++; argc--;
    if(InitAL(&argv, &argc) != 0)
        return 1;

    if(!alIsExtensionPresent("AL_EXT_FLOAT32"))
    {
        fprintf(stderr, "Required AL_EXT_FLOAT32 extension not supported on this device!\n");
        CloseAL();
        return 1;
    }

    for(i = 0;i < argc;i++)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-?") == 0
            || strcmp(argv[i], "--help") == 0)
        {
            fprintf(stderr, "OpenAL Tone Generator\n"
"\n"
"Usage: %s [-device <name>] <options>\n"
"\n"
"Available options:\n"
"  --help/-h                 This help text\n"
"  -t <seconds>              Time to play a tone (default 5 seconds)\n"
"  --waveform/-w <type>      Waveform type: sine (default), square, sawtooth,\n"
"                                triangle, impulse, noise\n"
"  --freq/-f <hz>            Tone frequency (default 1000 hz)\n"
"  --gain/-g <gain>          gain 0.0 to 1 (default 1)\n"
"  --srate/-s <sample rate>  Sampling rate (default output rate)\n",
                appname
            );
            CloseAL();
            return 1;
        }
        else if(i+1 < argc && strcmp(argv[i], "-t") == 0)
        {
            i++;
            max_loops = atoi(argv[i]) - 1;
        }
        else if(i+1 < argc && (strcmp(argv[i], "--waveform") == 0 || strcmp(argv[i], "-w") == 0))
        {
            i++;
            if(strcmp(argv[i], "sine") == 0)
                wavetype = WT_Sine;
            else if(strcmp(argv[i], "square") == 0)
                wavetype = WT_Square;
            else if(strcmp(argv[i], "sawtooth") == 0)
                wavetype = WT_Sawtooth;
            else if(strcmp(argv[i], "triangle") == 0)
                wavetype = WT_Triangle;
            else if(strcmp(argv[i], "impulse") == 0)
                wavetype = WT_Impulse;
            else if(strcmp(argv[i], "noise") == 0)
                wavetype = WT_WhiteNoise;
            else
                fprintf(stderr, "Unhandled waveform: %s\n", argv[i]);
        }
        else if(i+1 < argc && (strcmp(argv[i], "--freq") == 0 || strcmp(argv[i], "-f") == 0))
        {
            i++;
            tone_freq = atoi(argv[i]);
            if(tone_freq < 1)
            {
                fprintf(stderr, "Invalid tone frequency: %s (min: 1hz)\n", argv[i]);
                tone_freq = 1;
            }
        }
        else if(i+1 < argc && (strcmp(argv[i], "--gain") == 0 || strcmp(argv[i], "-g") == 0))
        {
            i++;
            gain = (ALfloat)atof(argv[i]);
            if(gain < 0.0f || gain > 1.0f)
            {
                fprintf(stderr, "Invalid gain: %s (min: 0.0, max 1.0)\n", argv[i]);
                gain = 1.0f;
            }
        }
        else if(i+1 < argc && (strcmp(argv[i], "--srate") == 0 || strcmp(argv[i], "-s") == 0))
        {
            i++;
            srate = atoi(argv[i]);
            if(srate < 40)
            {
                fprintf(stderr, "Invalid sample rate: %s (min: 40hz)\n", argv[i]);
                srate = 40;
            }
        }
    }

    {
        ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());
        alcGetIntegerv(device, ALC_FREQUENCY, 1, &dev_rate);
        assert(alcGetError(device)==ALC_NO_ERROR && "Failed to get device sample rate");
    }
    if(srate < 0)
        srate = dev_rate;

    /* Load the sound into a buffer. */
    buffer = CreateWave(wavetype, (ALuint)tone_freq, (ALuint)srate, gain);
    if(!buffer)
    {
        CloseAL();
        return 1;
    }

    printf("Playing %dhz %s-wave tone with %dhz sample rate and %dhz output, for %d second%s...\n",
           tone_freq, GetWaveTypeName(wavetype), srate, dev_rate, max_loops+1, max_loops?"s":"");
    fflush(stdout);

    /* Create the source to play the sound with. */
    source = 0;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, (ALint)buffer);
    assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");

    /* Play the sound for a while. */
    num_loops = 0;
    last_pos = 0;
    alSourcei(source, AL_LOOPING, (max_loops > 0) ? AL_TRUE : AL_FALSE);
    alSourcePlay(source);
    do {
        ALint pos;
        al_nssleep(10000000);
        alGetSourcei(source, AL_SAMPLE_OFFSET, &pos);
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if(pos < last_pos && state == AL_PLAYING)
        {
            ++num_loops;
            if(num_loops >= max_loops)
                alSourcei(source, AL_LOOPING, AL_FALSE);
            printf("%d...\n", max_loops - num_loops + 1);
            fflush(stdout);
        }
        last_pos = pos;
    } while(alGetError() == AL_NO_ERROR && state == AL_PLAYING);

    /* All done. Delete resources, and close OpenAL. */
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);

    /* Close up OpenAL. */
    CloseAL();

    return 0;
}
