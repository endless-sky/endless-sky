/* Color.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the Color class.
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
