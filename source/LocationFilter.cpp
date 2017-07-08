/* LocationFilter.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "LocationFilter.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "DistanceMap.h"
#include "GameData.h"
#include "Government.h"
#include "Planet.h"
#include "Ship.h"
#include "System.h"

#include <mutex>

using namespace std;

namespace {
	bool SetsIntersect(const set<string> &a, const set<string> &b)
	{
		// Quickest way to find out if two sets contain common elements: iterate
		// through both of them in sorted order.
		auto ait = a.begin();
		auto bit = b.begin();
		while(ait != a.end() && bit != b.end())
		{
			int comp = ait->compare(*bit);
			if(!comp)
				return true;
			else if(comp < 0)
				++ait;
			else
				++bit;
		}
		return false;
	}
	
	// Check if the given system is within the given distance of the center.
	int Distance(const System *center, const System *system, int maximum)
	{
		// This function should only ever be called from the main thread, but
		// just to be sure, use mutex protection on the static locals.
		static mutex distanceMutex;
		lock_guard<mutex> lock(distanceMutex);
		
		static const System *previousCenter = center;
		static DistanceMap distance(center, -1, maximum);
		int previousMaximum = maximum;
		
		if(center != previousCenter || maximum > previousMaximum)
		{
			previousCenter = center;
			previousMaximum = maximum;
			distance = DistanceMap(center, -1, maximum);
		}
		// If the distance is greater than the maximum, this is not a match.
		int d = distance.Days(system);
		return (d > maximum) ? -1 : d;
	}
}



// There is no need to save a location filter, because any mission that is
// in the saved game will already have "applied" the filter to choose a
// particular planet or system.
void LocationFilter::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "planet")
		{
			for(int i = 1; i < child.Size(); ++i)
				planets.insert(GameData::Planets().Get(child.Token(i)));
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
					planets.insert(GameData::Planets().Get(grand.Token(i)));
		}
		else if(child.Token(0) == "system")
		{
			for(int i = 1; i < child.Size(); ++i)
				systems.insert(GameData::Systems().Get(child.Token(i)));
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
					systems.insert(GameData::Systems().Get(grand.Token(i)));
		}
		else if(child.Token(0) == "government")
		{
			for(int i = 1; i < child.Size(); ++i)
				governments.insert(GameData::Governments().Get(child.Token(i)));
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
					governments.insert(GameData::Governments().Get(grand.Token(i)));
		}
		else if(child.Token(0) == "attributes")
		{
			attributes.push_back(set<string>());
			for(int i = 1; i < child.Size(); ++i)
				attributes.back().insert(child.Token(i));
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
					attributes.back().insert(grand.Token(i));
			// Don't allow empty attribute sets; that's probably a typo.
			if(attributes.back().empty())
				attributes.pop_back();
		}
		else if(child.Token(0) == "near" && child.Size() >= 2)
		{
			center = GameData::Systems().Get(child.Token(1));
			if(child.Size() == 3)
				centerMaxDistance = child.Value(2);
			else if(child.Size() == 4)
			{
				centerMinDistance = child.Value(2);
				centerMaxDistance = child.Value(3);
			}
		}
		else if(child.Token(0) == "distance" && child.Size() >= 2)
		{
			if(child.Size() == 2)
				originMaxDistance = child.Value(1);
			else if(child.Size() == 3)
			{
				originMinDistance = child.Value(1);
				originMaxDistance = child.Value(2);
			}
		}
	}
}



void LocationFilter::Save(DataWriter &out) const
{
	out.BeginChild();
	{
		if(!planets.empty())
		{
			out.Write("planet");
			out.BeginChild();
			{
				for(const Planet *planet : planets)
					out.Write(planet->Name());
			}
			out.EndChild();
		}
		if(!systems.empty())
		{
			out.Write("system");
			out.BeginChild();
			{
				for(const System *system : systems)
					out.Write(system->Name());
			}
			out.EndChild();
		}
		if(!governments.empty())
		{
			out.Write("government");
			out.BeginChild();
			{
				for(const Government *government : governments)
					out.Write(government->GetName());
			}
			out.EndChild();
		}
		for(const auto &it : attributes)
		{
			out.Write("attributes");
			out.BeginChild();
			{
				for(const string &name : it)
					out.Write(name);
			}
			out.EndChild();
		}
		if(center)
			out.Write("near", center->Name(), centerMinDistance, centerMaxDistance);
	}
	out.EndChild();
}



// Check if this filter contains any specifications.
bool LocationFilter::IsEmpty() const
{
	return planets.empty() && attributes.empty() && systems.empty() && governments.empty()
		&& !center && originMaxDistance < 0;
}



// If the player is in the given system, does this filter match?
bool LocationFilter::Matches(const Planet *planet, const System *origin) const
{
	if(!planet)
		return false;
	
	if(!planets.empty() && !planets.count(planet))
		return false;
	for(const set<string> &attr : attributes)
		if(!SetsIntersect(attr, planet->Attributes()))
			return false;
	
	return Matches(planet->GetSystem(), origin);
}



bool LocationFilter::Matches(const System *system, const System *origin) const
{
	if(!system)
		return false;
	if(!systems.empty() && !systems.count(system))
		return false;
	if(!governments.empty() && !governments.count(system->GetGovernment()))
		return false;
	
	if(center)
	{
		// Distance() will return -1 if the system was not within the given max
		// distance, so this checks for that as well as for the minimum:
		if(Distance(center, system, centerMaxDistance) < centerMinDistance)
			return false;
	}
	if(origin && originMaxDistance >= 0)
	{
		// Distance() will return -1 if the system was not within the given max
		// distance, so this checks for that as well as for the minimum:
		if(Distance(origin, system, originMaxDistance) < originMinDistance)
			return false;
	}
	
	return true;
}



bool LocationFilter::Matches(const Ship &ship) const
{
	if(!systems.empty() && !systems.count(ship.GetSystem()))
		return false;
	if(!governments.empty() && !governments.count(ship.GetGovernment()))
		return false;
	
	if(center)
	{
		// Distance() will return -1 if the system was not within the given max
		// distance, so this checks for that as well as for the minimum:
		if(Distance(center, ship.GetSystem(), centerMaxDistance) < centerMinDistance)
			return false;
	}
	return true;
}
