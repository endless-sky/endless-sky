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

#ifndef FORMATION_PATTERN_H_
#define FORMATION_PATTERN_H_

#include "Point.h"

#include <string>
#include <vector>

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
		PositionIterator(const FormationPattern &pattern);
		PositionIterator() = delete;

		// Iterator traits
		using iterator_category = std::input_iterator_tag;
		using value_type = Point;
		using difference_type = void;
		using pointer = const Point *;
		using reference = Point &;

		// A subset of the default input_iterator operations. Limiting to
		// only a subset, since not all operations are used in-game.
		const Point &operator*();
		PositionIterator &operator++();


	private:
		// The pattern for which we are calculating positions.
		const FormationPattern &pattern;
		// The iterator currently used below the position iterator.
		std::vector<Point>::const_iterator positionIt;
	};


public:
	// Load formation from a datafile.
	void Load(const DataNode &node);

	// Returns the name of this pattern.
	const std::string &Name() const;
	void SetName(const std::string &name);

	// Get an iterator to iterate over the formation positions in this pattern.
	PositionIterator begin() const;


private:
	// Name of the formation pattern.
	std::string name;
	// The positions that define the formation.
	std::vector<Point> positions;
};

#endif
