/* Asteroid.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Asteroid.h"

#include "DataNode.h"

using namespace std;



Asteroid::Asteroid(const std::string &name, const DataNode &node, int valueIndex)
		: name(name)
{
	Load(node, valueIndex, 0);
}



Asteroid::Asteroid(const Minable *type, const DataNode &node, int valueIndex, int beltCount)
		: type(type)
{
	Load(node, valueIndex, max(beltCount, 1));
}



const string &Asteroid::Name() const
{
	return name;
}



const Minable *Asteroid::Type() const
{
	return type;
}



int Asteroid::Count() const
{
	return count;
}



double Asteroid::Energy() const
{
	return energy;
}



int Asteroid::Belt() const
{
	return belt;
}



void Asteroid::Load(const DataNode &node, int valueIndex, int beltCount)
{
	const bool isMinable = beltCount > 0;

	if(node.Size() >= valueIndex + 2)
		count = node.Value(valueIndex + 1);
	if(node.Size() >= valueIndex + 3)
		energy = node.Value(valueIndex + 2);
	if(isMinable && node.Size() >= valueIndex + 4)
		belt = node.Value(valueIndex + 3);

	for(const DataNode &child : node)
	{
		if(child.Size() < 1)
			continue;
		const string &subKey = child.Token(0);
		if(child.Size() < 2)
			child.PrintTrace("Warning: Expected asteroid/minable sub-key to have a value:");
		else if(subKey == "count")
			count = child.Value(1);
		else if(subKey == "energy")
			energy = child.Value(1);
		else if(isMinable && subKey == "belt")
			belt = child.Value(1);
		else if(subKey == "to" && child.Token(1) == "spawn")
			toSpawn.Load(child);
		else
			child.PrintTrace("Warning: Unrecognized asteroid/minable sub-key:");
	}

	if(count <= 0)
		node.PrintTrace("Error: asteroid/minable must have a positive count:");
	else if(energy <= 0.)
		node.PrintTrace("Error: asteroid/minable must have a positive energy:");
	else if(valueIndex == 1 && static_cast<unsigned>(belt) > static_cast<unsigned>(beltCount))
		node.PrintTrace("Error: minable belt number out of bounds:");
}



bool Asteroid::ShouldSpawn(const ConditionsStore &conditionsStore) const
{
	return toSpawn.Test(conditionsStore);
}
