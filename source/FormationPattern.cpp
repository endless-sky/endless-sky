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
			Line &line = lines[lines.size() - 1];
			
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "start" && grand.Size() >= 3)
					line.start.AddLoad(grand);
				else if(grand.Token(0) == "end" && grand.Size() >= 3)
					line.end.AddLoad(grand);
				else if(grand.Token(0) == "slots" && grand.Size() >= 2)
					line.slots = static_cast<int>(grand.Value(1) + 0.5);
				else if(grand.Token(0) == "centered")
					line.centered = true;
				else if(grand.Token(0) == "repeat")
				{
					line.repeats.emplace_back();
					LineRepeat &repeat = line.repeats[line.repeats.size() - 1];
					for(const DataNode &grandGrand : grand)
						if (grandGrand.Token(0) == "start" && grandGrand.Size() >= 3)
							repeat.repeatStart.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "end" && grandGrand.Size() >= 3)
							repeat.repeatEnd.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "slots" && grandGrand.Size() >= 2)
							repeat.repeatSlots = static_cast<int>(grandGrand.Value(1) + 0.5);
						else if(grandGrand.Token(0) == "alternating")
							repeat.alternating = true;
						else
							grandGrand.PrintTrace("Skipping unrecognized attribute:");
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
}



// Get the number of lines in this formation.
unsigned int FormationPattern::Lines() const
{
	return lines.size();
}



// Get the number of repeat sections for this line.
unsigned int FormationPattern::Repeats(unsigned int lineNr) const
{
	if(lineNr >= lines.size())
		return 0;
	return lines[lineNr].repeats.size();
}



// Get the number of positions on a line for the given ring.
unsigned int FormationPattern::LineRepeatSlots(unsigned int ring, unsigned int lineNr, unsigned int repeatNr) const
{
	// Retrieve the relevant line.
	if(lineNr >= lines.size())
		return 0;
	const Line &line = lines[lineNr];
	
	// For the very first ring, only the initial positions are relevant.
	if(ring == 0)
		return line.slots;
	
	// For later rings we need to have repeat sections to perform repeating.
	if(repeatNr >= line.repeats.size())
		return 0;
	
	int lineRepeatSlots = line.slots + line.repeats[repeatNr].repeatSlots * ring;
	
	// If we are in a later ring, then skip lines that don't repeat.
	if(lineRepeatSlots < 0)
		return 0;
	
	return lineRepeatSlots;
}



// Check if a certain line is centered.
bool FormationPattern::IsCentered(unsigned int lineNr) const
{
	// Returns false if we have an invalid lineNr or when the line is not centered.
	return lineNr >= 0 && lineNr < lines.size() && lines[lineNr].centered;
}



// Get a formation position based on ring, line-number and position on the line.
Point FormationPattern::Position(unsigned int ring, unsigned int lineNr, unsigned int repeatNr, unsigned int lineSlot, double diameterToPx, double widthToPx, double heightToPx) const
{
	if(lineNr >= lines.size())
		return Point();
	const Line &line = lines[lineNr];
	
	if(ring > 0 && repeatNr >= line.repeats.size())
		return Point();

	// Calculate the start and end positions in pixels.
	Point startPx = line.start.GetPx(diameterToPx, widthToPx, heightToPx);
	Point endPx = line.end.GetPx(diameterToPx, widthToPx, heightToPx);
	
	// Apply repeat section if it is relevant.
	if(ring > 0 && repeatNr < line.repeats.size())
	{
		const LineRepeat &repeat = line.repeats[repeatNr];
		startPx += repeat.repeatStart.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		endPx += repeat.repeatEnd.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		
		// Swap start and end if we need to alternate.
		if(ring % 2 && repeat.alternating)
		{
			Point tmpPx = endPx;
			endPx = startPx;
			startPx = tmpPx;
		}
	}
	
	// Calculate the step from each slot between start and end.
	Point slotPx = endPx - startPx;
	
	// Divide by slots, but don't count the first (since it is at position 0, not at position 1).
	int slots = LineRepeatSlots(ring, lineNr, repeatNr);
	if(slots > 1)
		slotPx = slotPx / static_cast<double>(slots - 1);
	
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



Point FormationPattern::MultiAxisPoint::GetPx(double diameterToPx, double widthToPx, double heightToPx) const
{
	return position[PIXELS] +
		position[DIAMETERS] * diameterToPx +
		position[WIDTHS] * widthToPx +
		position[HEIGHTS] * heightToPx;
}
