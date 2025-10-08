/* Tooltip.h
Copyright (c) 2025 by Amazinite

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

class Color;
class Point;



// A class for drawing the tooltips in a UI panel.
class Tooltip {
public:
	// The direction from the draw point that the tooltip should be drawn in.
	enum class Direction {
		UP_LEFT,
		UP_RIGHT,
		DOWN_LEFT,
		DOWN_RIGHT
	};

	// The corner from the drawn rectangle that the tooltip should be drawn from.
	enum class Corner {
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT,
	};


public:
	Tooltip(int width, Alignment alignment, Direction direction, Corner corner,
		const Color *backColor, const Color *fontColor);

	void IncrementCount();
	void DecrementCount();
	void ResetCount();
	bool ShouldDraw() const;

	void SetZone(const Point &center, const Point &dimensions);
	void SetZone(const Rectangle &zone);
	void SetText(const std::string &newText, bool shrink = false);
	bool HasText() const;
	void Clear();

	void SetBackgroundColor(const Color *backColor);
	void SetFontColor(const Color *fontColor);

	// If forceDraw is true, the hover timer is skipped when determining whether
	// the tooltip should be drawn.
	void Draw(bool forceDraw = false) const;

	void UpdateActivationCount();


private:
	int width;
	Direction direction;
	Corner corner;

	const Color *backColor;
	const Color *fontColor;

	Rectangle zone;
	WrappedText text;

	int hoverCount = 0;
	// The hover value needed to activate the tooltip.
	int activationHover;
};
