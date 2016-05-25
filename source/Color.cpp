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



Color::Color(float i, float a)
	: color{i, i, i, a}
{
}



Color::Color(float r, float g, float b, float a)
	: color{r, g, b, a}
{
}



void Color::Load(double r, double g, double b, double a)
{
	color[0] = static_cast<float>(r);
	color[1] = static_cast<float>(g);
	color[2] = static_cast<float>(b);
	color[3] = static_cast<float>(a);
}



const float *Color::Get() const
{
	return color;
}



Color Color::Opaque() const
{
	Color opaque = *this;
	opaque.color[3] = 1.f;
	return opaque;
}



Color Color::Transparent(float alpha) const
{
	Color result;
	for(int i = 0; i < 3; ++i)
		result.color[i] = color[i] * alpha;
	result.color[3] = alpha;
	
	return result;
}



Color Color::Additive(float alpha) const
{
	Color result = Transparent(alpha);
	result.color[3] = 0.f;
	
	return result;
}
