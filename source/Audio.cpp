/* Audio.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Audio.h"

#include "Files.h"
#include "Music.h"
#include "Point.h"
#include "Random.h"
#include "Sound.h"

#ifndef __APPLE__
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std;

namespace {
	// This class represents a new sound source that is queued to be added. This
	// is so that any thread can add a sound, but the audio thread can control
	// when those sounds actually start playing.
	class QueueEntry {
	public:
		void Add(Point position);
		void Add(const QueueEntry &other);
		
		Point sum;
		double weight = 0.;
	};
	
	// OpenAL only allows a certain number of distinct sound sources. To work
	// around that limitation, multiple instances of the same sound playing at
	// the same time will be "coalesced" into a single source, and sources will
	// be recycled once they are no longer playing.
	class Source {
	public:
		Source(const Sound *sound, unsigned source);
		
		void Move(const QueueEntry &entry) const;
		unsigned ID() const;
		const Sound *GetSound() const;
		
	private:
		const Sound *sound = nullptr;
		unsigned source = 0;
	};
	
	// Tread entry point for loading the sound files.
	void Load();
	// Get the "name" of a sound based on its file path.
	string Name(const string &path);
	
	
	// Mutex to make sure different threads don't modify the audio at the same time.
	mutex audioMutex;
	
	// OpenAL settings.
	ALCdevice *device = nullptr;
	ALCcontext *context = nullptr;
	double volume = .5;
	
	// This queue keeps track of sounds that have been requested to play. Each
	// added sound is "deferred" until the next audio position update to make
	// sure that all sounds from a given frame start at the same time.
	map<const Sound *, QueueEntry> queue;
	map<const Sound *, QueueEntry> deferred;
	
	// Sound resources that have been loaded from files.
	map<string, Sound> sounds;
	// OpenAL "sources" available for playing sounds. There are a limited number
	// of these, so they must be reused.
	vector<Source> sources;
	vector<unsigned> recycledSources;
	vector<unsigned> endingSources;
	unsigned maxSources = 255;
	
	// Queue and thread for loading sound files in the background.
	vector<string> loadQueue;
	thread loadThread;
	
	// The current position of the "listener," i.e. the center of the screen.
	Point listener;
	
	// MP3 streaming:
	unsigned musicSource = 0;
	static const size_t MUSIC_BUFFERS = 3;
	unsigned musicBuffers[MUSIC_BUFFERS];
	shared_ptr<Music> currentTrack;
	shared_ptr<Music> previousTrack;
	int musicFade = 0;
	vector<int16_t> fadeBuffer;
}



// Begin loading sounds (in a separate thread).
void Audio::Init(const vector<string> &sources)
{
	device = alcOpenDevice(nullptr);
	if(!device)
		return;
	
	context = alcCreateContext(device, nullptr);
	if(!context || !alcMakeContextCurrent(context))
		return;
	
	// The listener is looking "into" the screen. This orientation vector is
	// used to determine what sounds should be in the right or left speaker.
	ALfloat zero[3] = {0., 0., 0.};
	ALfloat	orientation[6] = {0., 0., -1., 0., 1., 0.};
	
	alListenerf(AL_GAIN, volume);
	alListenerfv(AL_POSITION, zero);
	alListenerfv(AL_VELOCITY, zero);
	alListenerfv(AL_ORIENTATION, orientation);
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alDopplerFactor(0.);
	
	// Get all the sound files in the game data and all plugins.
	for(const string &source : sources)
		Files::RecursiveList(source + "sounds/", &loadQueue);
	// Begin loading the files.
	if(!loadQueue.empty())
		loadThread = thread(&Load);
	
	// Create the music-streaming threads.
	currentTrack.reset(new Music());
	previousTrack.reset(new Music());
	alGenSources(1, &musicSource);
	alGenBuffers(MUSIC_BUFFERS, musicBuffers);
	for(unsigned buffer : musicBuffers)
	{
		// Queue up blocks of silence to start out with.
		const vector<int16_t> &chunk = currentTrack->NextChunk();
		alBufferData(buffer, AL_FORMAT_STEREO16, &chunk.front(), 2 * chunk.size(), 44100);
	}
	alSourceQueueBuffers(musicSource, MUSIC_BUFFERS, musicBuffers);
	alSourcePlay(musicSource);
}



// Check the progress of loading sounds.
double Audio::Progress()
{
	unique_lock<mutex> lock(audioMutex);
	
	if(loadQueue.empty())
		return 1.;
	
	double done = sounds.size();
	double total = done + loadQueue.size();
	return done / total;
}



// Get the volume.
double Audio::Volume()
{
	return volume;
}



// Set the volume (to a value between 0 and 1).
void Audio::SetVolume(double level)
{
	volume = min(1., max(0., level));
	if(context)
		alListenerf(AL_GAIN, volume);
}



// Get a pointer to the named sound. The name is the path relative to the
// "sound/" folder, and without ~ if it's on the end, or the extension.
const Sound *Audio::Get(const string &name)
{
	unique_lock<mutex> lock(audioMutex);
	return &sounds[name];
}



// Set the listener's position, and also update any sounds that have been
// added but deferred because they were added from a thread other than the
// main one (the one that called Init()).
void Audio::Update(const Point &listenerPosition)
{
	listener = listenerPosition;
	
	for(const auto &it : deferred)
		queue[it.first].Add(it.second);
	deferred.clear();
}



// Play the given sound, at full volume.
void Audio::Play(const Sound *sound)
{
	Play(sound, listener);
}



// Play the given sound, as if it is at the given distance from the
// "listener". This will make it softer and change the left / right balance.
void Audio::Play(const Sound *sound, const Point &position)
{
	if(!sound || !sound->Buffer() || !volume)
		return;
	
	unique_lock<mutex> lock(audioMutex);
	deferred[sound].Add(position - listener);
}



// Play the given music. An empty string means to play nothing.
void Audio::PlayMusic(const string &name)
{
	// Don't worry about thread safety here, since music will always be started
	// by the main thread.
	musicFade = 65536;
	swap(currentTrack, previousTrack);
	// If the name is empty, it means to turn music off.
	currentTrack->SetSource(name.empty() ? name : Files::Sounds() + name + ".mp3");
}



// Begin playing all the sounds that have been added since the last time
// this function was called.
void Audio::Step()
{
	vector<Source> newSources;
	// For each sound that is looping, see if it is going to continue. For other
	// sounds, check if they are done playing.
	for(const Source &source : sources)
	{
		if(source.GetSound()->IsLooping())
		{
			auto it = queue.find(source.GetSound());
			if(it != queue.end())
			{
				source.Move(it->second);
				newSources.push_back(source);
				queue.erase(it);
			}
			else
			{
				alSourcei(source.ID(), AL_LOOPING, false);
				endingSources.push_back(source.ID());
			}
		}
		else
		{
			// Non-looping sounds: check if they're done playing.
			ALint state;
			alGetSourcei(source.ID(), AL_SOURCE_STATE, &state);
			if(state == AL_PLAYING)
				newSources.push_back(source);
			else
				recycledSources.push_back(source.ID());
		}
	}
	// These sources were looping and are now wrapping up a loop.
	auto it = endingSources.begin();
	while(it != endingSources.end())
	{
		ALint state;
		alGetSourcei(*it, AL_SOURCE_STATE, &state);
		if(state == AL_PLAYING)
		{
			// Fade out the sound. This avoids a clicking or rasping sound if a
			// sound is cut off in the middle of its loop.
			float gain = 1.f;
			alGetSourcef(*it, AL_GAIN, &gain);
			gain = max(0.f, gain - .05f);
			alSourcef(*it, AL_GAIN, gain);
			++it;
		}
		else
		{
			recycledSources.push_back(*it);
			it = endingSources.erase(it);
		}
	}
	newSources.swap(sources);
	
	// Now, what is left in the queue is sounds that want to play, and that do
	// not correspond to an existing source.
	for(const auto &it : queue)
	{
		// Use a recycled source if possible. Otherwise, create a new one.
		unsigned source = 0;
		if(recycledSources.empty())
		{
			if(sources.size() >= maxSources)
				break;
			
			alGenSources(1, &source);
			if(!source)
			{
				// If we just tried to generate a new source and OpenAL would
				// not give us one, we've reached this system's limit for the
				// number of concurrent sounds.
				maxSources = sources.size();
				break;
			}
		}
		else
		{
			source = recycledSources.back();
			recycledSources.pop_back();
		}
		// Begin playing this sound.
		sources.emplace_back(it.first, source);
		sources.back().Move(it.second);
		alSourcePlay(source);
	}
	queue.clear();
	
	// Queue up new buffers for the music, if necessary.
	int buffersDone = 0;
	alGetSourcei(musicSource, AL_BUFFERS_PROCESSED, &buffersDone);
	if(buffersDone)
	{
		unsigned buffer = 0;
		alSourceUnqueueBuffers(musicSource, 1, &buffer);
		
		const vector<int16_t> &chunk = currentTrack->NextChunk();
		
		if(!musicFade)
			alBufferData(buffer, AL_FORMAT_STEREO16, &chunk.front(), 2 * chunk.size(), 44100);
		else
		{
			fadeBuffer.clear();
			const vector<int16_t> &other = previousTrack->NextChunk();
			for(size_t i = 0; i < chunk.size(); ++i)
			{
				// Blend the two tracks together.
				fadeBuffer.push_back(
					(musicFade * other[i] + (65536 - musicFade) * chunk[i]) / 65536);
				
				// Slowly fade into the new track.
				if(musicFade)
					--musicFade;
			}
			alBufferData(buffer, AL_FORMAT_STEREO16, &fadeBuffer.front(), 2 * fadeBuffer.size(), 44100);
		}
		
		alSourceQueueBuffers(musicSource, 1, &buffer);
	}
}



// Shut down the audio system (because we're about to quit).
void Audio::Quit()
{
	// First, check if sounds are still being loaded in a separate thread, and
	// if so interrupt that thread and wait for it to quit.
	unique_lock<mutex> lock(audioMutex);
	if(!loadQueue.empty())
		loadQueue.clear();
	if(loadThread.joinable())
	{
		lock.unlock();
		loadThread.join();
		lock.lock();
	}
	
	// Now, stop and delete any OpenAL sources that are playing.
	for(const Source &source : sources)
	{
		alSourceStop(source.ID());
		ALuint id = source.ID();
		alDeleteSources(1, &id);
	}
	sources.clear();
	
	// Also clean up any sources that are fading out.
	for(unsigned id : endingSources)
	{
		alSourceStop(id);
		alDeleteSources(1, &id);
	}
	endingSources.clear();
	
	// And finally, clean up any sources that are done playing.
	for(unsigned id : recycledSources)
		alDeleteSources(1, &id);
	recycledSources.clear();
	
	// Free the memory buffers for all the sound resources.
	for(const auto &it : sounds)
	{
		ALuint id = it.second.Buffer();
		alDeleteBuffers(1, &id);
	}
	sounds.clear();
	
	// Clean up the music source and buffers.
	alSourceStop(musicSource);
	alDeleteSources(1, &musicSource);
	alDeleteBuffers(MUSIC_BUFFERS, musicBuffers);
	currentTrack.reset();
	previousTrack.reset();
	
	// Close the connection to the OpenAL library.
	if(context)
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
	}
	if(device)
		alcCloseDevice(device);
}



namespace {
	// Add a new source to this queue entry. Sources are weighted based on their
	// position, and multiple sources can be added together in the same entry.
	void QueueEntry::Add(Point position)
	{
		// A distance of 500 counts as 1 OpenAL unit of distance.
		position *= .002;
		// To avoid having sources at a distance of 0 be infinitely loud, have
		// the minimum distance be 1 unit away.
		double d = 1. / (1. + position.Dot(position));
		sum += d * position;
		weight += d;
	}
	
	
	
	// Combine two queue entries.
	void QueueEntry::Add(const QueueEntry &other)
	{
		sum += other.sum;
		weight += other.weight;
	}
	
	
	
	// This is a wrapper for an OpenAL audio source.
	Source::Source(const Sound *sound, unsigned source)
		: sound(sound), source(source)
	{
		// Give each source a small, random pitch variation. Otherwise, multiple
		// instances of the same sound playing at slightly different times
		// overlap and create a "grinding" interference sound.
		alSourcef(source, AL_PITCH, 1. + (Random::Real() - Random::Real()) * .04);
		alSourcef(source, AL_GAIN, 1.);
		alSourcef(source, AL_REFERENCE_DISTANCE, 1.);
		alSourcef(source, AL_ROLLOFF_FACTOR, 1.);
		alSourcef(source, AL_MAX_DISTANCE, 100.);
		alSourcei(source, AL_LOOPING, sound->IsLooping());
		alSourcei(source, AL_BUFFER, sound->Buffer());
	}
	
	
	
	// Reposition this source based on the given entry in a sound queue.
	void Source::Move(const QueueEntry &entry) const
	{
		Point angle = entry.sum / entry.weight;
		// The source should be along the vector (angle.X(), angle.Y(), 1).
		// The length of the vector should be sqrt(1 / weight).
		double scale = sqrt(1. / (entry.weight * (angle.LengthSquared() + 1.)));
		alSource3f(source, AL_POSITION, angle.X() * scale, angle.Y() * scale, scale);
	}
	
	
	
	// Get the OpenAL ID for this source.
	unsigned Source::ID() const
	{
		return source;
	}
	
	
	
	// Get the sound resource currently being played by this source.
	const Sound *Source::GetSound() const
	{
		return sound;
	}
	
	
	
	// Thread entry point for loading sounds.
	void Load()
	{
		set<string> loaded;
		
		string path;
		while(true)
		{
			{
				unique_lock<mutex> lock(audioMutex);
				// If this is not the first time through, remove the previous item
				// in the queue. This is a signal that it has been loaded, so we
				// must not remove it until after loading the file.
				if(!path.empty() && !loadQueue.empty())
					loadQueue.pop_back();
				if(loadQueue.empty())
					return;
				path = loadQueue.back();
			}
			
			// Unlock the mutex for the time-intensive part of the loop.
			string name = Name(path);
			if(!name.empty() && !loaded.count(name))
			{
				loaded.insert(name);
				sounds[name].Load(path);
			}
		}
	}
	
	
	
	// Convert a full file path into a name. The name is the path relative to
	// the "sounds" directory, but not including the ".wav" extension or the "~"
	// looping indicator.
	string Name(const string &path)
	{
		if(path.length() < 4 || path.compare(path.length() - 4, 4, ".wav"))
			return string();
		
		// Only take the end of the path (after "sounds/").
		size_t start = path.rfind("sounds/");
		if(start == string::npos)
			return string();
		start += 7;
		
		// Remove the extension and the "~", if any.
		size_t end = path.length() - 4;
		if(path[end - 1] == '~')
			--end;
		
		return path.substr(start, end - start);
	}
}
