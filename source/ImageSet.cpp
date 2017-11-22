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

using namespace std;

namespace {
	// Check if the given character is a valid blending mode.
	bool IsBlend(char c)
	{
		return (c == '-' || c == '~' || c == '+' || c == '=');
	}
	
	// Get the character index where the sprite name in the given path ends.
	size_t NameEnd(const std::string &path)
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
bool ImageSet::IsDeferred(const std::string &path)
{
	if(path.length() >= 5 && !path.compare(0, 5, "land/"))
		return true;
	
	return false;
}



// Determine whether the given path or name is to a sprite for which a
// collision mask ought to be generated.
bool ImageSet::IsMasked(const std::string &path)
{
	if(path.length() >= 5 && !path.compare(0, 5, "ship/"))
		return true;
	if(path.length() >= 9 && !path.compare(0, 9, "asteroid/"))
		return true;
	
	return false;
}
