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
		double centerBodyRadius)
	: pattern(pattern), centerBodyRadius(centerBodyRadius)
{
	MoveToValidPositionOutsideCenterBody();
}



const Point &FormationPattern::PositionIterator::operator*()
{
	return currentPoint;
}



FormationPattern::PositionIterator &FormationPattern::PositionIterator::operator++()
{
	if(!atEnd)
	{
		position++;
		MoveToValidPositionOutsideCenterBody();
	}

	return *this;
}



void FormationPattern::PositionIterator::MoveToValidPositionOutsideCenterBody()
{
	MoveToValidPosition();
	unsigned int maxTries = 50;
	// Skip positions too close to the center body
	while(!atEnd && currentPoint.Length() <= centerBodyRadius && maxTries > 0)
	{
		position++;
		MoveToValidPosition();
		maxTries--;
	}
	if(maxTries == 0)
		atEnd = true;
}



void FormationPattern::PositionIterator::MoveToValidPosition()
{
	unsigned int lines = pattern.Lines();

	// If we cannot calculate any new positions, then just return center point.
	if(atEnd || lines < 1)
	{
		atEnd = true;
		currentPoint = Point();
		return;
	}

	unsigned int ringsScanned = 0;
	unsigned int startingRing = ring;
	unsigned int lineRepeatPositions = pattern.Positions(ring, line, repeat);

	while(position >= lineRepeatPositions && !atEnd)
	{
		unsigned int patternRepeats = pattern.Repeats(line);
		// LinePosition number is beyond the amount of positions available on the line/arc.
		// Need to move a ring, a line/arc or a repeat-section forward.
		if(ring > 0 && line < lines && patternRepeats > 0 && repeat < patternRepeats - 1)
		{
			// First check if we are on a valid line and have another repeat section.
			++repeat;
			position = 0;
			lineRepeatPositions = pattern.Positions(ring, line, repeat);
		}
		else if(line < lines - 1)
		{
			// If we don't have another repeat section, then check for a next line.
			++line;
			repeat = 0;
			position = 0;
			lineRepeatPositions = pattern.Positions(ring, line, repeat);
		}
		else
		{
			// If we checked all lines and repeat sections, then go for the next ring.
			++ring;
			line = 0;
			repeat = 0;
			position = 0;
			lineRepeatPositions = pattern.Positions(ring, line, repeat);

			// If we scanned more than 5 rings without finding a new position, then we have an empty pattern.
			++ringsScanned;
			if(ringsScanned > 5)
			{
				// Restore starting ring and indicate that there are no more positions.
				ring = startingRing;
				atEnd = true;
			}
		}
	}

	if(atEnd)
		currentPoint = Point();
	else
		currentPoint = pattern.Position(ring, line, repeat, position);
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
					flippableX = true;
				else if(child.Token(i) == "y")
					flippableY = true;
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
		else if(child.Token(0) == "rotatable" && child.Size() >= 2)
			rotatable = child.Value(1);
		else if(child.Token(0) == "position" && child.Size() >= 3)
		{
			Line &line = lines.emplace_back();
			// A point is a line with just 1 position on it.
			line.positions = 1;
			// The specification of the coordinates is on the same line as the keyword.
			line.start.Set(child.Value(1), child.Value(2));
			line.endOrAnchor = line.start;
			// Also allow positions to have a repeat section, for single points only
			for(const DataNode &grand : child)
				if(grand.Token(0) == "repeat" && grand.Size() >= 3)
				{
					LineRepeat &repeat = line.repeats.emplace_back();
					repeat.repeatStart.Set(grand.Value(1), grand.Value(2));
				}
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
		}
		else if(child.Token(0) == "line" || child.Token(0) == "arc")
		{
			Line &line = lines.emplace_back();

			if(child.Token(0) == "arc")
				line.isArc = true;

			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "start" && grand.Size() >= 3)
					line.start.Set(grand.Value(1), grand.Value(2));
				else if(grand.Token(0) == "end" && grand.Size() >= 3 && !line.isArc)
					line.endOrAnchor.Set(grand.Value(1), grand.Value(2));
				else if(grand.Token(0) == "anchor" && grand.Size() >= 3 && line.isArc)
					line.endOrAnchor.Set(grand.Value(1), grand.Value(2));
				else if(grand.Token(0) == "angle" && grand.Size() >= 2 && line.isArc)
					line.angle = grand.Value(1);
				else if(grand.Token(0) == "positions" && grand.Size() >= 2)
					line.positions = static_cast<int>(grand.Value(1) + 0.5);
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
					LineRepeat &repeat = line.repeats.emplace_back();
					for(const DataNode &grandGrand : grand)
						if(grandGrand.Token(0) == "start" && grandGrand.Size() >= 3)
							repeat.repeatStart.Set(grandGrand.Value(1), grandGrand.Value(2));
						else if(grandGrand.Token(0) == "end" && grandGrand.Size() >= 3 && !line.isArc)
							repeat.repeatEndOrAnchor.Set(grandGrand.Value(1), grandGrand.Value(2));
						else if(grandGrand.Token(0) == "anchor" && grandGrand.Size() >= 3 && line.isArc)
							repeat.repeatEndOrAnchor.Set(grandGrand.Value(1), grandGrand.Value(2));
						else if(grandGrand.Token(0) == "angle" && grandGrand.Size() >= 2 && line.isArc)
							repeat.repeatAngle = grandGrand.Value(1);
						else if(grandGrand.Token(0) == "positions" && grandGrand.Size() >= 2)
							repeat.repeatPositions = static_cast<int>(grandGrand.Value(1) + 0.5);
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



const string &FormationPattern::Name() const
{
	return name;
}



void FormationPattern::SetName(const std::string &name)
{
	this->name = name;
}



// Get an iterator to iterate over the formation positions in this pattern.
FormationPattern::PositionIterator FormationPattern::begin(double centerBodyRadius) const
{
	return FormationPattern::PositionIterator(*this, centerBodyRadius);
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
unsigned int FormationPattern::Positions(unsigned int ring, unsigned int lineNr, unsigned int repeatNr) const
{
	// Retrieve the relevant line.
	if(lineNr >= lines.size())
		return 0;
	const Line &line = lines[lineNr];

	int lineRepeatPositions = line.positions;

	// For the very first ring, only the initial positions are relevant.
	if(ring > 0)
	{
		// For later rings we need to have repeat sections to perform repeating.
		if(repeatNr >= line.repeats.size())
			return 0;

		lineRepeatPositions += line.repeats[repeatNr].repeatPositions * ring;
	}

	// If we skip positions, then remove them from the counting.
	if(line.skipFirst && lineRepeatPositions > 0)
		--lineRepeatPositions;
	if(line.skipLast && lineRepeatPositions > 0)
		--lineRepeatPositions;

	// If we are in a later ring, then skip lines that don't repeat.
	if(lineRepeatPositions < 0)
		return 0;

	return lineRepeatPositions;
}



// Get a formation position based on ring, line (or arc)-number and position on the line.
Point FormationPattern::Position(unsigned int ring, unsigned int lineNr, unsigned int repeatNr,
	unsigned int linePosition) const
{
	// First check if the inputs result in a valid line or arc position.
	if(lineNr >= lines.size())
		return Point();
	const Line &line = lines[lineNr];
	if(ring > 0 && repeatNr >= line.repeats.size())
		return Point();

	// Perform common start and end/anchor position calculations in pixels.
	Point startPx = line.start;
	Point endOrAnchorPx = line.endOrAnchor;

	// Get the number of positions for this line or arc.
	int positions = line.positions;

	// Check if we have a valid repeat section and apply it to the common calculations if we have it.
	const LineRepeat *repeat = nullptr;
	if(ring > 0)
	{
		repeat = &(line.repeats[repeatNr]);
		startPx += repeat->repeatStart * ring;
		endOrAnchorPx += repeat->repeatEndOrAnchor * ring;
		positions += repeat->repeatPositions * ring;
	}

	if(line.skipFirst)
		++linePosition;

	// Switch to arc-specific calculations if this line is an arc.
	if(line.isArc)
	{
		// Calculate angles and radius from anchor to start.
		double startAngle = Angle(startPx).Degrees();
		double endAngle = line.angle;
		double radius = startPx.Length();

		// Apply repeat section for endAngle, startAngle and anchor were already done before.
		if(repeat)
			endAngle += repeat->repeatAngle * ring;

		// Apply positions to get the correct position-angle.
		if(positions > 1)
			endAngle /= positions - 1;
		double positionAngle = startAngle + endAngle * linePosition;

		// Get into the range of 0 to 360 for conversion to angle.
		if(positionAngle < 0)
			positionAngle = -fmod(-positionAngle, 360) + 360;
		else
			positionAngle = fmod(positionAngle, 360);

		// Combine anchor with the position and return the result.
		return endOrAnchorPx + Angle(positionAngle).Unit() * radius;
	}

	// This is not an arc, so perform the line-based calculations.

	// Calculate the step from each position between start and end.
	Point positionPx = endOrAnchorPx - startPx;

	// Divide by positions, but don't count the first (since it is at position 0, not at position 1).
	if(positions > 1)
		positionPx /= positions - 1;

	// Calculate position in the formation based on the position in the line.
	return startPx + positionPx * linePosition;
}



int FormationPattern::Rotatable() const
{
	return rotatable;
}



bool FormationPattern::FlippableY() const
{
	return flippableY;
}



bool FormationPattern::FlippableX() const
{
	return flippableX;
}
