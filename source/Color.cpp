/* Color.cpp
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

#include "Color.h"

using namespace std;



// Greyscale color constructor.
Color::Color(float i, float a)
	: color{i, i, i, a}
{
}



// Full color constructor.
Color::Color(float r, float g, float b, float a)
	: color{r, g, b, a}, isLoaded(true)
{
}



bool Color::operator==(const Color &other) const
{
	for(int i = 0; i < 4; ++i)
		if(color[i] != other.color[i])
			return false;
	return true;
}



bool Color::operator!=(const Color &other) const
{
	return !(*this == other);
}



// Set all four color components to the given values.
void Color::Load(double r, double g, double b, double a)
{
	color[0] = static_cast<float>(r);
	color[1] = static_cast<float>(g);
	color[2] = static_cast<float>(b);
	color[3] = static_cast<float>(a);

	isLoaded = true;
}



// Check if Load() has been called for this color.
bool Color::IsLoaded() const
{
	return isLoaded;
}



void Color::SetTrueName(const string &name)
{
	this->trueName = name;
}



const string &Color::TrueName() const
{
	return trueName;
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



Color Color::Combine(float a1, Color c1, float a2, Color c2)
{
	return Color(
			a1 * c1.color[0] + a2 * c2.color[0],
			a1 * c1.color[1] + a2 * c2.color[1],
			a1 * c1.color[2] + a2 * c2.color[2],
			a1 * c1.color[3] + a2 * c2.color[3]);
}



Color Color::Multiply(float scalar, const Color &base)
{
	return Color(
			scalar * base.color[0],
			scalar * base.color[1],
			scalar * base.color[2],
			scalar * base.color[3]);
}
