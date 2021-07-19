/* Color.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Color.h"


// Predefined Colors taken from the "Fixed Colors" defined by Apple
//  (https://developer.apple.com/documentation/uikit/uicolor/standard_colors)
const Color Color::Black 		= Color(0.f, 0.f, 0.f);
const Color Color::Blue			= Color(0.f, 0.f, 1.f);
const Color Color::Brown		= Color(.6f, .4f, .2f);
const Color Color::Cyan			= Color(0.f, 1.f, 1.f);
const Color Color::DarkGray		= Color(1/3.f, 1/3.f, 1/3.f);
const Color Color::Gray			= Color(.5f, .5f, .5f);
const Color Color::Green		= Color(0.f, 1.f, 0.f);
const Color Color::LightGray	= Color(2/3.f, 2/3.f, 2/3.f);
const Color Color::Magenta 		= Color(1.f, 0.f, 1.f);
const Color Color::Orange 		= Color(1.f, .5f, 0.f);
const Color Color::Purple 		= Color(.5f, 0.f, .5f);
const Color Color::Red 			= Color(1.f, 0.f, 0.f);
const Color Color::White 		= Color(1.f, 1.f, 1.f);
const Color Color::Yellow 		= Color(1.f, 1.f, 0.f);


// Greyscale color constructor.
Color::Color(float i, float a)
	: color{i, i, i, a}
{
}



// Full color constructor.
Color::Color(float r, float g, float b, float a)
	: color{r, g, b, a}
{
}



// Set all four color components to the given values.
void Color::Load(double r, double g, double b, double a)
{
	color[0] = static_cast<float>(r);
	color[1] = static_cast<float>(g);
	color[2] = static_cast<float>(b);
	color[3] = static_cast<float>(a);
}



// Get a float vector representing this color, for use by OpenGL.
const float *Color::Get() const
{
	return color;
}



// Get an opaque version of this color.
Color Color::Opaque() const
{
	Color opaque = *this;
	opaque.color[3] = 1.f;
	return opaque;
}



// Assuming this color is opaque, get a transparent version of it.
Color Color::Transparent(float alpha) const
{
	Color result;
	for(int i = 0; i < 3; ++i)
		result.color[i] = color[i] * alpha;
	result.color[3] = alpha;
	
	return result;
}



// Assuming this color is opaque, get an additive version of it.
Color Color::Additive(float alpha) const
{
	Color result = Transparent(alpha);
	result.color[3] = 0.f;
	
	return result;
}
