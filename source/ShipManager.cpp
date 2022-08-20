/* ShipManager.cpp
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameData.h"
#include "ShipManager.h"

#include "PlayerInfo.h"

#include <cstdlib>

using namespace std;



ShipManager::ShipManager(string name, int count, bool unconstrained, bool withOutfits)
	: name(name), count(count), unconstrained(unconstrained), withOutfits(withOutfits)
{
}



vector<shared_ptr<Ship>> ShipManager::SatisfyingShips(const PlayerInfo &player, const Ship *model) const
{
	const System *here = player.GetSystem();
	const auto &id = player.GiftedShips().find(model->VariantName() + " " + name);
	bool hasName = !name.empty();
	bool foundShip = id != player.GiftedShips().end();
	vector<shared_ptr<Ship>> toSell;

	for(const auto &ship : player.Ships())
		if((ship->ModelName() == model->ModelName())
			&& (unconstrained || (ship->GetSystem() == here && !ship->IsDisabled() && !ship->IsParked()))
			&& (!hasName || (foundShip && ship->UUID() == id->second)))
		{
			// If a variant has been specified this ship most have each outfit specified in that variant definition.
			// The same applies if the outfits are required.
			if(model->VariantName() != model->ModelName() || withOutfits)
				for(const auto &it : model->Outfits())
				{
					const auto &outfit = ship->Outfits().find(it.first);
					int amountEquipped = (outfit != ship->Outfits().end() ? outfit->second : 0);
					if(it.second > amountEquipped)
						continue;
				}

			toSell.emplace_back(ship);

			// We do not want any more ships than is specified.
			if(static_cast<int>(toSell.size()) == abs(count))
				break;
		}

	return toSell;
}



bool ShipManager::Satisfies(const PlayerInfo &player, const Ship *model) const
{
	return static_cast<int>(SatisfyingShips(player, model).size()) == abs(count);
}



const string &ShipManager::Name() const
{
	return name;
}



int ShipManager::Count() const
{
	return count;
}



bool ShipManager::Unconstrained() const
{
	return unconstrained;
}



bool ShipManager::WithOutfits() const
{
	return withOutfits;
}



void ShipManager::Load(const DataNode &child, map<const Ship *, ShipManager> shipsList)
{
	bool taking = child.Token(0) == "take";
	bool owns = child.Token(0) == "owns";
	const Ship *ship = GameData::Ships().Get(child.Token(2));
	string name = (child.Size() >= 4 ? child.Token(3) : "");
	int count = (child.Size() >= 5 ? static_cast<int>(child.Value(4)) : 1);

	bool unconstrained = ((child.Size() >= 6 && child.Token(5) == "unconstrained") ||
			(child.Size() >= 7 && child.Token(6) == "unconstrained"));
	bool withOutfits = owns ? ((child.Size() >= 6 && child.Token(5) == "with outfits") ||
			(child.Size() >= 7 && child.Token(6) == "with outfits")) : false;

	if(count <= 0)
		child.PrintTrace("Error: Skipping invalid ship quantity:" + to_string(count));
	else if(taking && !name.empty() && count > 1)
		child.PrintTrace("Error: Skipping invalid ship quantity with a specified name:");
	else
		shipsList.emplace(ship, (name, count * (taking ? -1 : 1), unconstrained, withOutfits));
}
