/* GameEvent.h
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

#include "ConditionAssignments.h"
#include "DataNode.h"
#include "Date.h"

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class ConditionsStore;
class DataWriter;
class Planet;
class PlayerInfo;
class System;



// This represents something that happens that changes the universe. For
// example, a system may be taken over by a different government, or a new type
// of ship or weapon may become available for purchase. Events that do not
// specify a date on which they occur will happen in response to missions. An
// event always sets the "event: <name>" condition when it occurs, which allows
// you to use the mission framework to specify a message that can be shown to
// the player the next time they land on a planet after that event happens.
class GameEvent {
public:
	// Determine the universe object definitions that are defined by the given list of changes.
	static std::map<std::string, std::set<std::string>> DeferredDefinitions(const std::list<DataNode> &changes);


public:
	GameEvent() = default;
	// Construct and Load() at the same time.
	explicit GameEvent(const DataNode &node, const ConditionsStore *playerConditions);

	void Load(const DataNode &node, const ConditionsStore *playerConditions);
	// Save a scheduled version of this event. This will only be called if the event is unnamed and has a scheduled
	// date. Otherwise, PlayerInfo saves the event's name and scheduled date instead of saving the full list of
	// data changes.
	void Save(DataWriter &out) const;
	// If disabled, an event will not Apply() or Save().
	void Disable();

	const std::string &TrueName() const;
	void SetTrueName(const std::string &name);

	// Check if this GameEvent has been loaded (vs. simply referred to) and
	// if it references any items that have not been defined.
	// Returns an empty string if it is valid. If not, a reason will be given in the string.
	std::string IsValid() const;

	const Date &GetDate() const;
	void SetDate(const Date &date);

	// Apply this event's changes to the player. Returns a list of data changes that need to
	// be applied in a batch with other events that are applied at the same time.
	std::list<DataNode> Apply(PlayerInfo &player, bool onlyDataChanges = false);

	const ConditionAssignments &Conditions() const;
	const std::list<DataNode> &Changes() const;

	bool SaveRawChanges() const;

	// Comparison operator, based on the date of the event.
	bool operator<(const GameEvent &other) const;

private:
	Date date;
	std::string trueName;
	bool isDisabled = false;
	bool isDefined = false;
	bool saveRawChanges = false;

	ConditionAssignments conditionsToApply;
	std::list<DataNode> changes;
	std::vector<const System *> systemsToVisit;
	std::vector<const Planet *> planetsToVisit;
	std::vector<const System *> systemsToUnvisit;
	std::vector<const Planet *> planetsToUnvisit;
};
