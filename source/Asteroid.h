/* Asteroid.h
Copyright (c) 2014 by Michael Zahniser

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

#include <string>

class Minable;



// Class representing an asteroid or minable in a star system ("asteroids" and "minables" keywords).
// Note: Not to be confused with AsteroidField::Asteroid.
class Asteroid {
public:
	Asteroid(const std::string &name, int count, double energy);
	Asteroid(const Minable *type, int count, double energy, int belt);

	const std::string &Name() const;
	const Minable *Type() const;
	int Count() const;
	double Energy() const;
	int Belt() const;

private:
	std::string name;
	const Minable *type = nullptr;
	int count;
	double energy;
	int belt;
};
