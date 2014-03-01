/* FillShader.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FILL_SHADER_H_
#define FILL_SHADER_H_

class Point;



// Class holding a function to fill a rectangular region of the screen with a given
// color (which may be partly translucent).
class FillShader {
public:
	static void Init();
	
	static void Fill(const Point &center, const Point &size, const float *color = nullptr);
};



#endif
