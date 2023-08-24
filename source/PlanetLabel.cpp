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
#include <array>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	// Label offset angles, in order of preference (non-negative only).
	constexpr array<double, 12> LINE_ANGLES = {60., 120., 300., 240., 30., 150., 330., 210., 90., 270., 0., 180.};
	constexpr double LINE_LENGTH = 60.;
	constexpr double INNER_SPACE = 10.;
	constexpr double LINE_GAP = 1.7;
	constexpr double GAP = 6.;
	constexpr double MIN_DISTANCE = 30.;

	// Find the intersection of a ray and a box, both centered on the origin.
	// Then shift the resulting point up and left by half the box size (since
	// text drawing coords start at the upper left, not center, of the text),
	// and shift 2 more away from the origin on whatever side it's on.
	Point GetOffset(const Point &unit, const Point &dimensions)
	{
		// Box ranges from (-width/2, -height/2) to (width/2, height/2).
		const Point box = dimensions * .5;

		// Special case to avoid division by zero.
		if(unit.X() == 0.)
		{
			if(unit.Y() < 0.)
				return Point(-box.X(), -dimensions.Y() - 2.);
			else
				return Point(-box.X(), 2.);
		}

		const double slope = unit.Y() / unit.X();
		const double y = slope * box.X();

		// Left and right sides.
		if(-box.Y() <= y && y <= box.Y())
		{
			if(unit.X() < 0.)
				return Point(-dimensions.X() - 2., -y - box.Y());
			else
				return Point(2., y - box.Y());
		}
		// Top and bottom sides.
		else
		{
			const double x = box.Y() / slope;
			if(unit.Y() < 0.)
				return Point(-x - box.X(), -dimensions.Y() - 2.);
			else
				return Point(x - box.X(), 2.);
		}
	}
}



PlanetLabel::PlanetLabel(const vector<PlanetLabel> &labels, const System &system, const StellarObject &object)
	: objectPosition(object.Position()), objectRadius(object.Radius())
{
	const Planet &planet = *object.GetPlanet();
	name = planet.Name();
	if(planet.IsWormhole())
		color = *planet.GetWormhole()->GetLinkColor();
	else if(planet.GetGovernment())
	{
		government = "(" + planet.GetGovernment()->GetName() + ")";
		color = Color::Combine(.5f, planet.GetGovernment()->GetColor(), 1.f, Color(.3f));
		if(!planet.CanLand())
			hostility = 3 + 2 * planet.GetGovernment()->IsEnemy();
	}
	else
	{
		government = "(No government)";
		color = Color(.3f);
	}

	// Figure out how big the label is.
	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);
	const double labelWidth = max(bigFont.Width(name) + 4., font.Width(government) + 8.);
	const double nameHeight = bigFont.Height();
	const double labelHeight = nameHeight + 1. + font.Height();
	const Point labelDimensions = {labelWidth, labelHeight};

	// Try to find a label direction that is not overlapping under any zoom.
	const double minZoom = Preferences::MinViewZoom();
	const double maxZoom = Preferences::MaxViewZoom();
	for(const double angle : LINE_ANGLES)
	{
		SetBoundingBox(labelDimensions, angle);
		if(!HasOverlaps(labels, system, object, minZoom) && !HasOverlaps(labels, system, object, maxZoom))
		{
			innerAngle = angle;
			break;
		}
	}

	// No good choices, so set this to the default.
	if(innerAngle < 0.)
	{
		innerAngle = LINE_ANGLES[0];
		SetBoundingBox(labelDimensions, innerAngle);
	}

	const bool leftSide = innerAngle > 180.;
	const Point offset = GetOffset(Angle(innerAngle).Unit(), box.Dimensions());

	// Cache the offsets for both labels.

	// Box was made with the larger of two widths, so adjust the smaller one if on the left.
	const double widthDiff = bigFont.Width(name) - font.Width(government);
	const double nameOffsetX = leftSide && widthDiff < 0. ? -widthDiff : 0.;
	nameOffset = Point(offset.X() + nameOffsetX, offset.Y());
	// Government is offset 2 in if it's the smaller label.
	const double governmentOffsetX = leftSide ? (widthDiff < 0. ? 0. : widthDiff - 2.) :
		(widthDiff < 0. ? 0. : 2.);
	governmentOffset = Point(offset.X() + governmentOffsetX, offset.Y() + nameHeight + 1.);
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
	const double outerAngle = innerAngle - 360. * GAP / (2. * PI * radius);
	RingShader::Draw(position, radius + INNER_SPACE, 2.3f, .9f, labelColor, 0.f, innerAngle);
	RingShader::Draw(position, radius + INNER_SPACE + GAP, 1.3f, .6f, labelColor, 0.f, outerAngle);

	const double barbRadius = radius + 25.;
	Angle barbAngle(innerAngle + 36.);
	for(int i = 0; i < hostility; ++i)
	{
		barbAngle += Angle(800. / barbRadius);
		PointerShader::Draw(position, barbAngle.Unit(), 15.f, 15.f, barbRadius, labelColor);
	}

	// Draw any active planet labels.
	if(!name.empty())
	{
		const Point unit = Angle(innerAngle).Unit();
		const Point from = position + (radius + INNER_SPACE + LINE_GAP) * unit;
		const Point to = from + LINE_LENGTH * unit;
		LineShader::Draw(from, to, 1.3f, labelColor);

		FontSet::Get(18).DrawAliased(name, to.X() + nameOffset.X(),
			to.Y() + nameOffset.Y(), labelColor);
		FontSet::Get(14).DrawAliased(government, to.X() + governmentOffset.X(),
			to.Y() + governmentOffset.Y(), labelColor);
	}

}



void PlanetLabel::SetBoundingBox(const Point &labelDimensions, const double angle)
{
	const Point unit = Angle(angle).Unit();
	zoomOffset = objectPosition + unit * objectRadius;
	box = Rectangle(unit * (INNER_SPACE + LINE_GAP + LINE_LENGTH) + GetOffset(unit, labelDimensions),
		labelDimensions);
}



Rectangle PlanetLabel::GetBoundingBox(const double zoom) const
{
	return box + zoomOffset * zoom;
}



// Check if the label for the given stellar object overlaps
// with any existing label or any other stellar object in the system.
bool PlanetLabel::HasOverlaps(const vector<PlanetLabel> &labels, const System &system,
		const StellarObject &object, const double zoom) const
{
	const Rectangle boundingBox = GetBoundingBox(zoom);

	for(const PlanetLabel &label : labels)
		if(boundingBox.Overlaps(label.GetBoundingBox(zoom)))
			return true;

	for(const StellarObject &other : system.Objects())
		if(&other != &object && boundingBox.Overlaps(other.Position() * zoom,
				other.Radius() * zoom + MIN_DISTANCE))
			return true;

	return false;
}
