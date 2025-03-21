/* Timer.cpp
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

#include "Timer.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Logger.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "StellarObject.h"
#include "UI.h"

using namespace std;

namespace {
	string TriggerToText(Timer::TimerTrigger trigger)
	{
		switch(trigger)
		{
			case Timer::TimerTrigger::TIMEUP:
				return "on timeup";
			case Timer::TimerTrigger::RESET:
				return "on reset";
			default:
				return "unknown trigger";
		}
	}
}



Timer::Timer(const DataNode &node)
{
	Load(node);
}



void Timer::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "time" && child.Size() > 1)
		{
			waitTime = child.Value(1);
			if(child.Size() > 2)
				randomWaitTime = child.Value(2);
		}
		else if(child.Token(0) == "elapsed")
			// This is only for saved timers; it is not intended for the data files.
			timeElapsed = child.Value(1);
		else if(child.Token(0) == "idle")
		{
			requireIdle = true;
			if(child.Size() > 1)
			{
				idleMaxSpeed = child.Value(1);
				// We square the max speed value here, so it can be conveniently
				// compared to the flagship's squared velocity length below.
				idleMaxSpeed *= idleMaxSpeed;
			}
		}
		else if(child.Token(0) == "peaceful")
			requirePeaceful = true;
		else if(child.Token(0) == "optional")
			optional = true;
		else if(child.Token(0) == "system")
		{
			if(child.Size() > 1)
				system = GameData::Systems().Get(child.Token(1));
			else
				systems.Load(child);
		}
		else if(child.Token(0) == "proximity")
		{
			if(child.Size() > 1)
				proximityCenter = GameData::Planets().Get(child.Token(1));
			else
				proximityCenters.Load(child);
		}
		else if(child.Token(0) == "proximity settings" && child.Size() > 1)
		{
			proximity = child.Value(1);
			if(child.Size() > 2 && child.Token(2) == "far")
				closeTo = false;
		}
		else if(child.Token(0) == "uncloaked")
			requireUncloaked = true;
		else if(child.Token(0) == "reset" && child.Size() > 1)
		{
			const string &value = child.Token(1);
			if(value == "leave system")
				resetCondition = ResetCondition::LEAVE_SYSTEM;
			else if(value == "leave zone")
				resetCondition = ResetCondition::LEAVE_ZONE;
			else if(value == "none")
				resetCondition = ResetCondition::NONE;
			else if(value == "pause")
				resetCondition = ResetCondition::PAUSE;
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(child.Token(0) == "repeat reset")
			repeatReset = true;
		else if(child.Token(0) == "reset fired")
			resetFired = true;
		else if(child.Token(0) == "on" && child.Size() > 1 && child.Token(1) == "timeup")
			actions[TimerTrigger::TIMEUP].Load(child);
		else if(child.Token(0) == "on" && child.Size() > 1 && child.Token(1) == "reset")
			actions[TimerTrigger::RESET].Load(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// Note: the Save() function can assume this is an instantiated timer, not a template,
// so the time to wait will be saved fully calculated, and with any elapsed time subtracted.
void Timer::Save(DataWriter &out) const
{
	// If this Timer should no longer appear in-game, don't serialize it.
	if(isComplete)
		return;

	out.Write("timer");
	out.BeginChild();
	{
		if(randomWaitTime)
			out.Write("time", waitTime, randomWaitTime);
		else
			out.Write("time", waitTime);
		out.Write("elapsed", timeElapsed);
		if(system)
			out.Write("system", system->TrueName());
		else if(!systems.IsEmpty())
		{
			out.Write("system");
			systems.Save(out);
		}
		if(requireIdle)
			out.Write("idle", sqrt(idleMaxSpeed));
		if(requirePeaceful)
			out.Write("peaceful");
		if(optional)
			out.Write("optional");
		if(requireUncloaked)
			out.Write("uncloaked");
		if(repeatReset)
			out.Write("repeat reset");
		if(resetFired)
			out.Write("reset fired");
		if(resetCondition != ResetCondition::PAUSE)
		{
			static const map<ResetCondition, string> resets = {
				{ResetCondition::NONE, "none"},
				{ResetCondition::LEAVE_SYSTEM, "leave system"},
				{ResetCondition::LEAVE_ZONE, "leave zone"}
			};
			out.Write("reset", resets.at(resetCondition));
		}
		if(proximity > 0.)
		{
			string distance;
			if(!closeTo)
				distance = "far";
			if(proximityCache.size() > 0)
			{
				out.Write("proximity");
				for(const StellarObject *proximityObject : proximityCache)
					out.Write(proximityObject->GetPlanet()->TrueName());
			}
			out.Write("proximity settings", proximity, distance);
		}

		for(const auto &it : actions)
			it.second.Save(out);
	}
	out.EndChild();
}



// Calculate the total time to wait, including any random value,
// and instantiate the triggered action.
Timer Timer::Instantiate(const ConditionsStore &store, map<string, string> &subs, const System *origin,
	int jumps, int64_t payload) const
{
	Timer result;
	result.requireIdle = requireIdle;
	result.requireUncloaked = requireUncloaked;
	result.requirePeaceful = requirePeaceful;
	result.system = system;
	result.systems = systems;
	result.closeTo = closeTo;
	result.resetCondition = resetCondition;
	result.repeatReset = repeatReset;
	result.resetFired = resetFired;
	result.idleMaxSpeed = idleMaxSpeed;

	// Validate all the actions attached to the timer, and if they're all valid, instantiate them too.
	string reason;
	auto ait = actions.begin();
	for( ; ait != actions.end(); ++ait)
	{
		reason = ait->second.Validate();
		if(!reason.empty())
			break;
	}
	if(ait != actions.end())
	{
		Logger::LogError("Instantiation Error: Timer action \"" + TriggerToText(ait->first)
			+ "\" uses invalid " + std::move(reason));
		return result;
	}
	for(const auto &it : actions)
		result.actions[it.first] = it.second.Instantiate(store, subs, origin, jumps, payload);

	// Calculate the random variance to the wait time.
	result.waitTime = waitTime;
	if(randomWaitTime > 1)
		result.waitTime += Random::Int(randomWaitTime);

	// We also build a cache of the matching proximity object(s) for the instantiated timer.
	// This avoids having to do all these comparisons every Step().
	if(system && (proximityCenter || !proximityCenters.IsEmpty()))
		for(const StellarObject &proximityObject : system->Objects())
			if(proximityObject.HasValidPlanet() && (proximityCenter == proximityObject.GetPlanet() ||
					proximityCenters.Matches(proximityObject.GetPlanet())))
				result.proximityCache.insert(&proximityObject);

	return result;
}



// Get whether the timer is optional to complete.
bool Timer::IsOptional() const
{
	return optional;
}



// Get whether the timer has completed.
bool Timer::IsComplete() const
{
	return isComplete;
}


// This method gets called every time the possible reset conditions are met,
// regardless of whether this particular timer is set to reset on them.
// If it is, then it resets the elapsed time to 0, marks the timer as inactive,
// and conditionally fires the reset action, if any.
void Timer::ResetOn(ResetCondition cond, PlayerInfo &player, UI *ui, const Mission &mission)
{
	// Check whether this timer actually wants to reset on the given condition.
	bool reset = cond == resetCondition;
	// If the reset condition being passed in is LEAVE_ZONE (ie, the player is no longer in proximity),
	// then we also reset if the timer's reset condition is PAUSE (ie, player no longer meets the criteria).
	reset |= (cond == ResetCondition::LEAVE_ZONE && resetCondition == ResetCondition::PAUSE);
	// If the reset condition being passed in is LEAVE_SYSTEM, then we also reset if either of the other two
	// conditions is specified for the timer, since we are then necessarily outside the zone, and thus the
	// clock is stopped.
	reset |= (cond == ResetCondition::LEAVE_SYSTEM && (resetCondition == ResetCondition::PAUSE
		|| resetCondition == ResetCondition::LEAVE_ZONE));
	if(isActive && reset)
	{
		timeElapsed = 0;
		// Perform the reset action, if there is one, assuming either it
		// hasn't fired yet, or the timer is configured to fire it every reset.
		if(repeatReset || !resetFired)
		{
			auto it = actions.find(TimerTrigger::RESET);
			if(it != actions.end())
				it->second.Do(player, ui, &mission);
			resetFired = true;
		}
		isActive = false;
	}
}



void Timer::Step(PlayerInfo &player, UI *ui, const Mission &mission)
{
	// Don't do any work for already-completed timers.
	if(isComplete)
		return;
	const Ship *flagship = player.Flagship();
	// Since we can only advance timers while flying, we can't do that if we have no flagship.
	if(!flagship)
		return;
	// Now we go through all the possible reset/do-not-advance conditions for the timer.
	// If and only if it passes all of them, the timer is marked as active and it advances.
	// For each, if it fails, it calls ResetOn() with that reset condition.

	// First we check to see if the player is in the specified system (or one of them).
	if((system && flagship->GetSystem() != system) ||
		(!systems.IsEmpty() && !systems.Matches(flagship->GetSystem())))
	{
		ResetOn(ResetCondition::LEAVE_SYSTEM, player, ui, mission);
		return;
	}

	// Then we check if the timer requires the player to be idle (not turning, accelerating, or moving faster than
	// the idle max speed) or peaceful (not firing any weapons).
	if(requireIdle || requirePeaceful)
	{
		bool shipIdle = true;
		if(requireIdle)
			shipIdle = !flagship->IsThrusting() && !flagship->IsSteering()
				&& !flagship->IsReversing() && flagship->Velocity().LengthSquared() < idleMaxSpeed;
		if(requirePeaceful)
			for(const Hardpoint &hardpoint : flagship->Weapons())
				shipIdle &= !hardpoint.WasFiring();
		if(!shipIdle)
		{
			ResetOn(ResetCondition::PAUSE, player, ui, mission);
			return;
		}
	}

	// Then we check if the flagship is required to be uncloaked.
	if(requireUncloaked)
	{
		if(flagship->Cloaking())
		{
			ResetOn(ResetCondition::PAUSE, player, ui, mission);
			return;
		}
	}

	// Finally, we check if the flagship is required to be close to a particular stellar object.
	if(proximity > 0.)
	{
		bool inProximity = false;
		if(proximityCache.size() > 0)
			for(const StellarObject *proximityObject : proximityCache)
			{
				double dist = flagship->Position().Distance(proximityObject->Position());
				inProximity = closeTo ? dist <= proximity : dist >= proximity;
				if(inProximity)
					break;
			}
		else
		{
			double dist = flagship->Position().Distance(Point(0., 0.));
			inProximity = closeTo ? dist <= proximity : dist >= proximity;
		}
		if(!inProximity)
		{
			ResetOn(ResetCondition::LEAVE_ZONE, player, ui, mission);
			return;
		}
	}

	// Saving our active state allows us to avoid unnecessary resets.
	isActive = true;

	// And here is the actual core of the timer: advance the time by 1 tick, and
	// if it's been long enough, fire the timeup action.
	if(++timeElapsed >= waitTime)
	{
		auto it = actions.find(TimerTrigger::TIMEUP);
		if(it != actions.end())
			it->second.Do(player, ui, &mission);
		isComplete = true;
	}
}
