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
		NONE
	};

	// The position of the ear on the side that it is attached to, observed facing the direction the ear is pointing.
	// E.g. when facing SOUTH, "CCW" will be on the bottom right.
	enum class Affinity {
		CCW,
		CENTER,
		CW,
		NONE
	};


public:
	explicit InfoTag() = default;

	// Auto-place the box based on the pointer definition.
	void InitShapeAndPlacement(Point anchor, Direction facing, Affinity affinity, std::string text, Alignment alignment,
		double width = 1000, bool shrink = true, double earLength = 15);

	// Auto-place the box based on the pointer definition and relative to an interface element by an offset.
	void InitShapeAndPlacement(
		std::string element,
		Point offset,
		Direction facing,
		Affinity affinity,

		std::string text,
		Alignment alignment,

		double width = 1000,
		bool shrink = true,
		double earLength = 15
	);

	// Box and anchor absolute placement.
	void InitShapeAndPlacement(Point center, Point anchor, std::string text, Alignment alignment, double width = 1000,
		bool shrink = true, double earWidth = 15);

	// Box placed absolute, pointer placed relative to interface element by an offset.
	void InitShapeAndPlacement(
		Point center,
		std::string element,
		Point offset,

		std::string text,
		Alignment alignment,

		double width = 1000,
		bool shrink = true,
		double earWidth = 15
		);

	void InitBorderAndFill(
		const Color *backColor,
		const Color *fontColor,
		const Color *borderColor,
		const Color *borderColor2 = nullptr,
		double borderWidth = 1
	);

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
	// When both of these are defined, the box will be dynamically placed.
	Direction facing;
	Affinity affinity;

	Point anchor;
	WrappedText wrap;
	bool shrink;

	double earLength = 15.;
	double earWidth = 15.;
	double borderWidth = 1.;

	const Color *backColor;
	const Color *backColor2;
	const Color *fontColor;
	const Color *borderColor;
	const Color *borderColor2;

	int padding = 10;
	Rectangle box;
	std::vector<Point> points;

	std::string element;
	Point offset;
};
