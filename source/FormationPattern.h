/* FormationPattern.h
Copyright (c) 2019-2021 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FORMATION_PATTERN_H_
#define FORMATION_PATTERN_H_

#include "DataNode.h"
#include "Point.h"

#include <string>
#include <vector>

class Body;



// Class that handles the loading and position calculations for a pattern that
// can be used for ships flying in formation.
// This class only deals with calculating the positions that exist in a formation
// pattern, the actual assignment of ships to positions is handled outside this class.
class FormationPattern {
public:
	// Struct that describes the properties of an active formation, like the maximum
	// sizes of the ships participating in the formation and some data on the Body
	// around which the formation is formed.
	class ActiveFormation
	{
	public:
		// Center radius of the formation that is to be kept clear. This
		// is typically used to avoid positions of ships overlapping with
		// the  body around which the formation is formed. The actual radius
		// that will be kept clear is this centerBodyRadius plus half of the
		// maxDiameter.
		double centerBodyRadius = 100;
		// Information on ships participating in the formation. Initialized
		// with some defaults for smaller ships.
		double maxDiameter = 80;
		double maxWidth = 80;
		double maxHeight = 80;
	
	
	public:
		// Helper function to clear meta-data from formation participants;
		void ClearParticipants();
	};
	
	
	// Iterator that provides sequential access to all formation positions.
	class PositionIterator : public std::iterator<
		std::input_iterator_tag, // iterator_category
		Point,                   // iterator: value_type
		std::ptrdiff_t,          // iterator: difference_type
		const Point *,           // iterator: pointer
		Point &>                 // iterator: reference
	{
	public:
		PositionIterator(const FormationPattern &pattern, const ActiveFormation &af,
			unsigned int startRing = 0, unsigned int shipsToPlace = 0);
		
		// A subset of the default input_iterator operations. Limiting to
		// only a subset, since not all operations are used in-game.
		const Point *operator->();
		const Point &operator*();
		PositionIterator &operator++();
		
		// Additional operators for status retrieval.
		unsigned int Ring() const;
		
	private:
		void MoveToValidPosition();
	
	private:
		// Data from the active formation for which we are calculating
		// positions.
		const ActiveFormation &activeFormation;
		// The pattern for which we are calculating positions.
		const FormationPattern &pattern;
		// The location in the pattern.
		unsigned int ring = 0;
		unsigned int line = 0;
		unsigned int repeat = 0;
		unsigned int slot = 0;
		// Number of ships that we expect to place using this iterator.
		// The number of ships mostly affects the last line that is placed
		// in case the last line is centered.
		// When zero is given then every line is treated as if many more
		// ships need to be placed.
		// The shipsToPlace can be a subset of the total ships in a formation
		// if the formation is constructed in parts (as sections of rings).
		unsigned int shipsToPlace= 0;
		// Currently calculated Point.
		Point currentPoint;
		// Internal status variable;
		bool atEnd = false;
	};



	// Returns the name of this pattern.
	const std::string &Name() const;
	void SetName(const std::string &name);
	
	// Load formation from a datafile.
	void Load(const DataNode &node);
	
	// Get an iterator to iterate over the formation positions in this pattern.
	PositionIterator begin(const ActiveFormation &af, unsigned int startRing = 0, unsigned int shipsToPlace = 0) const;
	
	// Retrieve properties like number of lines and arcs, number of repeat sections and number of positions.
	// TODO: Should we hide those properties and just provide a position iterator instead?
	unsigned int Lines() const;
	unsigned int Repeats(unsigned int lineNr) const;
	unsigned int Slots(unsigned int ring, unsigned int lineNr, unsigned int repeatNr) const;
	bool IsCentered(unsigned int lineNr) const;
	
	// Calculate a position based on the current ring, line/arc and slot on the line.
	Point Position(unsigned int ring, unsigned int lineNr, unsigned int repeatNr, unsigned int lineSlot, double diameterToPx, double widthToPx, double heightToPx) const;
	
	// Information about allowed rotating and mirroring that still results in the same formation.
	int Rotatable() const;
	bool FlippableY() const;
	bool FlippableX() const;
	
	
private:
	class MultiAxisPoint {
	public:
		// Coordinate axises for formations; Pixels (default) and heights, widths and diameters of the biggest ship in a formation.
		enum Axis { PIXELS, DIAMETERS, WIDTHS, HEIGHTS };
		
		// Add position information to one of the internal tracked points.
		void Add(Axis axis, const Point &toAdd);
		
		// Parse a position input from a data-node and add the values to this MultiAxisPoint.
		// This function is typically called when getting the first or last position on a
		// line or when getting an anchor for an arc.
		void AddLoad(const DataNode &node);
		
		// Get a point in pixel coordinates based on the conversion factors given for
		// the diameters, widths and heights.
		Point GetPx(double diameterToPx, double widthToPx, double heightToPx) const;
	
	
	private:
		// Position based on the possible axises.
		Point position[4];
	};
	
	class LineRepeat {
	public:
		// Vector to apply to get to the next start point for the next iteration.
		MultiAxisPoint repeatStart;
		MultiAxisPoint repeatEndOrAnchor;
		
		double repeatAngle = 0;

		// Slots to add or remove in this repeat section.
		int repeatSlots = 0;
		
		// Indicates if each odd repeat section should start from the end instead of the start.
		bool alternating = false;
	};

	class Line {
	public:
		// The starting point for this line.
		MultiAxisPoint start;
		MultiAxisPoint endOrAnchor;
		
		// Angle in case this line is an Arc.
		double angle = 0;
		
		// Sections of the line that repeat.
		std::vector<LineRepeat> repeats;
		
		// The number of initial positions for this line.
		int slots = 1;
		
		// Properties of how the line behaves
		bool centered = false;
		bool isArc = false;
		bool skipFirst = false;
		bool skipLast = false;
	};
	
	
private:
	// Name of the formation pattern.
	std::string name;
	// Indicates if the formation is rotatable, a value of -1 means not
	// rotatable, while a positive value is taken as the rotation angle
	// in relation to the full 360 degrees full angle:
	// Square and Diamond shapes could get a value of 90, since you can
	// rotate such a shape over 90 degrees and still have the same shape.
	// Triangles could get a value of 120, since you can rotate them over
	// 120 degrees and again get the same shape.
	int rotatable = -1;
	// Indicates if the formation is flippable along the longitudinal axis.
	bool flippable_y = false;
	// Indicates if the formation is flippable along the transverse axis.
	bool flippable_x = false;
	// The lines that define the formation.
	std::vector<Line> lines;
};



#endif
