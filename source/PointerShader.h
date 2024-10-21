/* PointerShader.h
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



// Functions for drawing triangular "pointers," e.g. for target crosshairs.
class PointerShader {
public:
	static void Init();

	static void Draw(const Point &center, const Point &angle, float width, float height, float offset, const Color &color);

	static void Bind();
	static void Add(const Point &center, const Point &angle, float width, float height, float offset, const Color &color);
	static void Unbind();
};
