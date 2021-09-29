/* ReadImage.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef READ_IMAGE_H_
#define READ_IMAGE_H_

#include <string>

class ImageBuffer;

namespace Read {
	bool PNG(const std::string &path, ImageBuffer &buffer, int frame);
	bool JPG(const std::string &path, ImageBuffer &buffer, int frame);
}

#endif
