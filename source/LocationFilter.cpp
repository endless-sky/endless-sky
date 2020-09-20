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
#include "Random.h"
#include "Ship.h"
#include "StellarObject.h"
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
	bool SetsIntersect(const set<const Outfit *> &a, const set<const Outfit *> &b)
	{
		auto ait = a.begin();
		auto bit = b.begin();
		// The stored values are pointers to the same GameData array:
		// directly compare them.
		while(ait != a.end() && bit != b.end())
		{
			if(*ait == *bit)
				return true;
			else if(*ait < *bit)
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
		static int previousMaximum = maximum;
		
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
	
	// Check that at least one neighbor of the hub system matches, for each of the neighbor filters.
	// False if at least one filter fails to match, true if all filters find at least one match.
	bool MatchesNeighborFilters(const list<LocationFilter> &neighborFilters, const System *hub, const System *origin)
	{
		for(const LocationFilter &filter : neighborFilters)
		{
			bool hasMatch = false;
			for(const System *neighbor : hub->Links())
				if(filter.Matches(neighbor, origin))
				{
					hasMatch = true;
					break;
				}
			if(!hasMatch)
				return false;
		}
		return true;
	}
}



// Construct and Load() at the same time.
LocationFilter::LocationFilter(const DataNode &node)
{
	Load(node);
}



void LocationFilter::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		// Handle filters that must not match, or must apply to a
		// neighboring system. If the token is alone on a line, it
		// introduces many lines of this type of filter. Otherwise, this
		// child is a normal LocationFilter line.
		if(child.Token(0) == "not" || child.Token(0) == "neighbor")
		{
			list<LocationFilter> &filters = ((child.Token(0) == "not") ? notFilters : neighborFilters);
			filters.emplace_back();
			if(child.Size() == 1)
				filters.back().Load(child);
			else
				filters.back().LoadChild(child);
		}
		else
			LoadChild(child);
	}
}



