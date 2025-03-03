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
#include "Preferences.h"

#include <algorithm>
#include <optional>

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
		else
			child.PrintTrace("Skipping unrecognized gamerule:");
	}
}



bool Gamerules::UniversalRamscoopActive() const
{
	return Preferences::GetGameruleRamscoop().value_or(universalRamscoop);
}



int Gamerules::PersonSpawnPeriod() const
{
	return Preferences::GetGamerulePersonPeriod().value_or(personSpawnPeriod);
}



int Gamerules::NoPersonSpawnWeight() const
{
	return Preferences::GetGamerulePersonWeight().value_or(noPersonSpawnWeight);
}



int Gamerules::NPCMaxMiningTime() const
{
	return Preferences::GetGameruleMiningTime().value_or(npcMaxMiningTime);
}



double Gamerules::UniversalFrugalThreshold() const
{
	return Preferences::GetGameruleFrugalThreshold().value_or(universalFrugalThreshold);
}



double Gamerules::DepreciationMin() const
{
	return Preferences::GetGameruleDepreciationMin().value_or(depreciationMin);
}



double Gamerules::DepreciationDaily() const
{
	return Preferences::GetGameruleDepreciationDaily().value_or(depreciationDaily);
}



int Gamerules::DepreciationGracePeriod() const
{
	return Preferences::GetGameruleDepreciationGracePeriod().value_or(depreciationGracePeriod);
}



int Gamerules::DepreciationMaxAge() const
{
	return Preferences::GetGameruleDepreciationMaxAge().value_or(depreciationMaxAge);
}



Gamerules::FighterDodgePolicy Gamerules::FightersHitWhenDisabled() const
{
	return Preferences::GetGameruleFighters().value_or(fighterHitPolicy);
}
