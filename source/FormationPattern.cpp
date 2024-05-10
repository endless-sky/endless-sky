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
		unsigned int shipsToPlace)
	: pattern(pattern), shipsToPlace(shipsToPlace), centerBodyRadius(centerBodyRadius),
		diameterToPx(diameterToPx), widthToPx(widthToPx), heightToPx(heightToPx)
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

	// Number of ships is used as number of remaining ships still to be placed.
	if(shipsToPlace > 0)
		--shipsToPlace;
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

	// If we are at the last line and we have less ships still to place than that
	// would fit on the line, then perform centering if required.
	if(!atEnd && position == 0 && shipsToPlace > 0 &&
			lineRepeatPositions - 1 > shipsToPlace && pattern.IsCentered(line))
		// Determine the amount to skip for centering and skip those.
		position += (lineRepeatPositions - shipsToPlace) / 2;

	if(atEnd)
		currentPoint = Point();
	else
		currentPoint = pattern.Position(ring, line, repeat, position,
			diameterToPx, widthToPx, heightToPx);
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
			line.start.AddLoad(child);
			line.endOrAnchor = line.start;
		}
		else
			child.PrintTrace("Skipping unrecognized or unsupported attribute:");
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
FormationPattern::PositionIterator FormationPattern::begin(
	double diameterToPx, double widthToPx, double heightToPx,
	double centerBodyRadius, unsigned int shipsToPlace) const
{
	return FormationPattern::PositionIterator(*this, diameterToPx, widthToPx,
		heightToPx, centerBodyRadius, shipsToPlace);
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



// Check if a certain arc or line is centered.
bool FormationPattern::IsCentered(unsigned int lineNr) const
{
	// Returns false if we have an invalid lineNr or when the line is not centered.
	return lineNr < lines.size() && lines[lineNr].centered;
}



// Get a formation position based on ring, line(or arc)-number and position on the line.
Point FormationPattern::Position(unsigned int ring, unsigned int lineNr, unsigned int repeatNr,
	unsigned int linePosition, double diameterToPx, double widthToPx, double heightToPx) const
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

	// Get the number of positions for this line or arc.
	int positions = line.positions;

	// Check if we have a valid repeat section and apply it to the common calculations if we have it.
	const LineRepeat *repeat = nullptr;
	if(ring > 0)
	{
		repeat = &(line.repeats[repeatNr]);
		startPx += repeat->repeatStart.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		endOrAnchorPx += repeat->repeatEndOrAnchor.GetPx(diameterToPx, widthToPx, heightToPx) * ring;
		positions += repeat->repeatPositions * ring;
	}

	// Compensate for any skipped positions. This would usually be the start-position on the line, but it can be the
	// end position if we are on an alternating line.
	if(ring % 2 && repeat && repeat->alternating && line.skipLast)
		++linePosition;
	else if(line.skipFirst)
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
		{
			endAngle += repeat->repeatAngle * ring;

			// Calculate new start and turn end angle if we need to alternate this repeat.
			if(ring % 2 && repeat->alternating && positions > 0)
			{
				startAngle += endAngle;
				endAngle = -endAngle;
			}
		}

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

	// This is not an arc, perform the line-based calculations.

	// Swap start and end if we need to alternate in the repeat section.
	// Apply repeat section if it is relevant.
	if(ring % 2 && repeat && repeat->alternating)
		swap(startPx, endOrAnchorPx);

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



void FormationPattern::MultiAxisPoint::Add(Axis axis, const Point &toAdd)
{
	position[axis] += toAdd;
}



void FormationPattern::MultiAxisPoint::AddLoad(const DataNode &node)
{
	// We need at least the position keyword and 2 coordinate numbers.
	if(node.Size() < 3)
		return;

	// By default we parse for pixels.
	Axis axis = PIXELS;
	double scalingFactor = 1.;

	// Parse all the keywords before the coordinate and warn if the datafiles
	// use tokens used in other formation-pattern formats (that are not supported
	// in this codebase at this moment).
	for(int i = 1; i < node.Size() - 2; ++i)
		node.PrintTrace("Ignoring unrecognized token " + node.Token(i) + ":");

	// The last 2 numbers are always the coordinate.
	double x = node.Value(node.Size() - 2);
	double y = node.Value(node.Size() - 1);
	Add(axis, Point(x, y) * scalingFactor);
}



Point FormationPattern::MultiAxisPoint::GetPx(double diameterToPx, double widthToPx, double heightToPx) const
{
	return position[PIXELS] +
		position[DIAMETERS] * diameterToPx +
		position[WIDTHS] * widthToPx +
		position[HEIGHTS] * heightToPx;
}
