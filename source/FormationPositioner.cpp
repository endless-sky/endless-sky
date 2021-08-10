/* FormationPositioner.cpp
Copyright (c) 2019-2021 by Peter van der Meer

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


namespace {
	void Tally(FormationPattern::ActiveFormation &af, const Body &body)
	{
		af.maxDiameter = max(af.maxDiameter, body.Radius() * 2.);
		af.maxHeight = max(af.maxHeight, body.Height());
		af.maxWidth = max(af.maxWidth, body.Width());
	}
}



void FormationPositioner::Step()
{
	// Set scaling based on results from previous run.
	activeData = nextActiveData;
	nextActiveData.ClearParticipants();
	
	// Run the iterators for the ring sections.
	unsigned int startRing = 0;
	for(auto &itShips : ringShips)
	{
		unsigned int desiredRing = itShips.first;
		auto &shipsInRing = itShips.second;
		
		// If a ring is completely empty, then we skip it.
		if(shipsInRing.empty())
			continue;
		
		// Set starting ring for current ring-section.
		startRing = max(startRing, desiredRing);
		
		// Initialize the new iterator for use for the current ring-section.
		auto itPos = pattern->begin(activeData, startRing, shipsInRing.size());
		
		// Run the iterator.
		size_t shipIndex = 0;
		while(shipIndex < shipsInRing.size())
		{
			// If the ship is no longer valid or not or no longer part of this
			// formation, then we need to remove it.
			auto ship = (shipsInRing[shipIndex]).lock();
			bool removeShip = !ship ||
				ship->GetFormationPattern() != pattern ||
				ship->GetFormationRing() != desiredRing ||
				ship->IsDisabled();
			
			// Check if this ship is set to follow the current formationleader
			// either through targetShip (for gather/keep-station commands) or
			// through the child/parent relationship.
			if(ship)
			{
				auto targetShip = ship->GetTargetShip();
				auto parentShip = ship->GetParent();
				removeShip = removeShip ||
					((!targetShip || &(*targetShip) != formationLead) &&
					(!parentShip || &(*parentShip) != formationLead));
			}
			
			// TODO: add removeShip check if ship and lead are in the same system.
			// TODO: add removeShip check for isBoarding
			// TODO: add removeShip check for isAssisting
			// TODO: add removeShip check for isLanded/Landing/Refueling
			
			// Lookup the ship in the positions map.
			auto itCoor = shipPositions.end();
			if(ship)
				itCoor = shipPositions.find(&(*ship));
			
			// If the ship is not in the overall table or not updated in the
			// last iteration, then we also remove it.
			removeShip = removeShip || itCoor == shipPositions.end() ||
					itCoor->second.second != tickTock;
			
			// Perform removes if we need to.
			if(removeShip)
			{
				// Move the last element to the current position and remove the last
				// element; this will let last ship take the position of the ship that
				// we will remove.
				if(shipIndex < (shipsInRing.size() - 1))
					shipsInRing[shipIndex].swap(shipsInRing.back());
				shipsInRing.pop_back();
				if(itCoor != shipPositions.end())
					shipPositions.erase(itCoor);
			}
			else
			{
				// Calculate the new coordinate for the current ship.
				itCoor->second.first = *itPos;
				++itPos;
				++shipIndex;
			}
		}
		// Store starting ring for next ring-positioner.
		startRing = itPos.Ring() + 1;
	}
	// Switch marker to detect stale/missing ships in the next iteration.
	tickTock = !tickTock;
	
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



Point FormationPositioner::Position(const Ship *ship)
{
	unsigned int formationRing = ship->GetFormationRing();
	
	Point relPos;
	
	auto it = shipPositions.find(ship);
	if(it != shipPositions.end())
	{
		// Retrieve the ships currently known coordinate in the formation.
		auto &status = it->second;
		if(status.second != tickTock)
		{
			// Register that this ship was seen.
			status.second = tickTock;

			// Set scaling for next round based on the sizes of the participating ships.
			Tally(nextActiveData, *ship);
		}
		// Return the cached position that we have for the ship.
		relPos = status.first;
	}
	else
	{
		// Add the ship to the set of coordinates. We add it with a default
		// coordinate of Point(0,0), it will gets its proper coordinate in
		// the next generate round.
		shipPositions[ship] = make_pair(relPos, tickTock);
		
		// Add the ship to the ring.
		ringShips[formationRing].push_back(ship->shared_from_this());
	}
	
	if(flippedY)
		relPos.Set(-relPos.X(), relPos.Y());
	if(flippedX)
		relPos.Set(relPos.X(), -relPos.Y());
	
	return formationLead->Position() + direction.Rotate(relPos);
}
