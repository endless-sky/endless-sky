/* ShipManager.cpp
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipManager.h"

#include "DataNode.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "Ship.h"

#include <cstdlib>

using namespace std;



void ShipManager::Load(const DataNode &node)
{
	const string token = node.Token(0);
	if(node.Size() < 3 || node.Token(1) != "ship")
	{
		node.PrintTrace("Error: Skipping unsupported \"" + token + "\" syntax:");
		return;
	}
	bool taking = token == "take";
	if(node.Size() >= 4)
		name = node.Token(3);

	for(const DataNode &child : node)
	{
		const string key = child.Token(0);
		if(taking)
		{
			if(key == "unconstrained")
				unconstrained = true;
			else if(key == "with outfits")
				withOutfits = true;
			else
				node.PrintTrace("Error: Skipping unrecognized take ship node argument:");
		}
		else if(child.Size() < 2)
			child.PrintTrace("Error: Expected a value argument:");
		else if(key == "id")
			id = child.Token(1);
		else if(key == "amount")
		{
			if(child.Value(1) <= 0)
				node.PrintTrace("Error: Skipping invalid negative ship quantity:" + node.Token(1));
			else
				count = child.Value(1) * (taking ? -1 : 1);
		}
		else
			node.PrintTrace("Error: Skipping unrecognized ship " + token + " node argument:");
	}

	if(taking && !id.empty() && count != 1)
		node.PrintTrace("Error: Invalid ship quantity with a specified unique id:");
}



vector<shared_ptr<Ship>> ShipManager::SatisfyingShips(const PlayerInfo &player, const Ship *model) const
{
	const System *here = player.GetSystem();
	const auto &shipID = player.GiftedShips().find(id);
	bool foundShip = shipID != player.GiftedShips().end();
	vector<shared_ptr<Ship>> toSell;

	for(const auto &ship : player.Ships())
		if((ship->ModelName() == model->ModelName())
			&& (unconstrained || (ship->GetSystem() == here && !ship->IsDisabled() && !ship->IsParked()))
			&& (id.empty() || (foundShip && ship->UUID() == shipID->second))
			&& (name.empty() || name == ship->Name()))
		{
			// If a variant has been specified, or the keyword "with outfits" is specified,
			// this ship must have each outfit specified in that variant definition.
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
			if(static_cast<int>(toSell.size()) >= abs(count))
				break;
		}

	return toSell;
}



bool ShipManager::CanBeDone(const PlayerInfo &player, const Ship *model) const
{
	// If we are giving ships this is always satisfied.
	return count > 0 || static_cast<int>(SatisfyingShips(player, model).size()) == abs(count);
}



const string &ShipManager::Name() const
{
	return name;
}



const string &ShipManager::Id() const
{
	return id;
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
