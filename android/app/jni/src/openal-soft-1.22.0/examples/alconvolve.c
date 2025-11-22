/*
 * OpenAL Convolution Reverb Example
 *
 * Copyright (c) 2020 by Chris Robinson <chris.kcat@gmail.com>
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

/* This file contains an example for applying convolution reverb to a source. */

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sndfile.h"

#include "AL/al.h"
#include "AL/alext.h"

#include "common/alhelpers.h"


#ifndef AL_SOFT_convolution_reverb
#define AL_SOFT_convolution_reverb
#define AL_EFFECT_CONVOLUTION_REVERB_SOFT        0xA000
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


/* This stuff defines a simple streaming player object, the same as alstream.c.
 * Comments are removed for brevity, see alstream.c for more details.
 */
#define NUM_BUFFERS 4
#define BUFFER_SAMPLES 8192

typedef struct StreamPlayer {
    ALuint buffers[NUM_BUFFERS];
    ALuint source;

    SNDFILE *sndfile;
    SF_INFO sfinfo;
    float *membuf;

    ALenum format;
} StreamPlayer;

static StreamPlayer *NewPlayer(void)
{
    StreamPlayer *player;

    player = calloc(1, sizeof(*player));
    assert(player != NULL);

    alGenBuffers(NUM_BUFFERS, player->buffers);
    assert(alGetError() == AL_NO_ERROR && "Could not create buffers");

    alGenSources(1, &player->source);
    assert(alGetError() == AL_NO_ERROR && "Could not create source");

    alSource3i(player->source, AL_POSITION, 0, 0, -1);
    alSourcei(player->source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(player->source, AL_ROLLOFF_FACTOR, 0);
    assert(alGetError() == AL_NO_ERROR && "Could not set source parameters");

    return player;
}

static void ClosePlayerFile(StreamPlayer *player)
{
    if(player->sndfile)
        sf_close(player->sndfile);
    player->sndfile = NULL;

    free(player->membuf);
    player->membuf = NULL;
}

static void DeletePlayer(StreamPlayer *player)
{
    ClosePlayerFile(player);

    alDeleteSources(1, &player->source);
    alDeleteBuffers(NUM_BUFFERS, player->buffers);
    if(alGetError() != AL_NO_ERROR)
        fprintf(stderr, "Failed to delete object IDs\n");

    memset(player, 0, sizeof(*player));
    free(player);
}

static int OpenPlayerFile(StreamPlayer *player, const char *filename)
{
    size_t frame_size;

    ClosePlayerFile(player);

    player->sndfile = sf_open(filename, SFM_READ, &player->sfinfo);
    if(!player->sndfile)
    {
        fprintf(stderr, "Could not open audio in %s: %s\n", filename, sf_strerror(NULL));
        return 0;
    }

    player->format = AL_NONE;
    if(player->sfinfo.channels == 1)
        player->format = AL_FORMAT_MONO_FLOAT32;
    else if(player->sfinfo.channels == 2)
        player->format = AL_FORMAT_STEREO_FLOAT32;
    else if(player->sfinfo.channels == 6)
        player->format = AL_FORMAT_51CHN32;
    else if(player->sfinfo.channels == 3)
    {
        if(sf_command(player->sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
            player->format = AL_FORMAT_BFORMAT2D_FLOAT32;
    }
    else if(player->sfinfo.channels == 4)
    {
        if(sf_command(player->sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
            player->format = AL_FORMAT_BFORMAT3D_FLOAT32;
    }
    if(!player->format)
    {
        fprintf(stderr, "Unsupported channel count: %d\n", player->sfinfo.channels);
        sf_close(player->sndfile);
        player->sndfile = NULL;
        return 0;
    }

    frame_size = (size_t)(BUFFER_SAMPLES * player->sfinfo.channels) * sizeof(float);
    player->membuf = malloc(frame_size);

    return 1;
}

static int StartPlayer(StreamPlayer *player)
{
    ALsizei i;

    alSourceRewind(player->source);
    alSourcei(player->source, AL_BUFFER, 0);

    for(i = 0;i < NUM_BUFFERS;i++)
    {
        sf_count_t slen = sf_readf_float(player->sndfile, player->membuf, BUFFER_SAMPLES);
        if(slen < 1) break;

        slen *= player->sfinfo.channels * (sf_count_t)sizeof(float);
        alBufferData(player->buffers[i], player->format, player->membuf, (ALsizei)slen,
            player->sfinfo.samplerate);
    }
    if(alGetError() != AL_NO_ERROR)
    {
        fprintf(stderr, "Error buffering for playback\n");
        return 0;
    }

    alSourceQueueBuffers(player->source, i, player->buffers);
    alSourcePlay(player->source);
    if(alGetError() != AL_NO_ERROR)
    {
        fprintf(stderr, "Error starting playback\n");
        return 0;
    }

    return 1;
}

static int UpdatePlayer(StreamPlayer *player)
{
    ALint processed, state;

    alGetSourcei(player->source, AL_SOURCE_STATE, &state);
    alGetSourcei(player->source, AL_BUFFERS_PROCESSED, &processed);
    if(alGetError() != AL_NO_ERROR)
    {
        fprintf(stderr, "Error checking source state\n");
        return 0;
    }

    while(processed > 0)
    {
        ALuint bufid;
        sf_count_t slen;

        alSourceUnqueueBuffers(player->source, 1, &bufid);
        processed--;

        slen = sf_readf_float(player->sndfile, player->membuf, BUFFER_SAMPLES);
        if(slen > 0)
        {
            slen *= player->sfinfo.channels * (sf_count_t)sizeof(float);
            alBufferData(bufid, player->format, player->membuf, (ALsizei)slen,
                player->sfinfo.samplerate);
            alSourceQueueBuffers(player->source, 1, &bufid);
        }
        if(alGetError() != AL_NO_ERROR)
        {
            fprintf(stderr, "Error buffering data\n");
            return 0;
        }
    }

    if(state != AL_PLAYING && state != AL_PAUSED)
    {
        ALint queued;

        alGetSourcei(player->source, AL_BUFFERS_QUEUED, &queued);
        if(queued == 0)
            return 0;

        alSourcePlay(player->source);
        if(alGetError() != AL_NO_ERROR)
        {
            fprintf(stderr, "Error restarting playback\n");
            return 0;
        }
    }

    return 1;
}


/* CreateEffect creates a new OpenAL effect object with a convolution reverb
 * type, and returns the new effect ID.
 */
static ALuint CreateEffect(void)
{
    ALuint effect = 0;
    ALenum err;

    printf("Using Convolution Reverb\n");

    /* Create the effect object and set the convolution reverb effect type. */
    alGenEffects(1, &effect);
    alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_CONVOLUTION_REVERB_SOFT);

    /* Check if an error occured, and clean up if so. */
    err = alGetError();
    if(err != AL_NO_ERROR)
    {
        fprintf(stderr, "OpenAL error: %s\n", alGetString(err));
        if(alIsEffect(effect))
            alDeleteEffects(1, &effect);
        return 0;
    }

    return effect;
}

/* LoadBuffer loads the named audio file into an OpenAL buffer object, and
 * returns the new buffer ID.
 */
static ALuint LoadSound(const char *filename)
{
    const char *namepart;
    ALenum err, format;
    ALuint buffer;
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    float *membuf;
    sf_count_t num_frames;
    ALsizei num_bytes;

    /* Open the audio file and check that it's usable. */
    sndfile = sf_open(filename, SFM_READ, &sfinfo);
    if(!sndfile)
    {
        fprintf(stderr, "Could not open audio in %s: %s\n", filename, sf_strerror(sndfile));
        return 0;
    }
    if(sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT_MAX/sizeof(float))/sfinfo.channels)
    {
        fprintf(stderr, "Bad sample count in %s (%" PRId64 ")\n", filename, sfinfo.frames);
        sf_close(sndfile);
        return 0;
    }

    /* Get the sound format, and figure out the OpenAL format. Use floats since
     * impulse responses will usually have more than 16-bit precision.
     */
    format = AL_NONE;
    if(sfinfo.channels == 1)
        format = AL_FORMAT_MONO_FLOAT32;
    else if(sfinfo.channels == 2)
        format = AL_FORMAT_STEREO_FLOAT32;
    else if(sfinfo.channels == 3)
    {
        if(sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
            format = AL_FORMAT_BFORMAT2D_FLOAT32;
    }
    else if(sfinfo.channels == 4)
    {
        if(sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
            format = AL_FORMAT_BFORMAT3D_FLOAT32;
    }
    if(!format)
    {
        fprintf(stderr, "Unsupported channel count: %d\n", sfinfo.channels);
        sf_close(sndfile);
        return 0;
    }

    namepart = strrchr(filename, '/');
    if(namepart || (namepart=strrchr(filename, '\\')))
        namepart++;
    else
        namepart = filename;
    printf("Loading: %s (%s, %dhz, %" PRId64 " samples / %.2f seconds)\n", namepart,
        FormatName(format), sfinfo.samplerate, sfinfo.frames,
        (double)sfinfo.frames / sfinfo.samplerate);
    fflush(stdout);

    /* Decode the whole audio file to a buffer. */
    membuf = malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(float));

    num_frames = sf_readf_float(sndfile, membuf, sfinfo.frames);
    if(num_frames < 1)
    {
        free(membuf);
        sf_close(sndfile);
        fprintf(stderr, "Failed to read samples in %s (%" PRId64 ")\n", filename, num_frames);
        return 0;
    }
    num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(float);

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
    ALuint ir_buffer, filter, effect, slot;
    StreamPlayer *player;
    int i;

    /* Print out usage if no arguments were specified */
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s [-device <name>] <impulse response file> "
            "<[-dry | -nodry] filename>...\n", argv[0]);
        return 1;
    }

    argv++; argc--;
    if(InitAL(&argv, &argc) != 0)
        return 1;

    if(!alIsExtensionPresent("AL_SOFTX_convolution_reverb"))
    {
        CloseAL();
        fprintf(stderr, "Error: Convolution revern not supported\n");
        return 1;
    }

    if(argc < 2)
    {
        CloseAL();
        fprintf(stderr, "Error: Missing impulse response or sound files\n");
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

    /* Load the reverb into an effect. */
    effect = CreateEffect();
    if(!effect)
    {
        CloseAL();
        return 1;
    }

    /* Load the impulse response sound into a buffer. */
    ir_buffer = LoadSound(argv[0]);
    if(!ir_buffer)
    {
        alDeleteEffects(1, &effect);
        CloseAL();
        return 1;
    }

    /* Create the effect slot object. This is what "plays" an effect on sources
     * that connect to it.
     */
    slot = 0;
    alGenAuxiliaryEffectSlots(1, &slot);

    /* Set the impulse response sound buffer on the effect slot. This allows
     * effects to access it as needed. In this case, convolution reverb uses it
     * as the filter source. NOTE: Unlike the effect object, the buffer *is*
     * kept referenced and may not be changed or deleted as long as it's set,
     * just like with a source. When another buffer is set, or the effect slot
     * is deleted, the buffer reference is released.
     *
     * The effect slot's gain is reduced because the impulse responses I've
     * tested with result in excessively loud reverb. Is that normal? Even with
     * this, it seems a bit on the loud side.
     *
     * Also note: unlike standard or EAX reverb, there is no automatic
     * attenuation of a source's reverb response with distance, so the reverb
     * will remain full volume regardless of a given sound's distance from the
     * listener. You can use a send filter to alter a given source's
     * contribution to reverb.
     */
    alAuxiliaryEffectSloti(slot, AL_BUFFER, (ALint)ir_buffer);
    alAuxiliaryEffectSlotf(slot, AL_EFFECTSLOT_GAIN, 1.0f / 16.0f);
    alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, (ALint)effect);
    assert(alGetError()==AL_NO_ERROR && "Failed to set effect slot");

    /* Create a filter that can silence the dry path. */
    filter = 0;
    alGenFilters(1, &filter);
    alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilterf(filter, AL_LOWPASS_GAIN, 0.0f);

    player = NewPlayer();
    /* Connect the player's source to the effect slot. */
    alSource3i(player->source, AL_AUXILIARY_SEND_FILTER, (ALint)slot, 0, AL_FILTER_NULL);
    assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");

    /* Play each file listed on the command line */
    for(i = 1;i < argc;i++)
    {
        const char *namepart;

        if(argc-i > 1)
        {
            if(strcasecmp(argv[i], "-nodry") == 0)
            {
                alSourcei(player->source, AL_DIRECT_FILTER, (ALint)filter);
                ++i;
            }
            else if(strcasecmp(argv[i], "-dry") == 0)
            {
                alSourcei(player->source, AL_DIRECT_FILTER, AL_FILTER_NULL);
                ++i;
            }
        }

        if(!OpenPlayerFile(player, argv[i]))
            continue;

        namepart = strrchr(argv[i], '/');
        if(namepart || (namepart=strrchr(argv[i], '\\')))
            namepart++;
        else
            namepart = argv[i];

        printf("Playing: %s (%s, %dhz)\n", namepart, FormatName(player->format),
            player->sfinfo.samplerate);
        fflush(stdout);

        if(!StartPlayer(player))
        {
            ClosePlayerFile(player);
            continue;
        }

        while(UpdatePlayer(player))
            al_nssleep(10000000);

        ClosePlayerFile(player);
    }
    printf("Done.\n");

    /* All files done. Delete the player and effect resources, and close down
     * OpenAL.
     */
    DeletePlayer(player);
    player = NULL;

    alDeleteAuxiliaryEffectSlots(1, &slot);
    alDeleteEffects(1, &effect);
    alDeleteFilters(1, &filter);
    alDeleteBuffers(1, &ir_buffer);

    CloseAL();

    return 0;
}
