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

using namespace std;



void FormationPositioner::Start()
{
	ring = 0;
	activeLine = 0;
	lineSlot = 0;
	lineSlots = -1;
}



Point FormationPositioner::NextPosition(int minimumRing)
{
	// Set the minimumRing if we have one.
	if(minimumRing > ring)
	{
		// Set ring to minimum.
		ring = minimumRing;
		// Reset all other iterator values.
		activeLine=0;
		lineSlots=-1;
		lineSlot=0;
	}
	
	// If there are no active lines, then just return center point.
	if(activeLine < 0)
		return Point();
	
	if(lineSlots < 0)
		lineSlots = pattern->LineSlots(ring, activeLine);
	
	// Iterate to next line if the current line is full.
	if(lineSlot >= lineSlots)
	{
		int nextLine = pattern->NextLine(ring, activeLine);
		// If no new active line, just return center point.
		if(nextLine < 0)
		{
			activeLine = -1;
			return Point();
		}
		// If we get back to an earlier line, then we moved a ring up.
		if(nextLine <= activeLine)
			ring++;
		
		lineSlot = 0;
		activeLine = nextLine;
		lineSlots = pattern->LineSlots(ring, activeLine);
	}
	
	Point relPos = pattern->Position(ring, activeLine, lineSlot) * activeScalingFactor;
	// Set values for next ring.
	lineSlot++;

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
