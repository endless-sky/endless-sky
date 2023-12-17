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

#include <vector>

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
	Preferences::ColorFilter filter = Preferences::GetColorFilterMode();
	// No color filters are enabled, so there's no need to adjust anything.
	if(filter == Preferences::ColorFilter::NORMAL)
		return color;

	// Color blindness accessibility filters are enabled.
	// LMS Daltonization is used to make the colors more distinct for
	// color-blind players.
	static float l, m, s, r, g, b;

	// Convert the colors from RGB to LMS, a color space that represents the
	// light received by the cones in the human eye better than RGB.
	l = (17.8824 * color[0]) + (43.5161 * color[1]) + (4.11935 * color[2]);
	m = (3.45565 * color[0]) + (27.1554 * color[1]) + (3.86714 * color[2]);
	s = (0.0299566 * color[0]) + (0.184309 * color[1]) + (1.46709 * color[2]);

	switch(filter)
	{
		// Simulate color blidness.
		case Preferences::ColorFilter::PROTANOPIA:
			l = (2.02344 * m) + (-2.52581 * s);
			break;
		case Preferences::ColorFilter::DEUTERANOPIA:
			m = (0.494207 * l) + (1.24827 * s);
			break;
		case Preferences::ColorFilter::TRITANOPIA:
			s = (-0.395913 * l) + (0.801109 * m);
			break;
		default:
			break;
	}

	// Convert the LMS colors back to RGB.
	r = (0.0809444479 * l) + (-0.130504409 * m) + (0.116721066 * s);
	g = (-0.0102485335 * l) + (0.0540193266 * m) + (-0.113614708 * s);
	b = (-0.000365296938 * l) + (-0.00412161469 * m) + (0.693511405 * s);

	// Compensate for invisible colors.
	r = color[0];
	g = color[1] + 0.7 * (color[0] - r) + (color[1] - g);
	b = color[2] + 0.7 * (color[0] - r) + (color[2] - b);

	// Clamp the RGB colors to within the 0. to 1. range used by OpenGL.
	r = max(0.f, min(r, 1.f));
	g = max(0.f, min(g, 1.f));
	b = max(0.f, min(b, 1.f));

	// Return the final values. The tritanopia mode is somewhat extreme, so
	// if it is in use the colors are blended with the original.
	if(Preferences::GetColorFilterMode() == Preferences::ColorFilter::TRITANOPIA)
		return new float[4]{(r + color[0]) / 2, (g + color[1]) / 2, (b + color[2]) / 2, color[3]};
	return new float[4]{r, g, b, color[3]};
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
