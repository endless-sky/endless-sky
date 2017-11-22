/* Sprite.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Sprite.h"

#include "ImageBuffer.h"
#include "Preferences.h"
#include "Screen.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;



Sprite::Sprite(const string &name)
	: name(name), width(0.f), height(0.f)
{
}



const string &Sprite::Name() const
{
	return name;
}



// Upload the given frames. The given buffer will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer, bool is2x)
{
	// Do nothing if the buffer is empty.
	if(!buffer.Pixels())
		return;
	
	// If this is the 1x image, its dimensions determine the sprite's size.
	if(!is2x)
	{
		width = buffer.Width();
		height = buffer.Height();
	}
	
	// Check whether this sprite is large enough to require size reduction.
	if(Preferences::Has("Reduce large graphics") && buffer.Width() * buffer.Width() >= 1000000)
		buffer.ShrinkToHalfSize();
	
	// Get a pointer to the image data.
	size_t imageSize = buffer.Width() * buffer.Height();
	const uint32_t *pixels = buffer.Pixels();
	
	// Generate the textures. Assume none of them have been allocated yet, i.e.
	// that all the textures are being uploaded in a single set here.
	textures[is2x].resize(buffer.Frames());
	for(int frame = 0; frame < buffer.Frames(); ++frame, pixels += imageSize)
	{
		glGenTextures(1, &textures[is2x][frame]);
		glBindTexture(GL_TEXTURE_2D, textures[is2x][frame]);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		// ImageBuffer always loads images into 32-bit BGRA buffers.
		// That is supposedly the fastest format to upload.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer.Width(), buffer.Height(), 0,
			GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	}
	// Unbind the texture.
	glBindTexture(GL_TEXTURE_2D, 0);
	
	// Free the ImageBuffer memory.
	buffer.Clear();
}



// Move the given masks into this sprite's internal storage. The given
// vector will be cleared.
void Sprite::AddMasks(std::vector<Mask> &masks)
{
	this->masks.swap(masks);
	masks.clear();
}



void Sprite::AddFrame(int frame, ImageBuffer *image, Mask *mask, bool is2x)
{
	if(!image || frame < 0)
		return;
	
	// If this is an @2x buffer, cut its dimensions in half. Then, if the
	// dimensions are larger than the current sprite dimensions, store them.
	width = max<float>(width, image->Width() >> is2x);
	height = max<float>(height, image->Height() >> is2x);
	
	if(textures[is2x].size() <= static_cast<unsigned>(frame))
		textures[is2x].resize(frame + 1, 0);
	if(!textures[is2x][frame])
		glGenTextures(1, &textures[is2x][frame]);
	glBindTexture(GL_TEXTURE_2D, textures[is2x][frame]);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	if(Preferences::Has("Reduce large graphics") && image->Width() * image->Height() >= 1000000)
		image->ShrinkToHalfSize();
	
	// ImageBuffer always loads images into 32-bit BGRA buffers.
	// That is supposedly the fastest format to upload.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image->Width(), image->Height(), 0,
		GL_BGRA, GL_UNSIGNED_BYTE, image->Pixels());
	
	glBindTexture(GL_TEXTURE_2D, 0);
	delete image;
	
	if(mask)
	{
		if(masks.size() <= static_cast<unsigned>(frame))
			masks.resize(frame + 1);
		masks[frame] = move(*mask);
		delete mask;
	}
}



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	for(int i = 0; i < 2; ++i)
		if(!textures[i].empty())
		{
			glDeleteTextures(textures[i].size(), textures[i].data());
			textures[i].clear();
		}
	
	masks.clear();
	width = 0.f;
	height = 0.f;
}



float Sprite::Width() const
{
	return width;
}



float Sprite::Height() const
{
	return height;
}



int Sprite::Frames() const
{
	return textures[0].size();
}



Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



uint32_t Sprite::Texture(int frame) const
{
	return Texture(frame, Screen::IsHighResolution());
}



uint32_t Sprite::Texture(int frame, bool isHighDPI) const
{
	if(!Frames() || frame < 0)
		return 0;
	
	size_t i = frame % Frames();
	if(isHighDPI && i < textures[1].size())
		return textures[i][i];
	
	return textures[0][i];
}


	
const Mask &Sprite::GetMask(int frame) const
{
	static const Mask EMPTY;
	if(frame < 0 || masks.size() != textures[0].size())
		return EMPTY;
	
	return masks[frame % masks.size()];
}
