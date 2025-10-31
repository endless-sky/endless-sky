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
#include "DataWriter.h"
#include "image/SpriteSet.h"

#include <algorithm>

using namespace std;



void Gamerules::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	else
		name = "Default";

	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
		{
			child.PrintTrace("Skipping gamerule with no value:");
			continue;
		}

		const string &key = child.Token(0);

		if(key == "description")
			description = child.Token(1);
		else if(key == "thumbnail")
			thumbnail = SpriteSet::Get(child.Token(1));
		else if(key == "lock gamerules")
			lockGamerules = child.BoolValue(1);
		else if(key == "universal ramscoop")
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



void Gamerules::Save(DataWriter &out, const Gamerules &preset) const
{
	out.Write("gamerules", name);
	out.BeginChild();
	{
		if(lockGamerules != preset.lockGamerules)
			out.Write("lock gamerules", lockGamerules ? 1 : 0);
		if(universalRamscoop != preset.universalRamscoop)
			out.Write("universal ramscoop", universalRamscoop ? 1 : 0);
		if(personSpawnPeriod != preset.personSpawnPeriod)
			out.Write("person spawn period", personSpawnPeriod);
		if(noPersonSpawnWeight != preset.noPersonSpawnWeight)
			out.Write("no person spawn weight", noPersonSpawnWeight);
		if(npcMaxMiningTime != preset.npcMaxMiningTime)
			out.Write("npc max mining time", npcMaxMiningTime);
		if(universalFrugalThreshold != preset.universalFrugalThreshold)
			out.Write("universal frugal threshold", universalFrugalThreshold);
		if(depreciationMin != preset.depreciationMin)
			out.Write("depreciation min", depreciationMin);
		if(depreciationDaily != preset.depreciationDaily)
			out.Write("depreciation daily", depreciationDaily);
		if(depreciationGracePeriod != preset.depreciationGracePeriod)
			out.Write("depreciation grace period", depreciationGracePeriod);
		if(depreciationMaxAge != preset.depreciationMaxAge)
			out.Write("depreciation max age", depreciationMaxAge);
		if(fighterHitPolicy != preset.fighterHitPolicy)
		{
			if(fighterHitPolicy == FighterDodgePolicy::ALL)
				out.Write("disabled fighters avoid projectiles", "all");
			else if(fighterHitPolicy == FighterDodgePolicy::ONLY_PLAYER)
				out.Write("disabled fighters avoid projectiles", "only player");
			else
				out.Write("disabled fighters avoid projectiles", "none");
		}
		if(systemDepartureMin != preset.systemDepartureMin)
			out.Write("system departure min", systemDepartureMin);
		if(systemArrivalMin != preset.systemArrivalMin)
		{
			if(systemArrivalMin.has_value())
				out.Write("system arrival min", *systemArrivalMin);
			else
				out.Write("system arrival min", "unset");
		}
		if(fleetMultiplier != preset.fleetMultiplier)
			out.Write("fleet multiplier", fleetMultiplier);

		const map<std::string, int> &otherMiscRules = preset.miscRules;
		for(const auto &[rule, value] : miscRules)
		{
			auto sit = otherMiscRules.find(rule);
			if(sit == otherMiscRules.end() || sit->second != value)
				out.Write(rule, value);
		}
	}
	out.EndChild();
}



void Gamerules::Replace(const Gamerules &rules)
{
	name = rules.name;
	lockGamerules = rules.lockGamerules;
	universalRamscoop = rules.universalRamscoop;
	personSpawnPeriod = rules.personSpawnPeriod;
	noPersonSpawnWeight = rules.noPersonSpawnWeight;
	npcMaxMiningTime = rules.npcMaxMiningTime;
	universalFrugalThreshold = rules.universalFrugalThreshold;
	depreciationMin = rules.depreciationMin;
	depreciationDaily = rules.depreciationDaily;
	depreciationGracePeriod = rules.depreciationGracePeriod;
	depreciationMaxAge = rules.depreciationMaxAge;
	fighterHitPolicy = rules.fighterHitPolicy;
	systemDepartureMin = rules.systemDepartureMin;
	systemArrivalMin = rules.systemArrivalMin;
	fleetMultiplier = rules.fleetMultiplier;
	miscRules = rules.miscRules;
}



