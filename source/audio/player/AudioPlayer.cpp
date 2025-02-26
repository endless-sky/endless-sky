/* AudioPlayer.cpp
Copyright (c) 2025 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AudioPlayer.h"

#include "../../Random.h"

#include <algorithm>

using namespace std;



std::vector<ALuint> AudioPlayer::availableSources{};



AudioPlayer::AudioPlayer(SoundCategory category, unique_ptr<AudioSupplier> supplier)
	: category(category), audioSupplier(std::move(supplier))
{
}



AudioPlayer::~AudioPlayer()
{
	// Maintenance note: this does not delete the buffers.
	// Make sure playback has stopped and the buffers were released appropriately.
	// (This can't be enforced as the game may exit at any point, even with audio playing.)
	ReleaseSource();
}



void AudioPlayer::Update()
{
	if(!alSource)
		return;

	ALint buffersDone = 0;
	alGetSourcei(alSource, AL_BUFFERS_PROCESSED, &buffersDone);

	if(!buffersDone)
		return;

	if(!audioSupplier->MaxChunkCount() || shouldStop) // no chunks left to play
	{
		ALint buffersQueued = 0;
		alGetSourcei(alSource, AL_BUFFERS_QUEUED, &buffersQueued);

		ALint state;
		alGetSourcei(alSource, AL_SOURCE_STATE, &state);

		if(buffersDone == buffersQueued && state == AL_STOPPED)
		{
			// All queued buffers finished, and we don't have any others left. Playback has finished.
			// Unqueue all buffers and return them, then release the source.
			std::vector<ALuint> buffers(buffersDone);
			alSourceUnqueueBuffers(alSource, buffers.size(), buffers.data());

			for(ALuint buffer : buffers)
				audioSupplier->ReturnBuffer(buffer);

			ReleaseSource();

			done = true;
		}
	}
	else if(audioSupplier->AvailableChunks())
	{
		// Queue as many buffers as possible.
		vector<ALuint> buffers(min(buffersDone, audioSupplier->AvailableChunks()));
		alSourceUnqueueBuffers(alSource, buffers.size(), buffers.data());

		for(ALuint &buffer : buffers)
		{
			if(audioSupplier->IsSynchronous())
			{
				audioSupplier->ReturnBuffer(buffer);
				buffer = audioSupplier->AwaitNextChunk();
			}
			else
				audioSupplier->NextChunk(buffer);
		}
		alSourceQueueBuffers(alSource, buffers.size(), buffers.data());
	}
}



bool AudioPlayer::IsFinished() const
{
	return done;
}



double AudioPlayer::Volume() const
{
	if(!alSource)
		return 0.;

	ALfloat value;
	alGetSourcef(alSource, AL_GAIN, &value);
	return value;
}



void AudioPlayer::SetVolume(double level) const
{
	if(!alSource)
		return;

	alSourcef(alSource, AL_GAIN, level);
}



void AudioPlayer::Move(double x, double y, double z) const
{
	alSource3f(alSource, AL_POSITION, x, y, z);
}



SoundCategory AudioPlayer::Category() const
{
	return category;
}



void AudioPlayer::Pause() const
{
	if(!alSource)
		return;

	alSourcePause(alSource);
}



void AudioPlayer::Play() const
{
	if(!alSource)
		return;

	ALint state;
	alGetSourcei(alSource, AL_SOURCE_STATE, &state);
	if(state != AL_PLAYING)
		alSourcePlay(alSource);
}



void AudioPlayer::Init()
{
	if(shouldStop)
		return;

	if(!alSource)
		if(!ClaimSource())
			return;

	int bufferCount = clamp(audioSupplier->MaxChunkCount(), 1, MAX_INITIAL_BUFFERS);

	std::vector<ALuint> buffers;
	for(int i = 0; i < bufferCount; ++i)
		buffers.emplace_back(audioSupplier->AwaitNextChunk());

	// Queue the actual audio buffers
	alSourceQueueBuffers(alSource, buffers.size(), buffers.data());
}



void AudioPlayer::Stop(bool stop)
{
	shouldStop = stop;
}



AudioSupplier *AudioPlayer::Supplier()
{
	return audioSupplier.get();
}



bool AudioPlayer::ClaimSource()
{
	if(alSource)
		return true;

	if(!availableSources.empty())
	{
		alSource = availableSources.back();
		availableSources.pop_back();
		ConfigureSource();
		return true;
	}
	else
	{
		alGenSources(1, &alSource);
		ConfigureSource();

		return alSource;
	}
}



void AudioPlayer::ConfigureSource()
{
	if(!alSource)
		return;

	alSourcef(alSource, AL_PITCH, 1. + (Random::Real() - Random::Real()) * .04);
	alSourcef(alSource, AL_REFERENCE_DISTANCE, 1.);
	alSourcef(alSource, AL_ROLLOFF_FACTOR, 1.);
	alSourcei(alSource, AL_LOOPING, 0);
	alSourcef(alSource, AL_MAX_DISTANCE, 100.);
}



void AudioPlayer::ReleaseSource()
{
	if(!alSource)
		return;

	availableSources.emplace_back(alSource);
	alSource = 0;
}
