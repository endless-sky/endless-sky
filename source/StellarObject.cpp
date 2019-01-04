/* StellarObject.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StellarObject.h"

#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "Politics.h"
#include "Radar.h"
#include "Ship.h"
#include "System.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Object default constructor.
StellarObject::StellarObject()
	: planet(nullptr),
	distance(0.), speed(0.), offset(0.), parent(-1),
	message(nullptr), isStar(false), isStation(false), isMoon(false),
	isDead(true)
{
	// Unlike ships and projectiles, stellar objects are not drawn shrunk to half size,
	// because they do not need to be so sharp.
	zoom = 2.;
}



// Get the radius of this planet, i.e. how close you must be to land.
double StellarObject::Radius() const
{
	double radius = -1.;
	if(HasSprite())
		radius = .5 * min(Width(), Height());
	
	// Special case: stars may have a huge cloud around them, but only count the
	// core of the cloud as part of the radius.
	if(isStar)
		radius = min(radius, 80.);
	
	return radius;
}



// If it is possible to land on this planet, this returns the Planet
// objects that gives more information about it. Otherwise, this
// function will just return nullptr.
const Planet *StellarObject::GetPlanet() const
{
	return planet;
}



// Only planets that you can land on have names.
const string &StellarObject::Name() const
{
	static const string UNKNOWN = "???";
	return (planet && !planet->Name().empty()) ? planet->Name() : UNKNOWN;
}



// If it is impossible to land on this planet, get the message
// explaining why (e.g. too hot, too cold, etc.).
const string &StellarObject::LandingMessage() const
{
	// Check if there's a custom message for this sprite type.
	if(GameData::HasLandingMessage(GetSprite()))
		return GameData::LandingMessage(GetSprite());
	
	static const string EMPTY;
	return (message ? *message : EMPTY);
}



// Get the color to be used for displaying this object.
int StellarObject::RadarType(const Ship *ship) const
{
	if(IsStar())
		return Radar::SPECIAL;
	else if(!planet || !planet->IsAccessible(ship))
		return Radar::INACTIVE;
	else if(planet->IsWormhole())
		return Radar::ANOMALOUS;
	else if(GameData::GetPolitics().HasDominated(planet))
		return Radar::PLAYER;
	else if(planet->CanLand())
		return Radar::FRIENDLY;
	else if(!planet->GetGovernment()->IsEnemy())
		return Radar::UNFRIENDLY;
	
	return Radar::HOSTILE;
}



// Check if this is a star.
bool StellarObject::IsStar() const
{
	return isStar;
}



// Check if this is a station.
bool StellarObject::IsStation() const
{
	return isStation;
}



// Check if this is a moon.
bool StellarObject::IsMoon() const
{
	return isMoon;
}



// Get this object's parent index (in the System's vector of objects).
int StellarObject::Parent() const
{
	return parent;
}



// Find out how far this object is from its parent.
double StellarObject::Distance() const
{
	return distance;
}




bool StellarObject::IsVisible() const
{
	return isDead && HasSprite();
}



bool StellarObject::HasShip() const
{
	return ship;
}



shared_ptr<Ship> StellarObject::GetShip(const System *system) const
{
	if(!ship)
		return nullptr;
	isDead = false;
	shared_ptr<Ship> local(new Ship(*ship));
	

	// Sets the ships government to the StellarObject's goverment if specified or the system's government.
	local->SetGovernment(government ? government : system->GetGovernment());

	local->SetName("Orbital Defense Platform");
	local->SetPersonality(personality);
	local->SetSystem(system);
	
	// Gets the orbited object and uses it as center of mass and a basis for the specific
	// gravitational constant. Also calculates the starting movement of the station.
	const StellarObject central = system->Objects()[parent];
	// Assumes the mass to be proportional to width*height.
	double constant = central.Width()*central.Height()*0.001;
	// Stars have a higher density than other StellarObjects.
	if(central.IsStar())
		constant *= 1000;
	// Non-defensive stations are normally lighter because most of their inside volume
	// consists of air while they don't fill the hole space of the sprite like a sphere does.
	if(central.IsStation())
		constant *= 0.02;
	local->SetStellar(this, constant, central.Position());
	// Assuming the station starts moving in perfect circle where a = v^2/r.
	Point r = central.Position()-position;
	Point velocity = r*(constant/(r.LengthSquared()));
	velocity /= sqrt(velocity.Length());
	local->Place(Position(), Point(velocity.Y(), -velocity.X()), Angle::Random());
	
	
	local->Restore();
	return local;
}


// Makes the StellarObject visible again after the station died.
void StellarObject::Die(Point position) const
{
	isDead = true;
	const_cast<Point &>(this->position) = position;
}
