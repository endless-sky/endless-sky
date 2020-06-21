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
#include "Body.h"
#include "FormationPattern.h"
#include "Point.h"
#include "Ship.h"

#include <cmath>

using namespace std;



void FormationPositioner::Start()
{
	unsigned int startRing = 0;
	for(auto &it : ringPos)
	{
		// Track in which ring the ring-positioner ended in last run.
		unsigned int endRing = it.second.ring;
		// Set starting ring for current ring-positioner.
		it.second.ring = max(startRing, it.first);
		// Store starting ring for next ring-positioner.
		startRing = endRing + 1;
		
		// Track the amount of positions in the ring
		it.second.prevLastPos = it.second.lastPos;
		it.second.lastPos = 0;
		
		// Reset all other iterator values to start of ring.
		it.second.activeLine = 0;
		it.second.activeRepeat = 0;
		it.second.lineSlot = 0;
		it.second.morePositions = true;
	}

	// Set scaling based on results from previous run.
	maxDiameter = nextMaxDiameter;
	maxHeight = nextMaxHeight;
	maxWidth = nextMaxWidth;
	nextMaxDiameter = 1.;
	nextMaxHeight = 1.;
	nextMaxWidth = 1.;
	
	// Calculate new direction, if the formationLead is moving, then we use the movement vector.
	// Otherwise we use the facing vector.
	Point velocity = formationLead->Velocity();
	Angle desiredDir;
	if(velocity.Length() > 0.1)
		desiredDir = Angle(velocity);
	else
		desiredDir = formationLead->Facing();
	
	Angle deltaDir = desiredDir - direction;
	
	// Change the desired direction according to rotational settings if that fits better.
	double symRot = pattern->Rotatable();
	if(symRot > 0 && fabs(deltaDir.Degrees()) > (symRot/2))
	{
		if(deltaDir.Degrees() > 0)
			symRot = -symRot;
		
		while(fabs(deltaDir.Degrees() + symRot) < fabs(deltaDir.Degrees()))
		{
			desiredDir += Angle(symRot);
			deltaDir = desiredDir - direction;
		}
	}
	
	// Angle at which to perform longitudinal or transverse mirror instead of turn.
	static const double MIN_FLIP_TRIGGER = 135.;
	
	// If we are beyond the triggers for flipping, then immediately go to the desired direction.
	if(fabs(deltaDir.Degrees()) >= MIN_FLIP_TRIGGER &&
		(pattern->FlippableY() || pattern->FlippableX()))
	{
		direction = desiredDir;
		deltaDir = Angle(0.);
		if(pattern->FlippableY())
			flippedY = !flippedY;
		if(pattern->FlippableX())
			flippedX = !flippedX;
	}
	else
	{
		// Turn max 1/4th degree per frame. The game runs at 60fps, so a turn of 180 degrees will take
		// about 12 seconds.
		static const double MAX_FORMATION_TURN = 0.25;
		
		if(deltaDir.Degrees() > MAX_FORMATION_TURN)
			deltaDir = Angle(MAX_FORMATION_TURN);
		else if(deltaDir.Degrees() < -MAX_FORMATION_TURN)
			deltaDir = Angle(-MAX_FORMATION_TURN);
		
		direction += deltaDir;
	}
}



Point FormationPositioner::NextPosition(const Ship * ship)
{
	// Retrieve the correct ring-positioner.
	RingPositioner &rPos = ringPos[ship->GetFormationRing()];
	
	// Set scaling for next round based on the sizes of the participating ships.
	nextMaxDiameter = max(nextMaxDiameter, ship->Radius() * 2.);
	nextMaxHeight = max(nextMaxHeight, ship->Height());
	nextMaxWidth = max(nextMaxWidth, ship->Width());
	
	// Count the number of positions on the ring.
	++(rPos.lastPos);
	
	// If there are no more positions to fill, then just return center point.
	if(!rPos.morePositions)
		return Point();
	
	// Check how many lines are available.
	unsigned int lines = pattern->Lines();
	if(lines < 1)
	{
		rPos.morePositions = false;
		return Point();
	}
	
	unsigned int ringsScanned = 0;
	unsigned int startingRing = rPos.ring;
	unsigned int lineRepeatSlots = pattern->Slots(rPos.ring, rPos.activeLine, rPos.activeRepeat);
	
	while(rPos.lineSlot >= lineRepeatSlots && rPos.morePositions)
	{
		unsigned int patternRepeats = pattern->Repeats(rPos.activeLine);
		// LineSlot number is beyond the amount of slots available.
		// Need to move a ring, a line or a repeat-section forward.
		if(rPos.ring > 0 && rPos.activeLine < lines && patternRepeats > 0 && rPos.activeRepeat < patternRepeats - 1 )
		{
			// First check if we are on a valid line and have another repeat section.
			++(rPos.activeRepeat);
			rPos.lineSlot = 0;
			lineRepeatSlots = pattern->Slots(rPos.ring, rPos.activeLine, rPos.activeRepeat);
		}
		else if(rPos.activeLine < lines - 1)
		{
			// If we don't have another repeat section, then check for a next line.
			++(rPos.activeLine);
			rPos.activeRepeat = 0;
			rPos.lineSlot = 0;
			lineRepeatSlots = pattern->Slots(rPos.ring, rPos.activeLine, rPos.activeRepeat);
		}
		else
		{
			// If we checked all lines and repeat sections, then go for the next ring.
			++(rPos.ring);
			rPos.activeLine = 0;
			rPos.activeRepeat = 0;
			rPos.lineSlot = 0;
			lineRepeatSlots = pattern->Slots(rPos.ring, rPos.activeLine, rPos.activeRepeat);
			
			// If we scanned more than 1 rings without finding a slot, then we have an empty pattern.
			++ringsScanned;
			if(ringsScanned > 1)
			{
				// Restore starting ring and indicate that there are no more positions.
				rPos.ring = startingRing;
				rPos.morePositions = false;
			}
		}
	}
	// No position(s) found
	if(!rPos.morePositions)
		return Point();
	
	// Estimate how many positions still to fill and do centering if required.
	if(rPos.lineSlot == 0 && rPos.lastPos < rPos.prevLastPos)
	{
		unsigned int remainingToFill = rPos.prevLastPos - rPos.lastPos;
		if(remainingToFill < lineRepeatSlots - 1 &&	pattern->IsCentered(rPos.activeLine))
		{
			// Determine the amount to skip for centering and skip those.
			int toSkip = (lineRepeatSlots - remainingToFill) / 2;
			rPos.lineSlot += toSkip;
		}
	}
	
	Point relPos = pattern->Position(rPos.ring, rPos.activeLine, rPos.activeRepeat, rPos.lineSlot, maxDiameter, maxWidth, maxHeight);
	
	if(flippedY)
		relPos.Set(-relPos.X(), relPos.Y());
	if(flippedX)
		relPos.Set(relPos.X(), -relPos.Y());
	
	// Set values for next ship to place.
	rPos.lineSlot++;

	return formationLead->Position() + direction.Rotate(relPos);
}
