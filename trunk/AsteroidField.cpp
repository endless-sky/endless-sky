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
#include "Projectile.h"

#include <cmath>
#include <cstdlib>

using namespace std;

namespace {
	static const int WRAP_MASK = 4095;
	static const double WRAP = (WRAP_MASK + 1);
}



AsteroidField::AsteroidField(const GameData &gameData)
	: gameData(gameData)
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



double AsteroidField::Collide(const Projectile &projectile, int step) const
{
	double distance = 1.;
	for(const Asteroid &asteroid : asteroids)
		distance = min(distance, asteroid.Collide(projectile, step));
	
	return distance;
}



AsteroidField::Asteroid::Asteroid(const Sprite *sprite, double energy)
	: animation(sprite, (rand() % 1000) * .004 * energy + 5.)
{
	location = Point(rand() & WRAP_MASK, rand() & WRAP_MASK);
	
	angle = Angle::Random(360.);
	spin = Angle((rand() % 2001 - 1000) * .001 * energy);
	
	velocity = angle.Unit() * (rand() % 1001) * .001 * energy;
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



void AsteroidField::Asteroid::Draw(DrawList &draw, const Point &center) const
{
	Point pos = location - center;
	pos = Point(remainder(pos.X(), WRAP), remainder(pos.Y(), WRAP));
	
	draw.Add(animation, pos, angle.Unit() * .5);
}



double AsteroidField::Asteroid::Collide(const Projectile &projectile, int step) const
{
	Point pos = location - projectile.Position();
	pos = Point(-remainder(pos.X(), WRAP), -remainder(pos.Y(), WRAP));
	
	return animation.GetMask(step).Collide(pos, projectile.Velocity(), angle);
}
