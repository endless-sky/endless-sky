/* Caret.h
Copyright (c) 2026 by thewierdnut

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

#include "Color.h"
#include "Rectangle.h"

#include <cstdint>

// Draws a caret at the given position
class Caret final
{
public:
	void Draw();
	void BlinkOn();

	void SetColor(const Color &c) { color = c; }

	void SetHeight(int h) { height = h; }
	void SetX(int x) { pos.X() = x; }
	void SetY(int y) { pos.Y() = y; }

private:
	uint32_t blinkCounter = 0;
	Color color{1.0, 1.0, 1.0, .75};

	int height = 14;
	Point pos{0, 0};
};
