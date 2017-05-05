/* PlanetLabel.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PlanetLabel.h"

#include "Angle.h"
#include "Font.h"
#include "FontSet.h"
#include "Government.h"
#include "LineShader.h"
#include "pi.h"
#include "Planet.h"
#include "PointerShader.h"
#include "RingShader.h"
#include "StellarObject.h"
#include "System.h"

#include <cmath>
#include <iostream>

using namespace std;

namespace {
	static const double LINE_ANGLE[4] = {60., 120., 300., 240.};
	static const double LINE_LENGTH = 60.;
	static const double INNER_SPACE = 10.;
	static const double LINE_GAP = 1.7;
	static const double GAP = 6.;
	static const double MIN_DISTANCE = 30.;
}



PlanetLabel::PlanetLabel(const Point &position, const StellarObject &object, const System *system, double zoom)
	: position(position * zoom), radius(object.Radius() * zoom)
{
	const Planet &planet = *object.GetPlanet();
	name = planet.Name();
	if(planet.IsWormhole())
		color = Color(.8, .3, 1., 1.);
	else if(planet.GetGovernment())
	{
		government = "(" + planet.GetGovernment()->GetName() + ")";
		if(planet.CanLand())
		{
			color = planet.GetGovernment()->GetColor();
			color = Color(color.Get()[0] * .5 + .3, color.Get()[1] * .5 + .3, color.Get()[2] * .5 + .3);
		}
		else
			hostility = 3 + 2 * planet.GetGovernment()->IsEnemy();
	}
	else
	{
		color = Color(.3, .3, .3, 1.);
		government = "(No government)";
	}
	double alpha = min(.5, max(0., .6 - (position.Length() - radius) * .001 * zoom));
	color = Color(color.Get()[0] * alpha, color.Get()[1] * alpha, color.Get()[2] * alpha, 0.);
	
	if(!system)
		return;
	
	// Figure out how big the label has to be.
	double width = max(FontSet::Get(18).Width(name), FontSet::Get(14).Width(government)) + 8.;
	for(int d = 0; d < 4; ++d)
	{
		bool overlaps = false;
		
		Point start = object.Position() +
			(radius + INNER_SPACE + LINE_GAP + LINE_LENGTH) * Angle(LINE_ANGLE[d]).Unit();
		Point unit(LINE_ANGLE[d] > 180. ? -1. : 1., 0.);
		Point end = start + unit * width;
		
		for(const StellarObject &other : system->Objects())
		{
			if(&other == &object)
				continue;
			
			double minDistance = other.Radius() + MIN_DISTANCE;
			
			double startDistance = other.Position().Distance(start);
			double endDistance = other.Position().Distance(end);
			overlaps |= (startDistance < minDistance || endDistance < minDistance);
			double projection = (other.Position() - start).Dot(unit);
			if(projection > 0. && projection < width)
			{
				double distance = sqrt(startDistance * startDistance - projection * projection);
				overlaps |= (distance < minDistance);
			}
			if(overlaps)
				break;
		}
		if(!overlaps)
		{
			direction = d;
			break;
		}
	}
}



void PlanetLabel::Draw() const
{
	// Draw any active planet labels.
	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);
	
	// The angle of the outer ring should be reduced by just enough that the
	// circumference is reduced by 6 pixels.
	double innerAngle = LINE_ANGLE[direction];
	double outerAngle = innerAngle - 360. * GAP / (2. * PI * radius);
	Point unit = Angle(innerAngle).Unit();
	RingShader::Draw(position, radius + INNER_SPACE, 2.3, .9, color, 0., innerAngle);
	RingShader::Draw(position, radius + INNER_SPACE + GAP, 1.3, .6, color, 0., outerAngle);
	
	if(!name.empty())
	{
		Point from = position + (radius + INNER_SPACE + LINE_GAP) * unit;
		Point to = from + LINE_LENGTH * unit;
		LineShader::Draw(from, to, 1.3, color);
		
		double nameX = to.X() + (direction < 2 ? 2. : -bigFont.Width(name) - 2.);
		bigFont.DrawAliased(name, nameX, to.Y() - .5 * bigFont.Height(), color);
		
		double governmentX = to.X() + (direction < 2 ? 4. : -font.Width(government) - 4.);
		font.DrawAliased(government, governmentX, to.Y() + .5 * bigFont.Height() + 1., color);
	}
	Angle barbAngle(innerAngle + 36.);
	for(int i = 0; i < hostility; ++i)
	{
		barbAngle += Angle(800. / (radius + 25.));
		PointerShader::Draw(position, barbAngle.Unit(), 15., 15., radius + 25., color);
	}
}
