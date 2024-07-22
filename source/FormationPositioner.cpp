/* FormationPositioner.cpp
Copyright (c) 2019-2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FormationPositioner.h"

#include "Angle.h"
#include "Body.h"
#include "FormationPattern.h"
#include "Point.h"
#include "Ship.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Initializer based on the formation pattern to follow.
FormationPositioner::FormationPositioner(const Body *formationLead, const FormationPattern *pattern)
	: formationLead(formationLead), pattern(pattern)
{
	if(!pattern->Rotatable())
		direction = 0;
	else
		direction = formationLead->Facing();
}



void FormationPositioner::Step()
{
	// Calculate the direction in which the formation is facing. We perform
	// this calculation every step, because we want to act on course-changes
	// from the lead ship as soon as they happen.
	CalculateDirection();

	// Calculate the set of positions for the participating ships. This calculation
	// can be performed once every 20 game-steps, because the positions are
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
		shipsInFormation.push_back(ship->shared_from_this());

		// Trigger immediate re-generation of the formation positions (to
		// ensure that this new ship also gets a valid position).
		positionsTimer = 0;
	}

	return formationLead->Position() + direction.Rotate(relPos);
}



// Re-generate the list of (relative) positions for the ships in the formation.
void FormationPositioner::CalculatePositions()
{
	// Run the position iterator for the ships in the formation.
	auto itPos = pattern->begin(centerBodyRadius);

	// Run the iterator.
	size_t shipIndex = 0;
	while(shipIndex < shipsInFormation.size())
	{
		// If the ship is no longer valid or not or no longer part of this
		// formation, then we need to remove it.
		auto ship = shipsInFormation[shipIndex].lock();
		bool removeShip = !ship || !IsActiveInFormation(ship.get());

		// Lookup the ship in the positions map.
		auto itCoor = shipPositions.end();
		if(ship)
			itCoor = shipPositions.find(ship.get());

		// If the ship is not in the overall table or if it was not
		// active since the last iteration, then we also remove it.
		removeShip |= itCoor == shipPositions.end() ||
				itCoor->second.second != tickTock;

		// Perform removes if we need to.
		if(removeShip)
		{
			// Remove the ship from the ring.
			Remove(shipIndex);
			// And remove the ship from the shipsPositions map.
			if(itCoor != shipPositions.end())
				shipPositions.erase(itCoor);
		}
		else
		{
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

	// Switch marker to detect stale/missing ships in the next iteration.
	tickTock = !tickTock;
}



void FormationPositioner::CalculateDirection()
{
	// Any direction is fine, just keep initial direction.
	if(!pattern->Rotatable())
		return;

	// Calculate new direction. If the formationLead is moving, then we use the movement vector,
	// otherwise use the facing vector.
	Point velocity = formationLead->Velocity();
	Angle desiredDir = velocity.Length() > .1 ? Angle(velocity) : formationLead->Facing();

	Angle deltaDir = desiredDir - direction;

	// Change the desired direction according to rotational settings if that fits well.
	double symRot = pattern->Rotatable();
	if(symRot > 0.)
	{
		if(deltaDir.Degrees() > 0)
			symRot = -symRot;

		while(fabs(deltaDir.Degrees()) > fabs(symRot / 2))
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
		if(pattern->FlippableY())
		{
			flippedY = !flippedY;
			positionsTimer = 0;
		}
		if(pattern->FlippableX())
		{
			flippedX = !flippedX;
			positionsTimer = 0;
		}
	}
	else
	{
		// Turn max 1/4th degree per frame. The game runs at 60fps, so a turn of 180 degrees will take
		// about 12 seconds.
		constexpr double MAX_FORMATION_TURN = .25;

		deltaDir = Angle(clamp(deltaDir.Degrees(), -MAX_FORMATION_TURN, MAX_FORMATION_TURN));

		direction += deltaDir;
	}
}



// Check if a ship is active in the current formation.
bool FormationPositioner::IsActiveInFormation(const Ship *ship) const
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
	if((!targetShip || targetShip.get() != formationLead) &&
			(!parentShip || parentShip.get() != formationLead))
		return false;

	return true;
}



// Remove a ship from the formation (based on its index). The last ship
// in the formation will take the position of the removed ship (if the removed
// ship itself is not the last ship).
void FormationPositioner::Remove(unsigned int index)
{
	if(shipsInFormation.empty())
		return;

	// Move the last element to the current position and remove the last
	// element; this will let last ship take the position of the ship that
	// we will remove.
	if(index < shipsInFormation.size() - 1)
		shipsInFormation[index].swap(shipsInFormation.back());
	shipsInFormation.pop_back();
}
