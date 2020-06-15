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
		else if(child.Token(0) == "rotatable" && child.Size() >= 2)
			rotatable = child.Value(1);
		else if(child.Token(0) == "line")
		{
			lines.emplace_back();
			Line &line = lines[lines.size()-1];
			
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "start" && grand.Size() >= 3)
					line.start.AddLoad(grand);
				else if(grand.Token(0) == "end" && grand.Size() >= 3)
					line.end.AddLoad(grand);
				else if(grand.Token(0) == "slots" && grand.Size() >= 2)
					line.slots = static_cast<int>(grand.Value(1) + 0.5);
				else if(grand.Token(0) == "repeat")
				{
					line.repeatSlots = 0;
					for(const DataNode &grandGrand : grand)
						if (grandGrand.Token(0) == "start" && grandGrand.Size() >= 3)
							line.repeatStart.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "end" && grandGrand.Size() >= 3)
							line.repeatEnd.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "slots" && grandGrand.Size() >= 2)
							line.repeatSlots = static_cast<int>(grandGrand.Value(1) + 0.5);
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
		if((lines[lineNr]).repeatSlots >= 0)
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
		return line.slots;
	
	// If we are in a later ring, then skip lines that don't repeat.
	if(line.repeatSlots < 0)
		return 0;
	
	return line.slots + line.repeatSlots * ring;
}



// Get a formation position based on ring, line-number and position on the line.
Point FormationPattern::Position(unsigned int ring, unsigned int lineNr, unsigned int lineSlot, double diameterToPx, double widthToPx, double heightToPx) const
{
	if(lineNr >= lines.size())
		return Point();
	
	Line line = lines[lineNr];
	
	// Calculate the start and end positions in pixels (based on the current ring).
	Point startPx = line.start.GetPx(diameterToPx, widthToPx, heightToPx) +
		line.repeatStart.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
	Point endPx = line.end.GetPx(diameterToPx, widthToPx, heightToPx) +
		line.repeatEnd.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		
	// Calculate the step from each slot between start and end.
	Point slotPx = endPx - startPx;

	// Divide by slots, but don't count the first (since it is at position 0, not at position 1).
	int slots = line.slots + line.repeatSlots * ring - 1;
	if(slots > 0)
		slotPx = slotPx / static_cast<double>(slots);
		
	// Calculate position of the current slot.
	return startPx + slotPx * lineSlot;
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


void FormationPattern::MultiAxisPoint::Add(Axis axis, const Point& toAdd)
{
	position[axis] += toAdd;
}



void FormationPattern::MultiAxisPoint::AddLoad(const DataNode &node)
{
	// We need at least the slot-name keyword and 2 coordinate numbers.
	if(node.Size() < 3)
		return;

	// Track if we are parsing a polar coordinate.
	bool parsePolar = false;

	// By default we parse for pixels.
	Axis axis = PIXELS;
	double scalingFactor = 1.;

	// Parse all the keywords before the coordinate
	for(int i=1; i < node.Size() - 2; i++)
	{
		if(node.Token(i) == "polar")
			parsePolar = true;
		else if(node.Token(i) == "px")
			axis = PIXELS;
		else if(node.Token(i) == "diameter")
			axis = DIAMETERS;
		else if(node.Token(i) == "radius")
		{
			scalingFactor = 2.;
			axis = DIAMETERS;
		}
		else if(node.Token(i) == "width")
			axis = WIDTHS;
		else if(node.Token(i) == "height")
			axis = HEIGHTS;
	}

	// The last 2 numbers are always the coordinate.
	if(parsePolar)
	{
		Angle dir = Angle(node.Value(node.Size() - 2));
		double len = node.Value(node.Size() - 1) * scalingFactor;
		Add(axis, dir.Unit() * len);
	}
	else
	{
		double x = node.Value(node.Size() - 2);
		double y = node.Value(node.Size() - 1);
		Add(axis, Point(x, y) * scalingFactor);
	}
}



Point FormationPattern::MultiAxisPoint::GetPx(double diameterToPx, double widthToPx, double heightToPx)
{
	return position[PIXELS] +
		position[DIAMETERS] * diameterToPx +
		position[WIDTHS] * widthToPx +
		position[HEIGHTS] * heightToPx;
}
