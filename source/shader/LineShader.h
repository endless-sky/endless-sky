/* LineShader.h
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



// Class to be used for drawing lines. The sides of a line are anti-aliased, but
// the start and end of the line are not.
class LineShader {
public:
	static void Init();
	static void Draw(const Point &from, const Point &to, float width, const Color &color, bool roundCap = true);
	static void DrawDashed(const Point &from, const Point &to, const Point &unit, float width, const Color &color,
		double dashLength, double spaceLength, bool roundCap = true);
	static void DrawGradient(const Point &from, const Point &to, float width,
		const Color &fromColor, const Color &toColor, bool roundCap = true);
	static void DrawGradientDashed(const Point &from, const Point &to, const Point &unit, float width,
		const Color &fromColor, const Color &toColor, double dashLength, double spaceLength, bool roundCap = true);
};
