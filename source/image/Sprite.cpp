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



// Add the given frames, optionally uploading them. The given buffer will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer, bool is2x, bool noReduction)
{
	// If this is the 1x image, its dimensions determine the sprite's size.
	if(!is2x)
	{
		width = buffer.Width();
		height = buffer.Height();
		frames = buffer.Frames();
	}

	// Only non-empty buffers need to be added to the sprite.
	if(buffer.Pixels())
		AddBuffer(buffer, &texture[is2x], noReduction);
}



// Upload the given frames. The given buffer will be cleared afterwards.
void Sprite::AddSwizzleMaskFrames(ImageBuffer &buffer, bool is2x, bool noReduction)
{
	// Do nothing if the buffer is empty.
	if(!buffer.Pixels())
		return;

	AddBuffer(buffer, &swizzleMask[is2x], noReduction);
}



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	if(texture[0] || texture[1])
	{
		glDeleteTextures(2, texture);
		texture[0] = texture[1] = 0;
	}

	if(swizzleMask[0] || swizzleMask[1])
	{
		glDeleteTextures(2, swizzleMask);
		swizzleMask[0] = swizzleMask[1] = 0;
	}

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
	return Texture(Screen::IsHighResolution());
}



// Get the index of the texture for the given high DPI mode.
uint32_t Sprite::Texture(bool isHighDPI) const
{
	return (isHighDPI && texture[1]) ? texture[1] : texture[0];
}



// Get the texture index, based on whether the screen is high DPI or not.
uint32_t Sprite::SwizzleMask() const
{
	return SwizzleMask(Screen::IsHighResolution());
}



// Get the index of the texture for the given high DPI mode.
uint32_t Sprite::SwizzleMask(bool isHighDPI) const
{
	return (isHighDPI && swizzleMask[1]) ? swizzleMask[1] : swizzleMask[0];
}
