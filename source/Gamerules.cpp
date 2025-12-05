/* Gamerules.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Gamerules.h"

#include "DataNode.h"

#include <algorithm>

using namespace std;



// Load a gamerules node.
void Gamerules::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping gamerule with no value:");
			continue;
		}

		const string &key = child.Token(0);

		if(key == "universal ramscoop")
			universalRamscoop = child.BoolValue(1);
		else if(key == "person spawn period")
			personSpawnPeriod = max<int>(1, child.Value(1));
		else if(key == "no person spawn weight")
			noPersonSpawnWeight = max<int>(0, child.Value(1));
		else if(key == "npc max mining time")
			npcMaxMiningTime = max<int>(0, child.Value(1));
		else if(key == "universal frugal threshold")
			universalFrugalThreshold = min<double>(1., max<double>(0., child.Value(1)));
		else if(key == "depreciation min")
			depreciationMin = min<double>(1., max<double>(0., child.Value(1)));
		else if(key == "depreciation daily")
			depreciationDaily = min<double>(1., max<double>(0., child.Value(1)));
		else if(key == "depreciation grace period")
			depreciationGracePeriod = max<int>(0, child.Value(1));
		else if(key == "depreciation max age")
			depreciationMaxAge = max<int>(0, child.Value(1));
		else if(key == "disabled fighters avoid projectiles")
		{
			const string &value = child.Token(1);
			if(value == "all")
				fighterHitPolicy = FighterDodgePolicy::ALL;
			else if(value == "none")
				fighterHitPolicy = FighterDodgePolicy::NONE;
			else if(value == "only player")
				fighterHitPolicy = FighterDodgePolicy::ONLY_PLAYER;
			else
				child.PrintTrace("Skipping unrecognized value for gamerule:");
		}
		else if(key == "system departure min")
			systemDepartureMin = max<double>(0., child.Value(1));
		else if(key == "system arrival min")
		{
			if(child.Token(1) == "unset")
				systemArrivalMin.reset();
			else
				systemArrivalMin = child.Value(1);
		}
		else if(key == "fleet multiplier")
			fleetMultiplier = max<double>(0., child.Value(1));
		else
			miscRules[key] = child.IsNumber(1) ? child.Value(1) : child.BoolValue(1);
	}
}



int Gamerules::GetValue(const string &rule) const
{
	if(rule == "universal ramscoop")
		return universalRamscoop;
	if(rule == "person spawn period")
		return personSpawnPeriod;
	if(rule == "no person spawn weight")
		return noPersonSpawnWeight;
	if(rule == "npc max mining time")
		return npcMaxMiningTime;
	if(rule == "universal frugal threshold")
		return universalFrugalThreshold * 1000;
	if(rule == "depreciation min")
		return depreciationMin * 1000;
	if(rule == "depreciation daily")
		return depreciationDaily * 1000;
	if(rule == "depreciation grace period")
		return depreciationGracePeriod;
	if(rule == "depreciation max age")
		return depreciationMaxAge;
	if(rule == "disabled fighters avoid projectiles")
		return static_cast<int>(fighterHitPolicy);
	if(rule == "system departure min")
		return systemDepartureMin * 1000;
	if(rule == "system arrival min")
		return systemArrivalMin.value_or(0.) * 1000;
	if(rule == "fleet multiplier")
		return fleetMultiplier * 1000;

	auto it = miscRules.find(rule);
	if(it == miscRules.end())
		return 0;
	return it->second;
}



bool Gamerules::UniversalRamscoopActive() const
{
	return universalRamscoop;
}



int Gamerules::PersonSpawnPeriod() const
{
	return personSpawnPeriod;
}



int Gamerules::NoPersonSpawnWeight() const
{
	return noPersonSpawnWeight;
}



int Gamerules::NPCMaxMiningTime() const
{
	return npcMaxMiningTime;
}



double Gamerules::UniversalFrugalThreshold() const
{
	return universalFrugalThreshold;
}



double Gamerules::DepreciationMin() const
{
	return depreciationMin;
}



double Gamerules::DepreciationDaily() const
{
	return depreciationDaily;
}



int Gamerules::DepreciationGracePeriod() const
{
	return depreciationGracePeriod;
}



int Gamerules::DepreciationMaxAge() const
{
	return depreciationMaxAge;
}



Gamerules::FighterDodgePolicy Gamerules::FightersHitWhenDisabled() const
{
	return fighterHitPolicy;
}



double Gamerules::SystemDepartureMin() const
{
	return systemDepartureMin;
}



optional<double> Gamerules::SystemArrivalMin() const
{
	return systemArrivalMin;
}



double Gamerules::FleetMultiplier() const
{
	return fleetMultiplier;
}
