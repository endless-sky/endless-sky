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
#include <iterator>

using namespace std;



// Calibrate this ship's jump navigation information, caching its jump costs, range, and capabilities.
void ShipJumpNavigation::Calibrate(const Ship &ship)
{
	currentSystem = ship.GetSystem();
	mass = ship.Mass();

	const Outfit &attributes = ship.Attributes();
	hasHyperdrive = attributes.Get("hyperdrive");
	hasScramDrive = attributes.Get("scram drive");
	hasJumpDrive = attributes.Get("jump drive");
	hasJumpMassCost = attributes.Get("jump mass cost");

	jumpDriveCosts.clear();
	hyperdriveCost = 0.;
	maxJumpRange = 0.;

	// Make it possible for a hyperdrive or jump drive to be integrated into a ship.
	ParseOutfit(ship.BaseAttributes());
	// Check each outfit from this ship to determine if it has jump capabilities.
	for(const auto &it : ship.Outfits())
		ParseOutfit(*it.first);
}



// Recalibrate jump costs for this ship, but only if necessary.
void ShipJumpNavigation::Recalibrate(const Ship &ship)
{
	// Recalibration is only necessary if this ship's mass has changed and it has drives
	// that would be affected by that change.
	if(hasJumpMassCost && mass != ship.Mass())
		Calibrate(ship);
}



// Pass the current system that the ship is in to the navigation.
void ShipJumpNavigation::SetSystem(const System *system)
{
	currentSystem = system;
}



// Get the amount of fuel that would be expended to jump to the destination. If the destination is
// nullptr then return the maximum amount of fuel that this ship could expend in one jump.
double ShipJumpNavigation::JumpFuel(const System *destination) const
{
	// A currently-carried ship requires no fuel to jump, because it cannot jump.
	if(!currentSystem)
		return 0.;

	// If no destination is given, return the maximum fuel per jump.
	if(!destination)
		return max(JumpDriveFuel(), HyperdriveFuel());

	return GetCheapestJumpType(currentSystem, destination).second;
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
	// Otherwise, find the first jump range that covers the distance.
	auto it = jumpDriveCosts.lower_bound(distance);
	return (it == jumpDriveCosts.end()) ? 0. : it->second;
}



// Get the cheapest jump method and its cost for a jump to the destination system.
// If no jump method is possible, returns JumpType::None with a jump cost of 0.
pair<JumpType, double> ShipJumpNavigation::GetCheapestJumpType(const System *destination) const
{
	if(!currentSystem || !destination)
		return make_pair(JumpType::NONE, 0.);
	return GetCheapestJumpType(currentSystem, destination);
}



// Get the cheapest jump method between the two given systems.
pair<JumpType, double> ShipJumpNavigation::GetCheapestJumpType(const System *from, const System *to) const
{
	if(!from || !to)
		return make_pair(JumpType::NONE, 0.);
	bool linked = from->Links().contains(to);
	double hyperFuelNeeded = HyperdriveFuel();
	// If these two systems are linked, or if the system we're jumping from has its own jump range,
	// then use the cheapest jump drive available, which is mapped to a distance of 0.
	const double distance = from->Position().Distance(to->Position());
	double jumpFuelNeeded = JumpDriveFuel((linked || from->JumpRange())
			? 0. : distance);
	bool canJump = jumpFuelNeeded && (linked || !from->JumpRange() || from->JumpRange() >= distance);
	if(linked && hasHyperdrive && (!canJump || hyperFuelNeeded <= jumpFuelNeeded))
		return make_pair(JumpType::HYPERDRIVE, hyperFuelNeeded);
	else if(hasJumpDrive && canJump)
		return make_pair(JumpType::JUMP_DRIVE, jumpFuelNeeded);
	else
		return make_pair(JumpType::NONE, 0.);
}



// Get if this ship can make a hyperspace or jump drive jump directly from one system to the other.
bool ShipJumpNavigation::CanJump(const System *from, const System *to) const
{
	if(!from || !to)
		return false;

	if(from->Links().contains(to) && (hasHyperdrive || hasJumpDrive))
		return true;

	if(!hasJumpDrive)
		return false;

	const double distanceSquared = from->Position().DistanceSquared(to->Position());
	double maxRange = from->JumpRange() ? from->JumpRange() : jumpDriveCosts.rbegin()->first;
	return maxRange * maxRange >= distanceSquared;
}



// Check what jump methods this ship has.
bool ShipJumpNavigation::HasAnyDrive() const
{
	return hasHyperdrive || hasJumpDrive;
}



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
void ShipJumpNavigation::ParseOutfit(const Outfit &outfit)
{
	auto CalculateFuelCost = [this, &outfit](bool isJumpDrive) -> double
	{
		double baseCost = outfit.Get(isJumpDrive ? "jump drive fuel" : "hyperdrive fuel");
		// Mass cost is the fuel cost per 100 tons of ship mass. The jump base mass of a drive reduces the
		// ship's effective mass for the jump mass cost calculation. A ship with a mass below the drive's
		// jump base mass is allowed to have a negative mass cost.
		double massCost = .01 * outfit.Get("jump mass cost") * (mass - outfit.Get("jump base mass"));
		// Prevent a drive with a high jump base mass on a ship with a low mass from pushing the total
		// cost too low. Put a floor at 1, as a floor of 0 would be assumed later on to mean you can't jump.
		// If and when explicit 0s are allowed for fuel cost, this floor can become 0.
		return max(1., baseCost + massCost);
	};

	if(outfit.Get("hyperdrive") && (!hasScramDrive || outfit.Get("scram drive")))
	{
		double cost = CalculateFuelCost(false);
		if(!hyperdriveCost || cost < hyperdriveCost)
			hyperdriveCost = cost;
	}
	if(outfit.Get("jump drive"))
	{
		double distance = outfit.Get("jump range");
		if(distance <= 0.)
			distance = System::DEFAULT_NEIGHBOR_DISTANCE;
		double cost = CalculateFuelCost(true);

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
	auto oit = jumpDriveCosts.find(distance);
	if(oit == jumpDriveCosts.end() || !oit->second || oit->second > cost)
	{
		jumpDriveCosts[distance] = cost;

		// If a cost was updated then we need to reassess other costs. The goal is to have
		// the cost for each distance be the cheapest possible fuel cost needed to jump to
		// a system that is that distance away. The keys of the map are the distances and
		// will be strictly increasing, while the values of the map are the fuel costs and
		// will be weakly increasing.
		auto it = jumpDriveCosts.find(distance);
		// If the jump range a step above this distance is cheaper, then the
		// cheaper jump cost already covers this range. We don't need to check
		// any other distances in this case because the rest of the map will
		// already be properly sorted.
		auto nit = next(it);
		if(nit != jumpDriveCosts.end() && it->second > nit->second)
			it->second = nit->second;
		else
		{
			// If any jump range below this one is more expensive, then use
			// this new, cheaper cost.
			for(auto sit = jumpDriveCosts.begin(); sit != it; ++sit)
				if(!sit->second || sit->second > it->second)
					sit->second = it->second;
		}
	}
}
