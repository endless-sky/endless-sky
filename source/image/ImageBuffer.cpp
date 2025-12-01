/* ImageBuffer.cpp
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

#include "ImageBuffer.h"

#include "../Files.h"
#include "ImageFileData.h"
#include "../Logger.h"

#include <avif/avif.h>
#include <jpeglib.h>
#include <png.h>

#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {
	const set<string> PNG_EXTENSIONS{".png"};
	const set<string> JPG_EXTENSIONS{".jpg", ".jpeg", ".jpe"};
	const set<string> AVIF_EXTENSIONS{".avif", ".avifs"};
	const set<string> IMAGE_EXTENSIONS = []()
	{
		set<string> extensions(PNG_EXTENSIONS);
		extensions.insert(JPG_EXTENSIONS.begin(), JPG_EXTENSIONS.end());
		extensions.insert(AVIF_EXTENSIONS.begin(), AVIF_EXTENSIONS.end());
		return extensions;
	}();
	const set<string> IMAGE_SEQUENCE_EXTENSIONS = AVIF_EXTENSIONS;

	bool ReadPNG(const filesystem::path &path, ImageBuffer &buffer, int frame);
	bool ReadJPG(const filesystem::path &path, ImageBuffer &buffer, int frame);
	int ReadAVIF(const filesystem::path &path, ImageBuffer &buffer, int frame, bool alphaPreMultiplied);
	void Premultiply(ImageBuffer &buffer, int frame, BlendingMode additive);
}



const set<string> &ImageBuffer::ImageExtensions()
{
	return IMAGE_EXTENSIONS;
}



const set<string> &ImageBuffer::ImageSequenceExtensions()
{
	return IMAGE_SEQUENCE_EXTENSIONS;
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



int ImageBuffer::Read(const ImageFileData &data, int frame)
{
	// First, make sure this is a supported file.
	bool isPNG = PNG_EXTENSIONS.contains(data.extension);
	bool isJPG = JPG_EXTENSIONS.contains(data.extension);
	bool isAVIF = AVIF_EXTENSIONS.contains(data.extension);

	if(!isPNG && !isJPG && !isAVIF)
		return false;

	int loaded;
	if(isPNG)
		loaded = ReadPNG(data.path, *this, frame);
	else if(isJPG)
		loaded = ReadJPG(data.path, *this, frame);
	else
		loaded = ReadAVIF(data.path, *this, frame, data.blendingMode == BlendingMode::PREMULTIPLIED_ALPHA);

	if(loaded <= 0)
		return 0;

	if(data.blendingMode != BlendingMode::PREMULTIPLIED_ALPHA)
	{
		if(isPNG || (isJPG && data.blendingMode == BlendingMode::ADDITIVE))
			Premultiply(*this, frame, data.blendingMode);
	}
	return loaded;
}



namespace {
	void ReadPNGInput(png_structp pngStruct, png_bytep outBytes, png_size_t byteCountToRead)
	{
		static_cast<iostream *>(png_get_io_ptr(pngStruct))->read(reinterpret_cast<char *>(outBytes), byteCountToRead);
	}

	bool ReadPNG(const filesystem::path &path, ImageBuffer &buffer, int frame)
	{
		// Open the file, and make sure it really is a PNG.
		shared_ptr<iostream> file = Files::Open(path.string());
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

		png_set_read_fn(png, file.get(), ReadPNGInput);
		png_set_sig_bytes(png, 0);

		png_read_info(png, info);
		int width = png_get_image_width(png, info);
		int height = png_get_image_height(png, info);
		// If the buffer is not yet allocated, allocate it.
		try {
			buffer.Allocate(width, height);
		}
		catch(const bad_alloc &)
		{
			png_destroy_read_struct(&png, &info, nullptr);
			const string message = "Failed to allocate contiguous memory for \"" + path.string() + "\"";
			Logger::Log(message, Logger::Level::ERROR);
			throw runtime_error(message);
		}
		// Make sure this frame's dimensions are valid.
		if(!width || !height || width != buffer.Width() || height != buffer.Height())
		{
			png_destroy_read_struct(&png, &info, nullptr);
			string message = "Skipped processing \"" + path.string() + "\":\n\tAll image frames must have equal ";
			if(width && width != buffer.Width())
				Logger::Log(message + "width: expected " + to_string(buffer.Width())
					+ " but was " + to_string(width), Logger::Level::WARNING);
			if(height && height != buffer.Height())
				Logger::Log(message + "height: expected " + to_string(buffer.Height())
					+ " but was " + to_string(height), Logger::Level::WARNING);
			return false;
		}

		// Adjust settings to make sure the result will be an RGBA file.
		int colorType = png_get_color_type(png, info);
		int bitDepth = png_get_bit_depth(png, info);

		if(colorType == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);
		if(png_get_valid(png, info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);
		if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
			png_set_expand_gray_1_2_4_to_8(png);
		if(bitDepth == 16)
		{
#if PNG_LIBPNG_VER >= 10504
			png_set_scale_16(png);
#else
			png_set_strip_16(png);
#endif
		}
		if(bitDepth < 8)
			png_set_packing(png);
		if(colorType == PNG_COLOR_TYPE_PALETTE || colorType == PNG_COLOR_TYPE_RGB
				|| colorType == PNG_COLOR_TYPE_GRAY)
			png_set_add_alpha(png, 0xFFFF, PNG_FILLER_AFTER);
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



	bool ReadJPG(const filesystem::path &path, ImageBuffer &buffer, int frame)
	{
		string data = Files::Read(path);
		if(data.empty())
			return false;

		jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
		jpeg_create_decompress(&cinfo);
#pragma GCC diagnostic pop

		jpeg_mem_src(&cinfo, reinterpret_cast<const unsigned char *>(data.data()), data.size());
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
		}
		catch(const bad_alloc &)
		{
			jpeg_destroy_decompress(&cinfo);
			const string message = "Failed to allocate contiguous memory for \"" + path.string() + "\"";
			Logger::Log(message, Logger::Level::ERROR);
			throw runtime_error(message);
		}
		// Make sure this frame's dimensions are valid.
		if(!width || !height || width != buffer.Width() || height != buffer.Height())
		{
			jpeg_destroy_decompress(&cinfo);
			string message = "Skipped processing \"" + path.string() + "\":\t\tAll image frames must have equal ";
			if(width && width != buffer.Width())
				Logger::Log(message + "width: expected " + to_string(buffer.Width())
					+ " but was " + to_string(width), Logger::Level::WARNING);
			if(height && height != buffer.Height())
				Logger::Log(message + "height: expected " + to_string(buffer.Height())
					+ " but was " + to_string(height), Logger::Level::WARNING);
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



	// Read an AVIF file, and return the number of frames. This might be
	// greater than the number of frames in the file due to frame time corrections.
	// Since sprite animation properties are not visible here, we take the shortest frame
	// duration, and treat that as our time unit. Every other frame is repeated
	// based on how much longer its duration is compared to this unit.
	// TODO: If animation properties are exposed here, we can have custom presentation
	// logic that avoids duplicating the frames.
	int ReadAVIF(const filesystem::path &path, ImageBuffer &buffer, int frame, bool alphaPreMultiplied)
	{
		unique_ptr<avifDecoder, void(*)(avifDecoder *)> decoder(avifDecoderCreate(), avifDecoderDestroy);
		if(!decoder)
		{
			Logger::Log("Could not create avif decoder.", Logger::Level::WARNING);
			return 0;
		}
		// Maintenance note: this is where decoder defaults should be overwritten (codec, exif/xmp, etc.)

		string data = Files::Read(path);
		avifResult result = avifDecoderSetIOMemory(decoder.get(), reinterpret_cast<const uint8_t *>(data.c_str()),
			data.size());
		if(result != AVIF_RESULT_OK)
		{
			Logger::Log("Could not read file: " + path.generic_string(), Logger::Level::WARNING);
			return 0;
		}

		result = avifDecoderParse(decoder.get());
		if(result != AVIF_RESULT_OK)
		{
			Logger::Log(string("Failed to decode image: ") + avifResultToString(result), Logger::Level::WARNING);
			return 0;
		}
		// Generic image information is now available (width, height, depth, color profile, metadata, alpha, etc.),
		// as well as image count and frame timings.
		if(!decoder->imageCount)
			return 0;

		// Find the shortest frame duration.
		double frameTimeUnit = -1;
		avifImageTiming timing;
		for(int i = 0; i < decoder->imageCount; ++i)
		{
			result = avifDecoderNthImageTiming(decoder.get(), i, &timing);
			if(result != AVIF_RESULT_OK)
			{
				Logger::Log("Could not get image timing for '" + path.generic_string() + "': "
					+ avifResultToString(result), Logger::Level::WARNING);
				return 0;
			}
			if(frameTimeUnit < 0 || (frameTimeUnit > timing.duration && timing.duration))
				frameTimeUnit = timing.duration;
		}
		// Based on this unit, we can calculate how many times each frame is repeated.
		vector<size_t> repeats(decoder->imageCount);
		size_t bufferFrameCount = 0;
		for(size_t i = 0; i < static_cast<size_t>(decoder->imageCount); ++i)
		{
			result = avifDecoderNthImageTiming(decoder.get(), i, &timing);
			if(result != AVIF_RESULT_OK)
			{
				Logger::Log("Could not get image timing for \"" + path.generic_string() + "\": "
					+ avifResultToString(result), Logger::Level::WARNING);
				return 0;
			}
			repeats[i] = round(timing.duration / frameTimeUnit);
			bufferFrameCount += repeats[i];
		}

		// Now that we know the buffer's frame count, we can allocate the memory for it.
		// If this is an image sequence, the preconfigured frame count is wrong.
		try {
			if(bufferFrameCount > 1)
				buffer.Clear(bufferFrameCount);
			buffer.Allocate(decoder->image->width, decoder->image->height);
		}
		catch(const bad_alloc &)
		{
			const string message = "Failed to allocate contiguous memory for \"" + path.generic_string() + "\"";
			Logger::Log(message, Logger::Level::ERROR);
			throw runtime_error(message);
		}
		if(static_cast<unsigned>(buffer.Width()) != decoder->image->width
			|| static_cast<unsigned>(buffer.Height()) != decoder->image->height)
		{
			Logger::Log("Invalid dimensions for \"" + path.generic_string() + "\"", Logger::Level::WARNING);
			return 0;
		}

		// Load each image in the sequence.
		int avifFrameIndex = 0;
		size_t bufferFrame = 0;
		while(avifDecoderNextImage(decoder.get()) == AVIF_RESULT_OK)
		{
			// Ignore frames with insufficient duration.
			if(!repeats[avifFrameIndex])
				continue;

			avifRGBImage image;
			avifRGBImageSetDefaults(&image, decoder->image);
			image.depth = 8; // Force 8-bit color depth.
			image.alphaPremultiplied = alphaPreMultiplied;
			image.rowBytes = image.width * avifRGBImagePixelSize(&image);
			image.pixels = reinterpret_cast<uint8_t *>(buffer.Begin(0, frame + bufferFrame));

			result = avifImageYUVToRGB(decoder->image, &image);
			if(result != AVIF_RESULT_OK)
			{
				Logger::Log("Conversion from YUV failed for \"" + path.generic_string() + "\": "
					+ avifResultToString(result), Logger::Level::WARNING);
				return bufferFrame;
			}

			// Now copy the image in the buffer to match frame timings.
			for(size_t i = 1; i < repeats[avifFrameIndex]; ++i)
			{
				uint8_t *end = reinterpret_cast<uint8_t *>(buffer.Begin(0, frame + bufferFrame + 1));
				uint8_t *dest = reinterpret_cast<uint8_t *>(buffer.Begin(0, frame + bufferFrame + i));
				copy(image.pixels, end, dest);
			}
			bufferFrame += repeats[avifFrameIndex];

			++avifFrameIndex;
		}

		if(avifFrameIndex != decoder->imageCount || bufferFrame != bufferFrameCount)
			Logger::Log("Skipped corrupted frames for \"" + path.generic_string() + "\"", Logger::Level::WARNING);

		return bufferFrameCount;
	}



	void Premultiply(ImageBuffer &buffer, int frame, BlendingMode blend)
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
				if(blend == BlendingMode::HALF_ADDITIVE)
					alpha >>= 2;
				if(blend != BlendingMode::ADDITIVE)
					value |= (alpha << 24);

				*it = static_cast<uint32_t>(value);
			}
		}
	}
}
