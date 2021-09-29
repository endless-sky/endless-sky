/* ReadImage.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../ReadImage.h"

#include "../File.h"
#include "../Files.h"
#include "../ImageBuffer.h"

#include <png.h>
#include <jpeglib.h>

#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace std;

bool Read::PNG(const string &path, ImageBuffer &buffer, int frame)
{
	// Open the file, and make sure it really is a PNG.
	File file(path);
	if(!file)
		return false;
	
	// Set up libpng.
	png_struct *png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if(!png)
		return false;
	
	png_info *info = png_create_info_struct(png);
	if(!info)
	{
		png_destroy_read_struct(&png, nullptr, nullptr);
		return false;
	}
	
	if(setjmp(png_jmpbuf(png)))
	{
		png_destroy_read_struct(&png, &info, nullptr);
		return false;
	}
	
	// MAYBE: Reading in lots of images in a 32-bit process gets really hairy using the standard approach due to
	// contiguous memory layout requirements. Investigate using an iterative loading scheme for large images.
	png_init_io(png, file);
	png_set_sig_bytes(png, 0);
	
	png_read_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	// If the buffer is not yet allocated, allocate it.
	try {
		buffer.Allocate(width, height);
	} catch (const bad_alloc &) {
		png_destroy_read_struct(&png, &info, nullptr);
		const string message = "Failed to allocate contiguous memory for \"" + path + "\"";
		Files::LogError(message);
		throw runtime_error(message);
	}
	// Make sure this frame's dimensions are valid.
	if(!width || !height || width != buffer.Width() || height != buffer.Height())
	{
		png_destroy_read_struct(&png, &info, nullptr);
		string message = "Skipped processing \"" + path + "\":\n\tAll image frames must have equal ";
		if(width && width != buffer.Width())
			Files::LogError(message + "width: expected " + to_string(buffer.Width()) + " but was " + to_string(width));
		if(height && height != buffer.Height())
			Files::LogError(message + "height: expected " + to_string(buffer.Height()) + " but was " + to_string(height));
		return false;
	}
	
	// Adjust settings to make sure the result will be a RGBA file.
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
	// Let libpng handle any interlaced image decoding.
	png_set_interlace_handling(png);
	png_read_update_info(png, info);
	
	// Read the file.
	vector<png_byte *> rows(height, nullptr);
	for(int y = 0; y < height; ++y)
		rows[y] = reinterpret_cast<png_byte *>(buffer.Begin(y, frame));
		
	png_read_image(png, &rows.front());
	
	// Clean up. The file will be closed automatically.
	png_destroy_read_struct(&png, &info, nullptr);
	
	return true;
}



bool Read::JPG(const string &path, ImageBuffer &buffer, int frame)
{
	File file(path);
	if(!file)
		return false;
	
	jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
	jpeg_create_decompress(&cinfo);
#pragma GCC diagnostic pop
	
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, true);
	cinfo.out_color_space = JCS_EXT_RGBA;
		
	// MAYBE: Reading in lots of images in a 32-bit process gets really hairy using the standard approach due to
	// contiguous memory layout requirements. Investigate using an iterative loading scheme for large images.
	jpeg_start_decompress(&cinfo);
	int width = cinfo.image_width;
	int height = cinfo.image_height;
	// If the buffer is not yet allocated, allocate it.
try {
		buffer.Allocate(width, height);
	} catch (const bad_alloc &) {
		jpeg_destroy_decompress(&cinfo);
		const string message = "Failed to allocate contiguous memory for \"" + path + "\"";
		Files::LogError(message);
			throw runtime_error(message);
	}
	// Make sure this frame's dimensions are valid.
	if(!width || !height || width != buffer.Width() || height != buffer.Height())
	{
		jpeg_destroy_decompress(&cinfo);
		string message = "Skipped processing \"" + path + "\":\t\tAll image frames must have equal ";
		if(width && width != buffer.Width())
			Files::LogError(message + "width: expected " + to_string(buffer.Width()) + " but was " + to_string(width));
		if(height && height != buffer.Height())
			Files::LogError(message + "height: expected " + to_string(buffer.Height()) + " but was " + to_string(height));
		return false;
	}
	
	// Read the file.
	vector<JSAMPLE *> rows(height, nullptr);
	for(int y = 0; y < height; ++y)
		rows[y] = reinterpret_cast<JSAMPLE *>(buffer.Begin(y, frame));
	
	while(height)
		height -= jpeg_read_scanlines(&cinfo, &rows.front() + cinfo.output_scanline, height);
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	return true;
}
