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
#include "DataWriter.h"
#include "EsUuid.h"
#include "text/Format.h"
#include "GameData.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Ship.h"

#include <cstdlib>

using namespace std;



void ShipManager::Load(const DataNode &node)
{
	if(node.Size() < 3 || node.Token(1) != "ship")
	{
		node.PrintTrace("Skipping unrecognized node.");
		return;
	}
	taking = node.Token(0) == "take";
	model = GameData::Ships().Get(node.Token(2));
	if(node.Size() >= 4)
		name = node.Token(3);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;

		if(key == "id" && hasValue)
			id = child.Token(1);
		else if(key == "count" && hasValue)
		{
			int val = child.Value(1);
			if(val <= 0)
				child.PrintTrace("\"count\" must be a non-zero, positive number.");
			else
				count = val;
		}
		else if(taking)
		{
			if(key == "unconstrained")
				unconstrained = true;
			else if(key == "with outfits")
				takeOutfits = true;
			else if(key == "require outfits")
				requireOutfits = true;
			else
				child.PrintTrace("Skipping unrecognized token.");
		}
		else
			child.PrintTrace("Skipping unrecognized token.");
	}

	if(taking && !id.empty() && count > 1)
		node.PrintTrace("Use of \"id\" to refer to the ship is only supported when \"count\" is equal to 1.");
}



void ShipManager::Save(DataWriter &out) const
{
	out.Write(Giving() ? "give" : "take", "ship", model->VariantName(), name);
	out.BeginChild();
	{
		out.Write("count", count);
		if(!id.empty())
			out.Write("id", id);
		if(unconstrained)
			out.Write("unconstrained");
		if(takeOutfits)
			out.Write("with outfits");
		if(requireOutfits)
			out.Write("require outfits");
	}
	out.EndChild();
}



bool ShipManager::CanBeDone(const PlayerInfo &player) const
{
	// If we are giving ships there are no conditions to meet.
	return Giving() || static_cast<int>(SatisfyingShips(player).size()) == count;
}



void ShipManager::Do(PlayerInfo &player) const
{
	if(model->TrueModelName().empty())
		return;

	string shipName;
	if(Giving())
	{
		for(int i = 0; i < count; ++i)
			shipName = player.GiftShip(model, name, id)->GivenName();
	}
	else
	{
		auto toTake = SatisfyingShips(player);
		if(toTake.size() == 1)
			shipName = toTake.begin()->get()->GivenName();
		for(const auto &ship : toTake)
			player.TakeShip(ship.get(), model, takeOutfits);
	}
	Messages::Add({(count == 1 ? "The " + model->DisplayModelName() + " \"" + shipName + "\" was " :
		to_string(count) + " " + model->PluralModelName() + " were ") +
		(Giving() ? "added to" : "removed from") + " your fleet.",
		GameData::MessageCategories().Get("normal")});
}



// Expands phrases and substitutions in the ship name, into a new copy of this ShipManager
ShipManager ShipManager::Instantiate(const map<string, string> &subs) const
{
	ShipManager result = *this;
	result.name = Format::Replace(Phrase::ExpandPhrases(name), subs);
	return result;
}


const Ship *ShipManager::ShipModel() const
{
	return model;
}



const string &ShipManager::Id() const
{
	return id;
}



bool ShipManager::Giving() const
{
	return !taking;
}



vector<shared_ptr<Ship>> ShipManager::SatisfyingShips(const PlayerInfo &player) const
{
	const System *here = player.GetSystem();
	const auto shipToTakeId = player.GiftedShips().find(id);
	bool foundShip = shipToTakeId != player.GiftedShips().end();
	vector<shared_ptr<Ship>> satisfyingShips;

	for(const auto &ship : player.Ships())
	{
		if(ship->TrueModelName() != model->TrueModelName())
			continue;
		if(!unconstrained)
		{
			if(ship->GetSystem() != here)
				continue;
			if(ship->IsDisabled())
				continue;
			if(ship->IsParked())
				continue;
		}
		if(!id.empty())
		{
			if(!foundShip)
				continue;
			if(ship->UUID() != shipToTakeId->second)
				continue;
		}
		if(!name.empty() && name != ship->GivenName())
			continue;
		bool hasRequiredOutfits = true;
		// If "with outfits" or "requires outfits" is specified,
		// this ship must have each outfit specified in that variant definition.
		if(requireOutfits)
			for(const auto &it : model->Outfits())
			{
				const auto &outfit = ship->Outfits().find(it.first);
				int amountEquipped = (outfit != ship->Outfits().end() ? outfit->second : 0);
				if(it.second > amountEquipped)
				{
					hasRequiredOutfits = false;
					break;
				}
			}
		if(hasRequiredOutfits)
			satisfyingShips.emplace_back(ship);
		// We do not want any more ships than is specified.
		if(static_cast<int>(satisfyingShips.size()) >= count)
			break;
	}

	return satisfyingShips;
}
