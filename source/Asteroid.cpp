/* Asteroid.cpp
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

#include "Asteroid.h"

using namespace std;



Asteroid::Asteroid(const string &name, int count, double energy)
		: name(name), count(count), energy(energy)
{
}



Asteroid::Asteroid(const Minable *type, int count, double energy, int belt)
		: type(type), count(count), energy(energy), belt(belt)
{
}



const string &Asteroid::Name() const
{
	return name;
}



const Minable *Asteroid::Type() const
{
	return type;
}



int Asteroid::Count() const
{
	return count;
}



double Asteroid::Energy() const
{
	return energy;
}



int Asteroid::Belt() const
{
	return belt;
}
