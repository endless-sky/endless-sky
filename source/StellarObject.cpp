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

#include "Audio.h"
#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "Politics.h"
#include "Radar.h"
#include "Random.h"
#include "Ship.h"
#include "System.h"
#include "Visual.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>

using namespace std;


namespace {
	// Create all the effects in the given list, at the given location, velocity, and angle.
	void CreateEffects(const map<const Effect *, int> &m, Point pos, Point vel, Angle angle, vector<Visual> &visuals)
	{
		for(const auto &it : m)
			for(int i = 0; i < it.second; ++i)
				visuals.emplace_back(*it.first, pos, vel, angle);
	}
}



// Object default constructor.
StellarObject::StellarObject()
	: planet(nullptr),
	distance(0.), speed(0.), offset(0.), parent(-1),
	message(nullptr), isStar(false), isStation(false), isMoon(false)
{
	// Unlike ships and projectiles, stellar objects are not drawn shrunk to half size,
	// because they do not need to be so sharp.
	zoom = 2.;
}


// needed to return the mutable Angle.
// Direction this Body is facing in.
const Angle &StellarObject::Facing() const
{
	return angle;
}


// needed to return the mutable Angle unit vector.
// Unit vector in the direction this body is facing. This represents the scale
// and transform that should be applied to the sprite before drawing it.
Point StellarObject::Unit() const
{
	return angle.Unit() * (.5 * Zoom());
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



// Selects the nearest potential target and fires projectiles.
void StellarObject::TryToFire(vector<Projectile> &projectiles, const System *system, vector<Visual> &visuals, const list<std::shared_ptr<Ship>> &ships) const
{
	if(!launcher)
		return;
	const Government *gov = planet ? planet->GetGovernment() : system->GetGovernment();
	if(!gov)
		return;
	if(!rotation && Random::Real() > reload)
		return;
	

	double maxRange = launcher->Range()*1.5;
	shared_ptr<Ship> enemy = nullptr;
	Angle aim;
	for(auto &target : ships) {
		if(target->IsTargetable() && gov->IsEnemy(target->GetGovernment())
				&& !(target->GetGovernment()->IsPlayer() && GameData::GetPolitics().CanLand(planet))
				&& !(target->IsHyperspacing() && target->Velocity().Length() > 10.)
				&& target->Position().Distance(position) < maxRange
				&& target->GetSystem() == system)
		{
			Angle a(target->Position() - position);
			if(!enemy)
			{
				enemy = target;
				aim = a;
			}
			else if((target->Position().Distance(position) < enemy->Position().Distance(position)
				&& !rotation) || (rotation && abs((aim-angle).Degrees()) > abs((a-angle).Degrees())))
			{
				enemy = target;
				aim = a;
			}
		}
	}
	if(!enemy)
		return;

	if(rotation) {
		aim -= angle;
		const double effectiveAim = aim.Degrees();
		if(effectiveAim >= 0)
		{
			if(effectiveAim < rotation)
				angle += Angle(effectiveAim);
			else
				angle += Angle(rotation);
		}
		else {
			if(effectiveAim > -rotation)
				angle += Angle(effectiveAim);
			else
				angle += Angle(-rotation);
		}
		aim = angle;
	}
	if(rotation && Random::Real() > reload)
		return;
	
	//Each launch comes from a random point inside r/2 from the middle of the object.
	//Assumes the biggest circle in the middle of the sprite is the planet/station.
	Point randomPosition(position);
	//Lasers and beams fire always from the middle of the station.
	if(launcher->Reload() > 1)
	{
		double r = Random::Real()/4;
		const Sprite *sprite = GetSprite();
		r *= sprite->Width() > sprite->Height() ? sprite->Height() : sprite->Width();
		randomPosition += Angle::Random().Unit()*r;
	}


	projectiles.emplace_back(gov, enemy, randomPosition, aim, launcher);
	CreateEffects(launcher->FireEffects(), randomPosition, Point(), aim, visuals);

	if(launcher->WeaponSound())
		Audio::Play(launcher->WeaponSound(), randomPosition);
}
