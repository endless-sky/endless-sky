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

#include <tbb/concurrent_queue.h>
#include <tbb/task_group.h>

#include <atomic>
#include <memory>
#include <string>

class ImageSet;



// Class for queuing up a list of sprites to be loaded from the disk, with a set of
// worker threads that begins loading them as soon as they are added.
class SpriteQueue {
public:
	SpriteQueue() noexcept = default;
	~SpriteQueue();

	// No moving or copying this class.
	SpriteQueue(const SpriteQueue &other) = delete;
	SpriteQueue(SpriteQueue &&other) = delete;
	SpriteQueue &operator=(const SpriteQueue &other) = delete;
	SpriteQueue &operator=(SpriteQueue &&other) = delete;

	// Add a sprite to load.
	void Add(const std::shared_ptr<ImageSet> &images);
	// Unload the texture for the given sprite (to free up memory).
	void Unload(const std::string &name);
	// Determine the fraction of sprites uploaded to the GPU.
	double GetProgress() const;
	// Uploads any available sprites to the GPU.
	void UploadSprites();
	// Finish loading.
	void Finish();


private:
	tbb::task_group tasks;

	// The total amount of images that have been added.
	std::atomic<int> added;
	// The total amount of images that have finished loading.
	int completed = 0;

	// These image sets have been loaded from disk but have not been uploaded.
	tbb::concurrent_queue<std::shared_ptr<ImageSet>> toLoad;
	// These sprites must be unloaded to reclaim GPU memory.
	tbb::concurrent_queue<std::string> toUnload;
};

#endif
