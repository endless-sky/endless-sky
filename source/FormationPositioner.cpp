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
	// Set scaling based on results from previous run.
	activeData = nextActiveData;
	nextActiveData.ClearParticipants();

	// Initialize the iterators and meta-data used for the ring sections.
	unsigned int startRing = 0;
	for(auto &it : ringPos)
	{
		// Track in which ring the ring-positioner ended in last run.
		unsigned int endRing = it.second.Ring();
		// Set starting ring for current ring-positioner.
		startRing = max(startRing, it.first);
		
		// Create the new iterator to use for this ring.
		it.second = pattern->begin(activeData, startRing, ringNrOfShips[it.first]);
		
		// Store starting ring for next ring-positioner.
		startRing = max(startRing, endRing) + 1;
		
		// Reset all other iterator values to start of ring.
		ringNrOfShips[it.first] = 0;
	}

	// Calculate new direction, if the formationLead is moving, then we use the movement vector.
	// Otherwise we use the facing vector.
	Point velocity = formationLead->Velocity();
	auto desiredDir = velocity.Length() > 0.1 ? Angle(velocity) : formationLead->Facing();
	
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



Point FormationPositioner::NextPosition(const Ship *ship)
{
	unsigned int formationRing = ship->GetFormationRing();

	// Retrieve the correct ring-positioner (and create it if it doesn't
	// exist yet).
	auto rPosIt = (ringPos.emplace(piecewise_construct,
		forward_as_tuple(formationRing),
		forward_as_tuple(*pattern, activeData, formationRing))).first;
	
	auto &rPos = rPosIt->second;
	
	// Set scaling for next round based on the sizes of the participating ships.
	nextActiveData.Tally(*ship);
	
	// Count the number of positions on the ring.
	++(ringNrOfShips[formationRing]);

	Point relPos = *rPos;
	
	if(flippedY)
		relPos.Set(-relPos.X(), relPos.Y());
	if(flippedX)
		relPos.Set(relPos.X(), -relPos.Y());
	
	// Set values for next ship to place.
	++rPos;

	return formationLead->Position() + direction.Rotate(relPos);
}
