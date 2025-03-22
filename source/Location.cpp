/* Location.cpp
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

#include "Location.h"

#include "Planet.h"
#include "System.h"

using namespace std;



Location::Location(const Planet *planet)
	: planet(planet), system(nullptr)
{
}



Location::Location(const System *system)
	: planet(nullptr), system(system)
{
}



Location &Location::operator=(const Location &location)
{
	if(location.planet)
		*this = location.planet;
		// this->planet = location.planet;
	else if(location.system)
		*this = location.system;
		// this->system = location.system;
	else
	{
		this->planet = nullptr;
		this->system = nullptr;
	}
	return *this;
}



Location &Location::operator=(const Planet *planet)
{
	this->planet = planet;
	this->system = nullptr;
	return *this;
}



Location &Location::operator=(const System *system)
{
	this->system = system;
	this->planet = nullptr;
	return *this;
}



Location::operator bool() const
{
	return planet || system;
}



const Planet *Location::GetPlanet() const
{
	return planet;
}



const System *Location::GetSystem() const
{
	if(planet)
		return planet->GetSystem();
	if(system)
		return system;
	return nullptr;
}




bool Location::IsPlanet() const
{
	return planet && !system;
}



bool Location::IsSystem() const
{
	return system && !planet;
}



bool Location::IsValid() const
{
	if(planet)
		return planet->IsValid();
	if(system)
		return system->IsValid();
	return false;
}

