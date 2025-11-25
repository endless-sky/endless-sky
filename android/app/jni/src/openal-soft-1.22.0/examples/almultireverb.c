/*
 * OpenAL Multi-Zone Reverb Example
 *
 * Copyright (c) 2018 by Chris Robinson <chris.kcat@gmail.com>
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

/* This file contains an example for controlling multiple reverb zones to
 * smoothly transition between reverb environments. The general concept is to
 * extend single-reverb by also tracking the closest adjacent environment, and
 * utilize EAX Reverb's panning vectors to position them relative to the
 * listener.
 */


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
#include "AL/efx.h"
#include "AL/efx-presets.h"

#include "common/alhelpers.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/* Filter object functions */
static LPALGENFILTERS alGenFilters;
static LPALDELETEFILTERS alDeleteFilters;
static LPALISFILTER alIsFilter;
static LPALFILTERI alFilteri;
static LPALFILTERIV alFilteriv;
static LPALFILTERF alFilterf;
static LPALFILTERFV alFilterfv;
static LPALGETFILTERI alGetFilteri;
static LPALGETFILTERIV alGetFilteriv;
static LPALGETFILTERF alGetFilterf;
static LPALGETFILTERFV alGetFilterfv;

/* Effect object functions */
static LPALGENEFFECTS alGenEffects;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALISEFFECT alIsEffect;
static LPALEFFECTI alEffecti;
static LPALEFFECTIV alEffectiv;
static LPALEFFECTF alEffectf;
static LPALEFFECTFV alEffectfv;
static LPALGETEFFECTI alGetEffecti;
static LPALGETEFFECTIV alGetEffectiv;
static LPALGETEFFECTF alGetEffectf;
static LPALGETEFFECTFV alGetEffectfv;

/* Auxiliary Effect Slot object functions */
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
static LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
static LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
static LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
static LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
static LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
static LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
static LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;


/* LoadEffect loads the given initial reverb properties into the given OpenAL
 * effect object, and returns non-zero on success.
 */
static int LoadEffect(ALuint effect, const EFXEAXREVERBPROPERTIES *reverb)
{
    ALenum err;

    alGetError();

    /* Prepare the effect for EAX Reverb (standard reverb doesn't contain
     * the needed panning vectors).
     */
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
    if((err=alGetError()) != AL_NO_ERROR)
    {
        fprintf(stderr, "Failed to set EAX Reverb: %s (0x%04x)\n", alGetString(err), err);
        return 0;
    }

    /* Load the reverb properties. */
    alEffectf(effect, AL_EAXREVERB_DENSITY, reverb->flDensity);
    alEffectf(effect, AL_EAXREVERB_DIFFUSION, reverb->flDiffusion);
    alEffectf(effect, AL_EAXREVERB_GAIN, reverb->flGain);
    alEffectf(effect, AL_EAXREVERB_GAINHF, reverb->flGainHF);
    alEffectf(effect, AL_EAXREVERB_GAINLF, reverb->flGainLF);
    alEffectf(effect, AL_EAXREVERB_DECAY_TIME, reverb->flDecayTime);
    alEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, reverb->flDecayHFRatio);
    alEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, reverb->flDecayLFRatio);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain);
    alEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, reverb->flReflectionsDelay);
    alEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, reverb->flReflectionsPan);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain);
    alEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay);
    alEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, reverb->flLateReverbPan);
    alEffectf(effect, AL_EAXREVERB_ECHO_TIME, reverb->flEchoTime);
    alEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, reverb->flEchoDepth);
    alEffectf(effect, AL_EAXREVERB_MODULATION_TIME, reverb->flModulationTime);
    alEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, reverb->flModulationDepth);
    alEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb->flAirAbsorptionGainHF);
    alEffectf(effect, AL_EAXREVERB_HFREFERENCE, reverb->flHFReference);
    alEffectf(effect, AL_EAXREVERB_LFREFERENCE, reverb->flLFReference);
    alEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb->flRoomRolloffFactor);
    alEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit);

    /* Check if an error occured, and return failure if so. */
    if((err=alGetError()) != AL_NO_ERROR)
    {
        fprintf(stderr, "Error setting up reverb: %s\n", alGetString(err));
        return 0;
    }

    return 1;
}


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
    if(sfinfo.channels == 1)
        format = AL_FORMAT_MONO16;
    else if(sfinfo.channels == 2)
        format = AL_FORMAT_STEREO16;
    else
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


