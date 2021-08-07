/* Hazard.cpp
Copyright (c) 2020 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Hazard.h"

#include "DataNode.h"
#include "Effect.h"
#include "GameData.h"
#include "Random.h"

using namespace std;


void Hazard::Load(const DataNode &node)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "weapon")
			LoadWeapon(child);
		else if(key == "constant strength")
			deviates = false;
		else if(child.Size() < 2)
			child.PrintTrace("Skipping hazard attribute with no value specified:");
		else if(key == "period")
			period = max(1, static_cast<int>(child.Value(1)));
		else if(key == "duration")
		{
			minDuration = max(0, static_cast<int>(child.Value(1)));
			maxDuration = max(minDuration, (child.Size() >= 3 ? static_cast<int>(child.Value(2)) : 0));
		}
		else if(key == "strength")
		{
			minStrength = max(0., child.Value(1));
			maxStrength = max(minStrength, (child.Size() >= 3) ? child.Value(2) : 0.);
		}
		else if(key == "range")
		{
			minRange = max(0., (child.Size() >= 3) ? child.Value(1) : 0.);
			maxRange = max(minRange, (child.Size() >= 3) ? child.Value(2) : child.Value(1));
		}
		else if(key == "environmental effect")
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			environmentalEffects[GameData::Effects().Get(child.Token(1))] += count;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const string &Hazard::Name() const
{
	return name;
}



// Does the strength of this hazard deviate over time?
bool Hazard::Deviates() const
{
	return deviates;
}



// How often this hazard deals its damage while active.
int Hazard::Period() const
{
	return period;
}



// Generates a random integer between the minimum and maximum duration of this hazard.
int Hazard::RandomDuration() const
{
	return minDuration + (maxDuration <= minDuration ? 0 : Random::Int(maxDuration - minDuration));
}




// Generates a random double between the minimum and maximum strength of this hazard.
double Hazard::RandomStrength() const
{
	return minStrength + (maxStrength <= minStrength ? 0. : (maxStrength - minStrength) * Random::Real());
}



// The minimum and maximum distances from the origin in which this hazard has an effect.
double Hazard::MinRange() const
{
	return minRange;
}



double Hazard::MaxRange() const
{
	return maxRange;
}



// Visuals to be created while this hazard is active.
const map<const Effect *, int> &Hazard::EnvironmentalEffects() const
{
	return environmentalEffects;
}
