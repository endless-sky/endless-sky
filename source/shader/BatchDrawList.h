/* BatchDrawList.h
Copyright (c) 2017 by Michael Zahniser

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

#include <map>
#include <vector>

class Body;
class Sprite;



// This class collects a set of OpenGL draw commands to issue and groups them by
// sprite, so all instances of each sprite can be drawn with a single command.
class BatchDrawList {
public:
	// Clear the list, also setting the global time step for animation.
	void Clear(int step = 0, double zoom = 1.);
	void SetCenter(const Point &center);

	// Add an unswizzled object based on the Body class.
	bool Add(const Body &body, float clip = 1.f);
	bool AddVisual(const Body &visual);

	// Draw all the items in this list.
	void Draw() const;


private:
	// Determine if the given body should be drawn at all.
	bool Cull(const Body &body, const Point &position) const;

	// Add the given body at the given position.
	bool Add(const Body &body, Point position, float clip);


private:
	int step = 0;
	double zoom = 1.;
	bool isHighDPI = false;
	Point center;

	// Each sprite consists of six vertices (four vertices to form a quad and
	// two dummy vertices to mark the break in between them). Each of those
	// vertices has six attributes: (x, y) position in pixels, (s, t) texture
	// coordinates, the index of the sprite frame, and the alpha value.
	std::map<const Sprite *, std::vector<float>> data;
};
