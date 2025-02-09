/* Raiders.h
Copyright (c) 2023 by Hurleveur

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

#include "RaidFleet.h"

#include <vector>

class DataNode;



// Class containing and managing raid fleets.
class Raiders {
public:
	// Helper function to load a single fleet and support the deprecated syntax.
	void LoadFleets(const DataNode &node, bool remove, int valueIndex, bool deprecated = false);
	void Load(const DataNode &node);
	const std::vector<RaidFleet> &RaidFleets() const;
	double EmptyCargoAttraction() const;
	bool ScoutsCargo() const;


private:
	std::vector<RaidFleet> raidFleets;

	double emptyCargoAttraction = 1.;
	bool scoutsCargo = false;
};
