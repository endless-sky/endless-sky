/* Hail.h
Copyright (c) 2025 by Endless Sky contributors

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

#include "ConditionsStore.h"
#include "DataNode.h"
#include "LocationFilter.h"
#include "Phrase.h"
#include "Ship.h"

#include <cstddef>
#include <string>

class UniverseObjects;

/// Represent a type of message from someone who send you non-blocking flavor text (hail) in space.
class Hail {
public:
	void Load(const DataNode &node);

	// Check if the message can be used given the condition and hailing ship.
	bool Matches(const ConditionsStore &conditions, const Ship &hailingShip) const;

	// Generate a message. The condition and hailing ship will not be used to filter among the hail, but allow
	// conditional phrases.
	std::string Message(const ConditionsStore &conditions, const Ship &hailingShip) const;

	int getWeight() const;
private:
	ConditionSet toHail;
	Phrase messages;
	// Might be 0, in which case this will never be displayed
	int weight = 10;
	LocationFilter filterHailingShip;

	friend UniverseObjects;
};
