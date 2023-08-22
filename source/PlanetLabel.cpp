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

	// Check if the label for the given stellar object overlaps
	// with any existing label or any other stellar object in the system.
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
		color = Color::Combine(.5f, planet.GetGovernment()->GetColor(), .3f, Color());
		if(!planet.CanLand())
			hostility = 3 + 2 * planet.GetGovernment()->IsEnemy();
	}
	else
	{
		color = Color(.3f);
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
		SetBoundingBox(label, d);
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
		SetBoundingBox(label, d);
		if(!CheckOverlaps(labels, system, object, zoom, box + zoomOffset * zoom))
		{
			direction = d;
			return;
		}
	}

	// No good choices, so set this to the default.
	SetBoundingBox(label, direction);
}



void PlanetLabel::Update(const Point &center, const double zoom)
{
	position = (objectPosition - center) * zoom;
	radius = objectRadius * zoom;
}



void PlanetLabel::Draw() const
{
	// Don't draw if too far away from the center of the screen.
	const double offset = position.Length() - radius;
	if(offset >= 600.)
		return;

	// Fade label as we get farther from the center of the screen.
	const Color labelColor = color.Additive(min(.5, .6 - offset * .001));

	// The angle of the outer ring should be reduced by just enough that the
	// circumference is reduced by GAP pixels.
	const double innerAngle = LINE_ANGLE[direction];
	const double outerAngle = innerAngle - 360. * GAP / (2. * PI * radius);
	const Point unit = Angle(innerAngle).Unit();
	RingShader::Draw(position, radius + INNER_SPACE, 2.3f, .9f, labelColor, 0.f, innerAngle);
	RingShader::Draw(position, radius + INNER_SPACE + GAP, 1.3f, .6f, labelColor, 0.f, outerAngle);

	// Draw any active planet labels.
	if(!name.empty())
	{
		const Point from = position + (radius + INNER_SPACE + LINE_GAP) * unit;
		const Point to = from + LINE_LENGTH * unit;
		LineShader::Draw(from, to, 1.3f, labelColor);

		const Font &bigFont = FontSet::Get(18);
		const double halfHeight = bigFont.Height() * .5;
		const double nameX = to.X() + (direction < 2 ? 2. : -bigFont.Width(name) - 2.);
		bigFont.DrawAliased(name, nameX, to.Y() - halfHeight, labelColor);

		const Font &font = FontSet::Get(14);
		const double governmentX = to.X() + (direction < 2 ? 4. : -font.Width(government) - 4.);
		font.DrawAliased(government, governmentX, to.Y() + halfHeight + 1., labelColor);
	}

	const double barbRadius = radius + 25.;
	Angle barbAngle(innerAngle + 36.);
	for(int i = 0; i < hostility; ++i)
	{
		barbAngle += Angle(800. / barbRadius);
		PointerShader::Draw(position, barbAngle.Unit(), 15.f, 15.f, barbRadius, labelColor);
	}
}



bool PlanetLabel::Overlaps(const Rectangle &otherBox, const double zoom) const
{
	return (box + zoomOffset * zoom).Overlaps(otherBox);
}



void PlanetLabel::SetBoundingBox(const Rectangle &label, const int d)
{
	const Point unit = Angle(LINE_ANGLE[d]).Unit();
	zoomOffset = objectPosition + unit * objectRadius;

	// Offset the label depending on its position relative to the stellar object.
	const Point halfUnit(LINE_ANGLE[d] < 180. ? .5 : -.5, 0.);
	box = label + unit * (INNER_SPACE + LINE_GAP + LINE_LENGTH) + halfUnit * label.Width();
}
