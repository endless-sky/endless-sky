/* SpriteQueue.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SPRITE_QUEUE_H_
#define SPRITE_QUEUE_H_

#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class ImageBuffer;
class Mask;
class Sprite;



// Class for queuing up a list of sprites to be loaded from the disk, with a set of
// worker threads that begins loading them as soon as they are added.
class SpriteQueue {
public:
	SpriteQueue();
	~SpriteQueue();
	
	// Add a sprite to load.
	void Add(const std::string &name, const std::string &path);
	// Unload the texture for the given sprite (to free up memory).
	void Unload(const std::string &name);
	// Find out our percent completion.
	double Progress() const;
	// Finish loading.
	void Finish() const;
	
	// Thread entry point.
	void operator()();
	
	
private:
	double DoLoad(std::unique_lock<std::mutex> &lock) const;
	
	
private:
	class Item {
	public:
		Item(Sprite *sprite, const std::string &name, const std::string &path, int frame, bool is2x);
		
		Sprite *sprite;
		std::string name;
		std::string path;
		ImageBuffer *image;
		Mask *mask;
		int frame;
		bool is2x;
	};
	
	
private:
	std::queue<Item> toRead;
	// We must read the value of "added" in const functions, so this mutex must
	// be mutable.
	mutable std::mutex readMutex;
	std::condition_variable readCondition;
	int added;
	std::map<std::string, int> count;
	std::map<std::string, int> count2x;
	
	mutable std::queue<Item> toLoad;
	mutable std::mutex loadMutex;
	mutable std::condition_variable loadCondition;
	mutable int completed;
	
	mutable std::queue<std::string> toUnload;
	
	std::vector<std::thread> threads;
};

#endif
