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
	
	
private:
	// This is the sprite that will be initialized from this image set.
	Sprite *sprite = nullptr;
	// Paths to all the images that must be loaded:
	std::vector<std::string> paths[2];
	// Data loaded from the images:
	ImageBuffer buffer;
	std::vector<Mask> masks;
};



#endif
