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
#include "../Logger.h"

#include <cmath>

using namespace std;

namespace {
	// Check if the given character is a valid blending mode.
	bool IsBlend(char c)
	{
		return (c == static_cast<char>(BlendingMode::ALPHA) || c == static_cast<char>(BlendingMode::HALF_ADDITIVE)
			|| c == static_cast<char>(BlendingMode::ADDITIVE) || c == static_cast<char>(BlendingMode::PREMULTIPLIED_ALPHA)
			|| c == static_cast<char>(BlendingMode::COMPAT_HALF_ADDITIVE));
	}
}



ImageFileData::ImageFileData(const filesystem::path &path, const filesystem::path &source)
	: path(path), extension(Format::LowerCase(path.extension().string()))
{
	string name = (path.lexically_relative(source).parent_path() / path.stem()).generic_string();
	if(name.ends_with("@2x"))
	{
		is2x = true;
		noReduction = true;
		name.resize(name.size() - 3);
	}
	if(name.ends_with("@1x"))
	{
		noReduction = true;
		name.resize(name.size() - 3);
	}
	if(name.ends_with("@sw"))
	{
		isSwizzleMask = true;
		name.resize(name.size() - 3);
	}

	size_t frameNumberStart = name.size();
	while(frameNumberStart > 0 && name[--frameNumberStart] >= '0' && name[frameNumberStart] <= '9')
		continue;

	if(frameNumberStart > 0 && IsBlend(name[frameNumberStart]))
	{
		frameNumber = Format::Parse(name.substr(frameNumberStart + 1));
		blendingMode = static_cast<BlendingMode>(name[frameNumberStart]);
		name.resize(frameNumberStart);

		if(blendingMode == BlendingMode::COMPAT_HALF_ADDITIVE)
		{
			blendingMode = BlendingMode::HALF_ADDITIVE;
			Logger::Log("File '" + path.string()
				+ "'uses legacy marker for half-additive blending mode; please use '^' instead of '~'.",
				Logger::Level::WARNING);
		}
	}

	this->name = std::move(name);
}
