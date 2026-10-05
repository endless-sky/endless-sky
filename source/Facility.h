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
#include <utility>
#include <vector>

class DataNode;



// A type of industrial facility that the player can build, such as a mining
// outpost or a factory. Each day, a facility uses up its inputs (if any) from its
// local stock and adds its outputs, up to its storage capacity per commodity.
class Facility {
public:
	// A commodity and an amount in tons per day.
	using Amounts = std::vector<std::pair<std::string, int>>;


public:
	Facility() = default;

	void Load(const DataNode &node);
	// A facility is defined once it has been loaded from the game data.
	bool IsDefined() const;

	const std::string &TrueName() const;
	const std::string &Description() const;
	int64_t Cost() const;
	// Credits per day to keep the facility running.
	int64_t Upkeep() const;
	// Commodities used up and produced each day.
	const Amounts &Inputs() const;
	const Amounts &Outputs() const;
	// The most tons of each commodity that can sit in this facility's stock.
	int Storage() const;

	// Check whether this facility can be built on a planet, given its true name
	// and attributes: either the planet is listed by name, or it has one of the
	// listed attributes.
	bool CanBuildOn(const std::string &planet, const std::set<std::string> &attributes) const;


private:
	std::string trueName;
	std::string description;
	int64_t cost = 0;
	int64_t upkeep = 0;
	Amounts inputs;
	Amounts outputs;
	int storage = 0;
	std::set<std::string> planets;
	std::set<std::string> attributes;
};
