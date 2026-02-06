/* ImageSet.h
Copyright (c) 2017 by Michael Zahniser

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

#include "ImageBuffer.h"

#include "ImageFileData.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class Mask;
class Sprite;



// An ImageSet is a collection of file paths for all the images that must be
// loaded for a given sprite, including 1x and 2x resolution variants. It also
// stores masks for any sprite for which they should be calculated.
class ImageSet {
public:
	// Check if the given path is to an image of a valid file type.
	static bool IsImage(const std::filesystem::path &path);
	// Determine whether the given path or name is for a sprite whose loading
	// should be deferred until needed.
	static bool IsDeferred(const std::filesystem::path &path);


public:
	// ImageSets should be created with a name, as some image paths (e.g. plugin icons)
	// do not contain the associated image name.
	explicit ImageSet(std::string name);

	// Get the name of the sprite for this image set.
	const std::string &Name() const;
	// Whether this image set is empty, i.e. has no images.
	bool IsEmpty() const;
	// Add a single image to this set. Assume the name of the image has already
	// been checked to make sure it belongs in this set.
	void Add(ImageFileData data);
	// Reduce all given paths to frame images into a sequence of consecutive frames.
	void ValidateFrames() noexcept(false);
	// Load all the frames. This should be called in one of the image-loading
	// worker threads. This also generates collision masks if needed.
	void Load() noexcept(false);
	// Create the sprite and optionally upload the image data to the GPU. After this is
	// called, the internal image buffers and mask vector will be cleared, but
	// the paths are saved in case the sprite needs to be loaded again.
	void Upload(Sprite *sprite, bool enableUpload);


private:
	// Name of the sprite that will be initialized with these images.
	std::string name;
	// Paths to all the images that were discovered during loading.
	std::map<std::size_t, std::filesystem::path> framePaths[4];
	// Paths that comprise a valid animation sequence of 1 or more frames.
	std::vector<std::filesystem::path> paths[4];
	// Data loaded from the images:
	ImageBuffer buffer[4];
	std::vector<Mask> masks;
	bool noReduction = false;
};