/* Helper to calculate the dot-product of the two given vectors. */
static ALfloat dot_product(const ALfloat vec0[3], const ALfloat vec1[3])
{
    return vec0[0]*vec1[0] + vec0[1]*vec1[1] + vec0[2]*vec1[2];
}

/* Helper to normalize a given vector. */
static void normalize(ALfloat vec[3])
{
    ALfloat mag = sqrtf(dot_product(vec, vec));
    if(mag > 0.00001f)
    {
        vec[0] /= mag;
        vec[1] /= mag;
        vec[2] /= mag;
    }
    else
    {
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = 0.0f;
    }
}


/* The main update function to update the listener and environment effects. */
static void UpdateListenerAndEffects(float timediff, const ALuint slots[2], const ALuint effects[2], const EFXEAXREVERBPROPERTIES reverbs[2])
{
    static const ALfloat listener_move_scale = 10.0f;
    /* Individual reverb zones are connected via "portals". Each portal has a
     * position (center point of the connecting area), a normal (facing
     * direction), and a radius (approximate size of the connecting area).
     */
    const ALfloat portal_pos[3] = { 0.0f, 0.0f, 0.0f };
    const ALfloat portal_norm[3] = { sqrtf(0.5f), 0.0f, -sqrtf(0.5f) };
    const ALfloat portal_radius = 2.5f;
    ALfloat other_dir[3], this_dir[3];
    ALfloat listener_pos[3];
    ALfloat local_norm[3];
    ALfloat local_dir[3];
    ALfloat near_edge[3];
    ALfloat far_edge[3];
    ALfloat dist, edist;

    /* Update the listener position for the amount of time passed. This uses a
     * simple triangular LFO to offset the position (moves along the X axis
     * between -listener_move_scale and +listener_move_scale for each
     * transition).
     */
    listener_pos[0] = (fabsf(2.0f - timediff/2.0f) - 1.0f) * listener_move_scale;
    listener_pos[1] = 0.0f;
    listener_pos[2] = 0.0f;
    alListenerfv(AL_POSITION, listener_pos);

    /* Calculate local_dir, which represents the listener-relative point to the
     * adjacent zone (should also include orientation). Because EAX Reverb uses
     * left-handed coordinates instead of right-handed like the rest of OpenAL,
     * negate Z for the local values.
     */
    local_dir[0] = portal_pos[0] - listener_pos[0];
    local_dir[1] = portal_pos[1] - listener_pos[1];
    local_dir[2] = -(portal_pos[2] - listener_pos[2]);
    /* A normal application would also rotate the portal's normal given the
     * listener orientation, to get the listener-relative normal.
     */
    local_norm[0] = portal_norm[0];
    local_norm[1] = portal_norm[1];
    local_norm[2] = -portal_norm[2];

    /* Calculate the distance from the listener to the portal, and ensure it's
     * far enough away to not suffer severe floating-point precision issues.
     */
    dist = sqrtf(dot_product(local_dir, local_dir));
    if(dist > 0.00001f)
    {
        const EFXEAXREVERBPROPERTIES *other_reverb, *this_reverb;
        ALuint other_effect, this_effect;
        ALfloat magnitude, dir_dot_norm;

        /* Normalize the direction to the portal. */
        local_dir[0] /= dist;
        local_dir[1] /= dist;
        local_dir[2] /= dist;

        /* Calculate the dot product of the portal's local direction and local
         * normal, which is used for angular and side checks later on.
         */
        dir_dot_norm = dot_product(local_dir, local_norm);

        /* Figure out which zone we're in. */
        if(dir_dot_norm <= 0.0f)
        {
            /* We're in front of the portal, so we're in Zone 0. */
            this_effect = effects[0];
            other_effect = effects[1];
            this_reverb = &reverbs[0];
            other_reverb = &reverbs[1];
        }
        else
        {
            /* We're behind the portal, so we're in Zone 1. */
            this_effect = effects[1];
            other_effect = effects[0];
            this_reverb = &reverbs[1];
            other_reverb = &reverbs[0];
        }

        /* Calculate the listener-relative extents of the portal. */
        /* First, project the listener-to-portal vector onto the portal's plane
         * to get the portal-relative direction along the plane that goes away
         * from the listener (toward the farthest edge of the portal).
         */
        far_edge[0] = local_dir[0] - local_norm[0]*dir_dot_norm;
        far_edge[1] = local_dir[1] - local_norm[1]*dir_dot_norm;
        far_edge[2] = local_dir[2] - local_norm[2]*dir_dot_norm;

        edist = sqrtf(dot_product(far_edge, far_edge));
        if(edist > 0.0001f)
        {
            /* Rescale the portal-relative vector to be at the radius edge. */
            ALfloat mag = portal_radius / edist;
            far_edge[0] *= mag;
            far_edge[1] *= mag;
            far_edge[2] *= mag;

            /* Calculate the closest edge of the portal by negating the
             * farthest, and add an offset to make them both relative to the
             * listener.
             */
            near_edge[0] = local_dir[0]*dist - far_edge[0];
            near_edge[1] = local_dir[1]*dist - far_edge[1];
            near_edge[2] = local_dir[2]*dist - far_edge[2];
            far_edge[0] += local_dir[0]*dist;
            far_edge[1] += local_dir[1]*dist;
            far_edge[2] += local_dir[2]*dist;

            /* Normalize the listener-relative extents of the portal, then
             * calculate the panning magnitude for the other zone given the
             * apparent size of the opening. The panning magnitude affects the
             * envelopment of the environment, with 1 being a point, 0.5 being
             * half coverage around the listener, and 0 being full coverage.
             */
            normalize(far_edge);
            normalize(near_edge);
            magnitude = 1.0f - acosf(dot_product(far_edge, near_edge))/(float)(M_PI*2.0);

            /* Recalculate the panning direction, to be directly between the
             * direction of the two extents.
             */
            local_dir[0] = far_edge[0] + near_edge[0];
            local_dir[1] = far_edge[1] + near_edge[1];
            local_dir[2] = far_edge[2] + near_edge[2];
            normalize(local_dir);
        }
        else
        {
            /* If we get here, the listener is directly in front of or behind
             * the center of the portal, making all aperture edges effectively
             * equidistant. Calculating the panning magnitude is simplified,
             * using the arctangent of the radius and distance.
             */
            magnitude = 1.0f - (atan2f(portal_radius, dist) / (float)M_PI);
        }

        /* Scale the other zone's panning vector. */
        other_dir[0] = local_dir[0] * magnitude;
        other_dir[1] = local_dir[1] * magnitude;
        other_dir[2] = local_dir[2] * magnitude;
        /* Pan the current zone to the opposite direction of the portal, and
         * take the remaining percentage of the portal's magnitude.
         */
        this_dir[0] = local_dir[0] * (magnitude-1.0f);
        this_dir[1] = local_dir[1] * (magnitude-1.0f);
        this_dir[2] = local_dir[2] * (magnitude-1.0f);

        /* Now set the effects' panning vectors and gain. Energy is shared
         * between environments, so attenuate according to each zone's
         * contribution (note: gain^2 = energy).
         */
        alEffectf(this_effect, AL_EAXREVERB_REFLECTIONS_GAIN, this_reverb->flReflectionsGain * sqrtf(magnitude));
        alEffectf(this_effect, AL_EAXREVERB_LATE_REVERB_GAIN, this_reverb->flLateReverbGain * sqrtf(magnitude));
        alEffectfv(this_effect, AL_EAXREVERB_REFLECTIONS_PAN, this_dir);
        alEffectfv(this_effect, AL_EAXREVERB_LATE_REVERB_PAN, this_dir);

        alEffectf(other_effect, AL_EAXREVERB_REFLECTIONS_GAIN, other_reverb->flReflectionsGain * sqrtf(1.0f-magnitude));
        alEffectf(other_effect, AL_EAXREVERB_LATE_REVERB_GAIN, other_reverb->flLateReverbGain * sqrtf(1.0f-magnitude));
        alEffectfv(other_effect, AL_EAXREVERB_REFLECTIONS_PAN, other_dir);
        alEffectfv(other_effect, AL_EAXREVERB_LATE_REVERB_PAN, other_dir);
    }
    else
    {
        /* We're practically in the center of the portal. Give the panning
         * vectors a 50/50 split, with Zone 0 covering the half in front of
         * the normal, and Zone 1 covering the half behind.
         */
        this_dir[0] = local_norm[0] / 2.0f;
        this_dir[1] = local_norm[1] / 2.0f;
        this_dir[2] = local_norm[2] / 2.0f;

        other_dir[0] = local_norm[0] / -2.0f;
        other_dir[1] = local_norm[1] / -2.0f;
        other_dir[2] = local_norm[2] / -2.0f;

        alEffectf(effects[0], AL_EAXREVERB_REFLECTIONS_GAIN, reverbs[0].flReflectionsGain * sqrtf(0.5f));
        alEffectf(effects[0], AL_EAXREVERB_LATE_REVERB_GAIN, reverbs[0].flLateReverbGain * sqrtf(0.5f));
        alEffectfv(effects[0], AL_EAXREVERB_REFLECTIONS_PAN, this_dir);
        alEffectfv(effects[0], AL_EAXREVERB_LATE_REVERB_PAN, this_dir);

        alEffectf(effects[1], AL_EAXREVERB_REFLECTIONS_GAIN, reverbs[1].flReflectionsGain * sqrtf(0.5f));
        alEffectf(effects[1], AL_EAXREVERB_LATE_REVERB_GAIN, reverbs[1].flLateReverbGain * sqrtf(0.5f));
        alEffectfv(effects[1], AL_EAXREVERB_REFLECTIONS_PAN, other_dir);
        alEffectfv(effects[1], AL_EAXREVERB_LATE_REVERB_PAN, other_dir);
    }

    /* Finally, update the effect slots with the updated effect parameters. */
    alAuxiliaryEffectSloti(slots[0], AL_EFFECTSLOT_EFFECT, (ALint)effects[0]);
    alAuxiliaryEffectSloti(slots[1], AL_EFFECTSLOT_EFFECT, (ALint)effects[1]);
}


