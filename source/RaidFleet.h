/* RaidFleet.h
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

#include <vector>

class DataNode;
class Fleet;



// Information about how a fleet intended for raiding behaves.
class RaidFleet {
public:
	RaidFleet(const Fleet *fleet, double minAttraction, double maxAttraction);
	void Load(const DataNode &node, const Fleet *fleet);
	const Fleet *GetFleet() const;
	double MinAttraction() const;
	double MaxAttraction() const;
	double CapAttraction() const;
	double FleetCap() const;


private:
	const Fleet *fleet = nullptr;
	double minAttraction = 0.;
	double maxAttraction;
	double capAttraction;
	double fleetCap = 10.;
};
