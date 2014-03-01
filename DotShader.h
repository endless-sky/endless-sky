/* DotShader.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DOT_SHADER_H_
#define DOT_SHADER_H_

class Color;
class Point;



// Class representing a shader that draws round "dots," wither filled in or with
// black in the center.
class DotShader {
public:
	static void Init();
	
	static void Draw(const Point &pos, float out, float in, const Color &color);
	
	static void Bind();
	static void Add(const Point &pos, float out, float in, const Color &color);
	static void Unbind();
};



#endif
