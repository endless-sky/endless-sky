/* ScrollBar.cpp
Copyright (c) 2024 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ScrollBar.h"

#include "shader/LineShader.h"
#include "shader/PointerShader.h"

#include <algorithm>

using namespace std;

namespace {
	// Additional distance the scrollbar's tab can be selected from.
	constexpr float SCROLLBAR_MOUSE_ADDITIONAL_RANGE = 5.;
}



ScrollBar::ScrollBar(float fraction, float displaySizeFraction, const Point &from, const Point &to,
	float tabWidth, float lineWidth, const Color &color, const Color &innerColor) noexcept
	: fraction(fraction), displaySizeFraction(displaySizeFraction), from(from), to(to),
		tabWidth(tabWidth), lineWidth(lineWidth), color(color), innerColor(innerColor)
{
}



ScrollBar::ScrollBar() noexcept
	: ScrollBar(0., 0., Point(), Point(),
		3, 3, Color(.6), Color(.25))
{
}



void ScrollBar::Draw()
{
	DrawAt(from);
}



void ScrollBar::DrawAt(const Point &from)
{
	Point delta = to - this->from;
	LineShader::Draw(
		from,
		from + delta,
		lineWidth,
		innerHighlighted ? innerColor : Color::Multiply(.5, innerColor)
	);

	Point deltaOffset = delta * displaySizeFraction;
	Point offset = delta * (1 - displaySizeFraction) * fraction;
	LineShader::Draw(
		from + offset + deltaOffset,
		from + offset,
		tabWidth,
		highlighted ? color : Color::Combine(.5, color, .5, innerColor)
	);

	PointerShader::Draw(from, Point(0., -1.), lineWidth * 3., 10.f, 5.f, fraction > 0. ? color : innerColor);
	PointerShader::Draw(from + delta, Point(0., 1.), lineWidth * 3., 10.f, 5.f, fraction < 1. ? color : innerColor);
}



bool ScrollBar::Hover(int x, int y)
{
	Point delta = to - from;
	Point offset = delta * (1.0 - displaySizeFraction) * fraction;

	// Find the distance from a point (p) to the line segment a->b
	// From https://www.shadertoy.com/view/3tdSDj
	constexpr auto LineSDF = [](Point a, Point b, Point p)
	{
		Point ab = b - a;
		Point ap = p - a;

		double h = clamp(ap.Dot(ab) / ab.LengthSquared(), 0., 1.);
		double d = (ap - ab * h).Length();

		return d;
	};

	Point a = from + offset;
	Point b = a + delta * displaySizeFraction;

	Point p(x, y);

	highlighted = LineSDF(a, b, p) <= tabWidth + SCROLLBAR_MOUSE_ADDITIONAL_RANGE;
	innerHighlighted = LineSDF(from, to, p) <= lineWidth + SCROLLBAR_MOUSE_ADDITIONAL_RANGE;

	return false;
}



bool ScrollBar::Drag(double dx, double dy)
{
	if(!highlighted)
		return false;

	Point dragVector(dx, dy);
	Point thisVector = to - from;

	double scalarProjectionOverLength = thisVector.Dot(dragVector) / thisVector.LengthSquared();

	fraction += scalarProjectionOverLength / (1. - displaySizeFraction);

	return true;
}



bool ScrollBar::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	Point clickPos(x, y);
	if((clickPos - from).Length() < 10.)
	{
		fraction = clamp(fraction - displaySizeFraction * .6f, 0.f, 1.f);
		return true;
	}
	if((clickPos - to).Length() < 10.)
	{
		fraction = clamp(fraction + displaySizeFraction * .6f, 0.f, 1.f);
		return true;
	}

	if(innerHighlighted && !highlighted)
	{
		Point cursorVector = clickPos - from;
		Point thisVector = to - from;

		double scalarProjectionOverLength = thisVector.Dot(cursorVector) / thisVector.LengthSquared();

		fraction = (scalarProjectionOverLength - .5) / (1. - displaySizeFraction) + .5;

		highlighted = true;
	}

	return highlighted;
}
