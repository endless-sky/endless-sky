/* PlanetLabel.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "PlanetLabel.h"

#include "Angle.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "Government.h"
#include "LineShader.h"
#include "pi.h"
#include "Planet.h"
#include "PointerShader.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "RingShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Wormhole.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	constexpr double LINE_ANGLE[4] = {60., 120., 300., 240.};
	constexpr double LINE_LENGTH = 60.;
	constexpr double INNER_SPACE = 10.;
	constexpr double LINE_GAP = 1.7;
	constexpr double GAP = 6.;
	constexpr double MIN_DISTANCE = 30.;

	// Check if the given label for the given stellar object and direction overlaps
	// with any existing label or any stellar object in the system.
	bool CheckOverlaps(const vector<PlanetLabel> &labels, const System &system,
			const StellarObject &object, const double zoom, const Rectangle &box)
	{
		for(const PlanetLabel &label : labels)
			if(label.Overlaps(box, zoom))
				return true;

		for(const StellarObject &other : system.Objects())
			if(&other != &object && box.Overlaps(other.Position() * zoom,
					other.Radius() * zoom + MIN_DISTANCE))
				return true;

		return false;
	}
}



PlanetLabel::PlanetLabel(const vector<PlanetLabel> &labels, const System &system,
		const StellarObject &object, const double zoom)
	: objectPosition(object.Position()), objectRadius(object.Radius())
{
	const Planet &planet = *object.GetPlanet();
	name = planet.Name();
	if(planet.IsWormhole())
		color = *planet.GetWormhole()->GetLinkColor();
	else if(planet.GetGovernment())
	{
		government = "(" + planet.GetGovernment()->GetName() + ")";
		color = planet.GetGovernment()->GetColor();
		color = Color(color.Get()[0] * .5f + .3f, color.Get()[1] * .5f + .3f, color.Get()[2] * .5f + .3f);
		if(!planet.CanLand())
			hostility = 3 + 2 * planet.GetGovernment()->IsEnemy();
	}
	else
	{
		color = Color(.3f, .3f, .3f, 1.f);
		government = "(No government)";
	}

	// Figure out how big the label has to be.
	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);
	const double width = max(bigFont.Width(name) + 4., font.Width(government) + 8.);
	const double height = bigFont.Height() + 1. + font.Height();
	// Adjust down so attachment point is at center of name.
	const Rectangle label({0., (height - bigFont.Height()) / 2.}, {width, height});

	const double minZoom = Preferences::MinViewZoom();
	const double maxZoom = Preferences::MaxViewZoom();
	// Try to find a label direction that is not overlapping under any zoom.
	for(int d = 0; d < 4; ++d)
	{
		SetBoundingBox(label, object, d);
		if(!CheckOverlaps(labels, system, object, minZoom, box + zoomOffset * minZoom)
				&& !CheckOverlaps(labels, system, object, maxZoom, box + zoomOffset * maxZoom))
		{
			direction = d;
			return;
		}
	}

	// If we can't find a suitable direction, then try to find a direction under the current
	// zoom that is not overlapping.
	for(int d = 0; d < 4; ++d)
	{
		SetBoundingBox(label, object, d);
		if(!CheckOverlaps(labels, system, object, zoom, box + zoomOffset * zoom))
		{
			direction = d;
			return;
		}
	}

	// No good choices, so reset this to the default.
	SetBoundingBox(label, object, direction);
}



void PlanetLabel::Update(const Point &center, const double zoom)
{
	position = (objectPosition - center) * zoom;
	radius = objectRadius * zoom;
}



void PlanetLabel::Draw() const
{
	// Don't draw if too far away from center of screen.
	const double offset = position.Length() - radius;
	if(offset >= 600.)
		return;

	// Fade label as we get farther from the center of the screen.
	const float alpha = static_cast<float>(min(.5, max(0., .6 - offset * .001)));
	const Color labelColor = Color(color.Get()[0] * alpha, color.Get()[1] * alpha, color.Get()[2] * alpha, 0.);

	// Draw any active planet labels.
	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);

	// The angle of the outer ring should be reduced by just enough that the
	// circumference is reduced by 6 pixels.
	double innerAngle = LINE_ANGLE[direction];
	double outerAngle = innerAngle - 360. * GAP / (2. * PI * radius);
	Point unit = Angle(innerAngle).Unit();
	RingShader::Draw(position, radius + INNER_SPACE, 2.3f, .9f, labelColor, 0.f, innerAngle);
	RingShader::Draw(position, radius + INNER_SPACE + GAP, 1.3f, .6f, labelColor, 0.f, outerAngle);

	if(!name.empty())
	{
		Point from = position + (radius + INNER_SPACE + LINE_GAP) * unit;
		Point to = from + LINE_LENGTH * unit;
		LineShader::Draw(from, to, 1.3f, labelColor);

		double nameX = to.X() + (direction < 2 ? 2. : -bigFont.Width(name) - 2.);
		bigFont.DrawAliased(name, nameX, to.Y() - .5 * bigFont.Height(), labelColor);

		double governmentX = to.X() + (direction < 2 ? 4. : -font.Width(government) - 4.);
		font.DrawAliased(government, governmentX, to.Y() + .5 * bigFont.Height() + 1., labelColor);
	}
	Angle barbAngle(innerAngle + 36.);
	for(int i = 0; i < hostility; ++i)
	{
		barbAngle += Angle(800. / (radius + 25.));
		PointerShader::Draw(position, barbAngle.Unit(), 15.f, 15.f, radius + 25., labelColor);
	}
}



bool PlanetLabel::Overlaps(const Rectangle &otherBox, const double zoom) const
{
	return (box + zoom * zoomOffset).Overlaps(otherBox);
}



void PlanetLabel::SetBoundingBox(const Rectangle &label, const StellarObject &object, const int direction)
{
	const Point unit = Angle(LINE_ANGLE[direction]).Unit();
	zoomOffset = object.Position() + unit * object.Radius();

	// Offset the label depending on its position relative to the stellar object.
	const Point halfUnit(LINE_ANGLE[direction] < 180. ? .5 : -.5, 0.);
	box = label + unit * (INNER_SPACE + LINE_GAP + LINE_LENGTH) + halfUnit * label.Width();
}
