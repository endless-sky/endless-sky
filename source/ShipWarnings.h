/* Ship.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIP_WARNINGS_H_
#define SHIP_WARNINGS_H_

#include "ClickZone.h"

#include <string>
#include <vector>

class Point;
class Ship;



// Class that detects flight check warnings of ships.
class ShipWarnings {
public:
	enum : int
	{
		// Flight check warnings.
		FLIGHT_CHECK_MASK = 0x7f,
		NO_ENERGY = 0x1,
		NO_THRUSTERS = 0x2,
		NO_THRUSTER_ENERGY = 0x4,
		NO_STEERING = 0x8,
		NO_STEERING_ENERGY = 0x10,
		SOLAR_POWER_WARNING = 0x20,
		OVERHEATING = 0x40,
	};
	
	
public:
	ShipWarnings();
	explicit ShipWarnings(const Ship &ship, int updateBits = -1);
	
	// Check for problems that prevent the ship from flying.
	void Update(const Ship &ship, int updateBits = -1);
	void Draw(const Point &center, bool blink = false);
	
	int Warnings() const;
	std::vector<std::string> WarningIcons() const;
	std::vector<std::string> WarningLabels() const;
	// Click zones matching what is drawn and tooltip names as values.
	std::vector<ClickZone<std::string>> TooltipZones(const Point &center) const;
	
	
private:
	int warnings = 0;
};



#endif
