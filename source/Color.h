/* Color.h
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

#pragma once

#include <string>



// Class representing an RGBA color (for use by OpenGL). The specified colors
// should be in premultiplied alpha mode. For example, a white color which is
// 50% translucent would be {.5, .5, .5} with an alpha of .5. This allows you to
// also specify colors that use additive blending. For example, if the alpha is
// zero the color's components will be added to whatever is underneath them.
class Color {
public:
	// Constructor for shades of gray. Opaque unless an alpha is also given.
	// IsLoaded is false when using this constructor.
	explicit Color(float i = 1.f, float a = 1.f);
	// Constructor for colors. Opaque unless an alpha is also given.
	Color(float r, float g, float b, float a = 1.f);

	bool operator==(const Color &other) const;
	bool operator!=(const Color &other) const;

	// Set this color to the given RGBA values.
	void Load(double r, double g, double b, double a);
	// Check if Load() has been called for this color.
	bool IsLoaded() const;
	void SetTrueName(const std::string &name);
	const std::string &TrueName() const;
	// Get the color as a float vector, suitable for use by OpenGL.
	const float *Get() const;

	// Get this color, but entirely opaque. That is, this is the color you would
	// get if drawing this color on top of opaque black.
	Color Opaque() const;
	// Assuming that this color is completely opaque, get the same color with
	// the given transparency.
	Color Transparent(float alpha) const;
	// Assuming that this is an opaque color, get its additive equivalent with
	// the given fraction of its full brightness.
	Color Additive(float alpha) const;

	// Compute a linear combination
	static Color Combine(float a1, Color c1, float a2, Color c2);

	// Multiply the RGBA values of the given Color by the given scalar and return a new Color.
	static Color Multiply(float scalar, const Color &base);


private:
	// Store the color as a float vector for easy interfacing with OpenGL.
	float color[4];

	std::string trueName;
	bool isLoaded = false;
};
