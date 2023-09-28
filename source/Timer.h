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

// Class representing a timer for triggering mission actions
// This requires a specification of how long to wait (base + randint(rand)),
// a system to wait in, and whether to reset the timer upon leaving the system
// and can optionally require the player to be idle (no acceleration), and/or
// within or beyond a certain proximity to the system center or a
// specific (named) system object
class Timer {
public:
	Timer() = default;
	Timer(const DataNode &node, const Mission *mission);
	// Set up the timer from its data file node
	void Load(const DataNode &node, const Mission *mission);
	// Note: the Save() function can assume this is an instantiated Timer, not a template,
	// so the time to wait will be saved fully calculated, and with any elapsed time subtracted
	void Save(DataWriter &out) const;

	// Get whether the timer is optional to complete
	bool IsOptional() const;
	// Get whether the timer is currently active
	bool IsComplete() const;

	// Calculate the total time to wait, including any random value
	Timer Instantiate(std::map<std::string, std::string> &subs,
					const System *origin, int jumps, int64_t payload) const;
	// Progress the timer within the main loop
	void Step(PlayerInfo &player, UI *ui);

private:
	enum class ResetCondition {
		NONE,
		PAUSE,
		LEAVE_ZONE,
		LEAVE_SYSTEM
	};
	void ResetOn(ResetCondition cond, PlayerInfo &player, UI *ui);

private:
	// The name of the timer
	std::string name;
	// The basic amount of time to wait, with the optional random values
	uint64_t base = 0;
	uint32_t rand = 0;

	// The system the timer is for
	const System *system = nullptr;
	// ...Or the systems it can be for
	LocationFilter systems;

	// If set, the mission the timer is attached to can be completed even without it
	bool optional = false;
	// Whether the timer requires the player to be idle
	bool requireIdle = false;
	// Whether the timer requires the player to be uncloaked to advance
	bool requireUncloaked = false;
	// What circumstances will reset the timer: leaving the system,
	// leaving the proximity zone (if applicable), or any circumstance that stops the timer
	ResetCondition resetCondition = Timer::ResetCondition::PAUSE;
	// If proximity is specified, this determines whether the timer will only advance while
	// close to or far from the specified center; default is close to
	bool closeTo = true;
	// This specifies the radius around the proximity center to be checked
	double proximity = 0.;
	// This specifies the system object to be the center; if this is nullptr, but proximity
	// is positive, then the system center is used instead
	const Planet *proximityCenter = nullptr;

	// The action to be performed when the timer elapses
	MissionAction action;
	// The action to be performed when the timer resets
	MissionAction resetAction;
	// Should the reset action be performed every time, or just the first?
	bool repeatReset = false;
	bool resetFired = false;

	// Used for holding the calculated time to wait and current timer value when it's actually active
	int64_t timeToWait = 0;
	int64_t timeElapsed = 0.;
	// Set to true once the timer has run to completion so we don't keep trying to save or run it
	bool isComplete = false;
	// Set to true when all the conditions are met
	bool isActive = false;

	const Mission *mission = nullptr;
};



#endif
