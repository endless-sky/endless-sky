/* SoundQueue.h
Copyright (c) 2022 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SOUND_QUEUE_H_
#define SOUND_QUEUE_H_

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class ImageBuffer;
class ImageSet;
class Mask;
class Sprite;
class SoundSet;



// Class for queuing up a list of sprites to be loaded from the disk, with a set of
// worker threads that begins loading them as soon as they are added.
class SoundQueue {
public:
	// A sound item used by this queue.
	struct Item {
		std::string path;
		std::string name;
	};


public:
	SoundQueue(SoundSet &sounds);
	~SoundQueue();

	// No moving or copying this class.
	SoundQueue(const SoundQueue &) = delete;
	SoundQueue(SoundQueue &&) = delete;
	SoundQueue &operator=(const SoundQueue &) = delete;
	SoundQueue &operator=(SoundQueue &&) = delete;

	// Add a sound to load.
	void Add(Item item);
	// Determine the fraction of sounds uploaded to the GPU.
	double GetProgress() const;

	// Thread entry point.
	void operator()();


private:
	SoundSet &sounds;

	// These are the sounds that need to be loaded from disk.
	std::queue<Item> toRead;
	mutable std::mutex readMutex;
	std::condition_variable readCondition;
	int added = 0;

	int completed = 0;

	// Worker threads for loading sounds from disk.
	std::vector<std::thread> threads;
};

#endif
