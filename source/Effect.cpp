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

#include "Audio.h"
#include "DataNode.h"
#include "Random.h"

using namespace std;



const string &Effect::Name() const
{
	return name;
}



void Effect::Load(const DataNode &node)
{
	if(node.Size() > 1)
		name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sprite")
			LoadSprite(child);
		else if(child.Token(0) == "sound" && child.Size() >= 2)
			sound = Audio::Get(child.Token(1));
		else if(child.Token(0) == "lifetime" && child.Size() >= 2)
			lifetime = child.Value(1);
		else if(child.Token(0) == "velocity scale" && child.Size() >= 2)
			velocityScale = child.Value(1);
		else if(child.Token(0) == "random velocity" && child.Size() >= 2)
			randomVelocity = child.Value(1);
		else if(child.Token(0) == "random angle" && child.Size() >= 2)
			randomAngle = child.Value(1);
		else if(child.Token(0) == "random spin" && child.Size() >= 2)
			randomSpin = child.Value(1);
		else if(child.Token(0) == "random frame rate" && child.Size() >= 2)
			randomFrameRate = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// If creating a new effect, the animation and lifetime are copied,
// but position, velocity, and angle are specific to this new effect.
void Effect::Place(Point pos, Point vel, Angle facing, Point hitVelocity)
{
	angle = facing + Angle::Random(randomAngle) - Angle::Random(randomAngle);
	spin = Angle::Random(randomSpin) - Angle::Random(randomSpin);
	
	position = pos;
	velocity = (vel - hitVelocity) * velocityScale + hitVelocity
		+ angle.Unit() * Random::Real() * randomVelocity;
	
	if(sound)
		Audio::Play(sound, position);
	
	if(randomFrameRate)
		AddFrameRate(randomFrameRate * Random::Real());
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
