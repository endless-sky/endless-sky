/* StellarObject.h
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

#ifndef STELLAR_OBJECT_H_
#define STELLAR_OBJECT_H_

#include "Body.h"
#include "Hazard.h"
#include "RandomEvent.h"

class Planet;
class Ship;



// Class representing a planet, star, moon, or other large object in space. This
// does not store the details of what is on that object, if anything; that is
// handled by the Planet class. Each object's position depends on what it is
// orbiting around and how far away it is from that object. Each day, all the
// objects in each system move slightly in their orbits.
class StellarObject : public Body {
public:
	StellarObject();

	// Functions provided by the Body base class:
	// bool HasSprite() const;
	// int Width() const;
	// int Height() const;
	// Frame GetFrame(int step = -1) const;
	// const Point &Position() const;
	// const Point &Velocity() const;
	// const Angle &Facing() const;
	// Point Unit() const;

	// Get the radius of this planet, i.e. how close you must be to land.
	double Radius() const;

	// Determine if this object represents a planet with valid data.
	bool HasValidPlanet() const;
	// Get this object's planet, if any. It may or may not be fully defined.
	const Planet *GetPlanet() const;

	// Only planets that you can land on have names.
	const std::string &Name() const;
	// If it is impossible to land on this planet, get the message
	// explaining why (e.g. too hot, too cold, etc.).
	const std::string &LandingMessage() const;

	// Get the radar color to be used for displaying this object.
	int RadarType(const Ship *ship) const;
	// Check if this is a star.
	bool IsStar() const;
	// Check if this is a station.
	bool IsStation() const;
	// Check if this is a moon.
	bool IsMoon() const;
	// Get this object's parent index (in the System's vector of objects).
	int Parent() const;
	// This object's system hazards.
	const std::vector<RandomEvent<Hazard>> &Hazards() const;
	// Find out how far this object is from its parent.
	double Distance() const;
	virtual double Parallax() const override;


private:
	const Planet *planet;

	double distance;
	double speed;
	double offset;
	std::vector<RandomEvent<Hazard>> hazards;
	int parent;

	const std::string *message;
	bool isStar;
	bool isStation;
	bool isMoon;

	// Let System handle setting all the values of an Object.
	friend class System;
};



#endif
