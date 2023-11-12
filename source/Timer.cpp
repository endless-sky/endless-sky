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
			timeToWait = child.Value(1);
			if(child.Size() > 2)
				rand = child.Value(2);
		}
		else if(child.Token(0) == "idle")
		{
			requireIdle = true;
			// We square the max speed value here, so it can be conveniently
			// compared to the flagship's squared velocity length below.
			if(child.Size() > 1)
				idleMaxSpeed = child.Value(1) * child.Value(1);
		}
		else if(child.Token(0) == "optional")
			optional = true;
		else if(child.Token(0) == "system" && child.Size() > 1)
			system = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "system")
			systems.Load(child);
		else if(child.Token(0) == "proximity" && child.Size() > 1)
		{
			proximityCenter = GameData::Planets().Get(child.Token(1));
		}
		else if(child.Token(0) == "proximity")
		{
			proximityCenters.Load(child);
		}
		else if(child.Token(0) == "proximity settings" && child.Size() > 1)
		{
			proximity = child.Value(1);
			if(child.Size() > 2)
			{
				if(child.Token(2) == "close")
					closeTo = true;
				else if(child.Token(2) == "far")
					closeTo = false;
			}
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
		// We keep "on timeup" as separate tokens so that it's compatible with MissionAction syntax.
		else if(child.Token(0) == "on" && child.Size() > 1 && child.Token(1) == "timeup")
			actions[TIMEUP].Load(child);
		else if(child.Token(0) == "on" && child.Size() > 1 && child.Token(1) == "reset")
			actions[RESET].Load(child);
		else
			child.PrintTrace("Skipping unrecognized attribute:");

	}
}



// Note: the Save() function can assume this is an instantiated Timer, not a template,
// so the time to wait will be saved fully calculated, and with any elapsed time subtracted.
void Timer::Save(DataWriter &out) const
{
	// If this Timer should no longer appear in-game, don't serialize it.
	if(isComplete)
		return;

	int64_t timeRemaining = timeToWait - timeElapsed;
	out.Write("timer");
	out.BeginChild();
	{
		out.Write("time", timeRemaining);
		if(system)
			out.Write("system", system->Name());
		else if(!systems.IsEmpty())
		{
			out.Write("system");
			systems.Save(out);
		}
		if(requireIdle)
		{
			out.Write("idle", sqrt(idleMaxSpeed));
		}
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
				{Timer::ResetCondition::NONE, "none"},
				{Timer::ResetCondition::LEAVE_SYSTEM, "leave system"},
				{Timer::ResetCondition::LEAVE_ZONE, "leave zone"}
			};
			auto it = resets.find(resetCondition);
			if(it != resets.end())
				out.Write("reset", it->second);
		}
		if(proximity > 0.)
		{
			string distance = "close";
			if(!closeTo)
				distance = "far";
			if(proximityCenter)
				out.Write("proximity", proximityCenter->Name());
			else if(!proximityCenters.IsEmpty())
			{
				out.Write("proximity");
				proximityCenters.Save(out);
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
Timer Timer::Instantiate(map<string, string> &subs,
						const System *origin, int jumps, int64_t payload) const
{
	Timer result;
	result.requireIdle = requireIdle;
	result.requireUncloaked = requireUncloaked;
	result.system = system;
	result.systems = systems;
	result.proximity = proximity;
	result.proximityCenter = proximityCenter;
	result.proximityCenters = proximityCenters;
	result.closeTo = closeTo;
	result.resetCondition = resetCondition;
	result.repeatReset = repeatReset;
	result.resetFired = resetFired;
	result.idleMaxSpeed = idleMaxSpeed;

	result.timeElapsed = timeElapsed;
	result.isComplete = isComplete;
	result.isActive = isActive;

	for(const auto &it : actions)
		result.actions[it.first] = it.second.Instantiate(subs, origin, jumps, payload);

	if(rand >= 1)
		result.timeToWait = timeToWait + Random::Int(rand);
	else
		result.timeToWait = timeToWait;

	return result;
}



bool Timer::IsComplete() const
{
	return isComplete;
}



bool Timer::IsOptional() const
{
	return optional;
}



void Timer::ResetOn(ResetCondition cond, PlayerInfo &player, UI *ui, const Mission &mission)
{
	bool reset = cond == resetCondition;
	reset |= (cond == Timer::ResetCondition::LEAVE_ZONE && resetCondition == Timer::ResetCondition::PAUSE);
	reset |= (cond == Timer::ResetCondition::LEAVE_SYSTEM && (resetCondition == Timer::ResetCondition::PAUSE
				|| resetCondition == Timer::ResetCondition::LEAVE_ZONE));
	if(isActive && reset)
	{
		timeToWait = timeToWait + timeElapsed;
		timeElapsed = 0;
		if(repeatReset || !resetFired)
		{
			auto it = actions.find(RESET)
			if(it != actions.end())
				it->second.Do(player, ui, &mission);
			resetFired = true;
		}
		isActive = false;
	}
}



void Timer::Step(PlayerInfo &player, UI *ui, const Mission &mission)
{
	if(isComplete)
		return;
	const Ship *flagship = player.Flagship();
	if(!flagship)
		return;
	if((system && flagship->GetSystem() != system) ||
		(!systems.IsEmpty() && !systems.Matches(flagship->GetSystem())))
	{
		ResetOn(Timer::ResetCondition::LEAVE_SYSTEM, player, ui, mission);
		return;
	}
	if(requireIdle)
	{
		bool shipIdle = (!flagship->IsThrusting() && !flagship->IsSteering()
						&& !flagship->IsReversing() && flagship->Velocity().LengthSquared() < idleMaxSpeed);
		for(const Hardpoint &weapon : flagship->Weapons())
			shipIdle &= !weapon.WasFiring();
		if(!shipIdle)
		{
			ResetOn(Timer::ResetCondition::PAUSE, player, ui, mission);
			return;
		}
	}
	if(requireUncloaked)
	{
		double cloak = flagship->Cloaking();
		if(cloak != 0.)
		{
			ResetOn(Timer::ResetCondition::PAUSE, player, ui, mission);
			return;
		}
	}
	if(proximity > 0.)
	{
		bool inProximity = false;
		if(proximityCenter || !proximityCenters.IsEmpty())
		{
			for(const StellarObject &proximityObject : system->Objects())
				if(proximityObject.HasValidPlanet() && (proximityCenter == proximityObject.GetPlanet() ||
					proximityCenters.Matches(proximityObject.GetPlanet())))
				{
					double dist = flagship->Position().Distance(proximityObject.Position());
					if((closeTo && dist <= proximity) || (!closeTo && dist >= proximity))
					{
						inProximity = true;
						break;
					}
				}
		}
		if(!inProximity)
		{
			ResetOn(Timer::ResetCondition::LEAVE_ZONE, player, ui, mission);
			return;
		}
	}
	isActive = true;
	if(++timeElapsed >= timeToWait)
	{
		auto it = actions.find(TIMEUP)
		if(it != actions.end())
			it->second.Do(player, ui, &mission);
		isComplete = true;
	}
}
