/* ImageFileData.h
Copyright (c) 2024 by tibetiroka

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

#include "BlendingMode.h"

#include <filesystem>
#include <string>



class ImageFileData {
public:
	// Computes the image file data from a path. If the path has a source directory,
	// it has to be specified here.
	ImageFileData(const std::filesystem::path &path, const std::filesystem::path &source = {});


public:
	std::filesystem::path path;
	std::string extension;
	std::string name;
	bool is2x = false;
	bool noReduction = false;
	bool isSwizzleMask = false;
	BlendingMode blendingMode = BlendingMode::ALPHA;
	size_t frameNumber = 0;
};
