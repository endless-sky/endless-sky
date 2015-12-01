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
	class QueueEntry {
	public:
		void Add(Point position);
		void Add(const QueueEntry &other);
		
		Point sum;
		double weight = 0.;
	};
	
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
	
	void Load();
	string Name(const string &path);
	
	
	mutex audioMutex;
	
	ALCdevice *device = nullptr;
	ALCcontext *context = nullptr;
	double volume = .5;
	
	thread::id mainThreadID;
	map<const Sound *, QueueEntry> queue;
	map<const Sound *, QueueEntry> deferred;
	
	map<string, Sound> sounds;
	vector<Source> sources;
	vector<unsigned> recycledSources;
	vector<unsigned> endingSources;
	unsigned maxSources = 255;
	
	vector<string> loadQueue;
	thread loadThread;
	
	Point listener;
	Point listenerVelocity;
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
	
	alListener3f(AL_POSITION, 0., 0., 0.);
	alListenerf(AL_GAIN, volume * .5);
	alDopplerFactor(0.);
	
	mainThreadID = this_thread::get_id();
	
	for(const string &source : sources)
		Files::RecursiveList(source + "sounds/", &loadQueue);
	if(!loadQueue.empty())
		loadThread = thread(&Load);
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



// Get or set the volume (between 0 and 1).
double Audio::Volume()
{
	return volume;
}



void Audio::SetVolume(double level)
{
	volume = min(1., max(0., level));
	if(context)
		alListenerf(AL_GAIN, volume * .5);
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
	
	if(this_thread::get_id() == mainThreadID)
		queue[sound].Add(position - listener);
	else
	{
		unique_lock<mutex> lock(audioMutex);
		deferred[sound].Add(position - listener);
	}
}



// Begin playing all the sounds that have been added since the last time
// this function was called.
void Audio::Step()
{
	// Just to be sure, check we're in the main thread.
	if(this_thread::get_id() != mainThreadID)
		return;
	
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
			// Fade out the sound.
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
		unsigned source = 0;
		if(recycledSources.empty())
		{
			if(sources.size() >= maxSources)
				break;
			
			alGenSources(1, &source);
			if(!source)
			{
				maxSources = sources.size();
				break;
			}
		}
		else
		{
			source = recycledSources.back();
			recycledSources.pop_back();
		}
		sources.emplace_back(it.first, source);
		sources.back().Move(it.second);
		alSourcePlay(source);
	}
	queue.clear();
}



// Shut down the audio system (because we're about to quit).
void Audio::Quit()
{
	unique_lock<mutex> lock(audioMutex);
	if(!loadQueue.empty())
		loadQueue.clear();
	if(loadThread.joinable())
	{
		lock.unlock();
		loadThread.join();
		lock.lock();
	}
	
	for(const Source &source : sources)
	{
		alSourceStop(source.ID());
		ALuint id = source.ID();
		alDeleteSources(1, &id);
	}
	sources.clear();
	
	for(unsigned id : endingSources)
	{
		alSourceStop(id);
		alDeleteSources(1, &id);
	}
	endingSources.clear();
	
	for(unsigned id : recycledSources)
		alDeleteSources(1, &id);
	recycledSources.clear();
	
	for(const auto &it : sounds)
	{
		ALuint id = it.second.Buffer();
		alDeleteBuffers(1, &id);
	}
	sounds.clear();
	
	if(context)
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
	}
	if(device)
		alcCloseDevice(device);
}



namespace {
	void QueueEntry::Add(Point position)
	{
		position *= .002;
		double d = 1. / (1. + position.Dot(position));
		sum += d * position;
		weight += d;
	}
	
	
	
	void QueueEntry::Add(const QueueEntry &other)
	{
		sum += other.sum;
		weight += other.weight;
	}
	
	
	
	Source::Source(const Sound *sound, unsigned source)
		: sound(sound), source(source)
	{
		alSourcef(source, AL_PITCH, 1. + (Random::Real() - Random::Real()) * .1);
		alSourcef(source, AL_GAIN, 1);
		alSourcei(source, AL_LOOPING, sound->IsLooping());
		alSourcei(source, AL_BUFFER, sound->Buffer());
	}
	
	
	
	void Source::Move(const QueueEntry &entry) const
	{
		Point angle = entry.sum / entry.weight;
		// The source should be along the vector (angle.X(), angle.Y(), 1).
		// The length of the vector should be sqrt(1 / weight).
		double scale = sqrt(1. / (entry.weight * (angle.LengthSquared() + 1.)));
		alSource3f(source, AL_POSITION, angle.X() * scale, angle.Y() * scale, scale);
	}
	
	
	
	unsigned Source::ID() const
	{
		return source;
	}
	
	
	
	const Sound *Source::GetSound() const
	{
		return sound;
	}
	
	
	
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
			if(!name.empty() && loaded.find(name) == loaded.end())
			{
				loaded.insert(name);
				sounds[name].Load(path);
			}
		}
	}
	
	
	
	string Name(const string &path)
	{
		if(path.length() < 4 || path.compare(path.length() - 4, 4, ".wav"))
			return string();
		
		size_t start = path.rfind("sounds/");
		if(start == string::npos)
			return string();
		start += 7;
		
		size_t end = path.length() - 4;
		if(path[end - 1] == '~')
			--end;
		
		return path.substr(start, end - start);
	}
}
