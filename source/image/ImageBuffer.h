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
#include <set>
#include <string>

class ImageFileData;



// This class stores the raw pixel data from an image, and handles reading that
// image from disk (so that multiple images can be read and decoded at the same
// time in different threads). It also handles converting images to
// premultiplied alpha or additive or half-additive color mixing mode depending
// on the file name, so that content creators do not have to save the images in
// some sort of special format.
class ImageBuffer {
public:
	// The supported image extensions, in lower case and with a leading period.
	static const std::set<std::string> &ImageExtensions();
	// Image extensions that signify image sequences. This is a subset of ImageExtensions().
	static const std::set<std::string> &ImageSequenceExtensions();


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

	int Width() const;
	int Height() const;
	int Frames() const;

	const uint32_t *Pixels() const;
	uint32_t *Pixels();

	const uint32_t *Begin(int y, int frame = 0) const;
	uint32_t *Begin(int y, int frame = 0);

	void ShrinkToHalfSize();

	// Read frames from a file. Return the number of frames read,
	// or 0 if an error is encountered - either the
	// image is the wrong size, or it is not a supported image format.
	// If the file is an image sequence, it overwrites the preconfigured
	// frame count with the number of frames found in the file.
	int Read(const ImageFileData &data, int frame = 0);


private:
	int width;
	int height;
	int frames;
	uint32_t *pixels;
};
