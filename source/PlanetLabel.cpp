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
	// Label offset angles, in order of preference (do not use 0).
	constexpr array<double, 8> LINE_ANGLES = {60., 120., 300., 240., 30., 150., 330., 210.};
	constexpr double LINE_LENGTH = 60.;
	constexpr double INNER_SPACE = 10.;
	constexpr double LINE_GAP = 1.7;
	constexpr double GAP = 6.;
	constexpr double MIN_DISTANCE = 30.;
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
		SetBoundingBox(labelDimensions, angle, nameHeight);
		if(!HasOverlaps(labels, system, object, minZoom) && !HasOverlaps(labels, system, object, maxZoom))
		{
			innerAngle = angle;
			break;
		}
	}

	// No good choices, so set this to the default.
	if(!innerAngle)
	{
		innerAngle = LINE_ANGLES[0];
		SetBoundingBox(labelDimensions, innerAngle, nameHeight);
	}

	// Where the label is relative to the object.
	const bool rightSide = innerAngle < 180.;
	const bool topSide = innerAngle < 45. || 315. < innerAngle;
	const bool bottomSide = 135. < innerAngle && innerAngle < 225.;

	// Have to adjust the more extreme angles differently or it looks bad.
	const double yOffset = topSide ? nameHeight * 2. / 3. : bottomSide ? nameHeight / 3. : nameHeight * .5;

	// Cache the offsets for both labels.
	nameOffset = Point(rightSide ? 2. : -bigFont.Width(name) - 2., -yOffset);
	governmentOffset = Point(rightSide ? 4. : -font.Width(government) - 4., nameOffset.Y() + nameHeight + 1.);
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



void PlanetLabel::SetBoundingBox(const Point &labelDimensions, const double angle, const int nameHeight)
{
	const Point unit = Angle(angle).Unit();
	zoomOffset = objectPosition + unit * objectRadius;

	// Where the label is relative to the object.
	const bool rightSide = angle < 180.;
	const bool topSide = angle < 45. || 315. < angle;
	const bool bottomSide = 135. < angle && angle < 225.;

	// Offset the label depending on its position relative to the stellar object.
	const double xOffset = (rightSide ? labelDimensions.X() : -labelDimensions.X()) * 0.5;
	const double yOffset = topSide ? nameHeight * 2. / 3. : bottomSide ? nameHeight / 3. : nameHeight * .5;
	box = Rectangle(unit * (INNER_SPACE + LINE_GAP + LINE_LENGTH) +
		Point(xOffset, labelDimensions.Y() * .5 - yOffset), labelDimensions);
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
	const Rectangle box = GetBoundingBox(zoom);

	for(const PlanetLabel &label : labels)
		if(box.Overlaps(label.GetBoundingBox(zoom)))
			return true;

	for(const StellarObject &other : system.Objects())
		if(&other != &object && box.Overlaps(other.Position() * zoom, other.Radius() * zoom + MIN_DISTANCE))
			return true;

	return false;
}
