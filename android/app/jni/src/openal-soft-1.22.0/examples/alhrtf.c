/*
 * OpenAL HRTF Example
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

/* This file contains an example for selecting an HRTF. */

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sndfile.h"

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#include "common/alhelpers.h"


#ifndef M_PI
#define M_PI                         (3.14159265358979323846)
#endif

static LPALCGETSTRINGISOFT alcGetStringiSOFT;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT;

/* LoadBuffer loads the named audio file into an OpenAL buffer object, and
 * returns the new buffer ID.
 */
static ALuint LoadSound(const char *filename)
{
    ALenum err, format;
    ALuint buffer;
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    short *membuf;
    sf_count_t num_frames;
    ALsizei num_bytes;

    /* Open the audio file and check that it's usable. */
    sndfile = sf_open(filename, SFM_READ, &sfinfo);
    if(!sndfile)
    {
        fprintf(stderr, "Could not open audio in %s: %s\n", filename, sf_strerror(sndfile));
        return 0;
    }
    if(sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT_MAX/sizeof(short))/sfinfo.channels)
    {
        fprintf(stderr, "Bad sample count in %s (%" PRId64 ")\n", filename, sfinfo.frames);
        sf_close(sndfile);
        return 0;
    }

    /* Get the sound format, and figure out the OpenAL format */
    format = AL_NONE;
    if(sfinfo.channels == 1)
        format = AL_FORMAT_MONO16;
    else if(sfinfo.channels == 2)
        format = AL_FORMAT_STEREO16;
    else if(sfinfo.channels == 3)
    {
        if(sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
            format = AL_FORMAT_BFORMAT2D_16;
    }
    else if(sfinfo.channels == 4)
    {
        if(sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
            format = AL_FORMAT_BFORMAT3D_16;
    }
    if(!format)
    {
        fprintf(stderr, "Unsupported channel count: %d\n", sfinfo.channels);
        sf_close(sndfile);
        return 0;
    }

    /* Decode the whole audio file to a buffer. */
    membuf = malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short));

    num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
    if(num_frames < 1)
    {
        free(membuf);
        sf_close(sndfile);
        fprintf(stderr, "Failed to read samples in %s (%" PRId64 ")\n", filename, num_frames);
        return 0;
    }
    num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

    /* Buffer the audio data into a new buffer object, then free the data and
     * close the file.
     */
    buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

    free(membuf);
    sf_close(sndfile);

    /* Check if an error occured, and clean up if so. */
    err = alGetError();
    if(err != AL_NO_ERROR)
    {
        fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
        if(buffer && alIsBuffer(buffer))
            alDeleteBuffers(1, &buffer);
        return 0;
    }

    return buffer;
}


int main(int argc, char **argv)
{
    ALCdevice *device;
    ALCcontext *context;
    ALboolean has_angle_ext;
    ALuint source, buffer;
    const char *soundname;
    const char *hrtfname;
    ALCint hrtf_state;
    ALCint num_hrtf;
    ALdouble angle;
    ALenum state;

    /* Print out usage if no arguments were specified */
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s [-device <name>] [-hrtf <name>] <soundfile>\n", argv[0]);
        return 1;
    }

    /* Initialize OpenAL, and check for HRTF support. */
    argv++; argc--;
    if(InitAL(&argv, &argc) != 0)
        return 1;

    context = alcGetCurrentContext();
    device = alcGetContextsDevice(context);
    if(!alcIsExtensionPresent(device, "ALC_SOFT_HRTF"))
    {
        fprintf(stderr, "Error: ALC_SOFT_HRTF not supported\n");
        CloseAL();
        return 1;
    }

    /* Define a macro to help load the function pointers. */
#define LOAD_PROC(d, T, x)  ((x) = FUNCTION_CAST(T, alcGetProcAddress((d), #x)))
    LOAD_PROC(device, LPALCGETSTRINGISOFT, alcGetStringiSOFT);
    LOAD_PROC(device, LPALCRESETDEVICESOFT, alcResetDeviceSOFT);
