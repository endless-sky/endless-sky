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

#include "Angle.h"
#include "FormationPattern.h"


#include <cmath>

using namespace std;


FormationPattern::PositionIterator::PositionIterator(const FormationPattern &pattern)
	: pattern(pattern)
{
	MoveToValidPosition();
}



const Point *FormationPattern::PositionIterator::operator->()
{
	return &currentPoint;
}



const Point &FormationPattern::PositionIterator::operator*()
{
	return currentPoint;
}



FormationPattern::PositionIterator &FormationPattern::PositionIterator::operator++()
{
	slot++;
	MoveToValidPosition();
	return *this;
}



void FormationPattern::PositionIterator::MoveToValidPosition()
{
	currentPoint = pattern.Position(slot);
}



const string &FormationPattern::Name() const
{
	return name;
}



void FormationPattern::SetName(const std::string &name)
{
	this->name = name;
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
		{
			slots.emplace_back();
			Point &slot = slots.back();
			slot.X() = child.Value(1);
			slot.Y() = child.Value(2);
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
}



// Get an iterator to iterate over the formation positions in this pattern.
FormationPattern::PositionIterator FormationPattern::begin() const
{
	return FormationPattern::PositionIterator(*this);
}



// Get the number of positions on an arc or line.
unsigned int FormationPattern::Slots() const
{
	return slots.size();
}



// Get a formation position based on ring, line(or arc)-number and position on the line.
Point FormationPattern::Position(unsigned int slotNr) const
{
	if(slotNr >= slots.size())
		return Point();
	return slots[slotNr];
}
