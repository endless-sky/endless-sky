/* Facility.h
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

#include <cstdint>
#include <set>
#include <string>

class DataNode;



// A type of industrial facility that the player can build, such as a mining
// outpost. Each facility produces a fixed amount of one commodity per day into
// a local stockpile, up to its storage capacity.
class Facility {
public:
	Facility() = default;

	void Load(const DataNode &node);
	// A facility is defined once it has been loaded from the game data.
	bool IsDefined() const;

	const std::string &TrueName() const;
	const std::string &Description() const;
	int64_t Cost() const;
	// The commodity this facility produces, and how many tons per day.
	const std::string &Output() const;
	int OutputPerDay() const;
	// The most tons that can sit in this facility's stockpile.
	int Storage() const;

	// Check whether this facility can be built on the planet with the given true name.
	bool CanBuildOn(const std::string &planet) const;


private:
	std::string trueName;
	std::string description;
	int64_t cost = 0;
	std::string output;
	int outputPerDay = 0;
	int storage = 0;
	std::set<std::string> planets;
};