void Gamerules::Reset(const string &rule, const Gamerules &preset)
{
	if(rule == "lock gamerules")
		lockGamerules = preset.lockGamerules;
	else if(rule == "universal ramscoop")
		universalRamscoop = preset.universalRamscoop;
	else if(rule == "person spawn period")
		personSpawnPeriod = preset.personSpawnPeriod;
	else if(rule == "no person spawn weight")
		noPersonSpawnWeight = preset.noPersonSpawnWeight;
	else if(rule == "npc max mining time")
		npcMaxMiningTime = preset.npcMaxMiningTime;
	else if(rule == "universal frugal threshold")
		universalFrugalThreshold = preset.universalFrugalThreshold;
	else if(rule == "depreciation min")
		depreciationMin = preset.depreciationMin;
	else if(rule == "depreciation daily")
		depreciationDaily = preset.depreciationDaily;
	else if(rule == "depreciation grace period")
		depreciationGracePeriod = preset.depreciationGracePeriod;
	else if(rule == "depreciation max age")
		depreciationMaxAge = preset.depreciationMaxAge;
	else if(rule == "disabled fighters avoid projectiles")
		fighterHitPolicy = preset.fighterHitPolicy;
	else if(rule == "system departure min")
		systemDepartureMin = preset.systemDepartureMin;
	else if(rule == "system arrival min")
	{
		if(systemArrivalMin == preset.systemArrivalMin)
			systemArrivalMin.reset();
		else
			systemArrivalMin = preset.systemArrivalMin;
	}
	else if(rule == "fleet multiplier")
		fleetMultiplier = preset.fleetMultiplier;
	else
	{
		auto it = preset.miscRules.find(rule);
		if(it != preset.miscRules.end())
			miscRules[rule] = it->second;
	}
}



const string &Gamerules::Name() const
{
	return name;
}



const string &Gamerules::Description() const
{
	return description;
}



const Sprite *Gamerules::Thumbnail() const
{
	return thumbnail;
}



void Gamerules::SetLockGamerules(bool value)
{
	lockGamerules = value;
}



void Gamerules::SetUniversalRamscoopActive(bool value)
{
	universalRamscoop = value;
}



void Gamerules::SetPersonSpawnPeriod(int value)
{
	personSpawnPeriod = max(1, value);
}



void Gamerules::SetNoPersonSpawnWeight(int value)
{
	noPersonSpawnWeight = max(0, value);
}



void Gamerules::SetNPCMaxMiningTime(int value)
{
	npcMaxMiningTime = max(0, value);
}



void Gamerules::SetUniversalFrugalThreshold(double value)
{
	universalFrugalThreshold = min(1., max(0., value));
}



void Gamerules::SetDepreciationMin(double value)
{
	depreciationMin = min(1., max(0., value));
}



void Gamerules::SetDepreciationDaily(double value)
{
	depreciationDaily = min(1., max(0., value));
}



void Gamerules::SetDepreciationGracePeriod(int value)
{
	depreciationGracePeriod = max(0, value);
}



void Gamerules::SetDepreciationMaxAge(int value)
{
	depreciationMaxAge = max(0, value);
}



void Gamerules::SetFighterDodgePolicy(FighterDodgePolicy value)
{
	fighterHitPolicy = value;
}



void Gamerules::SetSystemDepartureMin(double value)
{
	systemDepartureMin = max(0., value);
}



void Gamerules::SetSystemArrivalMin(double value)
{
	systemArrivalMin = value;
}



void Gamerules::SetFleetMultiplier(double value)
{
	fleetMultiplier = max(0., value);
}



void Gamerules::SetMiscValue(const string &rule, int value)
{
	miscRules[rule] = value;
}



int Gamerules::GetValue(const string &rule) const
{
	if(rule == "lock gamerules")
		return lockGamerules;
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



bool Gamerules::LockGamerules() const
{
	return lockGamerules;
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
