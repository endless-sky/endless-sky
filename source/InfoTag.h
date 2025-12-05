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
#include "Rectangle.h"
#include "text/WrappedText.h"

#include <string>
#include <vector>

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
	explicit InfoTag() = default;

	void Init(Point anchor, std::string text, double width, Alignment alignment, Direction facing, Affinity affinity,
	          const Color *backColor = nullptr, const Color *fontColor = nullptr, const Color *borderColor = nullptr,
	          bool shrink = true, double earLength = 15, double border = 1);
	// void Init(int width, Alignment alignment, Direction facing, Affinity affinity,
	// 	const Color *backColor, const Color *fontColor, const Color *borderColor,
	// 	double earLength = 10, double border = 1);

	void SetAnchor(const Point &anchor);
	void Draw() const;


private:
	void Recalculate();
	void SetText(const std::string &newText, bool shrink);
	void SetText(const std::string &newText);
	bool HasText() const;
	void Clear();

	void SetBackgroundColor(const Color *backColor, const Color *backColor2 = nullptr);
	void SetFontColor(const Color *fontColor);


private:
	// Original, standard, box has ears at 90 deg at corners or center.
	Direction facing;
	Affinity affinity;
	double earLength = 15.;

	// Common parameters.
	Point anchor;
	WrappedText wrap;
	bool shrink;

	double earWidth = 7.;
	double borderWidth = 1.;

	const Color *backColor;
	const Color *backColor2;
	const Color *fontColor;
	const Color *borderColor;
	const Color *borderColor2;

	Rectangle box;
	std::vector<Point> points;

	// TODO: consider adding an element (e.g. interface element) and an offset (Point(x, y))
	// string element, Point offset
};