int main(int argc, char **argv)
{
    static const int MaxTransitions = 8;
    EFXEAXREVERBPROPERTIES reverbs[2] = {
        EFX_REVERB_PRESET_CARPETEDHALLWAY,
        EFX_REVERB_PRESET_BATHROOM
    };
    ALCdevice *device = NULL;
    ALCcontext *context = NULL;
    ALuint effects[2] = { 0, 0 };
    ALuint slots[2] = { 0, 0 };
    ALuint direct_filter = 0;
    ALuint buffer = 0;
    ALuint source = 0;
    ALCint num_sends = 0;
    ALenum state = AL_INITIAL;
    ALfloat direct_gain = 1.0f;
    int basetime = 0;
    int loops = 0;

    /* Print out usage if no arguments were specified */
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s [-device <name>] [options] <filename>\n\n"
        "Options:\n"
        "\t-nodirect\tSilence direct path output (easier to hear reverb)\n\n",
        argv[0]);
        return 1;
    }

    /* Initialize OpenAL, and check for EFX support with at least 2 auxiliary
     * sends (if multiple sends are supported, 2 are provided by default; if
     * you want more, you have to request it through alcCreateContext).
     */
    argv++; argc--;
    if(InitAL(&argv, &argc) != 0)
        return 1;

    while(argc > 0)
    {
        if(strcmp(argv[0], "-nodirect") == 0)
            direct_gain = 0.0f;
        else
            break;
        argv++;
        argc--;
    }
    if(argc < 1)
    {
        fprintf(stderr, "No filename spacified.\n");
        CloseAL();
        return 1;
    }

    context = alcGetCurrentContext();
    device = alcGetContextsDevice(context);

    if(!alcIsExtensionPresent(device, "ALC_EXT_EFX"))
    {
        fprintf(stderr, "Error: EFX not supported\n");
        CloseAL();
        return 1;
    }

    num_sends = 0;
    alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, &num_sends);
    if(alcGetError(device) != ALC_NO_ERROR || num_sends < 2)
    {
        fprintf(stderr, "Error: Device does not support multiple sends (got %d, need 2)\n",
                num_sends);
        CloseAL();
        return 1;
    }

    /* Define a macro to help load the function pointers. */
