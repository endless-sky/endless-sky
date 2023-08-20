/* RaidFleets.cpp
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

#include "RaidFleets.h"

#include "DataNode.h"
#include "Fleet.h"
#include "GameData.h"


RaidFleet::RaidFleet(const Fleet *fleet, double minAttraction, double maxAttraction)
	: fleet(fleet), minAttraction(minAttraction), maxAttraction(maxAttraction)
{
}



const Fleet *RaidFleet::GetFleet() const
{
	return fleet;
}



double RaidFleet::MinAttraction() const
{
	return minAttraction;
}



double RaidFleet::MaxAttraction() const
{
	return maxAttraction;
}



void RaidFleets::Load(const DataNode &node, bool remove, int valueIndex)
{
	const Fleet *fleet = GameData::Fleets().Get(node.Token(valueIndex));
	if(remove)
	{
		for(auto it = begin(); it != end(); )
			if(it->GetFleet() == fleet)
				it = erase(it);
			else
				++it;
	}
	else
		emplace_back(fleet,
			node.Size() > (valueIndex + 1) ? node.Value(valueIndex + 1) : 2.,
			node.Size() > (valueIndex + 2) ? node.Value(valueIndex + 2) : 0.);
}
