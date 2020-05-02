/* ImageSet.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ImageSet.h"

#include "Files.h"
#include "Mask.h"
#include "Sprite.h"

using namespace std;

namespace {
	// Check if the given character is a valid blending mode.
	bool IsBlend(char c)
	{
		return (c == '-' || c == '~' || c == '+' || c == '=');
	}
	
	// Get the character index where the sprite name in the given path ends.
	size_t NameEnd(const string &path)
	{
		// The path always ends in a three-letter extension, ".png" or ".jpg".
		// In addition, 3 more characters may be taken up by an @2x label.
		size_t end = path.length() - (ImageSet::Is2x(path) ? 7 : 4);
		// This should never happen, but just in case:
		if(!end)
			return 0;
		
		// Skip any numbers at the end of the name.
		size_t pos = end;
		while(--pos)
			if(path[pos] < '0' || path[pos] > '9')
				break;
		
		// If there is not a blending mode specifier before the numbers, they
		// are part of the sprite name, not a frame index.
		return (IsBlend(path[pos]) ? pos : end);
	}
}



// Check if the given path is to an image of a valid file type.
bool ImageSet::IsImage(const string &path)
{
	if(path.length() < 4)
		return false;
	
	string ext = path.substr(path.length() - 4);
	return (ext == ".png" || ext == ".jpg" || ext == ".PNG" || ext == ".JPG");
}



// Get the base name for the given path. The path should be relative to one
// of the source image directories, not a full filesystem path.
string ImageSet::Name(const string &path)
{
	return path.substr(0, NameEnd(path));
}



// Get the frame index from the given path.
int ImageSet::FrameIndex(const string &path)
{
	// Get the character index where the "name" portion of the path ends.
	// A path's format is always: <name>(<blend><frame>)(@2x).(png|jpg)
	size_t i = NameEnd(path);
	
	// If the name contains a frame index, it must be separated from the name
	// by a character indicating the additive blending mode.
	if(!IsBlend(path[i]))
		return 0;
	
	int frame = 0;
	// The path ends in an extension, so there's no need to check for going off
	// the end of the string in this loop; we're guaranteed to hit a non-digit.
	for(++i; path[i] >= '0' && path[i] <= '9'; ++i)
		frame = (frame * 10) + (path[i] - '0');
	
	return frame;
}



// Determine whether the given path is to an @2x image.
bool ImageSet::Is2x(const string &path)
{
	if(path.length() < 7)
		return false;
	
	size_t pos = path.length() - 7;
	return (path[pos] == '@' && path[pos + 1] == '2' && path[pos + 2] == 'x');
}



// Determine whether the given path or name is for a sprite whose loading
// should be deferred until needed.
bool ImageSet::IsDeferred(const string &path)
{
	if(path.length() >= 5 && !path.compare(0, 5, "land/"))
		return true;
	
	return false;
}



// Determine whether the given path or name is to a sprite for which a
// collision mask ought to be generated.
bool ImageSet::IsMasked(const string &path)
{
	if(path.length() >= 5 && !path.compare(0, 5, "ship/"))
		return true;
	if(path.length() >= 9 && !path.compare(0, 9, "asteroid/"))
		return true;
	
	return false;
}



// Constructor, optionally specifying the name (for image sets like the
// plugin icons, whose name can't be determined from the path names).
ImageSet::ImageSet(const string &name)
	: name(name)
{
}



// Get the name of the sprite for this image set.
const string &ImageSet::Name() const
{
	return name;
}



// Add a single image to this set. Assume the name of the image has already
// been checked to make sure it belongs in this set.
void ImageSet::Add(const string &path)
{
	// Determine which frame of the sprite this image will be.
	bool is2x = Is2x(path);
	size_t frame = FrameIndex(path);
	
	// Allocate the string to store the path in, if necessary.
	if(paths[is2x].size() <= frame)
		paths[is2x].resize(frame + 1);
	
	// Store the path to this frame of the sprite.
	paths[is2x][frame] = path;
}



// Check this image set to determine whether any frames are missing. Report
// an error for each missing frame. (It will be left uninitialized.)
void ImageSet::Check() const
{
	string prefix = "Sprite \"" + name + "\": ";
	if(paths[1].size() > paths[0].size())
		Files::LogError(prefix + to_string(paths[1].size() - paths[0].size())
				+ " extra frames for the @2x sprite will be ignored.");
	
	for(size_t i = 0; i < paths[0].size(); ++i)
	{
		if(paths[0][i].empty())
			Files::LogError(prefix + "missing frame " + to_string(i) + ".");
		
		if(!paths[1].empty() && (i >= paths[1].size() || paths[1][i].empty()))
			Files::LogError(prefix + "missing @2x frame " + to_string(i) + ".");
	}
}



// Load all the frames. This should be called in one of the image-loading
// worker threads. This also generates collision masks if needed.
void ImageSet::Load()
{
	// Determine how many frames there will be, total. The image buffers will
	// not actually be allocated until the first image is loaded (at which point
	// the sprite's dimensions will be known).
	size_t frames = paths[0].size();
	buffer[0].Clear(frames);
	buffer[1].Clear(frames);
	
	// Check whether we need to generate collision masks.
	bool makeMasks = IsMasked(name);
	if(makeMasks)
		masks.resize(frames);
	
	// Load the 1x sprites first, then the 2x sprites, because they are likely
	// to be in separate locations on the disk. Create masks if needed.
	for(size_t i = 0; i < frames; ++i)
		if(buffer[0].Read(paths[0][i], i) && makeMasks)
			masks[i].Create(buffer[0], i);
	// Now, load the 2x sprites, if they exist. Because the number of 1x frames
	// is definitive, don't load any frames beyond the size of the 1x list.
	for(size_t i = 0; i < frames && i < paths[1].size(); ++i)
		buffer[1].Read(paths[1][i], i);
}



// Create the sprite and upload the image data to the GPU. After this is
// called, the internal image buffers and mask vector will be cleared, but
// the paths are saved in case the sprite needs to be loaded again.
void ImageSet::Upload(Sprite *sprite)
{
	// Load the frames. This will clear the buffers and the mask vector.
	sprite->AddFrames(buffer[0], false);
	sprite->AddFrames(buffer[1], true);
	sprite->AddMasks(masks);
}
