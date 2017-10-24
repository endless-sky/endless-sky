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
#include "Screen.h"
#include "Sprite.h"

#include "spatial/bits/spatial_overlap_region.hpp"

#include <cmath>
#include <cstdlib>

using namespace std;

namespace {
	const double WRAP = 4096.;
	const double BEFORE_WRAP = WRAP - numeric_limits<double>::epsilon();
	const int LOW_X = 0;
	const int LOW_Y = 1;
	const int HIGH_X = 2;
	const int HIGH_Y = 3;

	// Splits a box into 1 to 4 wrapped boxes.
	// It assumes the box is overlapping the wrap region.
	vector<box_bounds_t> SplitBox(const box_bounds_t &base)
	{
		vector<box_bounds_t> splits;
		splits.push_back(base);
		for(size_t i = 0; i < splits.size(); ++i)
		{
			box_bounds_t &box = splits[i];
			if(box[LOW_X] < 0.)
			{
				box_bounds_t split = { box[LOW_X] + WRAP, box[LOW_Y], BEFORE_WRAP, box[HIGH_Y] };
				splits.push_back(split);
				box[LOW_X] = 0.;
			}
			if(box[LOW_Y] < 0.)
			{
				box_bounds_t split = { box[LOW_X], box[LOW_Y] + WRAP, box[HIGH_X], BEFORE_WRAP };
				splits.push_back(split);
				box[LOW_Y] = 0.;
			}
			if(box[HIGH_X] >= WRAP)
			{
				box_bounds_t split = { 0., box[LOW_Y], box[HIGH_X] - WRAP, box[HIGH_Y] };
				splits.push_back(split);
				box[HIGH_X] = BEFORE_WRAP;
			}
			if(box[HIGH_Y] >= WRAP)
			{
				box_bounds_t split = { box[LOW_X], 0., box[HIGH_X], box[HIGH_Y] - WRAP };
				splits.push_back(split);
				box[HIGH_Y] = BEFORE_WRAP;
			}
		}
		return splits;
	}
	
	// Box around an asteroid.
	// It assumes the position is inside the wrap region.
	box_bounds_t AsteroidBox(const Point &position, const Point &size)
	{
		box_bounds_t box = {
			position.X() - size.X(),
			position.Y() - size.Y(),
			position.X() + size.X(),
			position.Y() + size.Y()
		};
		
		return box;
	}
	
	// Box around the body movement.
	box_bounds_t MovementBox(const Point &position, const Point &velocity)
	{
		box_bounds_t box;
		
		// Wrap X
		if(abs(velocity.X()) < WRAP)
		{
			double x = fmod(position.X(), WRAP);
			if(x < 0.)
				x += WRAP;
			box[LOW_X] = x;
			box[HIGH_X] = x;
			box[velocity.X() < 0. ? LOW_X : HIGH_X] += velocity.X();
		}
		else
		{
			box[LOW_X] = 0.;
			box[HIGH_X] = BEFORE_WRAP;
		}
		
		// Wrap Y
		if(abs(velocity.Y()) < WRAP)
		{
			double y = fmod(position.Y(), WRAP);
			if(y < 0.)
				y += WRAP;
			box[LOW_Y] = y;
			box[HIGH_Y] = y;
			box[velocity.Y() < 0. ? LOW_Y : HIGH_Y] += velocity.Y();
		}
		else
		{
			box[LOW_Y] = 0.;
			box[HIGH_Y] = BEFORE_WRAP;
		}
		
		return box;
	}
}



// Clear the list of asteroids.
void AsteroidField::Clear()
{
	asteroidBounds.clear();
	asteroids.clear();
	minables.clear();
}



// Add a new asteroid to the list, using the sprite with the given name.
void AsteroidField::Add(const string &name, int count, double energy)
{
	const Sprite *sprite = SpriteSet::Get("asteroid/" + name + "/spin");
	for(int i = 0; i < count; ++i)
		asteroids.emplace_back(sprite, energy);
}



void AsteroidField::Add(const Minable *minable, int count, double energy, double beltRadius)
{
	// Double check that the given asteroid is defined.
	if(!minable || !minable->GetMask().IsLoaded())
		return;
	
	// Place copies of the given minable asteroid throughout the system.
	for(int i = 0; i < count; ++i)
	{
		minables.emplace_back(new Minable(*minable));
		minables.back()->Place(energy, beltRadius);
	}
}



// Move all the asteroids forward one step.
void AsteroidField::Step(vector<Visual> &visuals, list<shared_ptr<Flotsam>> &flotsam)
{
	asteroidBounds.clear();
	for(Asteroid &asteroid : asteroids)
	{
		asteroid.Step();
		for(box_bounds_t &box : SplitBox(AsteroidBox(asteroid.Position(), asteroid.Size())))
			asteroidBounds.insert(std::make_pair(box, &asteroid));
	}
	asteroidBounds.rebalance();
	
	// Step through the minables. Since they are destructible, we may need to
	// remove them from the list.
	auto it = minables.begin();
	while(it != minables.end())
	{
		if((*it)->Move(visuals, flotsam))
			++it;
		else
			it = minables.erase(it);
	}
}



