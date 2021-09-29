/* ImageBuffer.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ImageBuffer.h"

#include "File.h"
#include "Files.h"
#include "ReadImage.h"

#include <png.h>
#include <jpeglib.h>

#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {
	void Premultiply(ImageBuffer &buffer, int frame, int additive);
}



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
	// First, make sure this is a JPG or PNG file.
	if(path.length() < 4)
		return false;
	
	string extension = path.substr(path.length() - 4);
	bool isPNG = (extension == ".png" || extension == ".PNG");
	bool isJPG = (extension == ".jpg" || extension == ".JPG");
	if(!isPNG && !isJPG)
		return false;
	
	if(isPNG && !Read::PNG(path, *this, frame))
		return false;
	if(isJPG && !Read::JPG(path, *this, frame))
		return false;
	
	// Check if the sprite uses additive blending. Start by getting the index of
	// the last character before the frame number (if one is specified).
	int pos = path.length() - 4;
	if(pos > 3 && !path.compare(pos - 3, 3, "@2x"))
		pos -= 3;
	while(--pos)
		if(path[pos] < '0' || path[pos] > '9')
			break;
	// Special case: if the image is already in premultiplied alpha format,
	// there is no need to apply premultiplication here.
	if(path[pos] != '=')
	{
		int additive = (path[pos] == '+') ? 2 : (path[pos] == '~') ? 1 : 0;
		if(isPNG || (isJPG && additive == 2))
			Premultiply(*this, frame, additive);
	}
	return true;
}



namespace {
	void Premultiply(ImageBuffer &buffer, int frame, int additive)
	{
		for(int y = 0; y < buffer.Height(); ++y)
		{
			uint32_t *it = buffer.Begin(y, frame);
			
			for(uint32_t *end = it + buffer.Width(); it != end; ++it)
			{
				uint64_t value = *it;
				uint64_t alpha = (value & 0xFF000000) >> 24;
				
				uint64_t red = (((value & 0xFF0000) * alpha) / 255) & 0xFF0000;
				uint64_t green = (((value & 0xFF00) * alpha) / 255) & 0xFF00;
				uint64_t blue = (((value & 0xFF) * alpha) / 255) & 0xFF;
				
				value = red | green | blue;
				if(additive == 1)
					alpha >>= 2;
				if(additive != 2)
					value |= (alpha << 24);
				
				*it = static_cast<uint32_t>(value);
			}
		}
	}
}
