/* DrawList.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DRAW_LIST_H_
#define DRAW_LIST_H_

#include "Point.h"

#include <cstdint>
#include <vector>

class Body;
class Sprite;



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
	bool Add(const Body &body, Point position);
	bool AddUnblurred(const Body &body);
	bool AddProjectile(const Body &body, const Point &adjustedVelocity, double clip);
	bool AddSwizzled(const Body &body, int swizzle);
	
	// Draw all the items in this list.
	void Draw() const;
	
	
private:
	bool Cull(const Body &body, const Point &position, const Point &blur) const;
	
	void Push(const Body &body, Point pos, Point blur, double cloak, double clip, int swizzle);
	
	
private:
	class Item {
	public:
		// Get the color swizzle.
		uint32_t Swizzle() const;
		
		float Clip() const;
		float Fade() const;
		
		void Cloak(double cloak);
		
	public:
		uint32_t tex0;
		uint32_t tex1;
		float position[2];
		float transform[4];
		float blur[2];
		float clip;
		uint32_t flags;
	};
	
	
private:
	int step = 0;
	double zoom = 1.;
	bool isHighDPI = false;
	std::vector<Item> items;
	
	Point center;
	Point centerVelocity;
};



#endif
