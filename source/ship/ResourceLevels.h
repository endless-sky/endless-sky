/* ResourceLevels.h
Copyright (c) 2025 by Amazinite

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



// A class representing the the magnitude of various resources
// that a ship has, including a ship's HP, energy, heat, fuel, and
// the amounts of various DoT applied to the ship. Resources can
// represent the values currently on a ship or the changes to be
// applied to a ship, such as an amount of damage to be taken or the
// resources required for repairs or movement.
class ResourceLevels {
public:
	double hull = 0.;
	double shields = 0.;
	double energy = 0.;
	double heat = 0.;
	double fuel = 0.;

	// Accrued "ion damage" that will affect this ship's energy over time.
	double ionization = 0.;
	// Accrued "scrambling damage" that will affect this ship's weaponry over time.
	double scrambling = 0.;
	// Accrued "disruption damage" that will affect this ship's shield effectiveness over time.
	double disruption = 0.;
	// Accrued "slowing damage" that will affect this ship's movement over time.
	double slowness = 0.;
	// Accrued "discharge damage" that will affect this ship's shields over time.
	double discharge = 0.;
	// Accrued "corrosion damage" that will affect this ship's hull over time.
	double corrosion = 0.;
	// Accrued "leak damage" that will affect this ship's fuel over time.
	double leakage = 0.;
	// Accrued "burn damage" that will affect this ship's heat over time.
	double burning = 0.;
};
