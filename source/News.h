/* News.h
Copyright (c) 2017 by Michael Zahniser

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

#include "ConditionSet.h"
#include "LocationFilter.h"
#include "Phrase.h"

#include <string>
#include <vector>

class ConditionsStore;
class DataNode;
class Planet;
class Sprite;
class System;



// This represents a person you can "talk to" in the spaceport to get some local
// news. One specification can contain many possible portraits and messages.
class News {
public:
	void Load(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);

	// Check whether this news item has anything to say.
	bool IsEmpty() const;
	// Check if this news item is available given the player's planet.
	bool Matches(const Planet *planet) const;

	// Get the speaker's name.
	std::string SpeakerName() const;
	// Pick a portrait at random out of the possible options.
	const Sprite *Portrait() const;
	// Get the speaker's message, chosen randomly.
	std::string Message() const;


private:
	LocationFilter location;
	ConditionSet toShow;

	Phrase speakerNames;
	std::vector<const Sprite *> portraits;
	Phrase messages;
};
