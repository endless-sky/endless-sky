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
#include "Screen.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;



Sprite::Sprite()
	: width(0.f), height(0.f)
{
}



void Sprite::AddFrame(int frame, ImageBuffer *image, Mask *mask, bool is2x)
{
	if(!image || frame < 0)
		return;
	
	width = max(width, static_cast<float>(image->Width() / (1 + is2x)));
	height = max(height, static_cast<float>(image->Height() / (1 + is2x)));
	
	vector<uint32_t> &textureIndex = (is2x ? textures2x : textures);
	if(textureIndex.size() <= static_cast<unsigned>(frame))
		textureIndex.resize(frame + 1, 0);
	if(!textureIndex[frame])
		glGenTextures(1, &textureIndex[frame]);
	glBindTexture(GL_TEXTURE_2D, textureIndex[frame]);
	
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



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	if(!textures.empty())
	{
		glDeleteTextures(textures.size(), &textures.front());
		textures.clear();
	}
	if(!textures2x.empty())
	{
		glDeleteTextures(textures2x.size(), &textures2x.front());
		textures2x.clear();
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
	return textures.size();
}



Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



uint32_t Sprite::Texture(int frame) const
{
	if(Screen::IsHighResolution() && !textures2x.empty())
		return textures2x[frame % textures2x.size()];
	
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
