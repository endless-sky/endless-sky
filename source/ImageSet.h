/* ImageSet.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef IMAGE_SET_H_
#define IMAGE_SET_H_

#include "ImageBuffer.h"

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
	static bool IsImage(const std::string &path);
	// Get the base name for the given path. The path should be relative to one
	// of the source image directories, not a full filesystem path.
	static std::string Name(const std::string &path);
	// Get the frame index from the given path.
	static int FrameIndex(const std::string &path);
	// Determine whether the given path is to an @2x image.
	static bool Is2x(const std::string &path);
	// Determine whether the given path or name is for a sprite whose loading
	// should be deferred until needed.
	static bool IsDeferred(const std::string &path);
	// Determine whether the given path or name is to a sprite for which a
	// collision mask ought to be generated.
	static bool IsMasked(const std::string &path);
	
	
public:
	// Constructor, optionally specifying the name (for image sets like the
	// plugin icons, whose name can't be determined from the path names).
	ImageSet(const std::string &name = "");
	
	// Get the name of the sprite for this image set.
	const std::string &Name() const;
	// Add a single image to this set. Assume the name of the image has already
	// been checked to make sure it belongs in this set.
	void Add(const std::string &path);
	// Check this image set to determine whether any frames are missing. Report
	// an error for each missing frame. (It will be left uninitialized.)
	void Check() const;
	// Load all the frames. This should be called in one of the image-loading
	// worker threads. This also generates collision masks if needed.
	void Load();
	// Create the sprite and upload the image data to the GPU. After this is
	// called, the internal image buffers and mask vector will be cleared, but
	// the paths are saved in case the sprite needs to be loaded again.
	void Upload(Sprite *sprite);
	
	
private:
	// Name of the sprite that will be initialized with these images.
	std::string name;
	// Paths to all the images that must be loaded:
	std::vector<std::string> paths[2];
	// Data loaded from the images:
	ImageBuffer buffer[2];
	std::vector<Mask> masks;
};



#endif
