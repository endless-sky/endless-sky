/* Industry.h
Copyright (c) 2026 by Endless Sky Expanded contributors

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

#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Facility;
template<class Type>
class Set;



// All of the industrial facilities the player owns, and the goods that each
// one has produced but the player has not collected yet.
class Industry {
public:
	// One facility the player has built on a particular planet.
	struct Holding {
		const Facility *type = nullptr;
		// The true name of the planet the facility is on.
		std::string planet;
		// Tons of the facility's output waiting to be collected.
		int stockpile = 0;
	};


public:
	// Load the "industry" node of a saved game.
	void Load(const DataNode &node, const Set<Facility> &facilities);
	void Save(DataWriter &out) const;

	const std::vector<Holding> &Holdings() const;
	// Find the given type of facility on the given planet, if the player owns one.
	Holding *Find(const Facility &type, const std::string &planet);
	const Holding *Find(const Facility &type, const std::string &planet) const;

	// Add a new, empty facility. Does nothing if the player already owns this
	// type of facility on this planet. Paying for it is up to the caller.
	void Build(const Facility &type, const std::string &planet);
	// Run one day of production for every facility.
	void AdvanceDay();
	// Take up to the given number of tons out of a holding's stockpile.
	// Returns how many tons were actually taken.
	static int Collect(Holding &holding, int space);


private:
	std::vector<Holding> holdings;
};
