/* StellarObject.cpp
Michael Zahniser, 27 Jan 2014

Function definitions for the StellarObject class.
*/

#include "StellarObject.h"

#include "Planet.h"

#include <algorithm>

using namespace std;



// Object default constructor.
StellarObject::StellarObject()
	: planet(nullptr),
	distance(0.), speed(0.), offset(0.), parent(-1),
	message(nullptr), isStar(false)
{
}



// Some objects do not have sprites, because they are just an orbital
// center for two or more other objects.
const Animation &StellarObject::GetSprite() const
{
	return animation;
}



// Get this object's position on the date most recently passed to this
// system's SetDate() function.
const Point &StellarObject::Position() const
{
	return position;
}



// Get the radius of this planet, i.e. how close you must be to land.
double StellarObject::Radius() const
{
	// If the object is rectangular, you must land in the middle section.
	return animation.IsEmpty() ? -1. :
		.5 * min(animation.Width(), animation.Height());
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
	static const string EMPTY;
	return planet ? planet->Name() : EMPTY;
}



// If it is impossible to land on this planet, get the message
// explaining why (e.g. too hot, too cold, etc.).
const string &StellarObject::LandingMessage() const
{
	if(!planet && Radius() >= 130.)
	{
		static const string GAS = "You cannot land on a gas giant.";
		return GAS;
	}
	static const string EMPTY;
	return (message) ? *message : EMPTY;
}



// Check if this is a star.
bool StellarObject::IsStar() const
{
	return isStar;
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
