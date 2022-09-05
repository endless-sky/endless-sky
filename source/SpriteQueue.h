/* SpriteQueue.h
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

#ifndef SPRITE_QUEUE_H_
#define SPRITE_QUEUE_H_

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



// Class for queuing up a list of sprites to be loaded from the disk, with a set of
// worker threads that begins loading them as soon as they are added.
class SpriteQueue {
public:
	SpriteQueue();
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

	// Thread entry point.
	void operator()();


private:
	void DoLoad(std::unique_lock<std::mutex> &lock);


private:
	// These are the image sets that need to be loaded from disk.
	std::queue<std::shared_ptr<ImageSet>> toRead;
	mutable std::mutex readMutex;
	std::condition_variable readCondition;
	int added = 0;

	// These image sets have been loaded from disk but have not been uploaded.
	std::queue<std::shared_ptr<ImageSet>> toLoad;
	std::mutex loadMutex;
	std::condition_variable loadCondition;
	int completed = 0;

	// These sprites must be unloaded to reclaim GPU memory.
	std::queue<std::string> toUnload;

	// Worker threads for loading sprites from disk.
	std::vector<std::thread> threads;
};

#endif
