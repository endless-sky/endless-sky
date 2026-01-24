/* Body.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Body.h"

#include "pi.h"
#include "image/Sprite.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Constructor, based on a Sprite.
Body::Body(const Sprite *sprite, Point position, Point velocity, Angle facing, double zoom, Point scale, double alpha)
	: Drawable(sprite, zoom, scale, alpha), position(position), velocity(velocity), angle(facing)
{
}



// Constructor, based on the animation from another Body object.
Body::Body(const Body &sprite, Point position, Point velocity, Angle facing, double zoom, Point scale, double alpha)
	: Drawable(sprite, zoom, scale, alpha)
{
	this->position = position;
	this->velocity = velocity;
	this->angle = facing;
}



// Position, in world coordinates (zero is the system center).
const Point &Body::Position() const
{
	return position;
}



// Velocity, in pixels per second.
const Point &Body::Velocity() const
{
	return velocity;
}



const Point Body::Center() const
{
	return -rotatedCenter + position;
}



// Direction this Body is facing in.
const Angle &Body::Facing() const
{
	return angle;
}



// Unit vector in the direction this body is facing. This represents the scale
// and transform that should be applied to the sprite before drawing it.
Point Body::Unit() const
{
	return angle.Unit() * (.5 * Zoom());
}



// Store the government here too, so that collision detection that is based
// on the Body class can figure out which objects will collide.
const Government *Body::GetGovernment() const
{
	return government;
}



double Body::Alpha(const Point &drawCenter) const
{
	return alpha * DistanceAlpha(drawCenter);
}



double Body::DistanceAlpha(const Point &drawCenter) const
{
	if(!distanceInvisible)
		return 1.;
	double distance = (drawCenter - position).Length();
	return clamp<double>((distance - distanceInvisible) / (distanceVisible - distanceInvisible), 0., 1.);
}



bool Body::IsVisible(const Point &drawCenter) const
{
	return DistanceAlpha(drawCenter) > 0.;
}



// Turn this object around its center of rotation.
void Body::Turn(double amount)
{
	angle += amount;
	if(!center)
		return;

	auto RotatePointAroundOrigin = [](Point &toRotate, double radians) -> Point {
		float si = sin(radians);
		float co = cos(radians);
		float newX = toRotate.X() * co - toRotate.Y() * si;
		float newY = toRotate.X() * si + toRotate.Y() * co;
		return Point(newX, newY);
	};

	rotatedCenter = -RotatePointAroundOrigin(center, (angle - amount).Degrees() * TO_RAD);

	position -= rotatedCenter;

	rotatedCenter = RotatePointAroundOrigin(rotatedCenter, Angle(amount).Degrees() * TO_RAD);

	position += rotatedCenter;
}



void Body::Turn(const Angle &amount)
{
	Turn(amount.Degrees());
}
