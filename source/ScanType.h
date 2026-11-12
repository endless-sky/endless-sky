/* ScanType.h
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



// All possible scanner types that a ship can have.
class ScanType {
public:
	enum {
		CARGO = 1 << 0,
		OUTFIT = 1 << 1,
		ASTEROID = 1 << 2,
		TACTICAL = 1 << 3,
		STRATEGIC = 1 << 4,
		CREW = 1 << 5,
		FUEL = 1 << 6,
		ENERGY = 1 << 7,
		THERMAL = 1 << 8,
		MANEUVER = 1 << 9,
		ACCELERATION = 1 << 10,
		VELOCITY = 1 << 11,
		WEAPON = 1 << 12,
		RANGE = 1 << 13,
	};
};
