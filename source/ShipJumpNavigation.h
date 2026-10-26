/* ShipJumpNavigation.h
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

#pragma once

#include "JumpType.h"

#include <map>

class Outfit;
class Ship;
class System;



// A class representing the jump capabilities of a ship. Calculates and caches a ship's
// jump methods, costs, and distances.
class ShipJumpNavigation {
public:
	// The default number of frames it takes to jump to a system for jump outfits
	// that don't specify a "jump time".
	static const int DEFAULT_JUMP_FRAMES;


public:
	struct TravelDetails {
		JumpType type = JumpType::NONE;
		// The cost of making the jump.
		double fuelCost = 0;
		// The number of frames it takes to make the jump.
		int travelTime = 100;
	};


public:
	ShipJumpNavigation() = default;

	// Calibrate this ship's jump navigation information, caching its jump costs, range, and capabilities.
	void Calibrate(const Ship &ship);
	// Recalibrate jump costs for this ship, but only if necessary.
	void Recalibrate(const Ship &ship);
	// Recalibrate only the jump speed and scram threshold.
	void RecalibrateJumpSpeed(const Ship &ship);

	// Pass the current system that the ship is in to the navigation.
	void SetSystem(const System *system);

	// Get the amount of fuel that would be expended to jump to the destination. If the destination is
	// nullptr then return the maximum amount of fuel that this ship could expend in one jump.
	double JumpFuel(const System *destination = nullptr) const;
	// Get the maximum distance that this ship can jump.
	double JumpRange() const;
	// Get the cost of making a jump of the given type (if possible). Returns 0 if the jump can't be made.
	double HyperdriveFuel() const;
	double JumpDriveFuel(double distance = 0.) const;
	// Get the cheapest jump method and its cost for a jump to the destination system.
	// If no jump method is possible, returns JumpType::None with a jump cost of 0.
	TravelDetails GetCheapestJump(const System *destination) const;
	// Get the cheapest jump method between the two given systems.
	TravelDetails GetCheapestJump(const System *from, const System *to) const;

	// Get if this ship can make a hyperspace or jump drive jump directly from one system to the other.
	bool CanJump(const System *from, const System *to) const;

	// Check what jump methods this ship has.
	bool HasAnyDrive() const;
	bool HasHyperdrive() const;
	bool HasScramDrive() const;
	bool HasJumpDrive() const;
	bool HasJumpMassCost() const;

	double JumpSpeedThreshold() const;
	double ScramThreshold() const;

	// Create a hash of the capabilities of this ship, for use in caching pathfinding.
	std::size_t Hash() const;


private:
	// Parse the given outfit to determine if it has the capability to jump, and update any
	// jump information accordingly.
	void ParseOutfit(const Outfit &outfit);
	// Add the given distance, cost pair to the jump drive costs and update the fuel cost
	// of each jump distance if necessary.
	void UpdateJumpDriveCosts(double distance, const TravelDetails &details);
	const TravelDetails &GetJumpDetails(double distance) const;


private:
	// Cached information about the ship. Checked against the ship's current
	// information during recalibration.
	double mass = 0.;
	const System *currentSystem = nullptr;

	// Cached jump navigation information.
	TravelDetails hyperDriveDetails;
	// Map allowable jump ranges to the jump details that should be used at that range.
	std::map<double, TravelDetails> jumpDriveDetails;
	double maxJumpRange = 0.;

	// What drive types and characteristics the ship has.
	bool hasHyperdrive = false;
	bool hasScramDrive = false;
	bool hasJumpDrive = false;
	bool hasJumpMassCost = false;

	// The speed that this ship can jump at.
	double jumpSpeedThreshold = 0.;
	double scramThreshold = 0.;
};
