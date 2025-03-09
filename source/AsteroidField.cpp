/* AsteroidField.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AsteroidField.h"

#include "Collision.h"
#include "CollisionType.h"
#include "shader/DrawList.h"
#include "image/Mask.h"
#include "Minable.h"
#include "Projectile.h"
#include "Random.h"
#include "Screen.h"
#include "image/SpriteSet.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace std;

namespace {
	constexpr double WRAP = 4096.;
	constexpr unsigned CELL_SIZE = 256u;
	constexpr unsigned CELL_COUNT = WRAP / CELL_SIZE;
}



// Constructor, to set up the collision set parameters.
AsteroidField::AsteroidField()
	: asteroidCollisions(CELL_SIZE, CELL_COUNT, CollisionType::ASTEROID),
	minableCollisions(CELL_SIZE, CELL_COUNT, CollisionType::MINABLE)
{
}



// Clear the list of asteroids.
void AsteroidField::Clear()
{
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



void AsteroidField::Add(const Minable *minable, int count, double energy, const WeightedList<double> &belts)
{
	// Double check that the given asteroid is defined.
	if(!minable || !minable->GetMask().IsLoaded())
		return;

	// Place copies of the given minable asteroid throughout the system.
	for(int i = 0; i < count; ++i)
	{
		minables.emplace_back(new Minable(*minable));
		minables.back()->Place(energy, belts.Get());
	}
}



// Move all the asteroids forward one step.
void AsteroidField::Step(vector<Visual> &visuals, list<shared_ptr<Flotsam>> &flotsam, int step)
{
	asteroidCollisions.Clear(step);
	for(Asteroid &asteroid : asteroids)
	{
		asteroidCollisions.Add(asteroid);
		asteroid.Step();
	}
	asteroidCollisions.Finish();

	// Step through the minables. Since they are destructible, we may need to
	// remove them from the list.
	minableCollisions.Clear(step);
	auto it = minables.begin();
	while(it != minables.end())
	{
		if((*it)->Move(visuals, flotsam))
		{
			minableCollisions.Add(**it);
			++it;
		}
		else
			it = minables.erase(it);
	}
	minableCollisions.Finish();
}



// Draw the asteroids, centered on the given location.
void AsteroidField::Draw(DrawList &draw, const Point &center, double zoom) const
{
	for(const Asteroid &asteroid : asteroids)
		asteroid.Draw(draw, center, zoom);
	for(const shared_ptr<Minable> &minable : minables)
		draw.Add(*minable);
}



// Check if the given projectile collides with any asteroids. This excludes minables.
void AsteroidField::CollideAsteroids(const Projectile &projectile, vector<Collision> &result) const
{
	// Check for collisions with ordinary asteroids, which are tiled.
	// Rather than tiling the collision set, tile the projectile.
	Point from = projectile.Position();
	Point to = from + projectile.Velocity();

	// Map the projectile to a position within the wrap square.
	Point minimum = Point(min(from.X(), to.X()), min(from.Y(), to.Y()));
	Point maximum = from + to - minimum;
	Point grid = WRAP * Point(floor(maximum.X() / WRAP), floor(maximum.Y() / WRAP));
	from -= grid;
	to -= grid;
	// The projectile's bounding rectangle now overlaps the wrap square. If it
	// extends outside that square, it does so only on the low end (assuming no
	// projectile has a length longer than the wrap distance). If it does extend
	// outside the square, it must be "tiled" once in that direction.
	int tileX = 1 + (minimum.X() < grid.X());
	int tileY = 1 + (minimum.Y() < grid.Y());
	for(int y = 0; y < tileY; ++y)
		for(int x = 0; x < tileX; ++x)
		{
			Point offset = Point(x, y) * WRAP;
			asteroidCollisions.Line(from + offset, to + offset, result);
		}
}



// Check if the given projectile collides with any minables.
void AsteroidField::CollideMinables(const Projectile &projectile, vector<Collision> &result) const
{
	minableCollisions.Line(projectile, result);
}



// Get a list of minables affected by an explosion with blast radius.
void AsteroidField::MinablesCollisionsCircle(const Point &center, double radius, vector<Body *> &result) const
{
	minableCollisions.Circle(center, radius, result);
}



// Get the list of minable asteroids.
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
