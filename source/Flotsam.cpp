/* Flotsam.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Flotsam.h"

#include "Angle.h"
#include "Effect.h"
#include "GameData.h"
#include "Outfit.h"
#include "Random.h"
#include "Ship.h"
#include "SpriteSet.h"
#include "Visual.h"

#include <cmath>

using namespace std;



const int Flotsam::TONS_PER_BOX = 5;



// Constructors for flotsam carrying either a commodity or an outfit.
Flotsam::Flotsam(const string &commodity, int count)
	: commodity(commodity), count(count)
{
	lifetime = Random::Int(300) + 360;
	// Scale lifetime in proportion to the expected amount per box.
	if(count != TONS_PER_BOX)
		lifetime = sqrt(count * (1. / TONS_PER_BOX)) * lifetime;
}



Flotsam::Flotsam(const Outfit *outfit, int count)
	: outfit(outfit), count(count)
{
	// The more the outfit costs, the faster this flotsam should disappear.
	int lifetimeBase = 300000000 / (outfit->Cost() * count + 1000000);
	lifetime = Random::Int(lifetimeBase) + lifetimeBase + 60;
}



// Place this flotsam, and set the given ship as its source. This is a
// separate function because a ship may queue up flotsam to dump but take
// several frames before it finishes dumping it all.
void Flotsam::Place(const Ship &source)
{
	this->source = &source;
	Place(source, Angle::Random().Unit() * (2. * Random::Real()) - 2. * source.Unit());
}



// Place flotsam coming from something other than a ship. Optionally specify
// the maximum relative velocity, or the exact relative velocity as a vector.
void Flotsam::Place(const Body &source, double maxVelocity)
{
	Place(source, Angle::Random().Unit() * (maxVelocity * Random::Real()));
}



void Flotsam::Place(const Body &source, const Point &dv)
{
	position = source.Position();
	velocity = source.Velocity() + dv;
	angle = Angle::Random();
	spin = Angle::Random(10.);
	
	// Special case: allow a harvested outfit item to define its flotsam sprite
	// using the field that usually defines a secondary weapon's icon.
	if(outfit && outfit->FlotsamSprite())
		SetSprite(outfit->FlotsamSprite());
	else
		SetSprite(SpriteSet::Get("effect/box"));
	SetFrameRate(4. * (1. + Random::Real()));
}



// Move the object one time-step forward.
void Flotsam::Move(vector<Visual> &visuals)
{
	position += velocity;
	angle += spin;
	--lifetime;
	if(lifetime > 0)
		return;
	
	// This flotsam has reached the end of its life. 
	const Effect *effect = GameData::Effects().Get("flotsam death");
	for(int i = 0; i < 3; ++i)
	{
		Angle smokeAngle = Angle::Random();
		velocity += smokeAngle.Unit() * Random::Real();
		
		visuals.emplace_back(*effect, position, velocity, smokeAngle);
	}
	MarkForRemoval();
}



// This is the one ship that cannot pick up this flotsam.
const Ship *Flotsam::Source() const
{
	return source;
}



// This is what the flotsam contains:
const string &Flotsam::CommodityType() const
{
	return commodity;
}



const Outfit *Flotsam::OutfitType() const
{
	return outfit;
}



int Flotsam::Count() const
{
	return count;
}



// This is how big one "unit" of the flotsam is (in tons). If a ship has
// less than this amount of space, it can't pick up anything here.
double Flotsam::UnitSize() const
{
	return outfit ? outfit->Mass() : 1.;
}



// Transfer contents to the collector ship. The flotsam velocity is
// stabilized in proportion to the amount being transferred.
int Flotsam::TransferTo(Ship *collector)
{
	int amount = outfit ?
		collector->Cargo().Add(outfit, count) :
		collector->Cargo().Add(commodity, count);
	
	Point relative = collector->Velocity() - velocity;
	double proportion = static_cast<double>(amount) / count;
	velocity += relative * proportion;
	
	count -= amount;
	// If this flotsam is now empty, remove it.
	if(count <= 0)
		MarkForRemoval();
	
	return amount;
}
