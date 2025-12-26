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
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "System.h"

#include <cmath>

using namespace std;

namespace {
	string TriggerToText(MissionTimer::TimerTrigger trigger)
	{
		switch(trigger)
		{
			case MissionTimer::TimerTrigger::TIMEUP:
				return "on timeup";
			case MissionTimer::TimerTrigger::DEACTIVATION:
				return "on deactivation";
			default:
				return "unknown trigger";
		}
	}
}



MissionTimer::MissionTimer(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	Load(node, playerConditions, visitedSystems, visitedPlanets);
}



void MissionTimer::Load(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("Expected key to have a value:");
		return;
	}

	waitTime = node.Value(1);
	if(node.Size() > 2)
		randomWaitTime = max<int>(1, node.Value(2));

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "elapsed" && hasValue)
			timeElapsed = child.Value(1);
		else if(key == "optional")
			optional = true;
		else if(key == "pause when inactive")
			pauses = true;
		else if(key == "activation requirements" && child.HasChildren())
		{
			hasRequirements = true;
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				hasValue = grand.Size() >= 2;

				if(grandKey == "idle")
				{
					requireIdle = true;
					if(hasValue)
					{
						idleMaxSpeed = grand.Value(1);
						// We square the max speed value here, so it can be conveniently
						// compared to the flagship's squared velocity length below.
						idleMaxSpeed *= idleMaxSpeed;
					}
				}
				else if(grandKey == "peaceful")
					requirePeaceful = true;
				else if(grandKey == "cloaked")
				{
					requireCloaked = true;
					if(requireUncloaked)
					{
						requireUncloaked = false;
						grand.PrintTrace("Disabling previously declared \"uncloaked\" requirement.");
					}
				}
				else if(grandKey == "uncloaked")
				{
					requireUncloaked = true;
					if(requireCloaked)
					{
						requireCloaked = false;
						grand.PrintTrace("Disabling previously declared \"cloaked\" requirement.");
					}
				}
				else if(grandKey == "solo")
					requireSolo = true;
				else if(grandKey == "system")
				{
					if(hasValue)
						system = GameData::Systems().Get(grand.Token(1));
					else if(grand.HasChildren())
						systems.Load(grand, visitedSystems, visitedPlanets);
					else
						grand.PrintTrace("Skipping unrecognized attribute:");
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(key == "on" && hasValue)
		{
			const string &trigger = child.Token(1);
			if(trigger == "timeup")
				actions[TimerTrigger::TIMEUP].Load(child, playerConditions, visitedSystems, visitedPlanets);
			else if(trigger == "deactivation")
				actions[TimerTrigger::DEACTIVATION].Load(child, playerConditions, visitedSystems, visitedPlanets);
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(key == "triggered actions")
		{
			for(const DataNode &grand : child)
			{
				const string &trigger = grand.Token(0);
				if(trigger == "deactivation")
					triggeredActions.insert(TimerTrigger::DEACTIVATION);
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void MissionTimer::Save(DataWriter &out) const
{
	// If this Timer should no longer appear in-game, don't serialize it.
	if(isComplete)
		return;

	out.Write("timer", waitTime);
	out.BeginChild();
	{
		out.Write("elapsed", timeElapsed);
		if(optional)
			out.Write("optional");
		if(pauses)
			out.Write("pause when inactive");
		if(hasRequirements)
		{
			out.Write("activation requirements");
			out.BeginChild();
			{
				if(requireIdle)
					out.Write("idle", sqrt(idleMaxSpeed));
				if(requirePeaceful)
					out.Write("peaceful");
				if(requireCloaked)
					out.Write("cloaked");
				else if(requireUncloaked)
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
			}
			out.EndChild();
		}
		if(!triggeredActions.empty())
		{
			out.Write("triggered actions");
			out.BeginChild();
			{
				for(const TimerTrigger &trigger : triggeredActions)
				{
					if(trigger == TimerTrigger::DEACTIVATION)
						out.Write("deactivation");
				}
			}
			out.EndChild();
		}
		for(const auto &[trigger, action] : actions)
			action.Save(out);
	}
	out.EndChild();
}



MissionTimer MissionTimer::Instantiate(map<string, string> &subs, const System *origin,
	int jumps, int64_t payload) const
{
	MissionTimer result;
	result.optional = optional;
	result.pauses = pauses;

	result.hasRequirements = hasRequirements;
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
	if(randomWaitTime)
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
		Logger::Log("Instantiation Error: Timer action \"" + TriggerToText(ait->first)
			+ "\" uses invalid " + std::move(reason), Logger::Level::WARNING);
		return result;
	}
	for(const auto &[trigger, action] : actions)
		result.actions[trigger] = action.Instantiate(subs, origin, jumps, payload);

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

	// Determine if the player meets the activation requirements for this timer.
	if(!CanActivate(flagship, player))
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



bool MissionTimer::CanActivate(const Ship *flagship, const PlayerInfo &player) const
{
	// If this timer has no requirements to check, then it should be active.
	if(!hasRequirements)
		return true;

	// Does the player's system match the system filter?
	const System *flagshipSystem = flagship->GetSystem();
	if((system && flagshipSystem != system) ||
			(!systems.IsEmpty() && !systems.Matches(flagshipSystem)))
		return false;

	// Does this timer require that the player is solo (i.e. there are
	// no escorts in the system with the player)?
	if(requireSolo)
	{
		for(const auto &escort : player.Ships())
		{
			// Using GetSystem instead of GetActualSystem so that docked
			// fighters on the player's flagship don't count against them.
			if(escort.get() != flagship && !escort->IsParked() && !escort->IsDestroyed()
					&& escort->GetSystem() == flagshipSystem)
				return false;
		}
	}

	// Does this timer require that the player is idle?
	if(requireIdle)
	{
		// The player can't be sending movement commands.
		if(flagship->IsThrusting() || flagship->IsSteering() || flagship->IsReversing())
			return false;
		// And their ship's velocity must be below the max speed threshold.
		if(flagship->Velocity().LengthSquared() > idleMaxSpeed)
			return false;
	}

	// Does this timer require that the player is peaceful?
	if(requirePeaceful)
	{
		// If the player is required to be peaceful, then none of their weapons
		// can have a fire command. Special weapons like anti-missile turrets which
		// are only used defensively and automatically do not count against the
		// player.
		for(const Hardpoint &hardpoint : flagship->Weapons())
			if(!hardpoint.IsSpecial() && hardpoint.WasFiring())
				return false;
	}

	// Does this timer require that the player is cloaked or uncloaked?
	double cloaking = flagship->Cloaking();
	if((requireUncloaked && cloaking) || (requireCloaked && cloaking != 1.))
		return false;

	return true;
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

	// Perform the DEACTIVATION action, if there is one, assuming it hasn't fired yet.
	if(triggeredActions.insert(TimerTrigger::DEACTIVATION).second)
	{
		auto it = actions.find(TimerTrigger::DEACTIVATION);
		if(it != actions.end())
			it->second.Do(player, ui, &mission);
	}
}
