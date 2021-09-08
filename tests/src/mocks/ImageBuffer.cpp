/* ImageBuffer.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/ImageBuffer.h"

#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace std;



ImageBuffer::ImageBuffer(int frames)
	: width(0), height(0), frames(frames), pixels(nullptr)
{
}



ImageBuffer::~ImageBuffer()
{
	Clear();
}



// Set the number of frames. This must be called before allocating.
void ImageBuffer::Clear(int frames)
{
	delete [] pixels;
	pixels = nullptr;
	this->frames = frames;
}



// Allocate the internal buffer. This must only be called once for each
// image buffer; subsequent calls will be ignored.
void ImageBuffer::Allocate(int width, int height)
{
	// Do nothing if the buffer is already allocated or if any of the dimensions
	// is set to zero.
	if(pixels || !width || !height || !frames)
		return;

	pixels = new uint32_t[width * height * frames];
	this->width = width;
	this->height = height;
}



int ImageBuffer::Width() const
{
	return width;
}



int ImageBuffer::Height() const
{
	return height;
}



int ImageBuffer::Frames() const
{
	return frames;
}



const uint32_t *ImageBuffer::Pixels() const
{
	return pixels;
}



uint32_t *ImageBuffer::Pixels()
{
	return pixels;
}



const uint32_t *ImageBuffer::Begin(int y, int frame) const
{
	return pixels + width * (y + height * frame);
}



uint32_t *ImageBuffer::Begin(int y, int frame)
{
	return pixels + width * (y + height * frame);
}



void ImageBuffer::ShrinkToHalfSize()
{
	ImageBuffer result(frames);
	result.Allocate(width / 2, height / 2);

	unsigned char *begin = reinterpret_cast<unsigned char *>(pixels);
	unsigned char *out = reinterpret_cast<unsigned char *>(result.pixels);
	// Loop through every line of every frame of the buffer.
	for(int y = 0; y < result.height * frames; ++y)
	{
		unsigned char *aIt = begin + (4 * width) * (2 * y);
		unsigned char *aEnd = aIt + 4 * 2 * result.width;
		unsigned char *bIt = begin + (4 * width) * (2 * y + 1);
		for( ; aIt != aEnd; aIt += 4, bIt += 4)
		{
			for(int channel = 0; channel < 4; ++channel, ++aIt, ++bIt, ++out)
				*out = (static_cast<unsigned>(aIt[0]) + static_cast<unsigned>(bIt[0])
					+ static_cast<unsigned>(aIt[4]) + static_cast<unsigned>(bIt[4]) + 2) / 4;
		}
	}
	swap(width, result.width);
	swap(height, result.height);
	swap(pixels, result.pixels);
}



bool ImageBuffer::Read(const string &path, int frame)
{
	return true;
}
