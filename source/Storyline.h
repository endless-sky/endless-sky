/* Storyline.h
Copyright (c) 2024 by Timothy Collett

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

#include "Color.h"
#include "ExclusiveItem.h"

#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Mission;



// Class representing a particular storyline of missions, whether main or side.
// Storylines have a particular color (which will be displayed on the Conversation panel)
// and are associated with missions.
class Storyline {
public:
	Storyline() = default;
	Storyline(const DataNode &node);
	// Set up the storyline from its data file node.
	void Load(const DataNode &node);

	// Get the storyline's name
	std::string Name() const;
	// Get the storyline's associated color.
	const Color &GetColor() const;
	// Get whether the storyline is part of the main plot.
	bool IsMain() const;

	// Associate a mission with this storyline
	void AddMission(Mission *mission);


private:
	// The storyline's name, which will be used by missions to link to it.
	std::string name;
	// Indicates whether this storyline is part of the main plot.
	bool main = false;

	// The storyline's associated color.
	ExclusiveItem<Color> color;

	// The list of missions belonging to the storyline.
	std::vector<Mission *> missions;
};
