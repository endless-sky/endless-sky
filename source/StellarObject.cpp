/* StellarObject.cpp
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

#include "StellarObject.h"

#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "Politics.h"
#include "Radar.h"

#include <algorithm>

using namespace std;

namespace {
	bool usingMatches = false;
}



void StellarObject::UsingMatchesCommand()
{
	usingMatches = true;
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



bool StellarObject::HasSprite() const
{
	return Body::HasSprite() || usingMatches;
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



bool StellarObject::HasValidPlanet() const
{
	return planet && planet->IsValid();
}



const Planet *StellarObject::GetPlanet() const
{
	return planet;
}



// Only planets that you can land on have names.
const string &StellarObject::DisplayName() const
{
	static const string UNKNOWN = "???";
	return (planet && !planet->DisplayName().empty()) ? planet->DisplayName() : UNKNOWN;
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
		return Radar::STAR;
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



const vector<RandomEvent<Hazard>> &StellarObject::Hazards() const
{
	return hazards;
}