void LocationFilter::Save(DataWriter &out) const
{
	out.BeginChild();
	{
		for(const LocationFilter &filter : notFilters)
		{
			out.Write("not");
			filter.Save(out);
		}
		for(const LocationFilter &filter : neighborFilters)
		{
			out.Write("neighbor");
			filter.Save(out);
		}
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
					out.Write(government->GetTrueName());
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
		for(const auto &it : outfits)
		{
			out.Write("outfits");
			out.BeginChild();
			{
				for(const Outfit *outfit : it)
					if(!outfit->Name().empty())
						out.Write(outfit->Name());
			}
			out.EndChild();
		}
		if(!shipCategory.empty())
		{
			out.Write("category");
			out.BeginChild();
			{
				for(const string &category : shipCategory)
					out.Write(category);
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
		&& !center && originMaxDistance < 0 && notFilters.empty() && neighborFilters.empty()
		&& outfits.empty() && shipCategory.empty();
}



// If the player is in the given system, does this filter match?
bool LocationFilter::Matches(const Planet *planet, const System *origin) const
{
	if(!planet || !planet->GetSystem())
		return false;
	
	// If a ship class was given, do not match planets.
	if(!shipCategory.empty())
		return false;
	
	if(!governments.empty() && !governments.count(planet->GetGovernment()))
		return false;
	
	if(!planets.empty() && !planets.count(planet))
		return false;
	for(const set<string> &attr : attributes)
		if(!SetsIntersect(attr, planet->Attributes()))
			return false;
	
	for(const LocationFilter &filter : notFilters)
		if(filter.Matches(planet, origin))
			return false;
	
	// If outfits are specified, make sure they can be bought here.
	for(const set<const Outfit *> &outfitList : outfits)
		if(!SetsIntersect(outfitList, planet->Outfitter()))
			return false;
	
	return Matches(planet->GetSystem(), origin, true);
}



bool LocationFilter::Matches(const System *system, const System *origin) const
{
	// If a ship class was given, do not match systems.
	if(!shipCategory.empty())
		return false;
	
	return Matches(system, origin, false);
}



// Check for matches with the ship's system, government, category,
// outfits (installed and carried), and attributes.
bool LocationFilter::Matches(const Ship &ship) const
{
	const System *origin = ship.GetSystem();
	if(!systems.empty() && !systems.count(origin))
		return false;
	if(!governments.empty() && !governments.count(ship.GetGovernment()))
		return false;
	
	if(!shipCategory.empty() && !shipCategory.count(ship.Attributes().Category()))
		return false;
	
	if(!attributes.empty())
	{
		// Create a set from the positive-valued attributes of this ship.
		set<string> shipAttributes;
		for(const auto &attr : ship.Attributes().Attributes())
			if(attr.second > 0.)
				shipAttributes.insert(shipAttributes.end(), attr.first);
		for(const set<string> &attr : attributes)
			if(!SetsIntersect(attr, shipAttributes))
				return false;
	}
	
	if(!outfits.empty())
	{
		// Create a set from all installed and carried outfits.
		set<const Outfit *> shipOutfits;
		for(const auto &oit : ship.Outfits())
			if(oit.second > 0)
				shipOutfits.insert(shipOutfits.end(), oit.first);
		for(const auto &cit : ship.Cargo().Outfits())
			if(cit.second > 0)
				shipOutfits.insert(cit.first);
		for(const auto &outfitSet : outfits)
			if(!SetsIntersect(outfitSet, shipOutfits))
				return false;
	}
	
	for(const LocationFilter &filter : notFilters)
		if(filter.Matches(ship))
			return false;
	
	if(!MatchesNeighborFilters(neighborFilters, origin, origin))
		return false;
	
	// Check if this ship's current system meets a "near <system>" criterion.
	// (Ships only offer missions, so no "distance" criteria need to be checked.)
	if(center && Distance(center, origin, centerMaxDistance) < centerMinDistance)
		return false;
	
	return true;
}



// Convert a "distance" filter into a "near" filter.
LocationFilter LocationFilter::SetOrigin(const System *origin) const
{
	// If there is no distance filter, then no conversion is needed.
	if(IsEmpty() || originMaxDistance < 0)
		return *this;
	
	// If the system is invalid, or a "near <system>" filter already
	// exists, do not convert "distance" to "near".
	if(!origin || center)
		return *this;
	
	// Copy all parts of this instantiated filter into the result.
	LocationFilter result = *this;
	// Perform the conversion.
	result.center = origin;
	result.centerMinDistance = originMinDistance;
	result.centerMaxDistance = originMaxDistance;
	// Revert "distance" parameters to their default.
	result.originMinDistance = 0;
	result.originMaxDistance = -1;
	
	return result;
}



// Pick a random system that matches this filter, based on the given origin.
const System *LocationFilter::PickSystem(const System *origin) const
{
	// Find a planet that satisfies the filter.
	vector<const System *> options;
	for(const auto &it : GameData::Systems())
	{
		// Skip entries with incomplete data.
		if(it.second.Name().empty())
			continue;
		if(Matches(&it.second, origin))
			options.push_back(&it.second);
	}
	return options.empty() ? nullptr : options[Random::Int(options.size())];
}



// Pick a random planet that matches this filter, based on the given origin.
const Planet *LocationFilter::PickPlanet(const System *origin, bool hasClearance, bool requireSpaceport) const
{
	// Find a planet that satisfies the filter.
	vector<const Planet *> options;
	for(const auto &it : GameData::Planets())
	{
		const Planet &planet = it.second;
		// Skip entries with incomplete data.
		if(planet.Name().empty() || !planet.GetSystem())
			continue;
		// Skip planets that do not offer special jobs or missions, unless they were explicitly listed as options.
		if(planet.IsWormhole() || (requireSpaceport && !planet.HasSpaceport()) || (!hasClearance && !planet.CanLand()))
			if(planets.empty() || !planets.count(&planet))
				continue;
		if(Matches(&planet, origin))
			options.push_back(&planet);
	}
	return options.empty() ? nullptr : options[Random::Int(options.size())];
}



// Load one particular line of conditions.
void LocationFilter::LoadChild(const DataNode &child)
{
	bool isNot = (child.Token(0) == "not" || child.Token(0) == "neighbor");
	int valueIndex = 1 + isNot;
	const string &key = child.Token(valueIndex - 1);
	if(key == "not" || key == "neighbor")
		child.PrintTrace("Skipping unsupported use of 'not' and 'neighbor'. These keywords must be nested if used together.");
	else if(key == "planet")
	{
		for(int i = valueIndex; i < child.Size(); ++i)
			planets.insert(GameData::Planets().Get(child.Token(i)));
		for(const DataNode &grand : child)
			for(int i = 0; i < grand.Size(); ++i)
				planets.insert(GameData::Planets().Get(grand.Token(i)));
	}
	else if(key == "system")
	{
		for(int i = valueIndex; i < child.Size(); ++i)
			systems.insert(GameData::Systems().Get(child.Token(i)));
		for(const DataNode &grand : child)
			for(int i = 0; i < grand.Size(); ++i)
				systems.insert(GameData::Systems().Get(grand.Token(i)));
	}
	else if(key == "government")
	{
		for(int i = valueIndex; i < child.Size(); ++i)
			governments.insert(GameData::Governments().Get(child.Token(i)));
		for(const DataNode &grand : child)
			for(int i = 0; i < grand.Size(); ++i)
				governments.insert(GameData::Governments().Get(grand.Token(i)));
	}
	else if(key == "attributes")
	{
		attributes.push_back(set<string>());
		for(int i = valueIndex; i < child.Size(); ++i)
			attributes.back().insert(child.Token(i));
		for(const DataNode &grand : child)
			for(int i = 0; i < grand.Size(); ++i)
				attributes.back().insert(grand.Token(i));
		// Don't allow empty attribute sets; that's probably a typo.
		if(attributes.back().empty())
			attributes.pop_back();
	}
	else if(key == "near" && child.Size() >= 1 + valueIndex)
	{
		center = GameData::Systems().Get(child.Token(valueIndex));
		if(child.Size() == 2 + valueIndex)
			centerMaxDistance = child.Value(1 + valueIndex);
		else if(child.Size() == 3 + valueIndex)
		{
			centerMinDistance = child.Value(1 + valueIndex);
			centerMaxDistance = child.Value(2 + valueIndex);
		}
	}
	else if(key == "distance" && child.Size() >= 1 + valueIndex)
	{
		if(child.Size() == 1 + valueIndex)
			originMaxDistance = child.Value(valueIndex);
		else if(child.Size() == 2 + valueIndex)
		{
			originMinDistance = child.Value(valueIndex);
			originMaxDistance = child.Value(1 + valueIndex);
		}
	}
	else if(key == "category" && child.Size() >= 2 + isNot)
	{
		// Ship categories cannot be combined in an "and" condition.
		static const set<string> allowed(Ship::CATEGORIES.begin(), Ship::CATEGORIES.end());
		for(int i = 1 + isNot; i < child.Size(); ++i)
		{
			const string &value = child.Token(i);
			if(allowed.count(value))
				shipCategory.insert(value);
			else
				child.PrintTrace("Invalid ship category: \"" + value + "\":");
		}
		for(const DataNode &grand : child)
			for(int i = 0; i < grand.Size(); ++i)
			{
				const string &value = grand.Token(i);
				if(allowed.count(value))
					shipCategory.insert(value);
				else
					child.PrintTrace("Invalid ship category: \"" + value + "\":");
			}
	}
	else if(key == "outfits" && child.Size() >= 2 + isNot)
	{
		outfits.push_back(set<const Outfit *>());
		for(int i = 1 + isNot; i < child.Size(); ++i)
			outfits.back().insert(GameData::Outfits().Get(child.Token(i)));
		for(const DataNode &grand : child)
			for(int i = 0; i < grand.Size(); ++i)
				outfits.back().insert(GameData::Outfits().Get(grand.Token(i)));
		// Don't allow empty outfit sets; that's probably a typo.
		if(outfits.back().empty())
			outfits.pop_back();
	}
	else
		child.PrintTrace("Unrecognized location filter:");
}



bool LocationFilter::Matches(const System *system, const System *origin, bool didPlanet) const
{
	if(!system)
		return false;
	if(!systems.empty() && !systems.count(system))
		return false;
	
	// Don't check these filters again if they were already checked as a part of
	// checking if a planet matches.
	if(!didPlanet)
	{
		if(!governments.empty() && !governments.count(system->GetGovernment()))
			return false;
		
		// This filter is being applied to a system, not a planet.
		// Check whether the system, or any planet within it, has one of the
		// required attributes from each set.
		if(!attributes.empty())
		{
			for(const set<string> &attr : attributes)
			{
				bool matches = SetsIntersect(attr, system->Attributes());
				for(const StellarObject &object : system->Objects())
					if(object.GetPlanet())
						matches |= SetsIntersect(attr, object.GetPlanet()->Attributes());
				
				if(!matches)
					return false;
			}
		}
		
		for(const LocationFilter &filter : notFilters)
			if(filter.Matches(system, origin))
				return false;
	}
	
	if(!MatchesNeighborFilters(neighborFilters, system, origin))
		return false;
	
	// Check this system's distance from the desired reference system.
	if(center && Distance(center, system, centerMaxDistance) < centerMinDistance)
		return false;
	if(origin && originMaxDistance >= 0
			&& Distance(origin, system, originMaxDistance) < originMinDistance)
		return false;
	
	return true;
}
