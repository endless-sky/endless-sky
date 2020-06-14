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
	int startRing = 0;
	for(auto &it : ringPos)
	{
		// Track in which ring the ring-positioner ended in last run.
		int endRing = it.second.ring;
		// Set starting ring for current ring-positioner.
		it.second.ring = max(startRing, it.first);
		// Store starting ring for next ring-positioner.
		startRing = endRing + 1;
		
		// Reset all other iterator values to start of ring.
		it.second.activeLine = 0;
		it.second.lineSlot = 0;
		it.second.lineSlots = -1;
		
		// Set scaling based on results from previous run.
		it.second.maxDiameter = it.second.nextMaxDiameter;
		it.second.maxHeight = it.second.nextMaxHeight;
		it.second.maxWidth = it.second.nextMaxWidth;
		it.second.nextMaxDiameter = 1.;
		it.second.nextMaxHeight = 1.;
		it.second.nextMaxWidth = 1.;
	}
	
	// Calculate new direction, if the formationLead is moving, then we use the movement vector.
	// Otherwise we use the facing vector.
	Point velocity = formationLead->Velocity();
	Angle desiredDir;
	if(velocity.Length() > 0.1)
		desiredDir = Angle(velocity);
	else
		desiredDir = formationLead->Facing();
	
	// If we don't have any rings (from previous iterations), then align the direction directly with the lead.
	if(ringPos.size() == 0)
		direction = desiredDir;
	
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
			mirroredLongitudinal = !mirroredLongitudinal;
		if(pattern->FlippableX())
			mirroredTransverse = !mirroredTransverse;
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
	rPos.nextMaxDiameter = max(rPos.nextMaxDiameter, ship->Radius() * 2.);
	rPos.nextMaxHeight = max(rPos.nextMaxHeight, ship->Height());
	rPos.nextMaxWidth = max(rPos.nextMaxWidth, ship->Width());
	
	// If there are no active lines, then just return center point.
	if(rPos.activeLine < 0)
		return Point();
	
	// Handle trigger to initialize lineSlots if required.
	if(rPos.lineSlots < 0)
		rPos.lineSlots = pattern->LineSlots(rPos.ring, rPos.activeLine);
	
	// Iterate to next line if the current line is full.
	if(rPos.lineSlot >= rPos.lineSlots)
	{
		int nextLine = pattern->NextLine(rPos.ring, rPos.activeLine);
		// If no new active line, just return center point.
		if(nextLine < 0)
		{
			rPos.activeLine = -1;
			return Point();
		}
		// If we get back to an earlier line, then we moved a ring up.
		if(nextLine <= rPos.activeLine)
			rPos.ring++;
		
		rPos.lineSlot = 0;
		rPos.activeLine = nextLine;
		rPos.lineSlots = pattern->LineSlots(rPos.ring, rPos.activeLine);
	}
	
	Point relPos = pattern->Position(rPos.ring, rPos.activeLine, rPos.lineSlot, rPos.maxDiameter, rPos.maxWidth, rPos.maxHeight);
	
	if(mirroredLongitudinal)
		relPos.Set(-relPos.X(), relPos.Y());
	if(mirroredTransverse)
		relPos.Set(relPos.X(), -relPos.Y());
	
	// Set values for next ring.
	rPos.lineSlot++;

	return formationLead->Position() + direction.Rotate(relPos);
}
