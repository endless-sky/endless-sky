/* ImageBuffer.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef IMAGE_BUFFER_H_
#define IMAGE_BUFFER_H_

#include <string>



// This class stores the raw pixel data from an image, and handles reading that
// image from disk (so that multiple images can be read and decoded at the same
// time in different threads). It also handles converting images to
// premultiplied alpha or additive or half-additive color mixing mode depending
// on the file name, so that content creators do not have to save the images in
// some sort of special format.
class ImageBuffer {
public:
	ImageBuffer();
	ImageBuffer(int width, int height);
	ImageBuffer(const ImageBuffer &) = delete;
	~ImageBuffer();
	
	ImageBuffer &operator=(const ImageBuffer &) = delete;
	
	int Width() const;
	int Height() const;
	
	const uint32_t *Pixels() const;
	uint32_t *Pixels();
	
	const uint32_t *Begin(int y) const;
	uint32_t *Begin(int y);
	
	void ShrinkToHalfSize();
	
	static ImageBuffer *Read(const std::string &path);
	
	
private:
	int width;
	int height;
	uint32_t *pixels;
};



#endif
