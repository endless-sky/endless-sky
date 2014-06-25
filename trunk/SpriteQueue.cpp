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

#include "Mask.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include "SDL2/SDL_image.h"

#include <iostream>
#include <vector>

using namespace std;



// TODO: Get multithreaded image loading working with SDL2.
// That might require using libjpeg and libpng directly.
SpriteQueue::SpriteQueue()
	: added(0), completed(0), threads(1)
{
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
		toRead.emplace(sprite, name, path, count[name]++);
		++added;
	}
	readCondition.notify_one();
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
			item.surface = IMG_Load(item.path.c_str());
			if(!item.surface)
				cerr << "Could not load image: \"" << item.path << "\"." << endl;
			
			// Check if the sprite uses additive blending.
			int pos = item.path.length() - 4;
			while(--pos)
				if(item.path[pos] < '0' || item.path[pos] > '9')
					break;
			int additive = (item.path[pos] == '+') ? 2 : (item.path[pos] == '~') ? 1 : 0;
			
			// Convert to premultiplied alpha, and if this is a ship or an
			// asteroid, also generate a collision mask.
			Premultiply(item.surface, additive);
			if(!item.name.compare(0, 5, "ship/") || !item.name.compare(0, 9, "asteroid/"))
			{
				item.mask = new Mask;
				item.mask->Create(item.surface);
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



void SpriteQueue::Premultiply(SDL_Surface *surface, int additive)
{
	SDL_LockSurface(surface);
	
	if(surface->format->Amask && surface->format->BytesPerPixel == 4)
	{
		for(int y = 0; y < surface->h; ++y)
		{
			uint32_t *it = reinterpret_cast<uint32_t *>(surface->pixels)
				+ (surface->pitch / 4) * y;
			
			for(uint32_t *end = it + surface->w; it != end; ++it)
			{
				uint64_t value = *it;
				uint64_t alpha = (value & surface->format->Amask);
				alpha >>= surface->format->Ashift;
				
				uint64_t red = (value & surface->format->Rmask) * alpha;
				red = (red / 255) & surface->format->Rmask;
				
				uint64_t green = (value & surface->format->Gmask) * alpha;
				green = (green / 255) & surface->format->Gmask;
				
				uint64_t blue = (value & surface->format->Bmask) * alpha;
				blue = (blue / 255) & surface->format->Bmask;
				
				value = red | green | blue;
				if(additive == 1)
					alpha >>= 2;
				if(additive != 2)
					value |= (alpha << surface->format->Ashift);
				*it = static_cast<uint32_t>(value);
			}
		}
	}
	
	SDL_UnlockSurface(surface);
}



double SpriteQueue::DoLoad(unique_lock<mutex> &lock) const
{
	for(int i = 0; !toLoad.empty() && i < 30; ++i)
	{
		Item item = toLoad.front();
		toLoad.pop();
		
		lock.unlock();
		
		item.sprite->AddFrame(item.frame, item.surface, item.mask);
		
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



SpriteQueue::Item::Item(Sprite *sprite, const string &name, const string &path, int frame)
	: sprite(sprite), name(name), path(path), surface(nullptr), mask(nullptr), frame(frame)
{
}
