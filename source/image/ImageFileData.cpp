/* ImageFileData.cpp
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

#include "ImageFileData.h"

#include "../text/Format.h"

#include <cmath>

using namespace std;

namespace {
	// Check if the given character is a valid blending mode.
	bool IsBlend(char c)
	{
		return (c == static_cast<char>(BlendingMode::ALPHA) || c == static_cast<char>(BlendingMode::HALF_ADDITIVE)
			|| c == static_cast<char>(BlendingMode::ADDITIVE) || c == static_cast<char>(BlendingMode::PREMULTIPLIED_ALPHA));
	}
}



ImageFileData::ImageFileData(const std::filesystem::path &path)
	: path(path), extension(Format::LowerCase(path.extension().string()))
{
	string name = path.stem().string();
	if(name.ends_with("@2x"))
	{
		is2x = true;
		name.resize(name.size() - 3);
	}
	if(name.ends_with("@sw"))
	{
		isSwizzleMask = true;
		name.resize(name.size() - 3);
	}

	while(!name.empty() && name[name.size() - 1] >= '0' && name[name.size() - 1] <= '9')
	{
		unsigned power = !frameNumber ? 0 : log10(frameNumber) + 1;
		frameNumber += pow(10, power) * (name[name.size() - 1] - '0');
		name.resize(name.size() - 1);
	}

	if(name.empty())
		return;
	else if(IsBlend(name[name.size() - 1]))
	{
		blendingMode = static_cast<BlendingMode>(name[name.size() - 1]);
		name.resize(name.length() - 1);
	}

	this->name = name;
}
