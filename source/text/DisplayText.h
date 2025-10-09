/* DisplayText.h
Copyright (c) 2020 by OOTA, Masato

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

#include "Layout.h"

#include <string>
#include <tuple>
#include <vector>

class Color;
class Point;
class Sprite;


// Class for holding a displayed text and layout.
class DisplayText {
public:
	// ASCII 28 is "File Separator"
	static constexpr char SPRITE_PLACEHOLDER = 28;


public:
	DisplayText() = default;
	DisplayText(const char *text, Layout layout);
	DisplayText(const std::string &text, Layout layout);

	const std::string &GetText() const noexcept;
	const Layout &GetLayout() const noexcept;

	void UpdateSpriteReferences();


private:
	Layout layout;
	std::string text;

	bool spritesLoaded = false;
	// Sprite, embossed text, center point.
	std::vector<std::tuple<const Sprite *, std::string, Point>> inlineSprites;

	friend class Font;
};
