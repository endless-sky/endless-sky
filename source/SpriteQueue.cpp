/* SpriteQueue.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SpriteQueue.h"

#include "ImageBuffer.h"
#include "ImageSet.h"
#include "Mask.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include <algorithm>
#include <functional>

using namespace std;



// Constructor, which allocates worker threads.
SpriteQueue::SpriteQueue()
{
	threads.resize(max(4u, thread::hardware_concurrency()));
	for(thread &t : threads)
		t = thread(ref(*this));
}



// Destructor, which waits for all worker threads to wrap up.
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
void SpriteQueue::Add(const shared_ptr<ImageSet> &images)
{
	{
		lock_guard<mutex> lock(readMutex);
		// Do nothing if we are destroying the queue already.
		if(added < 0)
			return;

		toRead.push(images);
		++added;
	}
	readCondition.notify_one();
}



// Unload the texture for the given sprite (to free up memory).
void SpriteQueue::Unload(const string &name)
{
	unique_lock<mutex> lock(loadMutex);
	toUnload.push(name);
}



// Determine the fraction of sprites uploaded to the GPU.
double SpriteQueue::GetProgress() const
{
	// Wait until we have completed loading of as many sprites as we have added.
	// The value of "added" is protected by readMutex.
	unique_lock<mutex> readLock(readMutex);
	// Special cases: we're bailing out, or we are done.
	if(added <= 0 || added == completed)
		return 1.;
	return static_cast<double>(completed) / static_cast<double>(added);
}



void SpriteQueue::UploadSprites()
{
	unique_lock<mutex> lock(loadMutex);
	DoLoad(lock);
}



// Finish loading.
void SpriteQueue::Finish()
{
	// Loop until done loading.
	while(true)
	{
		unique_lock<mutex> lock(loadMutex);

		// Load whatever is already queued up for loading.
		DoLoad(lock);
		if(GetProgress() == 1.)
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

			// Extract the one item we should work on reading right now.
			shared_ptr<ImageSet> imageSet = toRead.front();
			toRead.pop();

			// It's now safe to add to the lists.
			lock.unlock();

			// Load the sprite.
			// TODO: investigate catching exceptions from Load() (e.g. bad_alloc), to enable
			// the UI thread to display a message prior to terminating the process.
			imageSet->Load();

			{
				// The texture must be uploaded to OpenGL in the main thread.
				unique_lock<mutex> lock(loadMutex);
				toLoad.push(imageSet);
			}
			loadCondition.notify_one();

			lock.lock();
		}

		readCondition.wait(lock);
	}
}



void SpriteQueue::DoLoad(unique_lock<mutex> &lock)
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
		// Extract the one item we should work on uploading right now.
		shared_ptr<ImageSet> imageSet = toLoad.front();
		toLoad.pop();

		// It's now safe to modify the lists.
		lock.unlock();

		imageSet->Upload(SpriteSet::Modify(imageSet->Name()));

		lock.lock();
		++completed;
	}
}
