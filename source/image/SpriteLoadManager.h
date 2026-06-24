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

#pragma once

#include <map>
#include <memory>
#include <string>

class ImageSet;
class PlayerInfo;
class Sprite;
class System;
class TaskQueue;


// The class responsible for loading sprites at the start of the game, and
// for managing the loading and unloading of sprites that use deferred loading.
class SpriteLoadManager {
public:
	static void Init(TaskQueue &queue, std::map<std::string, std::shared_ptr<ImageSet>> images);
	static void PreventSpriteUpload();
	static void FindDeferredFolders();
	static double Progress();

	// Load an individual sprite in full.
	static void LoadSprite(TaskQueue &queue, const std::shared_ptr<ImageSet> &image);

	// Determine whether the given sprite uses deferred loading.
	static bool IsDeferred(const Sprite *sprite);
	// Begin loading a sprite that was previously deferred. This is done for various images to speed up
	// the program's startup and reduce VRAM usage.
	static void LoadDeferred(TaskQueue &queue, const Sprite *sprite);
	// Cull old stellar objects and thumbnails that haven't been seen in a while.
	static void CullOldImages(TaskQueue &queue);

	// Changes can be made by missions or events that cause new assets to appear.
	// When this happens, a class can signal to the SpriteLoadManager than a panel
	// needs to re-request that sprites be loaded.
	static void SetRecheckThumbnails();
	static bool RecheckThumbnails();
	static void SetRecheckStellarObjects();
	static bool RecheckStellarObjects();


private:
	// Preload a landscape image. If 20 landscape images have already been preloaded
	// previously, unload the least recently seen image.
	static void LoadLandscape(TaskQueue &queue, const Sprite *sprite, const std::shared_ptr<ImageSet> &image);
	// Load a stellar object. Stellar objects remain loaded for 100 in-game days.
	static void LoadStellarObject(TaskQueue &queue, const Sprite *sprite, const std::shared_ptr<ImageSet> &image);
	// Load a ship or outfit thumbnail. Stellar objects remain loaded for 100 in-game days.
	static void LoadThumbnail(TaskQueue &queue, const Sprite *sprite, const std::shared_ptr<ImageSet> &image);
	// Load a starting scenario, conversation, or logbook scene.
	static void LoadScene(TaskQueue &queue, const Sprite *sprite, const std::shared_ptr<ImageSet> &image);
};
