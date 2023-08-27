/* RaidFleets.h
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

#ifndef RAID_FLEETS_H_
#define RAID_FLEETS_H_

#include <vector>

class Fleet;
class DataNode;



// Information about how a fleet intended for raiding behaves.
class RaidFleet {
public:
	RaidFleet(const Fleet *fleet, double minAttraction, double maxAttraction);
	const Fleet *GetFleet() const;
	double MinAttraction() const;
	double MaxAttraction() const;

private:
	const Fleet *fleet = nullptr;
	double minAttraction;
	double maxAttraction;
};

class RaidFleets : public std::vector<RaidFleet> {
public:
	void Load(const DataNode &node, bool remove, int valueIndex);
};

#endif
