/* OrbitPlanetCamera.cpp
Copyright (c) 2024 by the Endless Sky developers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "OrbitPlanetCamera.h"

#include "Planet.h"
#include "StellarObject.h"

using namespace std;

const string OrbitPlanetCamera::name = "Orbit Planet";



OrbitPlanetCamera::OrbitPlanetCamera()
	: orbitAngle(0.), currentPosition(0., 0.), velocity(0., 0.)
{
}



Point OrbitPlanetCamera::GetTarget() const
{
	return currentPosition;
}



Point OrbitPlanetCamera::GetVelocity() const
{
	return velocity;
}



void OrbitPlanetCamera::Step()
{
	// Rotate slowly around the object
	orbitAngle += Angle(0.2);

	Point oldPosition = currentPosition;

	if(stellarObjects && currentIndex < visibleIndices.size())
	{
		size_t objIndex = visibleIndices[currentIndex];
		if(objIndex < stellarObjects->size())
		{
			const StellarObject &obj = (*stellarObjects)[objIndex];
			// Orbit distance based on object size
			double distance = orbitDistance + obj.Radius() * 1.5;
			currentPosition = obj.Position() + orbitAngle.Unit() * distance;
		}
	}

	velocity = currentPosition - oldPosition;
}



void OrbitPlanetCamera::SetStellarObjects(const vector<StellarObject> &newObjects)
{
	// Only rebuild if the system changed (different object list)
	if(stellarObjects == &newObjects)
		return;

	stellarObjects = &newObjects;
	visibleIndices.clear();

	// Only include objects with sprites (visible planets/stations)
	for(size_t i = 0; i < newObjects.size(); ++i)
		if(newObjects[i].HasSprite())
			visibleIndices.push_back(i);

	if(!visibleIndices.empty() && currentIndex >= visibleIndices.size())
		currentIndex = 0;
}



const string &OrbitPlanetCamera::ModeName() const
{
	return name;
}



string OrbitPlanetCamera::TargetName() const
{
	if(stellarObjects && currentIndex < visibleIndices.size())
	{
		size_t objIndex = visibleIndices[currentIndex];
		if(objIndex < stellarObjects->size())
		{
			const StellarObject &obj = (*stellarObjects)[objIndex];
			if(obj.HasValidPlanet())
				return obj.GetPlanet()->DisplayName();
			// For objects without planets (like stars), use DisplayName
			string name = obj.DisplayName();
			return name.empty() ? "Stellar Object" : name;
		}
	}
	return "";
}



void OrbitPlanetCamera::CycleTarget()
{
	if(!visibleIndices.empty())
		currentIndex = (currentIndex + 1) % visibleIndices.size();
}
