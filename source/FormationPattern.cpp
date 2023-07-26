/* FormationPattern.cpp
Copyright (c) 2019-2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FormationPattern.h"

#include "Angle.h"
#include "DataNode.h"

#include <cmath>

using namespace std;

namespace {
	const Point defaultFormationPoint = Point();
}



FormationPattern::PositionIterator::PositionIterator(const FormationPattern &pattern)
	: pattern(pattern)
{
	positionIt = pattern.positions.begin();
}



const Point &FormationPattern::PositionIterator::operator*()
{
	if(positionIt == pattern.positions.end())
		return defaultFormationPoint;
	return *positionIt;
}



FormationPattern::PositionIterator &FormationPattern::PositionIterator::operator++()
{
	if(positionIt != pattern.positions.end())
		++positionIt;
	return *this;
}



void FormationPattern::Load(const DataNode &node)
{
	if(!name.empty())
	{
		node.PrintTrace("Duplicate entry for formation-pattern \"" + name + "\":");
		return;
	}

	if(node.Size() >= 2)
		name = node.Token(1);
	else
	{
		node.PrintTrace("Skipping load of unnamed formation-pattern:");
		return;
	}

	for(const DataNode &child : node)
		if(child.Token(0) == "position" && child.Size() >= 3)
			positions.emplace_back(child.Value(1), child.Value(2));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
}



const string &FormationPattern::Name() const
{
	return name;
}



void FormationPattern::SetName(const std::string &name)
{
	this->name = name;
}



// Get an iterator to iterate over the formation positions in this pattern.
FormationPattern::PositionIterator FormationPattern::begin() const
{
	return FormationPattern::PositionIterator(*this);
}
