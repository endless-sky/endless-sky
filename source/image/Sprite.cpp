/* Sprite.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Sprite.h"

#include "ImageBuffer.h"
#include "../Preferences.h"
#include "../Screen.h"

#include "../opengl.h"

#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;

namespace {
	void AddBuffer(ImageBuffer &buffer, uint32_t *target, bool noReduction)
	{
		// Check whether this sprite is large enough to require size reduction.
		Preferences::LargeGraphicsReduction setting = Preferences::GetLargeGraphicsReduction();
		if(!noReduction && (setting == Preferences::LargeGraphicsReduction::ALL
				|| (setting == Preferences::LargeGraphicsReduction::LARGEST_ONLY
				&& buffer.Width() * buffer.Height() >= 1000000)))
			buffer.ShrinkToHalfSize();

		// Upload the images as a single array texture.
		int type = OpenGL::HasTexture2DArraySupport() ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_3D;
		glGenTextures(1, target);
		glBindTexture(type, *target);

		// Use linear interpolation and no wrapping.
		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if(type == GL_TEXTURE_3D)
			glTexParameteri(type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// Upload the image data.
		glTexImage3D(type, 0, GL_RGBA8, // target, mipmap level, internal format,
			buffer.Width(), buffer.Height(), buffer.Frames(), // width, height, depth,
			0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.Pixels()); // border, input format, data type, data.

		// Unbind the texture.
		glBindTexture(type, 0);

		// Free the ImageBuffer memory.
		buffer.Clear();
	}
}



Sprite::Sprite(const string &name)
	: name(name)
{
}



const string &Sprite::Name() const
{
	return name;
}



// Add the given frames, optionally uploading them. The given buffers will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction)
{
	// The 1x image determines the dimensions of the sprite's size.
	width = buffer1x.Width();
	height = buffer1x.Height();
	frames = buffer1x.Frames();
	// Do nothing else if the buffer is empty.
	// (The buffer can be empty yet still have a width and height if uploading is disabled.)
	if(!buffer1x.Pixels())
		return;

	// Only use the 2x resolution image if it is provided.
	if(buffer2x.Pixels())
	{
		AddBuffer(buffer2x, &texture, noReduction);
		buffer1x.Clear();
	}
	else
		AddBuffer(buffer1x, &texture, noReduction);
}



// Upload the given frames. The given buffers will be cleared afterwards.
void Sprite::AddSwizzleMaskFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction)
{
	if(!swizzleMaskFrames)
	{
		swizzleMaskFrames = buffer1x.Frames();
		if(swizzleMaskFrames > 1 && swizzleMaskFrames < frames)
			swizzleMaskFrames = 1;
	}

	// Do nothing if the buffer is empty.
	if(!buffer1x.Pixels())
		return;

	// Only use the 2x resolution image if it is provided.
	if(buffer2x.Pixels())
	{
		AddBuffer(buffer2x, &swizzleMask, noReduction);
		buffer1x.Clear();
	}
	else
		AddBuffer(buffer1x, &swizzleMask, noReduction);
}



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	if(texture)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	if(swizzleMask)
	{
		glDeleteTextures(1, &swizzleMask);
		swizzleMask = 0;
	}

	width = 0.f;
	height = 0.f;
	frames = 0;
	swizzleMaskFrames = 0;
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



int Sprite::SwizzleMaskFrames() const
{
	return swizzleMaskFrames;
}



// Get the offset of the center from the top left corner; this is for easy
// shifting of corner to center coordinates.
Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



// Get the texture index.
uint32_t Sprite::Texture() const
{
	return texture;
}



// Get the texture index.
uint32_t Sprite::SwizzleMask() const
{
	return swizzleMask;
}
