/* ScanOptions.h
Copyright (c) 2026 by Amazinite

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

#include <map>
#include <memory>

#include "ScanType.h"
#include "Ship.h"



// Stores the player ships that are within range of a target for each scan type.
class ScanOptions {
public:
	// Add the given ship as a scanning option for this scan type. If a closer ship
	// was already added, then it will remain as the option for that scan type.
	void AddOption(ScanType type, const std::shared_ptr<Ship> &ship, double distance);
	// Whether the player has a ship for the given scan type.
	bool HasOption(ScanType type) const;
	// The closest ship that the player has for the given scan type.
	std::shared_ptr<Ship> GetOption(ScanType type) const;


private:
	std::map<ScanType, std::pair<std::shared_ptr<Ship>, double>> closest;
};
