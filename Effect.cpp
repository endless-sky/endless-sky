/* Effect.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Effect.h"

#include "SpriteSet.h"



Effect::Effect()
	: velocityScale(1.), randomVelocity(1),
	randomAngle(0.), randomSpin(0.), lifetime(0)
{
}



const std::string &Effect::Name() const
{
	return name;
}



void Effect::Load(const DataFile::Node &node)
{
	if(node.Size() > 1)
		name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "sprite")
			animation.Load(child);
		if(child.Token(0) == "lifetime" && child.Size() >= 2)
			lifetime = child.Value(1);
		if(child.Token(0) == "velocity scale" && child.Size() >= 2)
			velocityScale = child.Value(1);
		if(child.Token(0) == "random velocity" && child.Size() >= 2)
			randomVelocity = child.Value(1) * 1000. + 1.;
		if(child.Token(0) == "random angle" && child.Size() >= 2)
			randomAngle = child.Value(1);
		if(child.Token(0) == "random spin" && child.Size() >= 2)
			randomSpin = child.Value(1);
	}
}



// If creating a new effect, the animation and lifetime are copied,
// but position, velocity, and angle are specific to this new effect.
void Effect::Place(Point pos, Point vel, Angle angle)
{
	this->angle = angle + Angle::Random(randomAngle) - Angle::Random(randomAngle);
	spin = Angle::Random(randomSpin) - Angle::Random(randomSpin);
	
	position = pos;
	velocity = vel * velocityScale
		+ this->angle.Unit() * ((rand() % randomVelocity) * .001);
}



// This returns false if it is time to delete this effect.
bool Effect::Move()
{
	if(lifetime-- <= 0)
		return false;
	
	position += velocity;
	angle += spin;
	
	return true;
}



// Get the projectiles characteristics, for drawing.
const Animation &Effect::GetSprite() const
{
	return animation;
}



const Point &Effect::Position() const
{
	return position;
}



// Get the facing unit vector times the scale factor.
Point Effect::Unit() const
{
	return angle.Unit() * .5;
}
