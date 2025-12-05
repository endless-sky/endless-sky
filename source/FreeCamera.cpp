/* FreeCamera.cpp
Copyright (c) 2024 by the Endless Sky developers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FreeCamera.h"

using namespace std;

const string FreeCamera::name = "Free Camera";



FreeCamera::FreeCamera()
	: position(0., 0.), velocity(0., 0.), inputDirection(0., 0.)
{
}



Point FreeCamera::GetTarget() const
{
	return position;
}



Point FreeCamera::GetVelocity() const
{
	return velocity;
}



void FreeCamera::Step()
{
	// Apply input as acceleration
	velocity += inputDirection * speed;

	// Apply friction
	velocity *= friction;

	// Update position
	position += velocity;

	// Clear input for next frame
	inputDirection = Point(0., 0.);
}



const string &FreeCamera::ModeName() const
{
	return name;
}



void FreeCamera::SetMovement(double x, double y)
{
	inputDirection = Point(x, y);
}



void FreeCamera::SetPosition(const Point &pos)
{
	position = pos;
	velocity = Point(0., 0.);
}
