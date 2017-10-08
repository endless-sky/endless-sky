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

#include <png.h>
#include <jpeglib.h>

#include <cstdio>
#include <vector>

using namespace std;

namespace {
	ImageBuffer *ReadPNG(const string &path);
	ImageBuffer *ReadJPG(const string &path);
	void Premultiply(ImageBuffer *buffer, int additive);
}



ImageBuffer::ImageBuffer()
	: width(0), height(0), pixels(nullptr)
{
}



ImageBuffer::ImageBuffer(int width, int height)
	: width(width), height(height)
{
	pixels = new uint32_t[width * height];
}



ImageBuffer::~ImageBuffer()
{
	delete [] pixels;
}



int ImageBuffer::Width() const
{
	return width;
}



int ImageBuffer::Height() const
{
	return height;
}


const uint32_t *ImageBuffer::Pixels() const
{
	return pixels;
}



uint32_t *ImageBuffer::Pixels()
{
	return pixels;
}



const uint32_t *ImageBuffer::Begin(int y) const
{
	return pixels + y * Width();
}



uint32_t *ImageBuffer::Begin(int y)
{
	return pixels + y * Width();
}



void ImageBuffer::ShrinkToHalfSize()
{
	ImageBuffer result(width / 2, height / 2);
	
	unsigned char *begin = reinterpret_cast<unsigned char *>(pixels);
	unsigned char *out = reinterpret_cast<unsigned char *>(result.pixels);
	for(int y = 0; y < result.height; ++y)
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



ImageBuffer *ImageBuffer::Read(const string &path)
{
	// First, make sure this is a JPG or PNG file.
	if(path.length() < 4)
		return nullptr;
	
	string extension = path.substr(path.length() - 4);
	bool isPNG = (extension == ".png" || extension == ".PNG");
	bool isJPG = (extension == ".jpg" || extension == ".JPG");
	if(!isPNG && !isJPG)
		return nullptr;
	
	ImageBuffer *buffer = isPNG ? ReadPNG(path) : ReadJPG(path);
	if(!buffer)
		return nullptr;
	
	// Check if the sprite uses additive blending.
	int pos = path.length() - 4;
	if(pos > 3 && !path.compare(pos - 3, 3, "@2x"))
		pos -= 3;
	while(--pos)
		if(path[pos] < '0' || path[pos] > '9')
			break;
	// Special case: the PNG is already premultiplied alpha.
	if(path[pos] == '=')
		return buffer;
	int additive = (path[pos] == '+') ? 2 : (path[pos] == '~') ? 1 : 0;
	
	if(isPNG || (isJPG && additive == 2))
		Premultiply(buffer, additive);
	
	return buffer;
}



namespace {
	ImageBuffer *ReadPNG(const string &path)
	{
		// Open the file, and make sure it really is a PNG.
		File file(path);
		if(!file)
			return nullptr;
		
		// Set up libpng.
		png_struct *png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if(!png)
			return nullptr;
		
		png_info *info = png_create_info_struct(png);
		if(!info)
		{
			png_destroy_read_struct(&png, nullptr, nullptr);
			return nullptr;
		}
		
		ImageBuffer *buffer = nullptr;
		if(setjmp(png_jmpbuf(png)))
		{
			png_destroy_read_struct(&png, &info, nullptr);
			delete buffer;
			return nullptr;
		}
		
		png_init_io(png, file);
		png_set_sig_bytes(png, 0);
		
		png_read_info(png, info);
		int width = png_get_image_width(png, info);
		int height = png_get_image_height(png, info);
		if(!width || !height)
		{
			png_destroy_read_struct(&png, &info, nullptr);
			delete buffer;
			return nullptr;
		}
		
		// Adjust settings to make sure the result will be a BGRA file.
		int colorType = png_get_color_type(png, info);
		int bitDepth = png_get_bit_depth(png, info);
		
		png_set_strip_16(png);
		png_set_packing(png);
		if(colorType == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);
		if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
			png_set_expand_gray_1_2_4_to_8(png);
		if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);
		if(colorType & PNG_COLOR_MASK_COLOR)
			png_set_bgr(png);
		png_read_update_info(png, info);
		
		// Read the file.
		buffer = new ImageBuffer(width, height);
		vector<png_byte *> rows(height, nullptr);
		for(int y = 0; y < height; ++y)
			rows[y] = reinterpret_cast<png_byte *>(buffer->Begin(y));
		
		png_read_image(png, &rows.front());
		
		// Clean up. The file will be closed automatically.
		png_destroy_read_struct(&png, &info, nullptr);
		
		return buffer;
	}
	
	
	
	ImageBuffer *ReadJPG(const string &path)
	{
		File file(path);
		if(!file)
			return nullptr;
		
		jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);
		
		jpeg_stdio_src(&cinfo, file);
		jpeg_read_header(&cinfo, true);
		cinfo.out_color_space = JCS_EXT_BGRA;
		
		jpeg_start_decompress(&cinfo);
		int width = cinfo.image_width;
		int height = cinfo.image_height;
		
		// Read the file.
		ImageBuffer *buffer = new ImageBuffer(width, height);
		vector<JSAMPLE *> rows(height, nullptr);
		for(int y = 0; y < height; ++y)
			rows[y] = reinterpret_cast<JSAMPLE *>(buffer->Begin(y));
		
		while(height)
			height -= jpeg_read_scanlines(&cinfo, &rows.front() + cinfo.output_scanline, height);
		
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		
		return buffer;
	}
	
	
	
	void Premultiply(ImageBuffer *buffer, int additive)
	{
		if(!buffer)
			return;
		
		for(int y = 0; y < buffer->Height(); ++y)
		{
			uint32_t *it = buffer->Begin(y);
			
			for(uint32_t *end = it + buffer->Width(); it != end; ++it)
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
