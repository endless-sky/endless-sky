/* Effect.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef EFFECT_H_
#define EFFECT_H_

#include "Angle.h"
#include "Animation.h"
#include "Point.h"

#include <string>

class DataNode;
class Sound;



// Class representing a graphic such as an explosion which is drawn for effect but
// has no impact on any other objects in the game.
class Effect {
public:
	Effect();
	
	const std::string &Name() const;
	
	void Load(const DataNode &node);
	// If creating a new effect, the animation and lifetime are copied,
	// but position, velocity, and angle are specific to this new effect.
	void Place(Point pos, Point vel, Angle angle, Point hitVelocity = Point());
	
	// This returns false if it is time to delete this effect.
	bool Move();
	
	// Get the projectiles characteristics, for drawing.
	const Animation &GetSprite() const;
	const Point &Position() const;
	const Point &Velocity() const;
	Point Unit() const;
	
	
private:
	std::string name;
	
	Animation animation;
	const Sound *sound;
	
	Point position;
	Point velocity;
	Angle angle;
	Angle spin;
	
	// Parameters used for randomizing spin and velocity. The random angle is
	// added to the parent angle, and then a random velocity in that direction
	// is added to the parent velocity.
	double velocityScale;
	double randomVelocity;
	double randomAngle;
	double randomSpin;
	double randomFrameRate;
	
	int lifetime;
};



#endif
