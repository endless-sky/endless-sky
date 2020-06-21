/* FormationPattern.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FORMATION_PATTERN_H_
#define FORMATION_PATTERN_H_

#include "Angle.h"
#include "DataNode.h"
#include "Point.h"

#include <string>
#include <vector>



// Class representing a definition of a spaceship flying formation as loaded
// from datafiles or savegame when used in GameData.
class FormationPattern {
public:
	const std::string Name() const;
	
	// Load formation from a datafile.
	void Load(const DataNode &node);
	
	// Retrieve properties like number of lines and arcs, number of repeat sections and number of positions.
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
	
	
protected:
	class MultiAxisPoint {
	public:
		// Coordinate axises for formations; Pixels (default) and heights, widths and diameters of the biggest ship in a formation.
		enum Axis { PIXELS, DIAMETERS, WIDTHS, HEIGHTS };
		// Position based on the 4 possible axises.
		Point position[4];
		
		// Add a point to one of the internal tracked points.
		void Add(Axis axis, const Point& toAdd);
		
		// Parse position from node and add the values to this slot-pos.
		void AddLoad(const DataNode &node);
		
		// Get a point in pixel coordinates based on the conversion factors given.
		Point GetPx(double diameterToPx, double widthToPx, double heightToPx) const;
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
	};
	
	
protected:
	// The lines that define the formation.
	std::vector<Line> lines;
	
	
private:
	std::string name;
	int rotatable = -1;
	// Indicates if the formation is flippable along the longitudinal axis.
	bool flippable_y = false;
	// Indicates if the formation is flippable along the transverse axis.
	bool flippable_x = false;
};



#endif
