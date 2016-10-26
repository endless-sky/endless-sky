/* AsteroidField.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "AsteroidField.h"

#include "SpriteSet.h"
#include "DrawList.h"
#include "Mask.h"
#include "Projectile.h"
#include "Random.h"

#include <cmath>
#include <cstdlib>

using namespace std;

namespace {
	static const int WRAP_MASK = 4095;
	static const double WRAP = (WRAP_MASK + 1);
}



AsteroidField::AsteroidField()
{
}



void AsteroidField::Clear()
{
	asteroids.clear();
}



void AsteroidField::Add(const string &name, int count, double energy)
{
	const Sprite *sprite = SpriteSet::Get("asteroid/" + name + "/spin");
	for(int i = 0; i < count; ++i)
		asteroids.emplace_back(sprite, energy);
}



void AsteroidField::Step()
{
	for(Asteroid &asteroid : asteroids)
		asteroid.Step();
}



void AsteroidField::Draw(DrawList &draw, const Point &center) const
{
	for(const Asteroid &asteroid : asteroids)
		asteroid.Draw(draw, center);
}



double AsteroidField::Collide(const Projectile &projectile, int step, Point *hitVelocity) const
{
	double distance = 1.;
	for(const Asteroid &asteroid : asteroids)
	{
		double thisDistance = asteroid.Collide(projectile, step);
		if(thisDistance < distance)
		{
			distance = thisDistance;
			if(hitVelocity)
				*hitVelocity = asteroid.Velocity();
		}
	}
	
	return distance;
}



AsteroidField::Asteroid::Asteroid(const Sprite *sprite, double energy)
{
	SetSprite(sprite);
	SetFrameRate(Random::Real() * 4. * energy + 5.);
	
	position = Point(Random::Int() & WRAP_MASK, Random::Int() & WRAP_MASK);
	
	angle = Angle::Random();
	spin = Angle::Random(energy) - Angle::Random(energy);
	
	velocity = angle.Unit() * Random::Real() * energy;
}



void AsteroidField::Asteroid::Step()
{
	angle += spin;
	position += velocity;
	
	if(position.X() < 0.)
		position = Point(position.X() + WRAP, position.Y());
	else if(position.X() >= WRAP)
		position = Point(position.X() - WRAP, position.Y());
	
	if(position.Y() < 0.)
		position = Point(position.X(), position.Y() + WRAP);
	else if(position.Y() >= WRAP)
		position = Point(position.X(), position.Y() - WRAP);
}



void AsteroidField::Asteroid::Draw(DrawList &draw, const Point &center) const
{
	Point pos(
		remainder(position.X() - center.X(), WRAP),
		remainder(position.Y() - center.Y(), WRAP));
	
	draw.Add(*this, pos + center);
}



double AsteroidField::Asteroid::Collide(const Projectile &projectile, int step) const
{
	Point pos = position - projectile.Position();
	pos = Point(-remainder(pos.X(), WRAP), -remainder(pos.Y(), WRAP));
	
	return GetMask(step).Collide(pos, projectile.Velocity(), angle);
}
