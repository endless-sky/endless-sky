/* OutlineShader.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef OUTLINE_SHADER_H_
#define OUTLINE_SHADER_H_

#include "Point.h"

class Color;
class Sprite;



// Functions for drawing the "outline" of a sprite, i.e. a Sobel filter of its
// alpha channel.
class OutlineShader {
public:
	static void Init();
	
	static void Draw(const Sprite *sprite, const Point &pos, const Point &size, const Color &color, const Point &unit = Point(0., -1.), float frame = 0.f);
};



#endif
