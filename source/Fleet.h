/* Fleet.h
Copyright (c) 2014 by Michael Zahniser

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

#include "FleetCargo.h"
#include "Personality.h"
#include "Variant.h"
#include "WeightedList.h"

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class DataNode;
class FormationPattern;
class Government;
class Outfit;
class Phrase;
class Planet;
class Ship;
class System;



// A fleet represents a collection of ships that may enter a system or be used
// as NPCs in a mission. Each fleet contains one or more "variants," each of
// which can occur with a different probability, and each of those variants
// lists one or more ships. All the ships in a fleet share a certain government,
// AI personality, and set of friendly and hostile "hail" messages, and the ship
// names are chosen based on a given random "phrase" generator.
class Fleet {
public:
	Fleet() = default;
	// Construct and Load() at the same time.
	Fleet(const DataNode &node);

	void Load(const DataNode &node);

	// Determine if this fleet template uses well-defined data.
	bool IsValid(bool requireGovernment = true) const;
	// Ensure any variant selected during gameplay will have at least one ship to spawn.
	void RemoveInvalidVariants();

	// Get the government of this fleet.
	const Government *GetGovernment() const;

	// Choose a fleet to be created during flight, and have it enter the system via jump or planetary departure.
	void Enter(const System &system, std::list<std::shared_ptr<Ship>> &ships, const Planet *planet = nullptr) const;
	// Place a fleet in the given system, already "in action." If the carried flag is set, only
	// uncarried ships will be added to the list (as any carriables will be stored in bays).
	void Place(const System &system, std::list<std::shared_ptr<Ship>> &ships,
			bool carried = true, bool addCargo = true) const;

	// Do the randomization to make a ship enter or be in the given system.
	// Return the system that was chosen for the ship to enter from.
	static const System *Enter(const System &system, Ship &ship, const System *source = nullptr);
	static void Place(const System &system, Ship &ship);

	int64_t Strength() const;


private:
	static std::pair<Point, double> ChooseCenter(const System &system);
	std::vector<std::shared_ptr<Ship>> Instantiate(const std::vector<const Ship *> &ships) const;
	bool PlaceFighter(const std::shared_ptr<Ship> &fighter, std::vector<std::shared_ptr<Ship>> &placed) const;


private:
	std::string fleetName;
	const Government *government = nullptr;
	const Phrase *names = nullptr;
	const Phrase *fighterNames = nullptr;
	const FormationPattern *formation = nullptr;
	WeightedList<Variant> variants;
	// The cargo ships in this fleet will carry.
	FleetCargo cargo;

	Personality personality;
	Personality fighterPersonality;
};
