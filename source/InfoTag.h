/* InfoTag.h
Copyright (c) 2025 by xobes

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

#include "text/Alignment.h"
#include "text/WrappedText.h"

#include <string>

class Color;
class Point;



// A class for drawing the InfoTags in a UI panel.
class InfoTag {
public:
	// The direction that the ear will point in toward the anchor.
	enum class Direction {
		NORTH,
		EAST,
		SOUTH,
		WEST,
	};

	// The position of the ear on the side that it is attached to, observed facing the direction the ear is pointing.
	// E.g. when facing SOUTH, "CCW" will be on the bottom right.
	enum class Affinity {
		CCW,
		CENTER,
		CW,
	};


public:
	///////////////////////// all of this will go away
	InfoTag() = default;

	InfoTag(int width, Alignment alignment, Direction facing, Affinity affinity,
		const Color *backColor, const Color *fontColor, const Color *borderColor,
		double earLength = 10, double border = 1);

	void SetAnchor(const Point &anchor);
	void SetText(const std::string &newText, bool shrink = false);
	bool HasText() const;
	void Clear();

	void SetBackgroundColor(const Color *backColor);
	void SetFontColor(const Color *fontColor);

	void Draw() const;
	void Draw2() const;
	///////////////////////// all of this will go away

	static void Draw(Point anchor, std::string text, int width, Alignment alignment,
		Direction facing, Affinity affinity,
		const Color *backColor = nullptr, const Color *fontColor = nullptr, const Color *borderColor = nullptr,
		bool shrink = true, double earLength = 10, double border = 1);


private:
	///////////////////////// all of this will go away
	int width;
	Direction facing;
	Affinity affinity;
	double earLength;
	double border;

	const Color *backColor;
	const Color *fontColor;
	const Color *borderColor;

	Point anchor;
	WrappedText text;
	/////////////////////////////////////////////

};
