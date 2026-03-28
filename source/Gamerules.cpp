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
			storage.lockGamerules = child.BoolValue(1);
		else if(key == "universal ramscoop")
			storage.universalRamscoop = child.BoolValue(1);
		else if(key == "person spawn period")
			storage.personSpawnPeriod = max<int>(1, child.Value(1));
		else if(key == "no person spawn weight")
			storage.noPersonSpawnWeight = max<int>(0, child.Value(1));
		else if(key == "npc max mining time")
			storage.npcMaxMiningTime = max<int>(0, child.Value(1));
		else if(key == "universal frugal threshold")
			storage.universalFrugalThreshold = min<double>(1., max<double>(0., child.Value(1)));
		else if(key == "depreciation min")
			storage.depreciationMin = min<double>(1., max<double>(0., child.Value(1)));
		else if(key == "depreciation daily")
			storage.depreciationDaily = min<double>(1., max<double>(0., child.Value(1)));
		else if(key == "depreciation grace period")
			storage.depreciationGracePeriod = max<int>(0, child.Value(1));
		else if(key == "depreciation max age")
			storage.depreciationMaxAge = max<int>(0, child.Value(1));
		else if(key == "disabled fighters avoid projectiles")
		{
			const string &value = child.Token(1);
			if(value == "all")
				storage.fighterHitPolicy = FighterDodgePolicy::ALL;
			else if(value == "none")
				storage.fighterHitPolicy = FighterDodgePolicy::NONE;
			else if(value == "only player")
				storage.fighterHitPolicy = FighterDodgePolicy::ONLY_PLAYER;
			else
				child.PrintTrace("Skipping unrecognized value for gamerule:");
		}
		else if(key == "system departure min")
			storage.systemDepartureMin = max<double>(0., child.Value(1));
		else if(key == "system arrival min")
		{
			if(child.Token(1) == "unset")
				storage.systemArrivalMin.reset();
			else
				storage.systemArrivalMin = child.Value(1);
		}
		else if(key == "fleet multiplier")
			storage.fleetMultiplier = max<double>(0., child.Value(1));
		else
			storage.miscRules[key] = child.IsNumber(1) ? child.Value(1) : child.BoolValue(1);
	}
}