#define LOAD_PROC(T, x)  ((x) = FUNCTION_CAST(T, alGetProcAddress(#x)))
    LOAD_PROC(LPALGENFILTERS, alGenFilters);
    LOAD_PROC(LPALDELETEFILTERS, alDeleteFilters);
    LOAD_PROC(LPALISFILTER, alIsFilter);
    LOAD_PROC(LPALFILTERI, alFilteri);
    LOAD_PROC(LPALFILTERIV, alFilteriv);
    LOAD_PROC(LPALFILTERF, alFilterf);
    LOAD_PROC(LPALFILTERFV, alFilterfv);
    LOAD_PROC(LPALGETFILTERI, alGetFilteri);
    LOAD_PROC(LPALGETFILTERIV, alGetFilteriv);
    LOAD_PROC(LPALGETFILTERF, alGetFilterf);
    LOAD_PROC(LPALGETFILTERFV, alGetFilterfv);

    LOAD_PROC(LPALGENEFFECTS, alGenEffects);
    LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
    LOAD_PROC(LPALISEFFECT, alIsEffect);
    LOAD_PROC(LPALEFFECTI, alEffecti);
    LOAD_PROC(LPALEFFECTIV, alEffectiv);
    LOAD_PROC(LPALEFFECTF, alEffectf);
    LOAD_PROC(LPALEFFECTFV, alEffectfv);
    LOAD_PROC(LPALGETEFFECTI, alGetEffecti);
    LOAD_PROC(LPALGETEFFECTIV, alGetEffectiv);
    LOAD_PROC(LPALGETEFFECTF, alGetEffectf);
    LOAD_PROC(LPALGETEFFECTFV, alGetEffectfv);

    LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
    LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
    LOAD_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
    LOAD_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);
    LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv);
