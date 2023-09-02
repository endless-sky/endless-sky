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

Timer::Timer(const DataNode &node, Mission *mission)
{
	Load(node, mission);
}

void Timer::Load(const DataNode &node, Mission *mission)
{
	if(node.Size() > 1)
		name = node.Token(1);
	if(node.Size() > 2)
		base = static_cast<int64_t>(node.Value(2));
	if(node.Size() > 3)
		rand = static_cast<uint32_t>(node.Value(3));

	this->mission = mission;

	timeToWait = base + Random::Int(rand);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "idle")
			requireIdle = true;
		else if(child.Token(0) == "system" && child.Size() > 1)
			system = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "systems")
			systems.Load(child);
		else if(child.Token(0) == "proximity")
		{
			if(child.Size() > 1)
				proximity = child.Value(1);
			if(child.Size() > 2)
			{
				if(child.Token(2) == "close")
					closeTo = true;
				else if(child.Token(2) == "far")
					closeTo = false;
			}
			if(child.HasChildren())
			{
				const DataNode &firstGrand = (*child.begin());
				proximityCenter = GameData::Planets().Get(firstGrand.Token(0));
			}
		}
		else if(child.Token(0) == "uncloaked")
			requireUncloaked = true;
		else if(child.Token(0) == "reset")
		{
			if(child.Size() > 1 && child.Token(1) == "leave system")
				resetCondition = ResetCondition::LEAVE_SYSTEM;
			else if(child.Size() > 1 && child.Token(1) == "leave zone")
				resetCondition = ResetCondition::LEAVE_ZONE;
			else if(child.Size() > 1 && child.Token(1) == "none")
				resetCondition = ResetCondition::NONE;
			else if(child.Size() > 1 && child.Token(1) == "pause")
				resetCondition = ResetCondition::PAUSE;
		}
		// We keep "on timeup" as separate tokens so that it's compatible with MissionAction syntax
		else if(child.Token(0) == "on" && child.Size() > 1 && child.Token(1) == "timeup")
			action.Load(child);

	}
}

// Note: the Save() function can assume this is an instantiated Timer, not a template,
// so the time to wait will be saved fully calculated, and with any elapsed time subtracted
void Timer::Save(DataWriter &out) const
{
	// If this Timer should no longer appear in-game, don't serialize it.
	if(isComplete)
		return;

	int64_t timeRemaining = timeToWait - timeElapsed;
	out.Write("timer", name, timeRemaining);
	out.BeginChild();
	{
		if(system)
			out.Write("system", system->Name());
		else if(!systems.IsEmpty())
		{
			out.Write("systems");
			systems.Save(out);
		}
		if(requireIdle)
			out.Write("idle");
		if(requireUncloaked)
			out.Write("uncloaked");
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
			out.Write("proximity", proximity, distance);
			if(proximityCenter != nullptr)
			{
				out.BeginChild();
				{
					out.Write(proximityCenter->Name());
				}
				out.EndChild();
			}
		}

		action.Save(out);
	}
	out.EndChild();
}

// Calculate the total time to wait, including any random value,
// and instantiate the triggered action
Timer Timer::Instantiate(map<string, string> &subs,
						const System *origin, int jumps, int64_t payload) const
{
	Timer result;
	result.name = name;
	result.base = base;
	result.rand = rand;
	result.requireIdle = requireIdle;
	result.requireUncloaked = requireUncloaked;
	result.system = system;
	result.systems = systems;
	result.proximity = proximity;
	result.proximityCenter = proximityCenter;
	result.closeTo = closeTo;
	result.resetCondition = resetCondition;

	result.timeToWait = timeToWait;
	result.timeElapsed = timeElapsed;
	result.isComplete = isComplete;
	result.isActive = isActive;

	result.action = action.Instantiate(subs, origin, jumps, payload);

	return result;
}

bool Timer::IsComplete() const
{
	return isComplete;
}

void Timer::ResetOn(ResetCondition cond)
{
	bool reset = cond == resetCondition;
	reset |= (cond == Timer::ResetCondition::LEAVE_ZONE && resetCondition == Timer::ResetCondition::PAUSE);
	reset |= (cond == Timer::ResetCondition::LEAVE_SYSTEM && (resetCondition == Timer::ResetCondition::PAUSE
				|| resetCondition == Timer::ResetCondition::LEAVE_ZONE));
	if(isActive && reset)
	{
		timeElapsed = 0;
		timeToWait = base + Random::Int(rand);
	}
}

void Timer::Step(PlayerInfo &player, UI *ui)
{
	if(isComplete)
		return;
	if((system && player.Flagship()->GetSystem() != system) ||
		(!systems.IsEmpty() && !systems.Matches(player.Flagship()->GetSystem())))
	{
		ResetOn(Timer::ResetCondition::LEAVE_SYSTEM);
		isActive = false;
		return;
	}
	if(requireIdle)
	{
		bool shipIdle = (!player.Flagship()->IsThrusting() && !player.Flagship()->IsSteering()
						&& !player.Flagship()->IsReversing());
		for(const Hardpoint &weapon : player.Flagship()->Weapons())
			shipIdle &= !weapon.WasFiring();
		if(!shipIdle)
		{
			ResetOn(Timer::ResetCondition::PAUSE);
			isActive = false;
			return;
		}
	}
	if(requireUncloaked)
	{
		double cloak = player.Flagship()->Cloaking();
		if(cloak != 0.)
		{
			ResetOn(Timer::ResetCondition::PAUSE);
			isActive = false;
			return;
		}
	}
	if(proximity > 0.)
	{
		Point center;
		if(proximityCenter)
		{
			const StellarObject *proximityObject = system->FindStellar(proximityCenter);
			center = proximityObject->Position();
		}
		double dist = (player.Flagship()->Position() - center).Length();
		if((closeTo && dist > proximity) || (!closeTo && dist < proximity))
		{
			ResetOn(Timer::ResetCondition::LEAVE_ZONE);
			isActive = false;
			return;
		}
	}
	isActive = true;
	timeElapsed += 1;
	if(timeElapsed >= timeToWait)
	{
		action.Do(player, ui, mission);
		player.Conditions().Add("timer: " + name + ": complete", 1);
		isComplete = true;
	}
}
