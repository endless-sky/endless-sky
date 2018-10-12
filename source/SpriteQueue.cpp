/* SpriteQueue.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpriteQueue.h"

#include "ImageBuffer.h"
#include "Mask.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include <algorithm>
#include <functional>

using namespace std;

namespace {
	// Check if the given image is an @2x image.
	bool Is2x(const string &path)
	{
		size_t len = path.length();
		return (len > 7 && path[len - 7] == '@' && path[len - 6] == '2' && path[len - 5] == 'x');
	}
}



SpriteQueue::SpriteQueue()
	: added(0), completed(0)
{
	threads.resize(max(4u, thread::hardware_concurrency()));
	for(thread &t : threads)
		t = thread(ref(*this));
}



SpriteQueue::~SpriteQueue()
{
	{
		lock_guard<mutex> lock(readMutex);
		added = -1;
	}
	readCondition.notify_all();
	for(thread &t : threads)
		t.join();
}



// Add a sprite to load.
void SpriteQueue::Add(const string &name, const string &path)
{
	Sprite *sprite = SpriteSet::Modify(name);
	{
		lock_guard<mutex> lock(readMutex);
		// Do nothing if we are destroying the queue already.
		if(added < 0)
			return;
		
		bool is2x = Is2x(path);
		int &frame = (is2x ? count2x[name] : count[name]);
		toRead.emplace(sprite, name, path, frame++, is2x);
		++added;
	}
	readCondition.notify_one();
}



// Unload the texture for the given sprite (to free up memory).
void SpriteQueue::Unload(const string &name)
{
	{
		lock_guard<mutex> lock(readMutex);
		count2x[name] = 0;
		count[name] = 0;
	}
	
	unique_lock<mutex> lock(loadMutex);
	toUnload.push(name);
}



// Find out our percent completion.
double SpriteQueue::Progress() const
{
	unique_lock<mutex> lock(loadMutex);
	return DoLoad(lock);
}



// Finish loading.
void SpriteQueue::Finish() const
{
	// Loop until done loading.
	while(true)
	{
		unique_lock<mutex> lock(loadMutex);
		
		// Load whatever is already queued up for loading.
		if(DoLoad(lock) == 1.)
			break;
		
		// We still have sprites to upload, but none of them have been read from
		// disk yet. Wait until one arrives.
		loadCondition.wait(lock);
	}
}



// Thread entry point.
void SpriteQueue::operator()()
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
			
			Item item = toRead.front();
			toRead.pop();
			
			lock.unlock();
			
			// Load the sprite.
			item.image = ImageBuffer::Read(item.path);
			// If sprite loading fails, just skip this sprite.
			if(!item.image)
			{
				lock.lock();
				continue;
			}
			// Don't ever create masks for @2x sprites; just use the ordinary
			// sprite masks instead.
			if(!item.is2x && (!item.name.compare(0, 5, "ship/") || !item.name.compare(0, 9, "asteroid/")))
			{
				item.mask = new Mask;
				item.mask->Create(item.image);
			}
			
			// Don't bother to copy the path, now that we've loaded the file.
			item.name.clear();
			item.path.clear();
			{
				// The texture must be uploaded to OpenGL in the main thread.
				unique_lock<mutex> lock(loadMutex);
				toLoad.push(item);
			}
			loadCondition.notify_one();
			
			lock.lock();
		}
		
		readCondition.wait(lock);
	}
}


double SpriteQueue::DoLoad(unique_lock<mutex> &lock) const
{
	while(!toUnload.empty())
	{
		Sprite *sprite = SpriteSet::Modify(toUnload.front());
		toUnload.pop();
		
		lock.unlock();
		sprite->Unload();
		lock.lock();
	}
	
	for(int i = 0; !toLoad.empty() && i < 100; ++i)
	{
		Item item = toLoad.front();
		toLoad.pop();
		
		lock.unlock();
		
		item.sprite->AddFrame(item.frame, item.image, item.mask, item.is2x);
		
		lock.lock();
		++completed;
	}
	
	// Wait until we have completed loading of as many sprites as we have added.
	// The value of "added" is protected by readMutex.
	unique_lock<mutex> readLock(readMutex);
	// Special cases: we're bailing out, or we are done.
	if(added <= 0 || added == completed)
		return 1.;
	return static_cast<double>(completed) / static_cast<double>(added);
}



SpriteQueue::Item::Item(Sprite *sprite, const string &name, const string &path, int frame, bool is2x)
	: sprite(sprite), name(name), path(path), image(nullptr), mask(nullptr), frame(frame), is2x(is2x)
{
}
