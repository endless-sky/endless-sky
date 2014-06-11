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

#include <algorithm>

#include <GL/glew.h>
#include <SDL/SDL.h>

using namespace std;



Sprite::Sprite()
	: width(0.f), height(0.f)
{
}



void Sprite::AddFrame(int frame, SDL_Surface *surface, Mask *mask)
{
	if(!surface || frame < 0)
		return;
	
	GLenum format;
	int bytes = surface->format->BytesPerPixel;
	if(bytes == 4)
		format = GL_RGBA;
	else if(bytes == 3)
		format = GL_RGB;
	else if(bytes == 1)
		format = GL_RED;
	else
		return;
	
	width = max(width, static_cast<float>(surface->w));
	height = max(height, static_cast<float>(surface->h));
	
	if(textures.size() <= static_cast<unsigned>(frame))
		textures.resize(frame + 1, 0);
	glGenTextures(1, &textures[frame]);
	glBindTexture(GL_TEXTURE_2D, textures[frame]);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	SDL_LockSurface(surface);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
		format, GL_UNSIGNED_BYTE, surface->pixels);
	
	SDL_UnlockSurface(surface);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	SDL_FreeSurface(surface);
	
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
