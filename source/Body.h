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

#include <string>

class Government;
class Sprite;



// Class representing any object in the game that has a position, velocity, and
// facing direction and usually also has a sprite.
class Body : public Drawable {
public:
	// Constructors.
	Body() = default;
	Body(const Sprite *sprite, Point position, Point velocity = Point(), Angle facing = Angle(),
		double zoom = 1., Point scale = Point(1., 1.), double alpha = 1.);
	Body(const Body &sprite, Point position, Point velocity = Point(), Angle facing = Angle(),
		double zoom = 1., Point scale = Point(1., 1.), double alpha = 1.);

	// Positional attributes.
	const Point &Position() const;
	const Point &Velocity() const;
	const Point Center() const;
	const Angle &Facing() const;
	Point Unit() const;

	// Store the government here too, so that collision detection that is based
	// on the Body class can figure out which objects will collide.
	const Government *GetGovernment() const;

	// Functions determining the current alpha value of the body,
	// dependent on the position of the body relative to the center of the screen.
	double Alpha(const Point &drawCenter) const;
	double DistanceAlpha(const Point &drawCenter) const;
	bool IsVisible(const Point &drawCenter) const;


protected:
	// Turn this object around its center of rotation.
	void Turn(double amount);
	void Turn(const Angle &amount);


protected:
	// Basic positional attributes.
	Point position;
	Point velocity;
	Angle angle;
	Point center;
	Point rotatedCenter;

	// The maximum distance at which the body is visible, and at which it becomes invisible again.
	double distanceVisible = 0.;
	double distanceInvisible = 0.;

	// Government, for use in collision checks.
	const Government *government = nullptr;


private:
	// Set what animation step we're on. This affects future calls to GetMask()
	// and GetFrame().
	void SetStep(int step) const;
};
