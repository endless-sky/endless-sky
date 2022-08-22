/* SoundQueue.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SoundQueue.h"

#include "Logger.h"
#include "Sound.h"
#include "SoundSet.h"

#include <algorithm>

using namespace std;



// Constructor, which allocates worker threads.
SoundQueue::SoundQueue(SoundSet &sounds)
	: sounds(sounds)
{
	threads.resize(2u);
	for(thread &t : threads)
		t = thread(ref(*this));
}



// Destructor, which waits for all worker threads to wrap up.
SoundQueue::~SoundQueue()
{
	{
		lock_guard<mutex> lock(readMutex);
		added = -1;
	}
	readCondition.notify_all();
	for(thread &t : threads)
		t.join();
}



// Add a sound to load.
void SoundQueue::Add(Item &&item)
{
	{
		lock_guard<mutex> lock(readMutex);
		// Do nothing if we are destroying the queue already.
		if(added < 0)
			return;

		toRead.push(std::move(item));
		++added;
	}
	readCondition.notify_one();
}



// Determine the fraction of sounds loaded by OpenAL.
double SoundQueue::GetProgress() const
{
	// Wait until we have completed loading of as many sounds as we have added.
	// The value of "added" is protected by readMutex.
	unique_lock<mutex> readLock(readMutex);
	// Special cases: we're bailing out, or we are done.
	if(added <= 0 || added == completed)
		return 1.;
	return static_cast<double>(completed) / static_cast<double>(added);
}



// Thread entry point.
void SoundQueue::operator()()
{
	while(true)
	{
		unique_lock<mutex> lock(readMutex);
		while(true)
		{
			// To signal this thread that it is time for it to quit, we set
			// "added" to -1.
			if(added < 0)
				return;
			if(toRead.empty())
				break;

			// Extract the one item we should work on reading right now.
			Item sound = std::move(toRead.front());
			toRead.pop();

			// It's now safe to add to the set.
			lock.unlock();

			// Load the sound file.
			if(!sounds.Modify(sound.name)->Load(sound.path, sound.name))
				Logger::LogError("Unable to load sound \"" + sound.name + "\" from path: " + sound.path);

			lock.lock();
			++completed;
		}

		readCondition.wait(lock);
	}
}
