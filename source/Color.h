/* Color.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef COLOR_H_
#define COLOR_H_



// Class representing an RGBA color (for use by OpenGL). The specified colors
// should be in premultiplied alpha mode. For example, a white color which is
// 50% translucent would be {.5, .5, .5} with an alpha of .5. This allows you to
// also specify colors that use additive blending. For example, if the alpha is
// zero the color's components will be added to whatever is underneath them.
class Color {
public:
	Color(float i = 1.f, float a = 1.f);
	Color(float r, float g, float b, float a = 1.f);
	
	void Load(double r, double g, double b, double a);
	const float *Get() const;
	
	Color Opaque() const;
	Color Transparent(float alpha) const;
	Color Additive(float alpha) const;
	
	
private:
	float color[4];
};



#endif
