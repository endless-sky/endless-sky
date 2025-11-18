/* MissionTimer.h
Copyright (c) 2023 by Timothy Collett

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

#include "LocationFilter.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>

class ConditionsStore;
class DataNode;
class DataWriter;
class Mission;
class MissionAction;
class Planet;
class PlayerInfo;
class Ship;
class System;
class UI;



// Class representing a timer for triggering mission actions. Timers count
// down a certain number of frames before triggering if the player meets the
// conditions for the timer starting (e.g. the player is moving slowly or is near
// a certain object), but may be reset by various actions the player takes.
class MissionTimer {
public:
	// The possible triggers for actions on this timer.
	enum class TimerTrigger {
		TIMEUP,
		DEACTIVATION
	};


public:
	MissionTimer() = default;
	MissionTimer(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);
	// Set up the timer from its data file node.
	void Load(const DataNode &node, const ConditionsStore *playerConditions,
		const std::set<const System *> *visitedSystems, const std::set<const Planet *> *visitedPlanets);
	void Save(DataWriter &out) const;

	// Calculate the total time to wait, including any random value.
	MissionTimer Instantiate(std::map<std::string, std::string> &subs, const System *origin,
		int jumps, int64_t payload) const;

	// Get whether the timer is optional to complete.
	bool IsOptional() const;
	// Get whether the timer has completed.
	bool IsComplete() const;
	// Progress the timer within the main loop.
	void Step(PlayerInfo &player, UI *ui, const Mission &mission);


private:
	// Determine if the player meets the criteria for this timer to be active.
	bool CanActivate(const Ship *flagship, const PlayerInfo &player) const;
	// The player does not meet the criteria for this timer to be active.
	// Deactivate the timer and determine if it should be reset.
	void Deactivate(PlayerInfo &player, UI *ui, const Mission &mission);


private:
	// The base number of frames to wait, with an optional maximum random added value.
	int waitTime = 0;
	int randomWaitTime = 0;
	// If set, the timer is not a necessary objective for the completion of its mission.
	bool optional = false;
	// If true, the timer pauses instead of resetting when deactivated.
	bool pauses = false;

	// Whether any of the activation requirements below must be checked by this timer.
	bool hasRequirements = false;

	// Whether the timer requires the player to be idle.
	bool requireIdle = false;
	// The square of the speed threshold the player's flagship must be under to count as "idle".
	double idleMaxSpeed = 25.;
	// Whether the timer requires the player to not be firing.
	bool requirePeaceful = false;
	// Whether the timer requires the player to be uncloaked or cloaked.
	bool requireUncloaked = false;
	bool requireCloaked = false;
	// Whether the player's flagship must be the only ship in their fleet in the system.
	bool requireSolo = false;

	// The system the timer is for.
	const System *system = nullptr;
	// The filter for the systems it can be for.
	LocationFilter systems;

	// Actions to be performed when triggers are fired.
	std::map<TimerTrigger, MissionAction> actions;
	// Actions that have already been performed.
	std::set<TimerTrigger> triggeredActions;

	// The number of frames that have elapsed while the timer is active.
	int timeElapsed = 0;
	// Set to true when all the conditions are met for the timer to count down.
	bool isActive = false;
	// Set to true once the timer has run to completion.
	bool isComplete = false;
};
