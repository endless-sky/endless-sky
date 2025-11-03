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
#include "shader/LineShader.h"
#include "pi.h"
#include "Planet.h"
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "shader/RingShader.h"
#include "StellarObject.h"
#include "System.h"
#include "Wormhole.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	// Label offset angles, in order of preference (non-negative only).
	constexpr double LINE_ANGLES[] = {60., 120., 300., 240., 30., 150., 330., 210., 90., 270., 0., 180.};
	constexpr double LINE_LENGTH = 60.;
	constexpr double INNER_SPACE = 10.;
	constexpr double LINE_GAP = 1.7;
	constexpr double GAP = 6.;
	constexpr double MIN_DISTANCE = 30.;
	constexpr double BORDER = 2.;

	// Find the intersection of a ray and a rectangle, both centered on the origin.
	Point GetOffset(const Point &unit, const Point &dimensions)
	{
		// Rectangle ranges from (-width/2, -height/2) to (width/2, height/2).
		const Point box = dimensions * .5;

		// Check to avoid division by zero.
		if(unit.X() == 0.)
			return Point(0., copysign(box.Y(), unit.Y()));

		const double slope = unit.Y() / unit.X();

		// Left and right sides.
		const double y = fabs(box.X() * slope);
		if(y <= box.Y())
			return Point(copysign(box.X(), unit.X()), copysign(y, unit.Y()));

		// Top and bottom sides.
		const double x = box.Y() / slope;
		return Point(copysign(x, unit.X()), copysign(box.Y(), unit.Y()));
	}
}



PlanetLabel::PlanetLabel(const vector<PlanetLabel> &labels, const System &system, const StellarObject &object)
	: object(&object)
{
	UpdateData(labels, system);
}



void PlanetLabel::Update(const Point &center, const double zoom, const vector<PlanetLabel> &labels,
	const System &system)
{
	drawCenter = center;
	position = (object->Position() - center) * zoom;
	radius = object->Radius() * zoom;
	UpdateData(labels, system);
}



void PlanetLabel::Draw() const
{
	// Don't draw if too far away from the center of the screen.
	const double offset = position.Length() - radius;
	const double objectDistanceAlpha = object->DistanceAlpha(drawCenter);
	if(offset >= 600. || objectDistanceAlpha == 0.)
		return;

	// Fade label as we get farther from the center of the screen.
	const Color labelColor = color.Additive(min(.5, .6 - offset * .001) * objectDistanceAlpha);

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

	// Draw planet name label, if any.
	if(!name.empty())
	{
		const Point unit = Angle(innerAngle).Unit();
		const Point from = position + unit * (radius + INNER_SPACE + LINE_GAP);
		const Point to = from + unit * LINE_LENGTH;
		LineShader::Draw(from, to, 1.3f, labelColor);

		// Use non-rounding version to prevent labels from jittering.
		FontSet::Get(18).DrawAliased(name, to.X() + nameOffset.X(),
			to.Y() + nameOffset.Y(), labelColor);
		FontSet::Get(14).DrawAliased(government, to.X() + governmentOffset.X(),
			to.Y() + governmentOffset.Y(), labelColor);
	}
}



void PlanetLabel::UpdateData(const vector<PlanetLabel> &labels, const System &system)
{
	bool reposition = false;
	const Planet &planet = *object->GetPlanet();
	if(planet.DisplayName() != name)
		reposition = true;
	name = planet.DisplayName();
	if(planet.IsWormhole())
		color = *planet.GetWormhole()->GetLinkColor();
	else if(planet.GetGovernment())
	{
		string newGovernment = "(" + planet.GetGovernment()->DisplayName() + ")";
		if(newGovernment != government)
			reposition = true;
		government = newGovernment;
		color = Color::Combine(.5f, planet.GetGovernment()->GetColor(), 1.f, Color(.3f));
		if(!planet.CanLand())
			hostility = 3 + 2 * planet.GetGovernment()->IsEnemy();
	}
	else
	{
		string newGovernment = "(No government)";
		if(newGovernment != government)
			reposition = true;
		color = Color(.3f);
	}

	if(!reposition)
		return;

	// Figure out how big the label is.
	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);
	const double labelWidth = max(bigFont.Width(name), font.Width(government));
	const double nameHeight = bigFont.Height();
	const double labelHeight = nameHeight + (government.empty() ? 0. : 1. + font.Height());
	const Point labelDimensions = {labelWidth + BORDER * 2., labelHeight + BORDER * 2.};

	// Try to find a label direction that is not overlapping under any zoom.
	const vector<double> &allZooms = Preferences::Zooms();
	for(const double angle : LINE_ANGLES)
	{
		SetBoundingBox(labelDimensions, angle);
		if(none_of(allZooms.begin(), allZooms.end(),
				[&](const double zoom)
				{
					return HasOverlaps(labels, system, *object, zoom);
				}))
		{
			innerAngle = angle;
			break;
		}
	}

	// No non-overlapping choices, so set this to the default.
	if(innerAngle < 0.)
	{
		innerAngle = LINE_ANGLES[0];
		SetBoundingBox(labelDimensions, innerAngle);
	}

	// Cache the offsets for both labels; center labels.
	const Point offset = GetOffset(Angle(innerAngle).Unit(), labelDimensions) - labelDimensions * .5;
	const double nameX = (labelDimensions.X() - bigFont.Width(name)) * .5;
	nameOffset = Point(offset.X() + nameX, offset.Y() + BORDER);
	const double governmentX = (labelDimensions.X() - font.Width(government)) * .5;
	governmentOffset = Point(offset.X() + governmentX, nameOffset.Y() + nameHeight + 1.);
}



void PlanetLabel::SetBoundingBox(const Point &labelDimensions, const double angle)
{
	const Point unit = Angle(angle).Unit();
	zoomOffset = object->Position() + unit * object->Radius();
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
