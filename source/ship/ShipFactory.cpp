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
	int startIndex = node.Token(0) == "add" ? 1 : 0;
	// A third token may be present to represent the given name of this ship.
	// Ships without a name given here will be given one later.
	const string &name = (node.Size() >= startIndex + 3) ? node.Token(startIndex + 2) : "";
	if(node.HasChildren())
	{
		shared_ptr<Ship> ship = make_shared<Ship>(node, playerConditions);
		// If this is a full ship definition, it should already have a given name instead
		// of the name being provided as a third token. Save the given name alongside
		// the definition so that we don't potentially replace it later.
		ships.emplace_back(ship, !ship->GivenName().empty() ? ship->GivenName() : name);
	}
	else
		ships.emplace_back(GameData::Ships().Get(node.Token(startIndex + 1)), name);
}



void ShipFactory::FinishLoading()
{
	for(const auto &ship : ships | views::keys)
		if(!ship.IsStock())
			ship.Mutable()->FinishLoading(true);
}



bool ShipFactory::IsValid() const
{
	return ranges::all_of(ships, [](const pair<ExclusiveItem<Ship>, string> &it) noexcept -> bool {
		return it.first->IsValid();
	});
}



void ShipFactory::RemoveModel(const string &shipModel)
{
	erase_if(ships, [&](const pair<ExclusiveItem<Ship>, string> &it) noexcept -> bool {
		return it.first->TrueModelName() == shipModel;
	});
}



void ShipFactory::Clear()
{
	ships.clear();
}



vector<const Ship *> ShipFactory::GetShips() const
{
	vector<const Ship *> shipPtrs;
	for(const auto &ship : ships | views::keys)
		shipPtrs.push_back(ship.Ptr());
	return shipPtrs;
}



void ShipFactory::Instantiate(list<shared_ptr<Ship>> &container, const map<string, string> *subs) const
{
	InstantiateContainer(container, subs);
}



void ShipFactory::Instantiate(vector<shared_ptr<Ship>> &container, const map<string, string> *subs) const
{
	InstantiateContainer(container, subs);
}



vector<shared_ptr<Ship>> ShipFactory::Instantiate(const map<string, string> *subs) const
{
	vector<shared_ptr<Ship>> ships;
	InstantiateContainer(ships, subs);
	return ships;
}
