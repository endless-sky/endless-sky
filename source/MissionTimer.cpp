/* MissionTimer.cpp
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

#include "MissionTimer.h"

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
	string TriggerToText(MissionTimer::TimerTrigger trigger)
	{
		switch(trigger)
		{
			case MissionTimer::TimerTrigger::TIMEUP:
				return "on timeup";
			case MissionTimer::TimerTrigger::RESET:
				return "on reset";
			default:
				return "unknown trigger";
		}
	}
}



MissionTimer::MissionTimer(const DataNode &node, const ConditionsStore *playerConditions)
{
	Load(node, playerConditions);
}



void MissionTimer::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "time" && hasValue)
		{
			waitTime = child.Value(1);
			if(child.Size() > 2)
				randomWaitTime = child.Value(2);
		}
		else if(key == "elapsed")
			timeElapsed = child.Value(1);
		else if(key == "optional")
			optional = true;
		else if(key == "pauses")
			pauses = true;
		else if(key == "repeat reset")
			repeatReset = true;
		else if(key == "reset fired")
			resetFired = true;
		else if(key == "idle")
		{
			requireIdle = true;
			if(hasValue)
			{
				idleMaxSpeed = child.Value(1);
				// We square the max speed value here, so it can be conveniently
				// compared to the flagship's squared velocity length below.
				idleMaxSpeed *= idleMaxSpeed;
			}
		}
		else if(key == "peaceful")
			requirePeaceful = true;
		else if(key == "cloaked")
		{
			requireCloaked = true;
			requireUncloaked = false;
		}
		else if(key == "uncloaked")
		{
			requireUncloaked = true;
			requireCloaked = false;
		}
		else if(key == "requireSolo")
			requireSolo = true;
		else if(key == "system")
		{
			if(hasValue)
				system = GameData::Systems().Get(child.Token(1));
			else if(child.HasChildren())
				systems.Load(child);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "on" && hasValue && child.Token(1) == "timeup")
			actions[TimerTrigger::TIMEUP].Load(child, playerConditions);
		else if(key == "on" && hasValue && child.Token(1) == "reset")
			actions[TimerTrigger::RESET].Load(child, playerConditions);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void MissionTimer::Save(DataWriter &out) const
{
	// If this Timer should no longer appear in-game, don't serialize it.
	if(isComplete)
		return;

	out.Write("timer");
	out.BeginChild();
	{
		out.Write("time", waitTime);
		out.Write("elapsed", timeElapsed);
		if(optional)
			out.Write("optional");
		if(pauses)
			out.Write("pauses");
		if(repeatReset)
			out.Write("repeat reset");
		if(resetFired)
			out.Write("reset fired");
		if(requireIdle)
			out.Write("idle", sqrt(idleMaxSpeed));
		if(requirePeaceful)
			out.Write("peaceful");
		if(requireCloaked)
			out.Write("cloaked");
		if(requireUncloaked)
			out.Write("uncloaked");
		if(requireSolo)
			out.Write("solo");
		if(system)
			out.Write("system", system->TrueName());
		else if(!systems.IsEmpty())
		{
			out.Write("system");
			systems.Save(out);
		}
		for(const auto &it : actions)
			it.second.Save(out);
	}
	out.EndChild();
}



MissionTimer MissionTimer::Instantiate(map<string, string> &subs, const System *origin,
	int jumps, int64_t payload) const
{
	MissionTimer result;
	result.optional = optional;
	result.pauses = pauses;
	result.repeatReset = repeatReset;
	result.requirePeaceful = requirePeaceful;
	result.requireUncloaked = requireUncloaked;
	result.requireCloaked = requireCloaked;
	result.requireIdle = requireIdle;
	result.idleMaxSpeed = idleMaxSpeed;
	result.requireSolo = requireSolo;
	result.system = system;
	result.systems = systems;

	// Calculate the random variance to the wait time.
	result.waitTime = waitTime;
	if(randomWaitTime > 1)
		result.waitTime += Random::Int(randomWaitTime);

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
		result.actions[it.first] = it.second.Instantiate(subs, origin, jumps, payload);

	return result;
}



bool MissionTimer::IsOptional() const
{
	return optional;
}



bool MissionTimer::IsComplete() const
{
	return isComplete;
}



void MissionTimer::Step(PlayerInfo &player, UI *ui, const Mission &mission)
{
	if(isComplete)
		return;

	const Ship *flagship = player.Flagship();
	if(!flagship)
		return;
	// Don't activate or deactivate the timer if the player is taking
	// off from a planet or traveling through hyperspace.
	if(flagship->Zoom() != 1. || flagship->IsHyperspacing())
		return;

	// Does the player's system match the system filter?
	if((system && flagship->GetSystem() != system) ||
		(!systems.IsEmpty() && !systems.Matches(flagship->GetSystem())))
	{
		Deactivate(player, ui, mission);
		return;
	}

	// Does this timer require that the player is solo (i.e. there are
	// no escorts in the system with the player)?
	if(requireSolo)
	{
		const System *flagshipSystem = flagship->GetSystem();
		for(const auto &escort : player.Ships())
		{
			// Using GetSystem instead of GetActualSystem so that docked
			// fighters on the player's flagship don't count against them.
			if(escort.get() != flagship && !escort->IsParked() && !escort->IsDestroyed()
				&& escort->GetSystem() == flagshipSystem)
			{
				Deactivate(player, ui, mission);
				return;
			}
		}
	}

	// Does this timer require that the player is idle?
	if(requireIdle)
	{
		bool shipIdle = true;
		// The player can't be sending movement commands.
		if(!flagship->IsThrusting() && !flagship->IsSteering() && !flagship->IsReversing())
			shipIdle = false;
		// And their ship's velocity must be below the max speed threshold.
		else if(flagship->Velocity().LengthSquared() < idleMaxSpeed)
			shipIdle = false;
		if(!shipIdle)
		{
			Deactivate(player, ui, mission);
			return;
		}
	}

	// Does this timer require that the player is peaceful?
	if(requirePeaceful)
	{
		// If the player is required to be peaceful, then none of their weapons
		// can have a fire command.
		for(const Hardpoint &hardpoint : flagship->Weapons())
			if(hardpoint.WasFiring())
			{
				Deactivate(player, ui, mission);
				return;
			}
	}

	// Does this timer require that the player is cloaked or uncloaked?
	double cloaking = flagship->Cloaking();
	if((requireUncloaked && cloaking) || (requireCloaked && cloaking != 1.))
	{
		Deactivate(player, ui, mission);
		return;
	}

	// This timer is now active and should advance its counter by 1 tick.
	// If the full wait time has elapsed, this timer is complete.
	isActive = true;
	if(++timeElapsed >= waitTime)
	{
		auto it = actions.find(TimerTrigger::TIMEUP);
		if(it != actions.end())
			it->second.Do(player, ui, &mission);
		isComplete = true;
	}
}



void MissionTimer::Deactivate(PlayerInfo &player, UI *ui, const Mission &mission)
{
	// If the timer wasn't active the frame before, don't do anything.
	if(!isActive)
		return;
	isActive = false;

	// Reset the timer if it isn't set to pause when deactivated.
	if(!pauses)
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
}
