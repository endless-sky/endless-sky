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

#include "Body.h"

#include <string>

class DataNode;
class Sound;



// Class representing the template for a visual effect such as an explosion,
// which is drawn for effect but has no impact on any other objects in the
// game. Because this class is more heavyweight than it needs to be, actual
// visual effects when they are placed use the Visual class, based on the
// template provided by an Effect.
class Effect : public Body {
public:
	/* Functions provided by the Body base class:
	Frame GetFrame(int step = -1) const;
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	Point Unit() const;
	double Zoom() const;
	*/
	
	const std::string &Name() const;
	void SetName(const std::string &name);
	
	void Load(const DataNode &node);
	
	
private:
	std::string name;
	
	const Sound *sound = nullptr;
	
	// Parameters used for randomizing spin and velocity. The random angle is
	// added to the parent angle, and then a random velocity in that direction
	// is added to the parent velocity.
	double velocityScale = 1.;
	double randomVelocity = 0.;
	double randomAngle = 0.;
	double randomSpin = 0.;
	double randomFrameRate = 0.;
	
	int lifetime = 0;
	int randomLifetime = 0;
	
	// Allow the Visual class to access all these private members.
	friend class Visual;
};



#endif
