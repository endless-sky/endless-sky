/* Raiders.cpp
Copyright (c) 2023 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Raiders.h"

#include "DataNode.h"
#include "Fleet.h"
#include "GameData.h"

#include <algorithm>

using namespace std;



void Raiders::LoadFleets(const DataNode &node, bool remove, int valueIndex, bool deprecated)
{
	const Fleet *fleet = GameData::Fleets().Get(node.Token(valueIndex));
	if(remove)
	{
		auto fleetMatcher = [fleet](const RaidFleet &raidFleet) noexcept -> bool {
			return raidFleet.GetFleet() == fleet;
		};
		raidFleets.erase(remove_if(raidFleets.begin(), raidFleets.end(), fleetMatcher), raidFleets.end());
	}
	else
	{
		if(deprecated)
			raidFleets.emplace_back(fleet,
				node.Size() > (valueIndex + 1) ? node.Value(valueIndex + 1) : 2.,
				node.Size() > (valueIndex + 2) ? node.Value(valueIndex + 2) : 0.);
		else
			raidFleets.emplace_back().Load(node, fleet);
	}
}



void Raiders::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		bool remove = child.Token(0) == "remove";
		bool add = child.Token(0) == "add";
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}

		const string &key = child.Token(add || remove);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = child.Size() > valueIndex;
		if(remove && !hasValue)
		{
			if(key == "scouts cargo hold")
				scoutsCargo = false;
			else if(key == "empty cargo attraction")
				emptyCargoAttraction = 1.;
			else if(key == "fleet")
				raidFleets.clear();
			else
				child.PrintTrace("Cannot \"remove\" the given key:");
		}
		else if(key == "scouts cargo hold")
			scoutsCargo = true;
		else if(!hasValue)
			child.PrintTrace("Error: Expected key to have a value:");
		else if(key == "fleet")
		{
			if(!add)
				raidFleets.clear();
			LoadFleets(child, remove, valueIndex);
		}
		else if(key == "empty cargo attraction")
			emptyCargoAttraction = child.Value(valueIndex);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const vector<RaidFleet> &Raiders::RaidFleets() const
{
	return raidFleets;
}



double Raiders::EmptyCargoAttraction() const
{
	return emptyCargoAttraction;
}



bool Raiders::ScoutsCargo() const
{
	return scoutsCargo;
}
