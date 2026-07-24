/* ShipFactory.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipFactory.h"

#include "../DataNode.h"
#include "../GameData.h"

#include <algorithm>
#include <ranges>

using namespace std;



void ShipFactory::Load(const DataNode &node, const ConditionsStore *playerConditions)
{
	bool add = node.Token(0) == "add";
	if(node.HasChildren())
	{
		// Loading a full ship definition.
		ships[shipOrder] = make_shared<Ship>(node, playerConditions);
		if(node.Size() >= 3 + add)
			ships[shipOrder]->SetGivenName(node.Token(2 + add));
	}
	else
	{
		// Loading a ship managed by GameData, i.e. "base models" and variants.
		stockShips[shipOrder] = GameData::Ships().Get(node.Token(1));
		shipNames.push_back(node.Token(1 + (node.Size() > 2)));
	}
	++shipOrder;
}



void ShipFactory::FinishLoading()
{
	for(const auto &ship : ships | views::values)
		ship->FinishLoading(true);
}



bool ShipFactory::IsValid() const
{
	if(ranges::any_of(ships, [](const pair<int, shared_ptr<Ship>> &it) noexcept -> bool {
		return !it.second->IsValid();
	}))
		return false;
	if(ranges::any_of(stockShips, [](const pair<int, const Ship *> &it) noexcept -> bool {
		return !it.second->IsValid();
	}))
		return false;
	return true;
}



void ShipFactory::RemoveModel(const string &shipModel)
{
	map<int, shared_ptr<Ship>> newShips;
	map<int, const Ship *> newStockShips;
	list<string> newShipNames;

	int newOrder = 0;
	auto shipIt = ships.begin();
	auto stockIt = stockShips.begin();
	auto nameIt = shipNames.begin();
	for(int i = 0; i < shipOrder; ++i)
	{
		if(shipIt != ships.end() && shipIt->first == i)
		{
			if(shipIt->second->TrueModelName() != shipModel)
				newShips[newOrder++] = shipIt->second;
			++shipIt;
		}
		else
		{
			if(stockIt->second->TrueModelName() != shipModel)
			{
				newStockShips[newOrder++] = stockIt->second;
				newShipNames.push_back(*nameIt);
			}
			++stockIt;
			++nameIt;
		}
	}
	shipOrder = newOrder;
	ships = newShips;
	stockShips = newStockShips;
	shipNames = newShipNames;
}



void ShipFactory::Clear()
{
	shipOrder = 0;
	ships.clear();
	stockShips.clear();
	shipNames.clear();
}



void ShipFactory::Instantiate(list<shared_ptr<Ship>> &container, const map<string, string> *subs) const
{
	InstantiateContainer(container, subs);
}



void ShipFactory::Instantiate(vector<shared_ptr<Ship>> &container, const map<string, string> *subs) const
{
	InstantiateContainer(container, subs);
}
