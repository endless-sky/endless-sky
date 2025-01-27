/* RaidFleet.cpp
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

#include "RaidFleet.h"

#include "DataNode.h"
#include "Fleet.h"
#include "GameData.h"

using namespace std;



RaidFleet::RaidFleet(const Fleet *fleet, double minAttraction, double maxAttraction)
	: fleet(fleet), minAttraction(minAttraction), maxAttraction(maxAttraction)
{
}



RaidFleet::RaidFleet()
{
}



void RaidFleet::Load(const DataNode &node, const Fleet *fleet)
{
	this->fleet = fleet;
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(child.Size() < 2)
			child.PrintTrace("Error: Expected key to have a value:");
		else if(key == "min attraction")
			minAttraction = child.Value(1);
		else if(key == "max attraction")
			maxAttraction = child.Value(1);
		else if(key == "fleet cap attraction")
			capAttraction = child.Value(1);
		else if(key == "fleet cap")
			fleetCap = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
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



double RaidFleet::CapAttraction() const
{
	return capAttraction ? capAttraction : maxAttraction;
}



double RaidFleet::FleetCap() const
{
	return fleetCap;
}
