/* Hail.cpp
Copyright (c) 2025 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Hail.h"

#include "ConditionContext.h"

void Hail::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "message")
			messages.Load(child);
		else if(child.Size() == 2 && child.Token(0) == "to" && child.Token(1) == "hail")
			toHail.Load(child);
		else if(child.Size() == 1 && child.Token(0) == "hailing ship")
			filterHailingShip.Load(child);
		else if(child.Size() == 2 && child.Token(0) == "weight")
			weight = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}

bool Hail::Matches(const ConditionsStore &conditions, const Ship &hailingShip) const
{
	if(weight == 0)
		return false;
	if(!filterHailingShip.Matches(hailingShip))
		return false;
	return toHail.Test(conditions, ConditionContext { .hailingShip = &hailingShip });
}

std::string Hail::Message(const ConditionsStore &conditions, const Ship &hailingShip) const
{
	return messages.Get();
}

int Hail::getWeight() const
{
	return weight;
}
