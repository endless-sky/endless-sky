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



// Class that detects problems in the ship.
class ShipWarnings {
public:
	enum : int
	{
		FLIGHT_CHECK_MASK = 0xff,
		// serious warnings
		SERIOUS_MASK = 0x0f,
		NO_ENERGY = 0x1,
		NO_STEERING = 0x2,
		NO_THRUSTER = 0x4,
		OVERHEATING = 0x8,
		// other warnings
		AFTERBURNER_ONLY = 0x10,
		BATTERY_ONLY = 0x20,
		LIMITED_MOVEMENT = 0x40,
		SOLAR_POWER = 0x80,
	};
	
	
public:
	ShipWarnings();
	explicit ShipWarnings(const Ship &ship, int updateBits = -1);
	
	// Check for problems in the ship.
	void Update(const Ship &ship, int updateBits = -1);
	void Draw(const Point &center);
	
	int Warnings() const;
	std::vector<std::string> WarningIcons() const;
	std::vector<std::string> WarningLabels() const;
	// Click zones matching what is drawn and tooltip names as values.
	std::vector<ClickZone<std::string>> TooltipZones(const Point &center) const;
	
	// Maximum number of warnings that will be drawn. A value of 0 means no limit.
	// Extra warnings are not displayed and instead have their labels concateneted with
	// '\n' to the label of the last icon that is displayed.
	int PackWarnings() const;
	void SetPackWarnings(size_t n);
	
	// Icons are square. The icon image is resized to match the target size.
	double IconSize() const;
	void SetIconSize(double size);
	
	Point Dimensions() const;
	
	
private:
	int warnings = 0;
	double iconSize = 24.;
	size_t packWarnings = 0;
};



#endif
