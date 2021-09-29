/* Sprite.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/Sprite.h"

#include <string>
#include <vector>

using namespace std;



Sprite::Sprite(const string &name)
	: name(name)
{
}



const string &Sprite::Name() const
{
	return name;
}



// Upload the given frames. The given buffer will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer, bool is2x)
{
}



// Move the given masks into this sprite's internal storage. The given
// vector will be cleared.
void Sprite::AddMasks(vector<Mask> &masks)
{
	this->masks.swap(masks);
	masks.clear();
}



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	texture[0] = texture[1] = 0;
	
	masks.clear();
	width = 0.f;
	height = 0.f;
	frames = 0;
}



// Get the width, in pixels, of the 1x image.
float Sprite::Width() const
{
	return width;
}



// Get the height, in pixels, of the 1x image.
float Sprite::Height() const
{
	return height;
}



// Get the number of frames in the animation.
int Sprite::Frames() const
{
	return frames;
}



// Get the offset of the center from the top left corner; this is for easy
// shifting of corner to center coordinates.
Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



// Get the texture index, based on whether the screen is high DPI or not.
uint32_t Sprite::Texture() const
{
	return 0;
}



// Get the index of the texture for the given high DPI mode.
uint32_t Sprite::Texture(bool isHighDPI) const
{
	return (isHighDPI && texture[1]) ? texture[1] : texture[0];
}



// Get the collision mask for the given frame of the animation.
const Mask &Sprite::GetMask(int frame) const
{
	static const Mask EMPTY;
	if(frame < 0 || masks.empty())
		return EMPTY;
	
	// Assume that if a masks array exists, it has the right number of frames.
	return masks[frame % masks.size()];
}
