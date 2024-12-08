/* FontSet.h
Copyright (c) 2014-2020 by Michael Zahniser

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

#include <filesystem>

class Font;



// Class for getting the Font object for a given point size. Each font must be
// based on a glyph image; right now only point sizes 14 and 18 exist.
class FontSet {
public:
	static void Add(const std::filesystem::path &path, int size);
	static const Font &Get(int size);
};
