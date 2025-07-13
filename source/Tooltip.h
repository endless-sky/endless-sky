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
		LEFT,
		RIGHT
	};

	// The corner from the drawn rectangle that the tooltip should be drawn from.
	enum class Corner {
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT,
	};

	// The tooltip's state determines its background color.
	enum class State {
		NORMAL,
		WARNING,
		ERROR
	};


public:
	Tooltip(int width, Alignment alignment, Direction direction, Corner corner);

	void IncrementCount();
	void DecrementCount();
	void MaxCount();
	void ResetCount();
	bool ShouldDraw() const;

	void SetZone(const Point &center, const Point &dimensions);
	void SetZone(const Rectangle &zone);
	void SetText(const std::string &text);
	bool HasText() const;
	void Clear();

	void SetState(State state);

	void Draw() const;


private:
	void Draw(const Point &point) const;


private:
	int width;
	Direction direction;
	Corner corner;
	State state = State::NORMAL;

	const Color *normal;
	const Color *warning;
	const Color *error;
	const Color *fontColor;

	Rectangle zone;
	WrappedText text;

	int hoverCount = 0;
};
