/* MissionHaulerObjective.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MissionHaulerObjective.h"

#include "GameData.h"
#include "Random.h"

#include <cmath>
#include <string>
#include <vector>

// Construct and Load() at the same time.
MissionHaulerObjective::MissionHaulerObjective(const DataNode &node, const int offset)
{
	Load(node, offset);
}

// Load a mission, either from the game data or from a saved game.
void MissionHaulerObjective::Load(const DataNode &node, const int offset)
{
	id = node.Token(1 + offset);
	count = node.Value(2 + offset);
	if(node.Size() >= (4 + offset))
		limit = node.Value(3 + offset);
	if(node.Size() >= (5 + offset))
		probability = node.Value(4 + offset);
}

int MissionHaulerObjective::RealizeCount() const
{
	if(probability)
		return Random::Polya(limit, probability) + count;
	else if(limit > count)
		return count + Random::Int(limit - count + 1);
	else
		return count;
}

bool MissionHaulerObjective::CanBeRealized() const
{
	return !id.empty() && count > 0;
}



MissionCargoObjective::MissionCargoObjective(const DataNode &node, const int offset):
	MissionHaulerObjective(node, offset) {};

// Pick a random commodity that would make sense to be exported from the
// first system to the second.
const Trade::Commodity *MissionCargoObjective::PickCommodity(const System &from, const System &to)
{
	std::vector<int> weight;
	int total = 0;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		// For every 100 credits in profit you can make, double the chance
		// of this commodity being chosen.
		double profit = to.Trade(commodity.name) - from.Trade(commodity.name);
		int w = std::max<int>(1, 100. * pow(2., profit * .01));
		weight.push_back(w);
		total += w;
	}
	total += !total;
	// Pick a random commodity based on those weights.
	int r = Random::Int(total);
	for(unsigned i = 0; i < weight.size(); ++i)
	{
		r -= weight[i];
		if(r < 0)
			return &GameData::Commodities()[i];
	}
	// Control will never reach here, but to satisfy the compiler:
	return nullptr;
}

std::string MissionCargoObjective::RealizeCargo(const System &from, const System &to) const
{
	const Trade::Commodity *commodity = nullptr;
	if(id == "random")
		commodity = PickCommodity(from, to);
	else
	{
		for(const Trade::Commodity &option : GameData::Commodities())
			if(option.name == id)
			{
				commodity = &option;
				break;
			}
		for(const Trade::Commodity &option : GameData::SpecialCommodities())
			if(option.name == id)
			{
				commodity = &option;
				break;
			}
	}
	if(commodity)
		return commodity->items[Random::Int(commodity->items.size())];
	else
		return id;
}



MissionOutfitObjective::MissionOutfitObjective(const DataNode &node, const int offset):
	MissionHaulerObjective(node, offset) {};

const Outfit *MissionOutfitObjective::RealizeOutfit() const
{
	return GameData::Outfits().Get(id);
}

bool MissionOutfitObjective::CanBeRealized() const
{
	return MissionHaulerObjective::CanBeRealized() && GameData::Outfits().Has(id);
}



MissionOutfitterObjective::MissionOutfitterObjective(const DataNode &node, const int offset):
	MissionHaulerObjective(node, offset) {};

const Outfit *MissionOutfitterObjective::RealizeOutfit() const
{
	return GameData::Outfitters().Get(id)->Sample();
}


bool MissionOutfitterObjective::CanBeRealized() const
{
	return MissionHaulerObjective::CanBeRealized() && GameData::Outfitters().Has(id);
}
