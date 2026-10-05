/* Facility.cpp
Copyright (c) 2026 by Endless Sky Expanded contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Facility.h"

#include "DataNode.h"

#include <algorithm>

using namespace std;



void Facility::Load(const DataNode &node)
{
	if(node.Size() < 2)
		return;
	trueName = node.Token(1);
	inputs.clear();
	outputs.clear();

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "description" && hasValue)
			description = child.Token(1);
		else if(key == "cost" && hasValue)
			cost = max<int64_t>(0, child.Value(1));
		else if(key == "upkeep" && hasValue)
			upkeep = max<int64_t>(0, child.Value(1));
		else if((key == "input" || key == "output") && child.Size() >= 3)
		{
			int amount = static_cast<int>(child.Value(2));
			if(amount > 0)
				(key == "input" ? inputs : outputs).emplace_back(child.Token(1), amount);
			else
				child.PrintTrace("Skipping non-positive amount:");
		}
		else if(key == "storage" && hasValue)
			storage = max(0, static_cast<int>(child.Value(1)));
		else if(key == "planet" && hasValue)
			for(int i = 1; i < child.Size(); ++i)
				planets.insert(child.Token(i));
		else if(key == "attributes" && hasValue)
			for(int i = 1; i < child.Size(); ++i)
				attributes.insert(child.Token(i));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



bool Facility::IsDefined() const
{
	return !trueName.empty();
}



const string &Facility::TrueName() const
{
	return trueName;
}



const string &Facility::Description() const
{
	return description;
}



int64_t Facility::Cost() const
{
	return cost;
}



int64_t Facility::Upkeep() const
{
	return upkeep;
}



const Facility::Amounts &Facility::Inputs() const
{
	return inputs;
}



const Facility::Amounts &Facility::Outputs() const
{
	return outputs;
}



int Facility::Storage() const
{
	return storage;
}



bool Facility::CanBuildOn(const string &planet, const set<string> &planetAttributes) const
{
	if(planets.contains(planet))
		return true;
	for(const string &attribute : attributes)
		if(planetAttributes.contains(attribute))
			return true;
	return false;
}
