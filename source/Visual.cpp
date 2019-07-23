/* Visual.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Visual.h"

#include "Audio.h"
#include "Effect.h"
#include "Random.h"

using namespace std;



// Generate a visual based on the given Effect.
Visual::Visual(const Effect &effect, Point pos, Point vel, Angle facing, Point hitVelocity)
	: Body(effect, pos, vel, facing), lifetime(effect.lifetime)
{
	angle += Angle::Random(effect.randomAngle) - Angle::Random(effect.randomAngle);
	spin = Angle::Random(effect.randomSpin) - Angle::Random(effect.randomSpin);
	
	velocity *= effect.velocityScale;
	velocity += hitVelocity * (1. - effect.velocityScale);
	if(effect.randomVelocity)
		velocity += angle.Unit() * Random::Real() * effect.randomVelocity;
	
	if(effect.sound)
		Audio::Play(effect.sound, position);
	
	if(effect.randomFrameRate)
		AddFrameRate(effect.randomFrameRate * Random::Real());
}



// Step the effect forward.
void Visual::Move()
{
	if(lifetime-- <= 0)
		MarkForRemoval();
	else
	{
		position += velocity;
		angle += spin;
	}
}
