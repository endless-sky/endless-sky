/* Collision.cpp
Copyright (c) 2023 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Collision.h"

using namespace std;



// Initialize a Collision.
Collision::Collision(Body *hit, CollisionType collisionType, double range)
	: hit(hit), collisionType(collisionType), range(range)
{

}



// The Body that was hit for this collision. May be a nullptr if nothing
// was directly hit.
Body *Collision::HitBody()
{
	return hit;
}



// The type of Body that was hit.
CollisionType Collision::GetCollisionType() const
{
	return collisionType;
}



// The intersection range at which the collision occurred with the Body.
double Collision::IntersectionRange() const
{
	return range;
}



// Compare two Collisions by their intersection range.
bool Collision::operator<(const Collision &rhs) const
{
	return range < rhs.range;
}
