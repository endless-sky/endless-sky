/* Orders.cpp
Copyright (c) 2024 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Orders.h"

using namespace std;



void Orders::SetTargetShip(shared_ptr<Ship> ship)
{
	targetShip = ship;
}



void Orders::SetTargetAsteroid(shared_ptr<Minable> asteroid)
{
	targetAsteroid = asteroid;
}



void Orders::SetTargetPoint(const Point &point)
{
	targetPoint = point;
}



void Orders::SetTargetSystem(const System *system)
{
	targetSystem = system;
}



shared_ptr<Ship> Orders::GetTargetShip() const
{
	return targetShip.lock();
}



shared_ptr<Minable> Orders::GetTargetAsteroid() const
{
	return targetAsteroid.lock();
}



const Point &Orders::GetTargetPoint() const
{
	return targetPoint;
}



const System *Orders::GetTargetSystem() const
{
	return targetSystem;
}
