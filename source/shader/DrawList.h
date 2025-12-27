/* DrawList.h
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

#pragma once

#include "../Point.h"
#include "SpriteShader.h"

#include <cstdint>
#include <vector>

class Body;
class Sprite;
class Swizzle;



// Class for storing a list of textures to blit to the screen. This allows the
// work of calculating the transformation matrices to be done in a separate
// thread from the graphics thread. However, the SpriteShader class is also
// available for drawing individual sprites in contexts where putting them into
// a DrawList first does not make sense.
class DrawList {
public:
	// Clear the list, also setting the global time step for animation.
	void Clear(int step = 0, double zoom = 1.);
	void SetCenter(const Point &center, const Point &centerVelocity = Point());

	// Add an object based on the Body class.
	bool Add(const Body &body, double cloak = 0.);
	// Add an object at the given position (rather than its own).
	bool Add(const Body &body, Point position, double cloak = 0.);

	// Add an object that should not be drawn with motion blur.
	bool AddUnblurred(const Body &body);
	// Add an object using a specific swizzle (rather than its own).
	bool AddSwizzled(const Body &body, const Swizzle *swizzle, double cloak = 0.);

	// Draw all the items in this list.
	void Draw() const;


private:
	// Determine if the given object should be drawn at all.
	bool Cull(const Body &body, const Point &position, const Point &blur) const;

	void Push(const Body &body, Point pos, Point blur, double cloak, const Swizzle *swizzle);


private:
	int step = 0;
	double zoom = 1.;
	bool isHighDPI = false;
	std::vector<SpriteShader::Item> items;

	Point center;
	Point centerVelocity;
};
