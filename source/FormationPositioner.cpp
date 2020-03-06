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



void FormationPositioner::Start()
{
	iteration = 0;
	activeLine = 0;
	posOnLine = 0;
	positionsOnLine = -1;
}



Point FormationPositioner::NextPosition()
{
	// If there are no active lines, then just return center point.
	if(activeLine < 0)
		return Point();
	
	if(positionsOnLine < 0)
		positionsOnLine = pattern->PositionsOnLine(iteration, activeLine);
	
	// Iterate to next line if the current line is full.
	if(posOnLine >= positionsOnLine)
	{
		int nextLine = pattern->NextLine(iteration, activeLine);
		// If no new active line, just return center point.
		if(nextLine < 0)
		{
			activeLine = -1;
			return Point();
		}
		if(nextLine <= activeLine)
			iteration++;
		
		posOnLine = 0;
		activeLine = nextLine;
		positionsOnLine = pattern->PositionsOnLine(iteration, activeLine);
	}
	
	Point relPos = pattern->Position(iteration, activeLine, posOnLine) * activeScalingFactor;
	// Set values for next iteration.
	posOnLine++;

	// Calculate new direction, if the formationLead is moving, then we use the movement vector.
	// Otherwise we use the facing vector.
	Point velocity = formationLead->Velocity();
	Angle direction;
	if(velocity.Length() > 0.1)
		direction = Angle(velocity);
	else
		direction = formationLead->Facing();

	return formationLead->Position() + direction.Rotate(relPos);
}
