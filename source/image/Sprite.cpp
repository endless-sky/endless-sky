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

#include "../text/Format.h"
#include "ImageBuffer.h"
#include "../Logger.h"
#include "../Preferences.h"

#include "../opengl.h"

#include <SDL2/SDL.h>

using namespace std;

namespace {
	void AddBuffer(const string &name, ImageBuffer &buffer, uint32_t *target, bool noReduction)
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

		// Use linear interpolation (if not disabled in preferences) and no wrapping.
		int filter = Preferences::Has("Texture filtering") ? GL_LINEAR : GL_NEAREST;
		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if(type == GL_TEXTURE_3D)
			glTexParameteri(type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

#ifndef ES_GLES
		// Check if the texture can be uploaded by using a proxy target,
		// and shrink the buffer if the current texture size is too large.
		unsigned forcedShrinkCount = 0;
		int proxySuccess = 0;
		int proxyType = type == GL_TEXTURE_3D ? GL_PROXY_TEXTURE_3D : GL_PROXY_TEXTURE_2D_ARRAY;
		auto doProxyCheck = [&proxySuccess, proxyType, &buffer]() -> void {
			glTexImage3D(proxyType, 0, GL_RGBA8, // target, mipmap level, internal format,
				buffer.Width(), buffer.Height(), buffer.Frames(), // width, height, depth,
				0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // border, input format, data type, data.
			glGetTexLevelParameteriv(proxyType, 0, GL_TEXTURE_WIDTH, &proxySuccess);
		};
		doProxyCheck();
		while(!proxySuccess)
		{
			// TODO: We can implement a separate algorithm for shrinking by arbitrary factors,
			// but the main use case is, considering the common sprite sizes in current vanilla data,
			// weak and old hardware, where highest quality isn't to be expected anyway.
			if(!buffer.ShrinkToHalfSize())
				// Since the proxy upload failed and we can't shrink any more, give up.
				break;
			++forcedShrinkCount;
			doProxyCheck();
		}

		if(proxySuccess)
		{
			if(forcedShrinkCount)
				Logger::Log("Shrank sprite \"" + name + "\" " + Format::SimplePluralization(forcedShrinkCount, "time")
					+ " to fit within hardware limits. The new dimensions: W = " + to_string(buffer.Width())
					+ ", H = " + to_string(buffer.Height()) + '.', Logger::Level::INFO);
#endif
			// Upload the image data.
			glTexImage3D(type, 0, GL_RGBA8, // target, mipmap level, internal format,
				buffer.Width(), buffer.Height(), buffer.Frames(), // width, height, depth,
				0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.Pixels()); // border, input format, data type, data.
#ifndef ES_GLES
		}
		else
			Logger::Log("Sprite \"" + name + "\" could not be uploaded to the GPU. Dimensions: W = "
				+ to_string(buffer.Width()) + ", H = " + to_string(buffer.Height())
				+ ", D = " + to_string(buffer.Frames()) + '.', Logger::Level::WARNING);
#endif

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



void Sprite::LoadDimensions(const ImageBuffer &buffer)
{
	width = buffer.Width();
	height = buffer.Height();
	frames = buffer.Frames();
}



bool Sprite::HasDimensions() const
{
	return width != 0 && height != 0 && frames != 0;
}




// Add the given frames, optionally uploading them. The given buffers will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction)
{
	isLoaded = true;
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
		AddBuffer(name, buffer2x, &texture, noReduction);
		buffer1x.Clear();
	}
	else
		AddBuffer(name, buffer1x, &texture, noReduction);
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
		AddBuffer(name, buffer2x, &swizzleMask, noReduction);
		buffer1x.Clear();
	}
	else
		AddBuffer(name, buffer1x, &swizzleMask, noReduction);
}



bool Sprite::IsLoaded() const
{
	return isLoaded;
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
	isLoaded = false;
	// Dimension and frame information is retained.
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
