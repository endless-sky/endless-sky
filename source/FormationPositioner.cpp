/* FormationPositioner.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FormationPositioner.h"

#include "Angle.h"
#include "Point.h"
#include "Ship.h"

#include <map>
#include <vector>

using namespace std;



FormationPositioner::FormationPositioner()
{
	direction = Angle::Random();
}



void FormationPositioner::Start(Ship &ship)
{
	patterns.clear();
	
	anchor = ship.Position();
	
	// Calculate new direction, but only if the ship is moving so that we can calculate it.
	Point velocity = ship.Velocity();
	if(velocity.Length() > 0.1)
		direction = Angle(velocity);
}



Point FormationPositioner::NextPosition(Ship &ship)
{
	const FormationPattern *shipsPattern = ship.GetFormationPattern();
	
	// If the ship has no pattern, then just return center point
	if(!shipsPattern)
		return Point();
	
	auto it = patterns.find(shipsPattern);
	if(it == patterns.end())
	{
		patterns.emplace(shipsPattern, Iter(*shipsPattern));
	}
	
	return anchor + direction.Rotate((patterns.at(shipsPattern)).NextPosition(ship));
}



FormationPositioner::Iter::Iter(const FormationPattern &pattern): pattern(pattern)
{
	iteration = 0;
	activeLine = 0;
	posOnLine = 0;
	positionsOnLine = pattern.PositionsOnLine(iteration, activeLine);
}



Point FormationPositioner::Iter::NextPosition(Ship &ship)
{
	// If there are no active lines, then just return center point
	if(activeLine < 0)
		return Point();
	
	if(positionsOnLine < 0)
		positionsOnLine = pattern.PositionsOnLine(iteration, activeLine);
	
	// Iterate to next line if the current line is full
	if(posOnLine >= positionsOnLine)
	{
		int nextLine = pattern.NextLine(iteration, activeLine);
		// If no new active line, just return center point
		if(nextLine < 0)
		{
			activeLine = -1;
			return Point();
		}
		if (nextLine <= activeLine)
			iteration++;
		
		posOnLine = 0;
		activeLine = nextLine;
		positionsOnLine = pattern.PositionsOnLine(iteration, activeLine);
	}
	
	Point retVal = pattern.Position(iteration, activeLine, posOnLine) * activeScalingFactor;
	// Set values for next iteration
	posOnLine++;
	return retVal;
}
