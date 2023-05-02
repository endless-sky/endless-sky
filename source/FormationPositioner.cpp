/* FormationPositioner.cpp
Copyright (c) 2019-2022 by Peter van der Meer

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


// Initializer based on the formation pattern to follow.
FormationPositioner::FormationPositioner(const Body *formationLead, const FormationPattern *pattern)
	: formationLead(formationLead), pattern(pattern), direction(formationLead->Facing())
{
}



void FormationPositioner::Step()
{
	// Calculate the direction in which the formation is facing. We perform
	// this calculation every step, because we want to act on course-changes
	// from the lead ship as soon as they happen.
	CalculateDirection();

	// Calculate the set of positions for the participating ships. This calculation
	// can be performed once very 20 game-steps, because the positions are
	// relatively stable and slower changes don't impact the formation a lot.
	constexpr int POSITIONS_INTERVAL = 20;
	if(positionsTimer == 0)
	{
		CalculatePositions();
		positionsTimer = POSITIONS_INTERVAL;
	}
	else
		positionsTimer--;
}



Point FormationPositioner::Position(const Ship *ship)
{
	// TODO: Remove formationRing completely (including the ringShips variable)
	//unsigned int formationRing = ship->GetFormationRing();
	unsigned int formationRing = 0;

	Point relPos;

	auto it = shipPositions.find(ship);
	if(it != shipPositions.end())
	{
		// Retrieve the ships currently known coordinate in the formation.
		auto &status = it->second;

		// Register that this ship was seen.
		if(status.second != tickTock)
			status.second = tickTock;

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

		// Trigger immediate re-generation of the formation positions (to
		// ensure that this new ship also gets a valid position).
		positionsTimer = 0;		
	}
	
	return formationLead->Position() + direction.Rotate(relPos);
}



// Re-generate the list of (relative) positions for the ships in the formation.
void FormationPositioner::CalculatePositions()
{
	// Set scaling based on results from previous run.
	double diameterToPx = maxDiameter;
	double widthToPx = maxWidth;
	double heightToPx = maxHeight;
	maxDiameter = 0;
	maxWidth = 0;
	maxHeight = 0;

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
		auto itPos = pattern->begin(diameterToPx, widthToPx, heightToPx,
			centerBodyRadius, startRing, shipsInRing.size());

		// Run the iterator.
		size_t shipIndex = 0;
		while(shipIndex < shipsInRing.size())
		{
			// If the ship is no longer valid or not or no longer part of this
			// formation, then we need to remove it.
			auto ship = (shipsInRing[shipIndex]).lock();
			bool removeShip = !ship || !IsActiveInFormation(desiredRing, ship.get());

			// Lookup the ship in the positions map.
			auto itCoor = shipPositions.end();
			if(ship)
				itCoor = shipPositions.find(&(*ship));

			// If the ship is not in the overall table or if it was not
			// active since the last iteration, then we also remove it.
			removeShip = removeShip || itCoor == shipPositions.end() ||
					itCoor->second.second != tickTock;

			// Perform removes if we need to.
			if(removeShip)
			{
				// Remove the ship from the ring.
				RemoveFromRing(desiredRing, shipIndex);
				// And remove the ship from the shipsPositions map.
				if(itCoor != shipPositions.end())
					shipPositions.erase(itCoor);
			}
			else
			{
				// Set scaling for next round based on the sizes of the
				// participating ships.
				Tally(*ship);

				// Calculate the new coordinate for the current ship.
				Point &shipRelPos = itCoor->second.first;
				shipRelPos = *itPos;
				if(flippedY)
					shipRelPos.Set(-shipRelPos.X(), shipRelPos.Y());
				if(flippedX)
					shipRelPos.Set(shipRelPos.X(), -shipRelPos.Y());
				++itPos;
				++shipIndex;
			}
		}
		// Store starting ring for next ring-positioner.
		startRing = itPos.Ring() + 1;
	}
	// Switch marker to detect stale/missing ships in the next iteration.
	tickTock = !tickTock;
}



void FormationPositioner::Tally(const Body &body)
{
	maxDiameter = max(maxDiameter, body.Radius() * 2.);
	maxHeight = max(maxHeight, body.Height());
	maxWidth = max(maxWidth, body.Width());
}



void FormationPositioner::CalculateDirection()
{
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
	constexpr double MIN_FLIP_TRIGGER = 135.;

	// If we are beyond the triggers for flipping, then immediately go to the desired direction.
	if(fabs(deltaDir.Degrees()) >= MIN_FLIP_TRIGGER &&
		(pattern->FlippableY() || pattern->FlippableX()))
	{
		direction = desiredDir;
		deltaDir = Angle(0.);
		if(pattern->FlippableY()){
			flippedY = !flippedY;
			positionsTimer = 0;
		}
		if(pattern->FlippableX()){
			flippedX = !flippedX;
			positionsTimer = 0;
		}
	}
	else
	{
		// Turn max 1/4th degree per frame. The game runs at 60fps, so a turn of 180 degrees will take
		// about 12 seconds.
		constexpr double MAX_FORMATION_TURN = 0.25;

		if(deltaDir.Degrees() > MAX_FORMATION_TURN)
			deltaDir = Angle(MAX_FORMATION_TURN);
		else if(deltaDir.Degrees() < -MAX_FORMATION_TURN)
			deltaDir = Angle(-MAX_FORMATION_TURN);

		direction += deltaDir;
	}
}



// Check if a ship is active in the current formation.
bool FormationPositioner::IsActiveInFormation(unsigned int ring, const Ship *ship) const
{
	// Ships need to be active, need to have the same formation pattern and
	// need to be in the same system as their formation lead in order to
	// participate in the formation.
	if(ship->GetFormationPattern() != pattern ||
			ship->IsDisabled() || ship->IsLanding() || ship->IsBoarding())
		return false;

	// TODO: add check if ship is attacking.
	// TODO: add check if ship and lead are in the same system.
	// TODO: add check for isAssisting.
	// TODO: check if we can move many checks to Ship and get an ship->IsStationKeeping() instead.

	// A ship active in the formation should follow the current formationleader
	// either through targetShip (for gather/keep-station commands) or through
	// the child/parent relationship.
	auto targetShip = ship->GetTargetShip();
	auto parentShip = ship->GetParent();
	if ((!targetShip || &(*targetShip) != formationLead) &&
		(!parentShip || &(*parentShip) != formationLead))
		return false;

	return true;
}



// Remove a ship from the formation-ring (based on its index). The last ship
// in the ring will take the position of the removed ship (if the removed
// ship itself is not the last ship).
void FormationPositioner::RemoveFromRing(unsigned int ring, unsigned int index)
{
	auto &shipsInRing = ringShips[ring];
	if(shipsInRing.empty())
		return;

	// Move the last element to the current position and remove the last
	// element; this will let last ship take the position of the ship that
	// we will remove.
	if(index < (shipsInRing.size() - 1))
		shipsInRing[index].swap(shipsInRing.back());
	shipsInRing.pop_back();
}
