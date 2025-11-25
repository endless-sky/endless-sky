/* UiRectShader.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef UI_RECT_SHADER_H_
#define UI_RECT_SHADER_H_

class Point;
class Color;



// Class holding a function to fill a rectangular region of the screen with a
// given color, outlined with a single pixel wide diagonal gradient.
class UiRectShader {
public:
	static void Init(const Color& border1, const Color& border2, const Color& border3);
	static void Fill(const Point &center, const Point &size, const Color &color);
};



#endif
