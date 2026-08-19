/* ShipFactory.h
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

#pragma once

#include "../text/Format.h"
#include "../Phrase.h"
#include "../Ship.h"

#include <list>
#include <map>
#include <memory>
#include <vector>

class ConditionsStore;
class DataNode;
class Ship;



// A class that handles the parsing and instantiation of ship nodes
// for other classes where the ship can either be provided as a full
// definition or a reference to an existing ship definition.
class ShipFactory {
public:
	ShipFactory() = default;

	void Load(const DataNode &node, const ConditionsStore *playerConditions);
	void FinishLoading();
	bool IsValid() const;

	void RemoveModel(const std::string &shipModel);
	void Clear();

	// Instantiate this factory's ships into the provided vector.
	// Makes text substitutions in the names of any ships if a substitution map is provided.
	void Instantiate(std::list<std::shared_ptr<Ship>> &container,
		const std::map<std::string, std::string> *subs = nullptr) const;
	void Instantiate(std::vector<std::shared_ptr<Ship>> &container,
		const std::map<std::string, std::string> *subs = nullptr) const;


private:
	template<class T>
	void InstantiateContainer(T &container, const std::map<std::string, std::string> *subs = nullptr) const;


private:
	// A count of the total number of ships in this factory.
	// Used for maintaining the load order for the instantiation order.
	int shipOrder = 0;
	// Ships provided as full definitions.
	std::map<int, std::shared_ptr<Ship>> ships;
	// Ships provided from stock definitions, alongside the names to give them.
	std::map<int, const Ship *> stockShips;
	std::list<std::string> shipNames;
};



template<class T>
void ShipFactory::InstantiateContainer(T &container, const std::map<std::string, std::string> *subs) const
{
	auto shipIt = ships.begin();
	auto stockIt = stockShips.begin();
	auto nameIt = shipNames.begin();
	for(int i = 0; i < shipOrder; ++i)
	{
		if(shipIt != ships.end() && shipIt->first == i)
		{
			container.emplace_back(std::make_shared<Ship>(*shipIt->second));
			++shipIt;
		}
		else
		{
			std::shared_ptr<Ship> &ship = container.emplace_back(std::make_shared<Ship>(*stockIt->second));
			std::string name = Phrase::ExpandPhrases(*nameIt);
			if(subs)
				name = Format::Replace(name, *subs);
			ship->SetGivenName(name);
			++stockIt;
			++nameIt;
		}
	}
}
