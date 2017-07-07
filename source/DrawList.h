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
	DrawList();
	// Clear the list, also setting the global time step for animation.
	void Clear(int step = 0, double zoom = 1.);
	void SetCenter(const Point &center, const Point &centerVelocity = Point());
	void setAmbiant(const float amb[3]);
	
	// Add an object based on the Body class.
	bool Add(const Body &body, double cloak = 0., float normUse = 0.f);
	bool Add(const Body &body, Point position, float normUse = 0.f);
	bool AddUnblurred(const Body &body, float normUse = 0.f);
	bool AddProjectile(const Body &body, const Point &adjustedVelocity, double clip, bool lighted = false, float normUse = 0.f);
	bool AddSwizzled(const Body &body, int swizzle, float normUse = 0.f);
	bool AddUnlighted(const Body& body);
	bool AddEffect(const Body& body);
	bool AddStar(const Body& body, bool blur);

	bool AddLightSource(const float pos[3], const float emit[3]);
	
	// Draw all the items in this list.
	void Draw(bool lights = false) const;
	
	
private:
	bool Cull(const Body &body, const Point &position, const Point &blur) const;
	
	void Push(const Body &body, Point posRel, Point posGS, Point blurRel, double cloak, double clip, int swizzle, bool lighted = false, float normUse = 0.f);
	bool CullPush(const Body &body, Point posGS, Point vel, double cloak, double clip, int swizzle, bool lighted = false, float normUse = 0.f);
	
	
private:
	class Item {
	public:
		//constants for the flags
		static const uint32_t swizzle_mask;
		static const uint32_t fade_mask;
		static const float fade_factor;
		static const int fade_shift;
		static const float normal_factor;
		static const int normal_shift;
		static const uint32_t normal_mask;
		static const uint32_t light_mask;
		static const int light_shift;
		// Get the color swizzle.
		uint32_t Swizzle() const;
		
		float Clip() const;
		float Fade() const;
		bool Lighted() const;
		float NormalUse() const;
		
		void Cloak(double cloak);
		
	public:
		uint32_t tex0;
		uint32_t tex1;
		float position[2];
		float transform[4];
		float posGS[2];
		float transformGS[4];
		float blur[2];
		float clip;
		uint32_t flags;
	};
	
	
private:
	int step = 0;
	double zoom = 1.;
	bool isHighDPI = false;
	std::vector<Item> items;
	std::vector<float> lightsPos;
	std::vector<float> lightsEmit;
	
	Point center;
	Point centerVelocity;
	float lightAmbiant[3];
};



#endif