// Draw the asteroids, centered on the given location.
void AsteroidField::Draw(DrawList &draw, const Point &center, double zoom) const
{
	for(const Asteroid &asteroid : asteroids)
		asteroid.Draw(draw, center, zoom);
	for(const shared_ptr<Minable> &minable : minables)
		draw.Add(*minable);
}



// Check if the given projectile collides with any asteroids.
double AsteroidField::Collide(const Projectile &projectile, int step, double closestHit, Point *hitVelocity)
{
	// First, check for collisions with ordinary asteroids.
	for(box_bounds_t &overlap : SplitBox(MovementBox(projectile.Position(), projectile.Velocity())))
	{
		for(auto it = spatial::overlap_region_begin(asteroidBounds, overlap);
			it != spatial::overlap_region_end(asteroidBounds, overlap); ++it)
		{
			const Asteroid *asteroid = it->second;
			double thisDistance = asteroid->Collide(projectile, step);
			if(thisDistance < closestHit)
			{
				closestHit = thisDistance;
				if(hitVelocity)
					*hitVelocity = asteroid->Velocity();
			}
		}
	}
	
	// Now, check for collisions with minable asteroids. Because this is the
	// very last collision check to be done, if a minable asteroid is the
	// closest hit, it really is what the projectile struck - that is, we are
	// not going to later find a ship or something else that is closer.
	Minable *hit = nullptr;
	for(const shared_ptr<Minable> &minable : minables)
	{
		double thisDistance = minable->Collide(projectile, step);
		if(thisDistance < closestHit)
		{
			closestHit = thisDistance;
			hit = &*minable;
			if(hitVelocity)
				*hitVelocity = minable->Velocity();
		}
	}
	if(hit)
		hit->TakeDamage(projectile);
	
	return closestHit;
}



// Get the list of mainable asteroids.
const list<shared_ptr<Minable>> &AsteroidField::Minables() const
{
	return minables;
}



// Construct an asteroid with the given sprite and "energy level."
AsteroidField::Asteroid::Asteroid(const Sprite *sprite, double energy)
{
	// Energy level determines how fast the asteroid rotates.
	SetSprite(sprite);
	SetFrameRate(Random::Real() * 4. * energy + 5.);
	
	// Pick a random position within the wrapped square.
	position = Point(Random::Real() * WRAP, Random::Real() * WRAP);
	
	// In addition to the "spin" inherent in the animation, the asteroid should
	// spin in screen coordinates. This makes the animation more interesting
	// because every time it comes back to the same frame it is pointing in a
	// new direction, so the asteroids don't all appear to be spinning on
	// exactly the same axis.
	angle = Angle::Random();
	spin = Angle::Random(energy) - Angle::Random(energy);
	
	// The asteroid's velocity is also determined by the energy level.
	velocity = angle.Unit() * Random::Real() * energy;
	
	// Store how big an area the asteroid can cover, so we can figure out when
	// it is potentially on screen.
	size = Point(1., 1.) * Radius();
}



// Move the asteroid forward one time step.
void AsteroidField::Asteroid::Step()
{
	angle += spin;
	position += velocity;
	
	// Keep the position within the wrap square.
	if(position.X() < 0.)
		position = Point(position.X() + WRAP, position.Y());
	else if(position.X() >= WRAP)
		position = Point(position.X() - WRAP, position.Y());
	
	if(position.Y() < 0.)
		position = Point(position.X(), position.Y() + WRAP);
	else if(position.Y() >= WRAP)
		position = Point(position.X(), position.Y() - WRAP);
}



// Draw any instances of this asteroid that are on screen.
void AsteroidField::Asteroid::Draw(DrawList &draw, const Point &center, double zoom) const
{
	// Any object within this range must be drawn.
	Point topLeft = center + (Screen::TopLeft() - size) / zoom;
	Point bottomRight = center + (Screen::BottomRight() + size) / zoom;
	
	// Figure out the position of the first instance of this asteroid that is to
	// the right of and below the top left corner of the screen.
	double startX = fmod(position.X() - topLeft.X(), WRAP);
	startX += topLeft.X() + WRAP * (startX < 0.);
	double startY = fmod(position.Y() - topLeft.Y(), WRAP);
	startY += topLeft.Y() + WRAP * (startY < 0.);
	
	// Draw any instances of this asteroid that are on screen.
	for(double y = startY; y < bottomRight.Y(); y += WRAP)
		for(double x = startX; x < bottomRight.X(); x += WRAP)
			draw.Add(*this, Point(x, y));
}



// Check if the given projectile collides with this asteroid. If so, a value
// less than 1 is returned indicating how far along its path the collision occurs.
double AsteroidField::Asteroid::Collide(const Projectile &projectile, int step) const
{
	// Note: this only checks the instance of the asteroid that is closest to
	// the projectile. If a projectile has a range longer than 4096 pixels, it
	// may "pass through" asteroids near the end of its range.
	
	// Find the asteroid instance closest to the center of the projectile,
	// i.e. where it will be when halfway through its path.
	Point halfVelocity = .5 * projectile.Velocity();
	Point pos = position - (projectile.Position() + halfVelocity);
	pos = Point(-remainder(pos.X(), WRAP), -remainder(pos.Y(), WRAP));
	
	return GetMask(step).Collide(pos - halfVelocity, projectile.Velocity(), angle);
}



const Point &AsteroidField::Asteroid::Size() const
{
	return size;
}