#undef LOAD_PROC

    /* Load the sound into a buffer. */
    buffer = LoadSound(argv[0]);
    if(!buffer)
    {
        CloseAL();
        return 1;
    }

    /* Generate two effects for two "zones", and load a reverb into each one.
     * Note that unlike single-zone reverb, where you can store one effect per
     * preset, for multi-zone reverb you should have one effect per environment
     * instance, or one per audible zone. This is because we'll be changing the
     * effects' properties in real-time based on the environment instance
     * relative to the listener.
     */
    alGenEffects(2, effects);
    if(!LoadEffect(effects[0], &reverbs[0]) || !LoadEffect(effects[1], &reverbs[1]))
    {
        alDeleteEffects(2, effects);
        alDeleteBuffers(1, &buffer);
        CloseAL();
        return 1;
    }

    /* Create the effect slot objects, one for each "active" effect. */
    alGenAuxiliaryEffectSlots(2, slots);

    /* Tell the effect slots to use the loaded effect objects, with slot 0 for
     * Zone 0 and slot 1 for Zone 1. Note that this effectively copies the
     * effect properties. Modifying or deleting the effect object afterward
     * won't directly affect the effect slot until they're reapplied like this.
     */
    alAuxiliaryEffectSloti(slots[0], AL_EFFECTSLOT_EFFECT, (ALint)effects[0]);
    alAuxiliaryEffectSloti(slots[1], AL_EFFECTSLOT_EFFECT, (ALint)effects[1]);
    assert(alGetError()==AL_NO_ERROR && "Failed to set effect slot");

    /* For the purposes of this example, prepare a filter that optionally
     * silences the direct path which allows us to hear just the reverberation.
     * A filter like this is normally used for obstruction, where the path
     * directly between the listener and source is blocked (the exact
     * properties depending on the type and thickness of the obstructing
     * material).
     */
    alGenFilters(1, &direct_filter);
    alFilteri(direct_filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(direct_filter, AL_LOWPASS_GAIN, direct_gain);
    assert(alGetError()==AL_NO_ERROR && "Failed to set direct filter");

    /* Create the source to play the sound with, place it in front of the
     * listener's path in the left zone.
     */
    source = 0;
    alGenSources(1, &source);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSource3f(source, AL_POSITION, -5.0f, 0.0f, -2.0f);
    alSourcei(source, AL_DIRECT_FILTER, (ALint)direct_filter);
    alSourcei(source, AL_BUFFER, (ALint)buffer);

    /* Connect the source to the effect slots. Here, we connect source send 0
     * to Zone 0's slot, and send 1 to Zone 1's slot. Filters can be specified
     * to occlude the source from each zone by varying amounts; for example, a
     * source within a particular zone would be unfiltered, while a source that
     * can only see a zone through a window or thin wall may be attenuated for
     * that zone.
     */
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, (ALint)slots[0], 0, AL_FILTER_NULL);
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, (ALint)slots[1], 1, AL_FILTER_NULL);
    assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");

    /* Get the current time as the base for timing in the main loop. */
    basetime = altime_get();
    loops = 0;
    printf("Transition %d of %d...\n", loops+1, MaxTransitions);

    /* Play the sound for a while. */
    alSourcePlay(source);
    do {
        int curtime;
        ALfloat timediff;

        /* Start a batch update, to ensure all changes apply simultaneously. */
        alcSuspendContext(context);

        /* Get the current time to track the amount of time that passed.
         * Convert the difference to seconds.
         */
        curtime = altime_get();
        timediff = (float)(curtime - basetime) / 1000.0f;

        /* Avoid negative time deltas, in case of non-monotonic clocks. */
        if(timediff < 0.0f)
            timediff = 0.0f;
        else while(timediff >= 4.0f*(float)((loops&1)+1))
        {
            /* For this example, each transition occurs over 4 seconds, and
             * there's 2 transitions per cycle.
             */
            if(++loops < MaxTransitions)
                printf("Transition %d of %d...\n", loops+1, MaxTransitions);
            if(!(loops&1))
            {
                /* Cycle completed. Decrease the delta and increase the base
                 * time to start a new cycle.
                 */
                timediff -= 8.0f;
                basetime += 8000;
            }
        }

        /* Update the listener and effects, and finish the batch. */
        UpdateListenerAndEffects(timediff, slots, effects, reverbs);
        alcProcessContext(context);

        al_nssleep(10000000);

        alGetSourcei(source, AL_SOURCE_STATE, &state);
    } while(alGetError() == AL_NO_ERROR && state == AL_PLAYING && loops < MaxTransitions);

    /* All done. Delete resources, and close down OpenAL. */
    alDeleteSources(1, &source);
    alDeleteAuxiliaryEffectSlots(2, slots);
    alDeleteEffects(2, effects);
    alDeleteFilters(1, &direct_filter);
    alDeleteBuffers(1, &buffer);

    CloseAL();

    return 0;
}
