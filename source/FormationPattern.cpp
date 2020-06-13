/* FormationPattern.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FormationPattern.h"

using namespace std;



const string FormationPattern::Name() const
{
	return name;
}



void FormationPattern::Load(const DataNode &node)
{
	if(node.Size() >=2)
		name = node.Token(1);
	
	for(const DataNode &child : node)
		if(child.Token(0) == "flippable" && child.Size()>=2)
			for(int i=1; i<child.Size(); i++)
			{
				if(child.Token(i) == "x")
					flippable_x = true;
				else if(child.Token(i) == "y")
					flippable_y = true;
			}
		else if(child.Token(0) == "rotational" && child.Size() >= 2)
			rotatable = child.Value(1);
		else if(child.Size() >= 5 && child.Token(0) == "line")
		{
			lines.emplace_back(Point(child.Value(1), child.Value(2)), static_cast<int>(child.Value(3) + 0.5), Angle(child.Value(4)));
			Line &line = lines[lines.size()-1];
			for(const DataNode &grand : child)
			{
				if(grand.Size() >= 2 && grand.Token(0) == "spacing")
					line.spacing = grand.Value(1);
				else if(grand.Size() >= 3 && grand.Token(0) == "repeat")
				{
					line.repeatVector = Point(grand.Value(1), grand.Value(2));
					line.slotsIncrease = 0;
					for(const DataNode &grandGrand : grand)
						if(grandGrand.Size() >= 2 && grandGrand.Token(0) == "increase")
							line.slotsIncrease = static_cast<int>(grandGrand.Value(1) + 0.5);
				}
			}
		}
}



// Get the next line that has space for placement of ships. Returns -1 if none found/available.
int FormationPattern::NextLine(unsigned int ring, unsigned int lineNr) const
{
	// All lines participate in the first initial ring.
	if(ring == 0 && lineNr < (lines.size()-1))
		return lineNr + 1;
	
	// For later rings only lines that repeat will participate.
	unsigned int linesScanned = 0;
	while(linesScanned <= lines.size())
	{
		lineNr = (lineNr + 1) % lines.size();
		if((lines[lineNr]).slotsIncrease >= 0)
			return lineNr;
		
		// Safety mechanism to avoid endless loops if the formation has a limited size.
		linesScanned++;
	}
	return -1;
}



// Get the number of positions on a line for the given ring.
int FormationPattern::LineSlots(unsigned int ring, unsigned int lineNr) const
{
	if(lineNr >= lines.size())
		return 0;
	
	// Retrieve the relevant line.
	Line line = lines[lineNr];
	
	// For the first ring, only the initial positions are relevant.
	if(ring == 0)
		return line.initialSlots;
	
	// If we are in a later ring, then skip lines that don't repeat.
	if(line.slotsIncrease < 0)
		return 0;
	
	return line.initialSlots + line.slotsIncrease * ring;
}



// Get a formation position based on ring, line-number and position on the line.
Point FormationPattern::Position(unsigned int ring, unsigned int lineNr, unsigned int lineSlot) const
{
	if(lineNr >= lines.size())
		return Point();
	
	Line line = lines[lineNr];
	
	// Calculate position based on the initial anchor, the ring on which we are, the
	// line-position on the current line and the rotation of the current line.
	return line.anchor +
		line.repeatVector * ring +
		line.direction.Rotate(Point(0, -line.spacing * lineSlot));
}



int FormationPattern::Rotatable() const
{
	return rotatable;
}



bool FormationPattern::FlippableY() const
{
	return flippable_y;
}



bool FormationPattern::FlippableX() const
{
	return flippable_x;
}
