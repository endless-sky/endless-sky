/* SpriteLoadManager.h
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SpriteLoadManager.h"

#include "ImageSet.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "../TaskQueue.h"

#include <atomic>
#include <map>
#include <queue>
#include <set>

using namespace std;

namespace {
	bool preventSpriteUpload = false;

	// Tracks the progress of loading the sprites when the game starts.
	std::atomic<bool> queuedAllImages = false;
	std::atomic<int> spritesLoaded = 0;
	std::atomic<int> totalSprites = 0;

	// List of image sets that are waiting to be uploaded to the GPU or minimally loaded.
	mutex imageQueueMutex;
	queue<shared_ptr<ImageSet>> imageQueue;

	map<const Sprite *, shared_ptr<ImageSet>> deferred;
	map<const Sprite *, int> preloadedLandscapes;
	map<const Sprite *, int> loadedStellarObjects;
	set<const Sprite *> loadedThumbnails;
	set<const Sprite *> loadedScenes;
	// The maximum number of sprites of varying types that
	// can be loaded at once before old sprites start being unloaded.
	const int LANDSCAPE_LIMIT = 20;
	const int STELLAR_OBJECT_LIMIT = 100;

	// Loads a sprite and queues it for upload to the GPU.
	void UnloadSprite(TaskQueue &queue, const Sprite *sprite)
	{
		// Don't unload sprites if they were never uploaded to begin with.
		if(preventSpriteUpload)
			return;
		// Unloading needs to be queued on the main thread.
		queue.Run({}, [name = sprite->Name()] { SpriteSet::Modify(name)->Unload(); });
	}

	void LoadSpriteQueued(TaskQueue &queue, const shared_ptr<ImageSet> &image);
	// Loads a sprite from the image queue, recursively.
	void LoadSpriteQueued(TaskQueue &queue)
	{
		if(imageQueue.empty())
			return;

		// Start loading the next image in the list.
		// This is done to save memory on startup.
		LoadSpriteQueued(queue, imageQueue.front());
		imageQueue.pop();
	}

	// Loads a sprite from the given image, with progress tracking.
	// Recursively loads the next image in the queue, if any.
	void LoadSpriteQueued(TaskQueue &queue, const shared_ptr<ImageSet> &image)
	{
		// Deferred images are only minimally loaded.
		Sprite *sprite = SpriteSet::Modify(image->Name());
		if(deferred.contains(sprite))
		{
			queue.Run([image, sprite] { image->MinimalLoad(sprite); },
				[&queue]
				{
					++spritesLoaded;
					// Start loading the next image in the queue, if any.
					lock_guard lock(imageQueueMutex);
					LoadSpriteQueued(queue);
				});
		}
		else
		{
			queue.Run([image] { image->Load(); },
				[image, sprite, &queue]
				{
					image->Upload(sprite, !preventSpriteUpload);
					++spritesLoaded;

					// Start loading the next image in the queue, if any.
					lock_guard lock(imageQueueMutex);
					LoadSpriteQueued(queue);
				});
		}
	}
}



void SpriteLoadManager::Init(TaskQueue &queue, map<string, shared_ptr<ImageSet>> images)
{
	// From the name, strip out any frame number, plus the extension.
	for(auto &[name, imageSet] : images)
	{
		// This should never happen, but just in case:
		if(!imageSet)
			continue;

		// Reduce the set of images to those that are valid.
		imageSet->ValidateFrames();
		// Keep track of which images should use deferred loading.
		if(ImageSet::IsDeferred(name))
			deferred[SpriteSet::Get(name)] = imageSet;
		lock_guard lock(imageQueueMutex);
		imageQueue.push(std::move(std::move(imageSet)));
		++totalSprites;
	}
	queuedAllImages = true;

	// Launch the tasks to actually load the images, making sure not to exceed the amount
	// of tasks the main thread can handle in a single frame to limit peak memory usage.
	{
		lock_guard lock(imageQueueMutex);
		for(int i = 0; i < TaskQueue::MAX_SYNC_TASKS; ++i)
			LoadSpriteQueued(queue);
	}
}



void SpriteLoadManager::PreventSpriteUpload()
{
	preventSpriteUpload = true;
}



double SpriteLoadManager::Progress()
{
	double spriteProgress = 0.;
	if(queuedAllImages)
	{
		if(!totalSprites)
			spriteProgress = 1.;
		else
			spriteProgress = static_cast<double>(spritesLoaded) / totalSprites;
	}
	return spriteProgress;
}



void SpriteLoadManager::LoadSprite(TaskQueue &queue, const shared_ptr<ImageSet> &image)
{
	queue.Run([image] { image->Load(); },
		[image] { image->Upload(SpriteSet::Modify(image->Name()), !preventSpriteUpload); });
}



bool SpriteLoadManager::IsDeferred(const Sprite *sprite)
{
	return deferred.contains(sprite);
}



void SpriteLoadManager::PreloadLandscape(TaskQueue &queue, const Sprite *sprite)
{
	// Make sure this sprite actually is one that uses deferred loading.
	auto dit = deferred.find(sprite);
	if(!sprite || dit == deferred.end())
		return;

	// If this sprite is one of the currently loaded ones, there is no need to
	// load it again. But, make note of the fact that it is the most recently
	// asked-for sprite.
	map<const Sprite *, int>::iterator pit = preloadedLandscapes.find(sprite);
	if(pit != preloadedLandscapes.end())
	{
		for(pair<const Sprite * const, int> &it : preloadedLandscapes)
			if(it.second < pit->second)
				++it.second;

		pit->second = 0;
		return;
	}

	// This sprite is not currently preloaded. Check to see whether we already
	// have the maximum number of sprites loaded, in which case the oldest one
	// must be unloaded to make room for this one.
	pit = preloadedLandscapes.begin();
	while(pit != preloadedLandscapes.end())
	{
		++pit->second;
		if(pit->second >= LANDSCAPE_LIMIT)
		{
			UnloadSprite(queue, pit->first);
			pit = preloadedLandscapes.erase(pit);
		}
		else
			++pit;
	}

	// Now, load all the files for this sprite.
	preloadedLandscapes[sprite] = 0;
	LoadSprite(queue, dit->second);
}



void SpriteLoadManager::LoadStellarObject(TaskQueue &queue, const Sprite *sprite, bool skipCulling)
{
	// Make sure this sprite actually is one that uses deferred loading.
	auto dit = deferred.find(sprite);
	if(!sprite || dit == deferred.end())
		return;

	// If this sprite is one of the currently loaded ones, there is no need to
	// load it again. But, make note of the fact that it is the most recently
	// asked-for sprite, unless culling is being skipped.
	map<const Sprite *, int>::iterator pit = loadedStellarObjects.find(sprite);
	if(pit != loadedStellarObjects.end())
	{
		if(skipCulling)
			return;
		for(pair<const Sprite * const, int> &it : loadedStellarObjects)
			if(it.second < pit->second)
				++it.second;

		pit->second = 0;
		return;
	}

	// This sprite is not currently loaded. Check to see whether we already
	// have the maximum number of sprites loaded, in which case the oldest one
	// must be unloaded to make room for this one.
	pit = loadedStellarObjects.begin();
	while(pit != loadedStellarObjects.end() && !skipCulling)
	{
		++pit->second;
		if(pit->second >= STELLAR_OBJECT_LIMIT)
		{
			UnloadSprite(queue, pit->first);
			pit = loadedStellarObjects.erase(pit);
		}
		else
			++pit;
	}

	// Now, load all the files for this sprite. If this sprite is being loaded from a panel
	// that skips culling, then set the sprite to be culled as soon as culling isn't being
	// skipped anymore.
	loadedStellarObjects[sprite] = skipCulling ? STELLAR_OBJECT_LIMIT : 0;
	LoadSprite(queue, dit->second);
}



void SpriteLoadManager::LoadThumbnail(TaskQueue &queue, const Sprite *sprite)
{
	// Make sure this sprite actually is one that uses deferred loading.
	auto dit = deferred.find(sprite);
	if(!sprite || dit == deferred.end())
		return;

	if(!loadedThumbnails.insert(sprite).second)
		return;

	LoadSprite(queue, dit->second);
}



void SpriteLoadManager::UnloadThumbnails(TaskQueue &queue)
{
	for(const Sprite *sprite : loadedThumbnails)
		UnloadSprite(queue, sprite);
	loadedThumbnails.clear();
}



void SpriteLoadManager::LoadScene(TaskQueue &queue, const Sprite *sprite)
{
	// Make sure this sprite actually is one that uses deferred loading.
	auto dit = deferred.find(sprite);
	if(!sprite || dit == deferred.end())
		return;

	if(!loadedScenes.insert(sprite).second)
		return;

	LoadSprite(queue, dit->second);
}



void SpriteLoadManager::UnloadScenes(TaskQueue &queue)
{
	for(const Sprite *sprite : loadedScenes)
		UnloadSprite(queue, sprite);
	loadedScenes.clear();
}
