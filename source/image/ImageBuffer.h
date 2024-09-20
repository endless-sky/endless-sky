/* ImageBuffer.h
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

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>



// This class stores the raw pixel data from an image, and handles reading that
// image from disk (so that multiple images can be read and decoded at the same
// time in different threads). It also handles converting images to
// premultiplied alpha or additive or half-additive color mixing mode depending
// on the file name, so that content creators do not have to save the images in
// some sort of special format.
class ImageBuffer {
public:
	// When initializing a buffer, we know the number of frames but not the size
	// of them. So, it must be Allocate()d later.
	ImageBuffer(int frames = 1);
	ImageBuffer(const ImageBuffer &) = delete;
	~ImageBuffer();

	ImageBuffer &operator=(const ImageBuffer &) = delete;

	// Set the number of frames. This must be called before allocating.
	void Clear(int frames = 1);
	// Allocate the internal buffer. This must only be called once for each
	// image buffer; subsequent calls will be ignored.
	void Allocate(int width, int height);

	// Assign the internal buffer all at once
	void Assign(const void* data, size_t size, int width, int height, uint32_t compressed_format);
	void SetDisplaySize(int dw, int dh);

	int Width() const;
	int Height() const;
	int DisplayWidth() const;
	int DisplayHeight() const;
	int Frames() const;

	const uint32_t *Pixels() const;
	uint32_t *Pixels();

	const uint32_t *Begin(int y, int frame = 0) const;
	uint32_t *Begin(int y, int frame = 0);

	// get the alpha component without making assumptions about the buffer format
	uint8_t GetAlpha(int frame, int x, int y) const;

	void ShrinkToHalfSize();

	// Read a single frame. Return false if an error is encountered - either the
	// image is the wrong size, or it is not a supported image format.
	bool Read(const std::filesystem::path &path, int frame = 0);


	uint32_t CompressedFormat() const { return compressed_format; }
	uint32_t CompressedSize() const { return compressed_size; }

private:
	int width;
	int height;
	int display_width;
	int display_height;
	int frames;
	uint32_t *pixels;

	uint32_t compressed_format = 0;
	uint32_t compressed_size = 0;
};
