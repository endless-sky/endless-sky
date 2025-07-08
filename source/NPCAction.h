/* NPCAction.h
Copyright (c) 2023 by Amazinite

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

#include "MissionAction.h"

class ConditionsStore;
class DataNode;
class DataWriter;
class Mission;
class Planet;
class PlayerInfo;
class System;
class UI;



// A wrapper for a MissionAction that can be triggered by an NPC. Records whether
// the action has already been done previously, which NPC checks to prevent
// the action from being done more than once.
// More functionality to come later, such as changing the personality of an NPC
// on the fly.
class NPCAction {
public:
	NPCAction() = default;
	// Construct and Load() at the same time.
	explicit NPCAction(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);

	void Load(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);
	// Note: the Save() function can assume this is an instantiated mission, not
	// a template, so it only has to save a subset of the data.
	void Save(DataWriter &out) const;
	// Determine if this NPCAction references content that is not fully defined.
	std::string Validate() const;

	// Perform this action.
	void Do(PlayerInfo &player, UI *ui, const Mission *caller, const std::shared_ptr<Ship> &target);

	// "Instantiate" this action by filling in the wildcard text for the actual
	// destination, payment, cargo, etc.
	NPCAction Instantiate(std::map<std::string, std::string> &subs,
		const System *origin, int jumps, int64_t payload) const;


private:
	std::string trigger;
	bool triggered = false;

	// Tasks this NPC action performs, such as modifying accounts, inventory, or conditions.
	MissionAction action;
};
