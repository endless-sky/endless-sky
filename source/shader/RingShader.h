/* RingShader.h
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



// Class representing a shader that draws round "dots," either filled in or with
// transparent centers (i.e. circles or rings).
class RingShader {
public:
	static void Init();

	static void Draw(const Point &pos, float out, float in, const Color &color);
	static void Draw(const Point &pos, float radius, float width, float fraction,
		const Color &color, float dash = 0.f, float startAngle = 0.f);

	static void Bind();
	static void Add(const Point &pos, float out, float in, const Color &color);
	static void Add(const Point &pos, float radius, float width, float fraction,
		const Color &color, float dash = 0.f, float startAngle = 0.f);
	static void Unbind();
};
