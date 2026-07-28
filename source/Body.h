/* Body.h
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

#pragma once

#include "Drawable.h"

#include "Angle.h"
#include "Point.h"

class Government;
class Mask;
class Sprite;



// Class representing any object in the game that has a position, velocity, and
// facing direction and usually also has a sprite.
class Body : public Drawable {
public:
	// Constructors.
	Body() = default;
	Body(const Sprite *sprite, Point position, Point velocity = Point(), Angle facing = Angle(),
		double zoom = 1., Point scale = Point(1., 1.), double alpha = 1.);
	Body(const Body &other, Point position, Point velocity = Point(), Angle facing = Angle(),
		double zoom = 1., Point scale = Point(1., 1.), double alpha = 1.);

	// Get the sprite mask for the given time step.
	const Mask &GetMask(int step = -1) const;

	// Positional attributes.
	const Point &Position() const;
	const Point &Velocity() const;
	Point Center() const;
	const Angle &Facing() const;
	Point Unit() const;

	// Check if this object is marked for removal from the game.
	bool ShouldBeRemoved() const;

	// Store the government here too, so that collision detection that is based
	// on the Body class can figure out which objects will collide.
	const Government *GetGovernment() const;

	// Functions determining the current alpha value of the body,
	// dependent on the position of the body relative to the center of the screen.
	double Alpha(const Point &drawCenter) const;
	double DistanceAlpha(const Point &drawCenter) const;
	bool IsVisible(const Point &drawCenter) const;


protected:
	// Mark this object to be removed from the game.
	void MarkForRemoval();
	// Mark that this object should not be removed (e.g. a launched fighter).
	void UnmarkForRemoval();
	// Turn this object around its center of rotation.
	void Turn(double amount);
	void Turn(const Angle &amount);


protected:
	// Basic positional attributes.
	Point position;
	Point velocity;
	Angle angle;
	Point rotatedCenter;

	// The maximum distance at which the body is visible, and at which it becomes invisible again.
	double distanceVisible = 0.;
	double distanceInvisible = 0.;

	// Government, for use in collision checks.
	const Government *government = nullptr;


private:
	// Record when this object is marked for removal from the game.
	bool shouldBeRemoved = false;
};
