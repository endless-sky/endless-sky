/* FormationPattern.h
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

#pragma once

#include "Point.h"

#include <string>
#include <vector>

class Body;
class DataNode;



// Class that handles the loading and position calculations for a pattern that
// can be used for ships flying in formation.
// This class only deals with calculating the positions that exist in a formation
// pattern, the actual assignment of ships to positions is handled outside this class.
class FormationPattern {
public:
	// Iterator that provides sequential access to all formation positions.
	class PositionIterator {
	public:
		explicit PositionIterator(const FormationPattern &pattern,
			double centerBodyRadius);

		PositionIterator() = delete;

		// Iterator traits
		using iterator_category = std::input_iterator_tag;
		using value_type = Point;
		using difference_type = void;
		using pointer = const Point *;
		using reference = Point &;

		// A subset of the default input_iterator operations. Limiting to
		// only a subset, since not all operations are used in-game.
		const Point &operator*() const;
		PositionIterator &operator++();

	private:
		void MoveToValidPositionOutsideCenterBody();
		void MoveToValidPosition();

	private:
		// The pattern for which we are calculating positions.
		const FormationPattern &pattern;

		// The iteration of the (repeating) pattern we are processing.
		// Most formationpatterns grow from the inside to the outside.
		unsigned int ring = 0;
		// The line (or point, or arc) in the pattern that we are processing.
		unsigned int line = 0;
		// The active repeat-section on the line or arc. (Lines or arcs can have more than 1 repeat section)
		unsigned int repeat = 0;
		// The position on the current repeat section of the line or arc.
		unsigned int position = 0;
		// Center radius that is to be kept clear. This is used to avoid
		// positions of ships overlapping with the body around which the
		// formation is formed.
		double centerBodyRadius = 0;
		// Currently calculated Point.
		Point currentPoint;
		// Internal status variable;
		bool atEnd = false;
	};

	// Load formation from a datafile.
	void Load(const DataNode &node);

	// Returns the name of this pattern.
	const std::string &TrueName() const;
	void SetTrueName(const std::string &name);

	// Get an iterator to iterate over the formation positions in this pattern.
	PositionIterator begin(double centerBodyRadius) const;

	// Information about allowed rotating and mirroring that still results in the same formation.
	int Rotatable() const;
	bool FlippableY() const;
	bool FlippableX() const;

private:
	// Retrieve properties like number of lines and arcs, number of repeat sections and number of positions.
	// Private, because we hide those properties and just provide a position iterator instead.
	unsigned int Lines() const;
	// Number of repeat sections on the current line.
	unsigned int Repeats(unsigned int lineNr) const;
	// Number of positions on the current repeat section of the active line or arc.
	unsigned int Positions(unsigned int ring, unsigned int lineNr, unsigned int repeatNr) const;

	// Calculate a position based on the current ring, line/arc, repeat-section and position on the line-repeat-section.
	Point Position(unsigned int ring, unsigned int lineNr, unsigned int repeatNr,
		unsigned int lineRepeatPosition) const;

private:
	class LineRepeat {
	public:
		// Vector to apply to get to the next start point for the next iteration.
		Point repeatStart;
		Point repeatEndOrAnchor;

		double repeatAngle = 0;

		// Positions to add or remove in this repeat section.
		int repeatPositions = 0;
	};

	class Line {
	public:
		// The starting point for this line.
		Point start;
		Point endOrAnchor;

		// Angle in case this line is an arc.
		double angle = 0;

		// Sections of the line that repeat.
		std::vector<LineRepeat> repeats;

		// The number of initial positions for this line.
		int positions = 1;

		// Properties of how the line behaves.
		bool isArc = false;
		bool skipFirst = false;
		bool skipLast = false;
	};


private:
	// Name of the formation pattern.
	std::string trueName;
	// Indicates if the formation is rotatable. A value of -1 means not
	// rotatable, while a positive value is taken as the rotation angle
	// in relation to the full 360 degrees full angle:
	// Square and Diamond shapes could get a value of 90, since you can
	// rotate such a shape over 90 degrees and still have the same shape.
	// Triangles could get a value of 120, since you can rotate them over
	// 120 degrees and again get the same shape.
	// A value of 0 means that the formation can be rotated in any way and
	// still be fine. This could be used for shapes like (perfect) circles
	// and for formations where rotation just shouldn't happen.
	int rotatable = -1;
	// Indicates if the formation is flippable along the longitudinal axis.
	bool flippableY = false;
	// Indicates if the formation is flippable along the transverse axis.
	bool flippableX = false;
	// The lines that define the formation.
	std::vector<Line> lines;
};
