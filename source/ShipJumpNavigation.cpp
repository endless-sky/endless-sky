/* ShipJumpNavigation.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipJumpNavigation.h"

#include "Outfit.h"
#include "Ship.h"
#include "System.h"

#include <algorithm>
#include <cassert>
#include <iterator>

using namespace std;

const double ShipJumpNavigation::DEFAULT_HYPERDRIVE_COST = 100.;
const double ShipJumpNavigation::DEFAULT_SCRAM_DRIVE_COST = 150.;
const double ShipJumpNavigation::DEFAULT_JUMP_DRIVE_COST = 200.;



// Set the owner of this jump navigation, storing a pointer to it for later use.
void ShipJumpNavigation::SetOwner(const Ship *ship)
{
	this->ship = ship;
}



// Calibrate this ship's jump navigation information, caching its jump costs, range, and capabilities.
void ShipJumpNavigation::Calibrate()
{
	assert(ship && "A jump navigation's ship cannot be null.");
	Outfit attributes = ship->Attributes();
	hasHyperdrive = attributes.Get("hyperdrive");
	hasScramDrive = attributes.Get("scram drive");
	hasJumpDrive = attributes.Get("jump drive");

	jumpDriveCosts.clear();
	jumpDriveCosts[0.] = 0.;
	hyperdriveCost = 0.;
	maxJumpRange = 0.;

	double shipHyperCost = hasScramDrive ? DEFAULT_SCRAM_DRIVE_COST : DEFAULT_HYPERDRIVE_COST;

	// Make it possible for a hyperdrive or jump drive to be integrated into a ship.
	ParseOutfit(ship->BaseAttributes(), shipHyperCost);
	// Check each outfit from this ship to determine if it has jump capabilities.
	for(const auto &it : ship->Outfits())
		ParseOutfit(*it.first, shipHyperCost);
}



// Get the amount of fuel that would be expended to jump to the destination. If the destination is
// nullptr then return the maximum amount of fuel that this ship could expend in one jump.
double ShipJumpNavigation::JumpFuel(const System *destination) const
{
	assert(ship && "A jump navigation's ship cannot be null.");
	const System *currentSystem = ship->GetSystem();
	// A currently-carried ship requires no fuel to jump, because it cannot jump.
	if(!currentSystem)
		return 0.;

	// If no destination is given, return the maximum fuel per jump.
	if(!destination)
		return max(JumpDriveFuel(maxJumpRange), HyperdriveFuel());

	return GetCheapestJumpType(destination).second;
}



// Get the maximum distance that this ship can jump.
double ShipJumpNavigation::JumpRange() const
{
	return maxJumpRange;
}



// Get the cost of making a jump of the given type (if possible). Returns 0 if the jump can't be made.
double ShipJumpNavigation::HyperdriveFuel() const
{
	// If this ship doesn't have a hyperdrive then hyperdriveCost will already be 0.
	return hyperdriveCost;
}



double ShipJumpNavigation::JumpDriveFuel(double distance) const
{
	// If this ship has no jump drive then return 0.
	if(!hasJumpDrive)
		return 0.;
	// If this exact distance is in the list then return its cost. This will
	// likely only occur if the given distance is 0 or maxJumpRange.
	if(jumpDriveCosts.count(distance))
		return jumpDriveCosts.find(distance)->second;
	// Otherwise, find the first jump range that covers the distance. Iterate over
	// the costs map until we reach the jump range just above the given distance.
	auto it = std::find_if(jumpDriveCosts.begin(), jumpDriveCosts.end(),
		[distance](const pair<double, double> &range) -> bool { return range.first > distance; });
	return (it == jumpDriveCosts.end()) ? 0. : it->second;
}



// Get the cheapest jump method and its cost for a jump to the destination system.
// If no jump method is possible, returns JumpType::None with a jump cost of 0.
pair<JumpType, double> ShipJumpNavigation::GetCheapestJumpType(const System *destination) const
{
	assert(ship && "A jump navigation's ship cannot be null.");
	return GetCheapestJumpType(ship->GetSystem(), destination);
}



pair<JumpType, double> ShipJumpNavigation::GetCheapestJumpType(const System *from, const System *to) const
{
	bool linked = from->Links().count(to);
	double hyperFuelNeeded = HyperdriveFuel();
	double jumpFuelNeeded = JumpDriveFuel((linked || from->JumpRange())
			? 0. : from->Position().Distance(to->Position()));
	if(linked && hasHyperdrive && (!jumpFuelNeeded || hyperFuelNeeded <= jumpFuelNeeded))
		return make_pair(JumpType::HYPERDRIVE, hyperFuelNeeded);
	else if(hasJumpDrive)
		return make_pair(JumpType::JUMPDRIVE, jumpFuelNeeded);
	else
		return make_pair(JumpType::NONE, 0.);
}



// Check what jump methods this ship has.
bool ShipJumpNavigation::HasHyperdrive() const
{
	return hasHyperdrive;
}



bool ShipJumpNavigation::HasScramDrive() const
{
	return hasScramDrive;
}



bool ShipJumpNavigation::HasJumpDrive() const
{
	return hasJumpDrive;
}



// Parse the given outfit to determine if it has the capability to jump, and update any
// jump information accordingly.
void ShipJumpNavigation::ParseOutfit(const Outfit &outfit, double shipHyperCost)
{
	if(outfit.Get("hyperdrive") && (!hasScramDrive || outfit.Get("scram drive")))
	{
		double cost = outfit.Get("jump fuel");
		if(!cost)
			cost = shipHyperCost;
		if(!hyperdriveCost || cost < hyperdriveCost)
			hyperdriveCost = cost;
	}
	if(outfit.Get("jump drive"))
	{
		double distance = outfit.Get("jump range");
		if(!distance)
			distance = System::DEFAULT_NEIGHBOR_DISTANCE;
		double cost = outfit.Get("jump fuel");
		if(!cost)
			cost = DEFAULT_JUMP_DRIVE_COST;

		UpdateJumpDriveCosts(distance, cost);
	}
}



// Add the given distance, cost pair to the jump drive costs and update the fuel cost
// of each jump distance if necessary.
void ShipJumpNavigation::UpdateJumpDriveCosts(double distance, double cost)
{
	if(!maxJumpRange || maxJumpRange < distance)
		maxJumpRange = distance;
	// If a jump drive range isn't already accounted for or the existing cost
	// for this range is more expensive, use the given cost.
	if(!jumpDriveCosts.count(distance) || !jumpDriveCosts[distance] || jumpDriveCosts[distance] > cost) {
		jumpDriveCosts[distance] = cost;

		// If a cost was updated then we need to reassess other costs.
		auto it = jumpDriveCosts.find(distance);
		// If the jump range a step above this distance is cheaper, then the
		// cheaper jump cost already covers this range.
		auto nit = std::next(it);
		if(nit != jumpDriveCosts.end() && it->second > nit->second)
			it->second = nit->second;
		else
		{
			// If any jump range below this one is more expensive, then use
			// this new, cheaper cost.
			for(auto sit = jumpDriveCosts.begin() ; sit != it; ++sit)
				if(!sit->second || sit->second > it->second)
					sit->second = it->second;
		}
	}
}
