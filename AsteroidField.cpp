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
#include "GameData.h"
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



void AsteroidField::Draw(DrawList &draw, const Point &center, const Point &centerVelocity) const
{
	for(const Asteroid &asteroid : asteroids)
		asteroid.Draw(draw, center, centerVelocity);
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
	: animation(sprite, Random::Real() * 4. * energy + 5.)
{
	location = Point(Random::Int() & WRAP_MASK, Random::Int() & WRAP_MASK);
	
	angle = Angle::Random(360.);
	spin = Angle((Random::Real() * 2. - 1.) * energy);
	
	velocity = angle.Unit() * Random::Real() * energy;
}



void AsteroidField::Asteroid::Step()
{
	angle += spin;
	location += velocity;
	
	if(location.X() < 0.)
		location = Point(location.X() + WRAP, location.Y());
	else if(location.X() >= WRAP)
		location = Point(location.X() - WRAP, location.Y());
	
	if(location.Y() < 0.)
		location = Point(location.X(), location.Y() + WRAP);
	else if(location.Y() >= WRAP)
		location = Point(location.X(), location.Y() - WRAP);
}



void AsteroidField::Asteroid::Draw(DrawList &draw, const Point &center, const Point &centerVelocity) const
{
	Point pos = location - center;
	pos = Point(remainder(pos.X(), WRAP), remainder(pos.Y(), WRAP));
	
	draw.Add(animation, pos, angle.Unit() * .5, velocity - centerVelocity);
}



double AsteroidField::Asteroid::Collide(const Projectile &projectile, int step) const
{
	Point pos = location - projectile.Position();
	pos = Point(-remainder(pos.X(), WRAP), -remainder(pos.Y(), WRAP));
	
	return animation.GetMask(step).Collide(pos, projectile.Velocity(), angle);
}



Point AsteroidField::Asteroid::Velocity() const
{
	return velocity;
}
