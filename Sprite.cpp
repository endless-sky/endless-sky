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

#include <algorithm>

#ifdef __APPLE__
#include <OpenGL/GL3.h>
#else
#include <GL/glew.h>
#endif

#include <SDL2/SDL.h>

#include <iostream>

using namespace std;



Sprite::Sprite()
	: width(0.f), height(0.f)
{
}



void Sprite::AddFrame(int frame, ImageBuffer *image, Mask *mask)
{
	if(!image || frame < 0)
		return;
	
	width = max(width, static_cast<float>(image->Width()));
	height = max(height, static_cast<float>(image->Height()));
	
	if(textures.size() <= static_cast<unsigned>(frame))
		textures.resize(frame + 1, 0);
	glGenTextures(1, &textures[frame]);
	glBindTexture(GL_TEXTURE_2D, textures[frame]);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
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
	return textures.size();
}



Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



uint32_t Sprite::Texture(int frame) const
{
	if(textures.empty())
		return 0;
	
	return textures[frame % textures.size()];
}


	
const Mask &Sprite::GetMask(int frame) const
{
	static const Mask empty;
	if(masks.empty() || masks.size() != textures.size())
		return empty;
	
	return masks[frame % masks.size()];
}
