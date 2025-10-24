/* FillShader.h
Copyright (c) 2014 by Michael Zahniser

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

class Color;
class Point;
class Rectangle;



// Class holding a function to fill a rectangular region of the screen with a
// given color. This can be used with translucent colors to darken or lighten a
// part of the screen, or with additive colors (alpha = 0) as well.
class FillShader {
public:
	static void Init();
	static void Fill(const Rectangle &area, const Color &color);
	static void Fill(const Point &center, const Point &size, const Color &color);
};
