/* Location.h
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LOCATION_H_
#define LOCATION_H_

class Planet;
class System;



// An object that can hold a pointer to either a planet or a system.
class Location {
public:
	Location() = default;
	Location(const Planet *planet);
	Location(const System *system);

	Location &operator=(const Location &location);
	Location &operator=(const Planet *planet);
	Location &operator=(const System *system);
	operator bool() const;

	const Planet *GetPlanet() const;
	const System *GetSystem() const;

	bool IsPlanet() const;
	bool IsSystem() const;
	bool IsValid() const;

	const Planet *planet = nullptr;
	const System *system = nullptr;
};



#endif
