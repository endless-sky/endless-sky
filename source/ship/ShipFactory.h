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

#include "../ExclusiveItem.h"
#include "../text/Format.h"
#include "../Phrase.h"
#include "../Ship.h"

#include <list>
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
	// A pairing of the stored ship definition and the name of the ship.
	// The name may be blank if no name was given alongside the definition.
	std::vector<std::pair<ExclusiveItem<Ship>, std::string>> ships;
};



template<class T>
void ShipFactory::InstantiateContainer(T &container, const std::map<std::string, std::string> *subs) const
{
	for(const auto &[shipDef, name] : ships)
	{
		// Instantiation of a container involves copying the contents of this factory into new
		// shared pointers. This factory remains unchanged after instantiation of its ships.
		const std::shared_ptr<Ship> &ship = container.emplace_back(std::make_shared<Ship>(*shipDef));
		if(!name.empty())
		{
			std::string giveName = Phrase::ExpandPhrases(name);
			if(subs)
				giveName = Format::Replace(giveName, *subs);
			ship->SetGivenName(giveName);
		}
	}
}
