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


FormationPattern::PositionIterator::PositionIterator(const FormationPattern &pattern,
		double diameterToPx, double widthToPx, double heightToPx, double centerBodyRadius,
		unsigned int startRing, unsigned int shipsToPlace)
	: pattern(pattern), ring(startRing), shipsToPlace(shipsToPlace), centerBodyRadius(centerBodyRadius),
		diameterToPx(diameterToPx), widthToPx(widthToPx), heightToPx(heightToPx)
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
	if(!atEnd)
		slot++;
	// Number of ships is used as number of remaining ships still to be placed.
	if(shipsToPlace > 0)
		--shipsToPlace;
	MoveToValidPosition();
	return *this;
}



unsigned int FormationPattern::PositionIterator::Ring() const
{
	return ring;
}



void FormationPattern::PositionIterator::MoveToValidPosition()
{
	// If we cannot calculate any new positions, then just return center point.
	if(atEnd)
	{
		currentPoint = Point();
		return;
	}

	// Check if there are any lines available.
	unsigned int lines = pattern.Lines();
	if(lines < 1)
		atEnd = true;

	unsigned int ringsScanned = 0;
	unsigned int startingRing = ring;
	unsigned int lineRepeatSlots = pattern.Slots(ring, line, repeat);

	while(slot >= lineRepeatSlots && !atEnd)
	{
		unsigned int patternRepeats = pattern.Repeats(line);
		// LineSlot number is beyond the amount of slots available.
		// Need to move a ring, a line or a repeat-section forward.
		if(ring > 0 && line < lines && patternRepeats > 0 && repeat < patternRepeats - 1)
		{
			// First check if we are on a valid line and have another repeat section.
			++repeat;
			slot = 0;
			lineRepeatSlots = pattern.Slots(ring, line, repeat);
		}
		else if(line < lines - 1)
		{
			// If we don't have another repeat section, then check for a next line.
			++line;
			repeat = 0;
			slot = 0;
			lineRepeatSlots = pattern.Slots(ring, line, repeat);
		}
		else
		{
			// If we checked all lines and repeat sections, then go for the next ring.
			++ring;
			line = 0;
			repeat = 0;
			slot = 0;
			lineRepeatSlots = pattern.Slots(ring, line, repeat);

			// If we scanned more than 5 rings without finding a slot, then we have an empty pattern.
			++ringsScanned;
			if(ringsScanned > 5)
			{
				// Restore starting ring and indicate that there are no more positions.
				ring = startingRing;
				atEnd = true;
			}
		}
	}

	// If we are at the last line and we have less ships still to place than that
	// would fit on the line, then perform centering if required.
	if(!atEnd && slot == 0 && shipsToPlace > 0 &&
			(lineRepeatSlots - 1) > shipsToPlace && pattern.IsCentered(line))
		// Determine the amount to skip for centering and skip those.
		slot += (lineRepeatSlots - shipsToPlace) / 2;

	if(atEnd)
		currentPoint = Point();
	else
		currentPoint = pattern.Position(ring, line, repeat, slot,
			diameterToPx, widthToPx, heightToPx);
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
		if(child.Token(0) == "flippable" && child.Size() >= 2)
			for(int i = 1; i < child.Size(); ++i)
			{
				if(child.Token(i) == "x")
					flippable_x = true;
				else if(child.Token(i) == "y")
					flippable_y = true;
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
		else if(child.Token(0) == "rotatable" && child.Size() >= 2)
			rotatable = child.Value(1);
		else if((child.Token(0) == "point" || child.Token(0) == "position") && child.Size() >= 3)
		{
			lines.emplace_back();
			Line &line = lines.back();
			// A point is a line with just 1 slot.
			line.slots = 1;
			// The specification of the coordinates is on the same line as the keyword.
			line.start.AddLoad(child);
			line.endOrAnchor = line.start;
		}
		else if(child.Token(0) == "line" || child.Token(0) == "arc")
		{
			lines.emplace_back();
			Line &line = lines.back();

			if(child.Token(0) == "arc")
				line.isArc = true;

			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "start" && grand.Size() >= 3)
					line.start.AddLoad(grand);
				else if(grand.Token(0) == "end" && grand.Size() >= 3 && !line.isArc)
					line.endOrAnchor.AddLoad(grand);
				else if(grand.Token(0) == "anchor" && grand.Size() >= 3 && line.isArc)
					line.endOrAnchor.AddLoad(grand);
				else if(grand.Token(0) == "angle" && grand.Size() >= 2 && line.isArc)
					line.angle = grand.Value(1);
				else if(grand.Token(0) == "slots" && grand.Size() >= 2)
					line.slots = static_cast<int>(grand.Value(1) + 0.5);
				else if(grand.Token(0) == "centered")
					line.centered = true;
				else if(grand.Token(0) == "skip")
					for(int i = 1; i < grand.Size(); ++i)
					{
						if(grand.Token(i) == "first")
							line.skipFirst = true;
						else if(grand.Token(i) == "last")
							line.skipLast = true;
						else
							grand.PrintTrace("Skipping unrecognized attribute:");
					}
				else if(grand.Token(0) == "repeat")
				{
					line.repeats.emplace_back();
					LineRepeat &repeat = line.repeats[line.repeats.size() - 1];
					for(const DataNode &grandGrand : grand)
						if (grandGrand.Token(0) == "start" && grandGrand.Size() >= 3)
							repeat.repeatStart.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "end" && grandGrand.Size() >= 3 && !line.isArc)
							repeat.repeatEndOrAnchor.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "anchor" && grandGrand.Size() >= 3 && line.isArc)
							repeat.repeatEndOrAnchor.AddLoad(grandGrand);
						else if(grandGrand.Token(0) == "angle" && grandGrand.Size() >= 2 && line.isArc)
							repeat.repeatAngle = grandGrand.Value(1);
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



// Get an iterator to iterate over the formation positions in this pattern.
FormationPattern::PositionIterator FormationPattern::begin(
	double diameterToPx, double widthToPx, double heightToPx,
	double centerBodyRadius, unsigned int startRing, unsigned int shipsToPlace) const
{
	return FormationPattern::PositionIterator(*this, diameterToPx, widthToPx,
		heightToPx, centerBodyRadius, startRing, shipsToPlace);
}



// Get the number of lines (and arcs) in this formation.
unsigned int FormationPattern::Lines() const
{
	return lines.size();
}



// Get the number of repeat sections for the given arc or line.
unsigned int FormationPattern::Repeats(unsigned int lineNr) const
{
	if(lineNr >= lines.size())
		return 0;
	return lines[lineNr].repeats.size();
}



// Get the number of positions on an arc or line.
unsigned int FormationPattern::Slots(unsigned int ring, unsigned int lineNr, unsigned int repeatNr) const
{
	// Retrieve the relevant line.
	if(lineNr >= lines.size())
		return 0;
	const Line &line = lines[lineNr];

	int lineRepeatSlots = line.slots;

	// For the very first ring, only the initial positions are relevant.
	if(ring > 0)
	{
		// For later rings we need to have repeat sections to perform repeating.
		if(repeatNr >= line.repeats.size())
			return 0;

		lineRepeatSlots += line.repeats[repeatNr].repeatSlots * ring;
	}

	// If we skip positions, then remove them from the counting.
	if(line.skipFirst)
		--lineRepeatSlots;
	if(line.skipLast)
		--lineRepeatSlots;

	// If we are in a later ring, then skip lines that don't repeat.
	if(lineRepeatSlots < 0)
		return 0;

	return lineRepeatSlots;
}



// Check if a certain arc or line is centered.
bool FormationPattern::IsCentered(unsigned int lineNr) const
{
	// Returns false if we have an invalid lineNr or when the line is not centered.
	return lineNr < lines.size() && lines[lineNr].centered;
}



// Get a formation position based on ring, line(or arc)-number and position on the line.
Point FormationPattern::Position(unsigned int ring, unsigned int lineNr, unsigned int repeatNr, unsigned int lineSlot, double diameterToPx, double widthToPx, double heightToPx) const
{
	// First check if the inputs result in a valid line or arc position.
	if(lineNr >= lines.size())
		return Point();
	const Line &line = lines[lineNr];
	if(ring > 0 && repeatNr >= line.repeats.size())
		return Point();

	// Perform common start and end/anchor position calculations in pixels.
	Point startPx = line.start.GetPx(diameterToPx, widthToPx, heightToPx);
	Point endOrAnchorPx = line.endOrAnchor.GetPx(diameterToPx, widthToPx, heightToPx);

	// Get the number of slots for this line or arc.
	int slots = line.slots;

	// Check if we have a valid repeat section and apply it to the common calculations if we have it.
	const LineRepeat *repeat = nullptr;
	if(ring > 0 && repeatNr < line.repeats.size())
	{
		repeat = &(line.repeats[repeatNr]);
		startPx += repeat->repeatStart.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		endOrAnchorPx += repeat->repeatEndOrAnchor.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		slots += repeat->repeatSlots * ring;
	}

	// Compensate for any skipped slots. This would usually be the start-slot, but it can be the
	// end slot if we are on an alternating line.
	if(ring % 2 && repeat && repeat->alternating && line.skipLast)
		++lineSlot;
	else if(line.skipFirst)
		++lineSlot;

	// Switch to arc-specific calculations if this line is an arc.
	if(line.isArc)
	{
		// Calculate angles and radius from anchor to start.
		double startAngle = Angle(startPx).Degrees();
		double endAngle = line.angle;
		double radius = startPx.Length();

		// Apply repeat section for endAngle, startAngle and anchor were already done before.
		if(repeat)
		{
			endAngle += repeat->repeatAngle * ring;

			// Calculate new start and turn end angle if we need to alternate this repeat.
			if(ring % 2 && repeat->alternating && slots > 0)
			{
				startAngle += endAngle;
				endAngle = -endAngle;
			}
		}

		// Apply slots to get the correct slot-angle.
		if(slots > 1)
			endAngle /= slots - 1;
		double slotAngle = startAngle + endAngle * lineSlot;

		// Get into the range of 0 to 360 for conversion to angle)
		if(slotAngle < 0)
			slotAngle = -fmod(-slotAngle, 360) + 360;
		else
			slotAngle = fmod(slotAngle, 360);

		// Combine anchor with the slot-position and return the result.
		return endOrAnchorPx + Angle(slotAngle).Unit() * radius;
	}

	// This is not an arc, perform the line-based calculations.

	// Swap start and end if we need to alternate in the repeat section.
	// Apply repeat section if it is relevant.
	if(ring % 2 && repeat && repeat->alternating)
	{
		Point tmpPx = endOrAnchorPx;
		endOrAnchorPx = startPx;
		startPx = tmpPx;
	}

	// Calculate the step from each slot between start and end.
	Point slotPx = endOrAnchorPx - startPx;

	// Divide by slots, but don't count the first (since it is at position 0, not at position 1).
	if(slots > 1)
		slotPx /= slots - 1;

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
	for(int i = 1; i < node.Size() - 2; ++i)
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
		else
			node.PrintTrace("Skipping unrecognized token " + node.Token(i) + ":");
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