void Gamerules::Save(DataWriter &out, const Gamerules &preset) const
{
	out.Write("gamerules", name);
	out.BeginChild();
	{
		if(storage.lockGamerules != preset.storage.lockGamerules)
			out.Write("lock gamerules", storage.lockGamerules ? 1 : 0);
		if(storage.universalRamscoop != preset.storage.universalRamscoop)
			out.Write("universal ramscoop", storage.universalRamscoop ? 1 : 0);
		if(storage.personSpawnPeriod != preset.storage.personSpawnPeriod)
			out.Write("person spawn period", storage.personSpawnPeriod);
		if(storage.noPersonSpawnWeight != preset.storage.noPersonSpawnWeight)
			out.Write("no person spawn weight", storage.noPersonSpawnWeight);
		if(storage.npcMaxMiningTime != preset.storage.npcMaxMiningTime)
			out.Write("npc max mining time", storage.npcMaxMiningTime);
		if(storage.universalFrugalThreshold != preset.storage.universalFrugalThreshold)
			out.Write("universal frugal threshold", storage.universalFrugalThreshold);
		if(storage.depreciationMin != preset.storage.depreciationMin)
			out.Write("depreciation min", storage.depreciationMin);
		if(storage.depreciationDaily != preset.storage.depreciationDaily)
			out.Write("depreciation daily", storage.depreciationDaily);
		if(storage.depreciationGracePeriod != preset.storage.depreciationGracePeriod)
			out.Write("depreciation grace period", storage.depreciationGracePeriod);
		if(storage.depreciationMaxAge != preset.storage.depreciationMaxAge)
			out.Write("depreciation max age", storage.depreciationMaxAge);
		if(storage.fighterHitPolicy != preset.storage.fighterHitPolicy)
		{
			if(storage.fighterHitPolicy == FighterDodgePolicy::ALL)
				out.Write("disabled fighters avoid projectiles", "all");
			else if(storage.fighterHitPolicy == FighterDodgePolicy::ONLY_PLAYER)
				out.Write("disabled fighters avoid projectiles", "only player");
			else
				out.Write("disabled fighters avoid projectiles", "none");
		}
		if(storage.systemDepartureMin != preset.storage.systemDepartureMin)
			out.Write("system departure min", storage.systemDepartureMin);
		if(storage.systemArrivalMin != preset.storage.systemArrivalMin)
		{
			if(storage.systemArrivalMin.has_value())
				out.Write("system arrival min", *storage.systemArrivalMin);
			else
				out.Write("system arrival min", "unset");
		}
		if(storage.fleetMultiplier != preset.storage.fleetMultiplier)
			out.Write("fleet multiplier", storage.fleetMultiplier);

		const map<string, int> &otherMiscRules = preset.storage.miscRules;
		for(const auto &[rule, value] : storage.miscRules)
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
	storage = rules.storage;
}



void Gamerules::Reset(const string &rule, const Gamerules &preset)
{
	if(rule == "lock gamerules")
		storage.lockGamerules = preset.storage.lockGamerules;
	else if(rule == "universal ramscoop")
		storage.universalRamscoop = preset.storage.universalRamscoop;
	else if(rule == "person spawn period")
		storage.personSpawnPeriod = preset.storage.personSpawnPeriod;
	else if(rule == "no person spawn weight")
		storage.noPersonSpawnWeight = preset.storage.noPersonSpawnWeight;
	else if(rule == "npc max mining time")
		storage.npcMaxMiningTime = preset.storage.npcMaxMiningTime;
	else if(rule == "universal frugal threshold")
		storage.universalFrugalThreshold = preset.storage.universalFrugalThreshold;
	else if(rule == "depreciation min")
		storage.depreciationMin = preset.storage.depreciationMin;
	else if(rule == "depreciation daily")
		storage.depreciationDaily = preset.storage.depreciationDaily;
	else if(rule == "depreciation grace period")
		storage.depreciationGracePeriod = preset.storage.depreciationGracePeriod;
	else if(rule == "depreciation max age")
		storage.depreciationMaxAge = preset.storage.depreciationMaxAge;
	else if(rule == "disabled fighters avoid projectiles")
		storage.fighterHitPolicy = preset.storage.fighterHitPolicy;
	else if(rule == "system departure min")
		storage.systemDepartureMin = preset.storage.systemDepartureMin;
	else if(rule == "system arrival min")
		storage.systemArrivalMin = preset.storage.systemArrivalMin;
	else if(rule == "fleet multiplier")
		storage.fleetMultiplier = preset.storage.fleetMultiplier;
	else
	{
		auto it = preset.storage.miscRules.find(rule);
		if(it != preset.storage.miscRules.end())
			storage.miscRules[rule] = it->second;
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
	storage.lockGamerules = value;
}



void Gamerules::SetUniversalRamscoopActive(bool value)
{
	storage.universalRamscoop = value;
}



void Gamerules::SetPersonSpawnPeriod(int value)
{
	storage.personSpawnPeriod = max(1, value);
}



void Gamerules::SetNoPersonSpawnWeight(int value)
{
	storage.noPersonSpawnWeight = max(0, value);
}



void Gamerules::SetNPCMaxMiningTime(int value)
{
	storage.npcMaxMiningTime = max(0, value);
}



void Gamerules::SetUniversalFrugalThreshold(double value)
{
	storage.universalFrugalThreshold = clamp(value, 0., 1.);
}



void Gamerules::SetDepreciationMin(double value)
{
	storage.depreciationMin = clamp(value, 0., 1.);
}



void Gamerules::SetDepreciationDaily(double value)
{
	storage.depreciationDaily = clamp(value, 0., 1.);
}



void Gamerules::SetDepreciationGracePeriod(int value)
{
	storage.depreciationGracePeriod = max(0, value);
}



void Gamerules::SetDepreciationMaxAge(int value)
{
	storage.depreciationMaxAge = max(0, value);
}



void Gamerules::SetFighterDodgePolicy(FighterDodgePolicy value)
{
	storage.fighterHitPolicy = value;
}



void Gamerules::SetSystemDepartureMin(double value)
{
	storage.systemDepartureMin = max(0., value);
}



void Gamerules::SetSystemArrivalMin(std::optional<double> value)
{
	storage.systemArrivalMin = value;
}



void Gamerules::SetFleetMultiplier(double value)
{
	storage.fleetMultiplier = max(0., value);
}



void Gamerules::SetMiscValue(const string &rule, int value)
{
	storage.miscRules[rule] = value;
}



int Gamerules::GetValue(const string &rule) const
{
	if(rule == "lock gamerules")
		return storage.lockGamerules;
	if(rule == "universal ramscoop")
		return storage.universalRamscoop;
	if(rule == "person spawn period")
		return storage.personSpawnPeriod;
	if(rule == "no person spawn weight")
		return storage.noPersonSpawnWeight;
	if(rule == "npc max mining time")
		return storage.npcMaxMiningTime;
	if(rule == "universal frugal threshold")
		return storage.universalFrugalThreshold * 1000;
	if(rule == "depreciation min")
		return storage.depreciationMin * 1000;
	if(rule == "depreciation daily")
		return storage.depreciationDaily * 1000;
	if(rule == "depreciation grace period")
		return storage.depreciationGracePeriod;
	if(rule == "depreciation max age")
		return storage.depreciationMaxAge;
	if(rule == "disabled fighters avoid projectiles")
		return static_cast<int>(storage.fighterHitPolicy);
	if(rule == "system departure min")
		return storage.systemDepartureMin * 1000;
	if(rule == "system arrival min")
		return storage.systemArrivalMin.value_or(0.) * 1000;
	if(rule == "fleet multiplier")
		return storage.fleetMultiplier * 1000;

	auto it = storage.miscRules.find(rule);
	if(it == storage.miscRules.end())
		return 0;
	return it->second;
}



bool Gamerules::LockGamerules() const
{
	return storage.lockGamerules;
}



bool Gamerules::UniversalRamscoopActive() const
{
	return storage.universalRamscoop;
}



int Gamerules::PersonSpawnPeriod() const
{
	return storage.personSpawnPeriod;
}



int Gamerules::NoPersonSpawnWeight() const
{
	return storage.noPersonSpawnWeight;
}



int Gamerules::NPCMaxMiningTime() const
{
	return storage.npcMaxMiningTime;
}



double Gamerules::UniversalFrugalThreshold() const
{
	return storage.universalFrugalThreshold;
}



double Gamerules::DepreciationMin() const
{
	return storage.depreciationMin;
}



double Gamerules::DepreciationDaily() const
{
	return storage.depreciationDaily;
}



int Gamerules::DepreciationGracePeriod() const
{
	return storage.depreciationGracePeriod;
}



int Gamerules::DepreciationMaxAge() const
{
	return storage.depreciationMaxAge;
}



Gamerules::FighterDodgePolicy Gamerules::FightersHitWhenDisabled() const
{
	return storage.fighterHitPolicy;
}



double Gamerules::SystemDepartureMin() const
{
	return storage.systemDepartureMin;
}



optional<double> Gamerules::SystemArrivalMin() const
{
	return storage.systemArrivalMin;
}



double Gamerules::FleetMultiplier() const
{
	return storage.fleetMultiplier;
}



bool Gamerules::operator==(const Gamerules &other) const
{
	return name == other.name && storage == other.storage;
}
