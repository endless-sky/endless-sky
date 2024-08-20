/* Timer.h
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

#ifndef TIMER_H_
#define TIMER_H_

#include "MissionAction.h"
#include "System.h"

#include <cstdint>
#include <map>
#include <string>

class DataNode;
class DataWriter;
class Planet;
class PlayerInfo;
class UI;



// Class representing a timer for triggering mission actions. Timers count
// down a certain number of frames before triggering if the player meets the
// conditions for the timer starting (e.g. the player is moving slowly or is near
// a certain object), but may be reset by various actions the player takes.
class Timer {
public:
	enum class TimerTrigger {
		// The possible triggers for actions on this timer.
		TIMEUP,
		RESET
	};


public:
	Timer() = default;
	Timer(const DataNode &node);
	// Set up the timer from its data file node.
	void Load(const DataNode &node);
	// Note: the Save() function can assume this is an instantiated Timer, not a template,
	// so the time to wait will be saved fully calculated, and with any elapsed time subtracted.
	void Save(DataWriter &out) const;

	// Get whether the timer is optional to complete.
	bool IsOptional() const;
	// Get whether the timer has completed.
	bool IsComplete() const;

	// Calculate the total time to wait, including any random value.
	Timer Instantiate(const ConditionsStore &store, std::map<std::string, std::string> &subs, const System *origin,
		int jumps, int64_t payload) const;
	// Progress the timer within the main loop.
	void Step(PlayerInfo &player, UI *ui, const Mission &mission);


private:
	enum class ResetCondition {
		NONE,
		PAUSE,
		LEAVE_ZONE,
		LEAVE_SYSTEM
	};
	void ResetOn(ResetCondition cond, PlayerInfo &player, UI *ui, const Mission &mission);


private:
	// The base number of frames to wait, with an optional maximum random added value.
	int waitTime = 0;
	int randomWaitTime = 0;

	// The system the timer is for.
	const System *system = nullptr;
	// The filter for the systems it can be for.
	LocationFilter systems;

	// If set, the timer is not a necessary objection for the completion of its mission.
	bool optional = false;
	// Whether the timer requires the player to be idle.
	bool requireIdle = false;
	// The speed threshold the player's flagship must be under to count as "idle".
	double idleMaxSpeed = 25.;
	// Whether the timer requires the player to not be firing.
	bool requirePeaceful = false;
	// Whether the timer requires the player to be uncloaked to advance.
	bool requireUncloaked = false;
	// What circumstances will reset the timer: leaving the system,
	// leaving the proximity zone (if applicable), or any circumstance that stops the timer.
	ResetCondition resetCondition = Timer::ResetCondition::PAUSE;
	// If proximity is specified, this determines whether the timer will only advance while
	// close to or far from the specified center; default is close to.
	bool closeTo = true;
	// This specifies the radius around the proximity center to be checked.
	double proximity = 0.;
	// This specifies the system object to be the center; if this is nullptr and the filter is empty,
	// but proximity is positive, then the system center is used instead.
	const Planet *proximityCenter = nullptr;
	// The filter to use for which planet(s) to hang out near if a single planet isn't set.
	LocationFilter proximityCenters;

	// The action to be performed when triggers are fired.
	std::map<TimerTrigger, MissionAction> actions;
	// Should the reset action be performed every time, or just the first?
	bool repeatReset = false;
	bool resetFired = false;

	// Used for holding the current timer value when it's actually active.
	int timeElapsed = 0.;
	// Set to true once the timer has run to completion so we don't keep trying to save or run it.
	bool isComplete = false;
	// Set to true when all the conditions are met for the timer to count down.
	bool isActive = false;

	// A place to cache the proximity object(s) so we don't have to test for them every time.
	std::set<const StellarObject *> proximityCache;
};



#endif
