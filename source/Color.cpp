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

#include "Preferences.h"

#include <algorithm>

using namespace std;



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



// Apply color blindness filters to this color.
Color Color::Filter(const Color &c)
{
	Preferences::ColorFilter filter = Preferences::GetColorFilterMode();
	// No color filters are enabled, so there's no need to adjust anything.
	if(filter == Preferences::ColorFilter::NORMAL)
		return c;

	// Color blindness accessibility filters are enabled.
	// LMS Daltonization is used to make the colors more distinct for
	// color blind players.

	float r = c.color[0];
	float g = c.color[1];
	float b = c.color[2];

	// Convert the colors from RGB to LMS, a color space that represents the
	// light received by the cones in the human eye better than RGB.
	float l = (17.8824 * r) + (43.5161 * g) + (4.11935 * b);
	float m = (3.45565 * r) + (27.1554 * g) + (3.86714 * b);
	float s = (0.0299566 * r) + (0.184309 * g) + (1.46709 * b);

	// Simulate color blindness.
	switch(filter)
	{
		case Preferences::ColorFilter::PROTANOPIA:
			l = (2.02344 * m) + (-2.52581 * s);
			break;
		case Preferences::ColorFilter::DEUTERANOPIA:
			m = (0.494207 * l) + (1.24827 * s);
			break;
		case Preferences::ColorFilter::TRITANOPIA:
			s = (-0.395913 * l) + (0.801109 * m);
			break;
		case Preferences::ColorFilter::NORMAL:
			break;
	}

	// Convert the LMS colors back to RGB.
	float convertedR = (0.0809444479 * l) + (-0.130504409 * m) + (0.116721066 * s);
	float convertedG = (-0.0102485335 * l) + (0.0540193266 * m) + (-0.113614708 * s);
	float convertedB = (-0.000365296938 * l) + (-0.00412161469 * m) + (0.693511405 * s);

	// Compensate for invisible colors.
	float finalR = r;
	float finalG = 2 * g - convertedG + 0.7 * (r - convertedR);
	float finalB = 2 * b - convertedB + 0.7 * (r - convertedR);

	// Clamp the RGB colors to within the 0. to 1. range used by OpenGL.
	finalR = clamp(finalR, 0.f, 1.f);
	finalG = clamp(finalG, 0.f, 1.f);
	finalB = clamp(finalB, 0.f, 1.f);


	return Color(finalR, finalG, finalB, c.color[3]);
}
