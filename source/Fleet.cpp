/* Fleet.cpp
Copyright (c) 2014 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Fleet.h"

using namespace std;



Fleet::Fleet(string name, shared_ptr<Ship> flagship, vector<shared_ptr<Ship>> ships)
: name(name), flagship(flagship), ships(ships) {}



string Fleet::Name() const
{
	return name;
}



void Fleet::SetName(std::string newName)
{
	name = newName;
}



std::shared_ptr<Ship> Fleet::Flagship() const
{
	return flagship;
}



void Fleet::SetFlagship(std::shared_ptr<Ship> newFlagship)
{
	flagship = newFlagship;
}



std::vector<std::shared_ptr<Ship>> Fleet::Ships() const
{
	return ships;
}



void Fleet::SetShips(std::vector<std::shared_ptr<Ship>> newShips)
{
	ships = newShips;
}



bool Fleet::ShouldBeRemoved() const
{
	bool shouldBeRemoved = true;
	for(shared_ptr<Ship> ship : ships)
		if(ship.get())
		{
			shouldBeRemoved = false;
			break;
		}

	return shouldBeRemoved;
}