#undef LOAD_PROC

    /* Check for the AL_EXT_STEREO_ANGLES extension to be able to also rotate
     * stereo sources.
     */
    has_angle_ext = alIsExtensionPresent("AL_EXT_STEREO_ANGLES");
    printf("AL_EXT_STEREO_ANGLES %sfound\n", has_angle_ext?"":"not ");

    /* Check for user-preferred HRTF */
    if(strcmp(argv[0], "-hrtf") == 0)
    {
        hrtfname = argv[1];
        soundname = argv[2];
    }
    else
    {
        hrtfname = NULL;
        soundname = argv[0];
    }

    /* Enumerate available HRTFs, and reset the device using one. */
    alcGetIntegerv(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
    if(!num_hrtf)
        printf("No HRTFs found\n");
    else
    {
        ALCint attr[5];
        ALCint index = -1;
        ALCint i;

        printf("Available HRTFs:\n");
        for(i = 0;i < num_hrtf;i++)
        {
            const ALCchar *name = alcGetStringiSOFT(device, ALC_HRTF_SPECIFIER_SOFT, i);
            printf("    %d: %s\n", i, name);

            /* Check if this is the HRTF the user requested. */
            if(hrtfname && strcmp(name, hrtfname) == 0)
                index = i;
        }

        i = 0;
        attr[i++] = ALC_HRTF_SOFT;
        attr[i++] = ALC_TRUE;
        if(index == -1)
        {
            if(hrtfname)
                printf("HRTF \"%s\" not found\n", hrtfname);
            printf("Using default HRTF...\n");
        }
        else
        {
            printf("Selecting HRTF %d...\n", index);
            attr[i++] = ALC_HRTF_ID_SOFT;
            attr[i++] = index;
        }
        attr[i] = 0;

        if(!alcResetDeviceSOFT(device, attr))
            printf("Failed to reset device: %s\n", alcGetString(device, alcGetError(device)));
    }

    /* Check if HRTF is enabled, and show which is being used. */
    alcGetIntegerv(device, ALC_HRTF_SOFT, 1, &hrtf_state);
    if(!hrtf_state)
        printf("HRTF not enabled!\n");
    else
    {
        const ALchar *name = alcGetString(device, ALC_HRTF_SPECIFIER_SOFT);
        printf("HRTF enabled, using %s\n", name);
    }
    fflush(stdout);

    /* Load the sound into a buffer. */
    buffer = LoadSound(soundname);
    if(!buffer)
    {
        CloseAL();
        return 1;
    }

    /* Create the source to play the sound with. */
    source = 0;
    alGenSources(1, &source);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(source, AL_POSITION, 0.0f, 0.0f, -1.0f);
    alSourcei(source, AL_BUFFER, (ALint)buffer);
    assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");

    /* Play the sound until it finishes. */
    angle = 0.0;
    alSourcePlay(source);
    do {
        al_nssleep(10000000);

        alcSuspendContext(context);

        /* Rotate the source around the listener by about 1/4 cycle per second,
         * and keep it within -pi...+pi.
         */
        angle += 0.01 * M_PI * 0.5;
        if(angle > M_PI)
            angle -= M_PI*2.0;

        /* This only rotates mono sounds. */
        alSource3f(source, AL_POSITION, (ALfloat)sin(angle), 0.0f, -(ALfloat)cos(angle));

        if(has_angle_ext)
        {
            /* This rotates stereo sounds with the AL_EXT_STEREO_ANGLES
             * extension. Angles are specified counter-clockwise in radians.
             */
            ALfloat angles[2] = { (ALfloat)(M_PI/6.0 - angle), (ALfloat)(-M_PI/6.0 - angle) };
            alSourcefv(source, AL_STEREO_ANGLES, angles);
        }
        alcProcessContext(context);

        alGetSourcei(source, AL_SOURCE_STATE, &state);
    } while(alGetError() == AL_NO_ERROR && state == AL_PLAYING);

    /* All done. Delete resources, and close down OpenAL. */
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    CloseAL();

    return 0;
}
