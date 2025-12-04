/* System.cpp
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

#include "System.h"

#include "Angle.h"
#include "DataNode.h"
#include "Date.h"
#include "Fleet.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Government.h"
#include "Hazard.h"
#include "Minable.h"
#include "Planet.h"
#include "Random.h"
#include "image/SpriteSet.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Dynamic economy parameters: how much of its production each system keeps
	// and exports each day:
	const double KEEP = .89;
	const double EXPORT = .10;
	// Standard deviation of the daily production of each commodity:
	const double VOLUME = 2000.;
	// Above this supply amount, price differences taper off:
	const double LIMIT = 20000.;
}

const double System::DEFAULT_NEIGHBOR_DISTANCE = 100.;



System::Asteroid::Asteroid(const string &name, int count, double energy)
	: name(name), count(count), energy(energy)
{
}



System::Asteroid::Asteroid(const Minable *type, int count, double energy)
	: type(type), count(count), energy(energy)
{
}



const string &System::Asteroid::Name() const
{
	return name;
}



const Minable *System::Asteroid::Type() const
{
	return type;
}



int System::Asteroid::Count() const
{
	return count;
}



double System::Asteroid::Energy() const
{
	return energy;
}



// Load a system's description.
void System::Load(const DataNode &node, Set<Planet> &planets, const ConditionsStore *playerConditions)
{
	if(node.Size() < 2)
		return;
	trueName = node.Token(1);
	isDefined = true;

	// Track planets associated with removed objects. Check if remaining objects
	// refer to any of the same planets and only unlink planets that have no
	// remaining references here.
	set<const Planet *> removedObjectPlanets;

	// For the following keys, if this data node defines a new value for that
	// key, the old values should be cleared (unless using the "add" keyword).
	set<string> shouldOverwrite = {"asteroids", "attributes", "belt", "fleet", "link", "object", "hazard"};

	for(const DataNode &child : node)
	{
		// Check for the "add" or "remove" keyword.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		if((add || remove) && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}

		// Get the key and value (if any).
		const string &key = child.Token((add || remove) ? 1 : 0);
		int valueIndex = (add || remove) ? 2 : 1;
		bool hasValue = (child.Size() > valueIndex);
		const string &value = child.Token(hasValue ? valueIndex : 0);

		// Check for conditions that require clearing this key's current value.
		// "remove <key>" means to clear the key's previous contents.
		// "remove <key> <value>" means to remove just that value from the key.
		// "remove object" should only remove all if the node lacks children, as the children
		// of an object node are its values.
		bool removeAll = (remove && !hasValue && !(key == "object" && child.HasChildren()));
		// If this is the first entry for the given key, and we are not in "add"
		// or "remove" mode, its previous value should be cleared.
		bool overwriteAll = (!add && !remove && shouldOverwrite.contains(key));
		overwriteAll |= (!add && !remove && key == "minables" && shouldOverwrite.contains("asteroids"));
		// Clear the data of the given type.
		if(removeAll || overwriteAll)
		{
			// Clear the data of the given type.
			if(key == "display name")
				displayName.clear();
			else if(key == "government")
				government = nullptr;
			else if(key == "music")
				music.clear();
			else if(key == "attributes")
				attributes.clear();
			else if(key == "link")
				links.clear();
			else if(key == "asteroids" || key == "minables")
				asteroids.clear();
			else if(key == "haze")
				haze = nullptr;
			else if(key == "starfield density")
				starfieldDensity = 1.;
			else if(key == "ramscoop")
			{
				universalRamscoop = true;
				ramscoopAddend = 0.;
				ramscoopMultiplier = 1.;
			}
			else if(key == "trade")
				trade.clear();
			else if(key == "fleet")
				fleets.clear();
			else if(key == "hazard")
				hazards.clear();
			else if(key == "belt")
				belts.clear();
			else if(key == "object")
			{
				// Make sure any planets that were linked to this system know
				// that they are no longer here.
				// Use const_cast to convert the "const Planet *" to "Planet *".
				// Non-const access is available through the passed parameter "Set<Planet> &planets"
				// but, in the case of an as-yet undefined Planet, the object will not have a name with which
				// it can be found in that collection.
				for(StellarObject &object : objects)
					if(object.planet)
						const_cast<Planet *>(object.planet)->RemoveSystem(this);

				objects.clear();
			}
			else if(key == "hidden")
				hidden = false;
			else if(key == "shrouded")
				shrouded = false;
			else if(key == "inaccessible")
				inaccessible = false;
			else if(key == "no raids")
				noRaids = false;

			// If not in "overwrite" mode, move on to the next node.
			if(overwriteAll)
				shouldOverwrite.erase(key == "minables" ? "asteroids" : key);
			else
				continue;
		}

		// Handle the attributes without values.
		if(key == "hidden")
			hidden = true;
		else if(key == "shrouded")
			shrouded = true;
		else if(key == "inaccessible")
			inaccessible = true;
		else if(key == "no raids")
			noRaids = true;
		else if(key == "ramscoop")
		{
			for(const DataNode &grand : child)
			{
				const string &key = grand.Token(0);
				bool hasValue = grand.Size() >= 2;
				if(key == "universal" && hasValue)
					universalRamscoop = grand.BoolValue(1);
				else if(key == "addend" && hasValue)
					ramscoopAddend = grand.Value(1);
				else if(key == "multiplier" && hasValue)
					ramscoopMultiplier = grand.Value(1);
				else
					child.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(!hasValue && key != "object")
		{
			child.PrintTrace("Expected key to have a value:");
			continue;
		}
		// Handle the attributes which can be "removed."
		else if(key == "attributes")
		{
			if(remove)
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.erase(child.Token(i));
			else
				for(int i = valueIndex; i < child.Size(); ++i)
					attributes.insert(child.Token(i));
		}
		else if(key == "link")
		{
			if(value == trueName)
			{
				child.PrintTrace("Systems cannot link to themselves.");
				continue;
			}
			if(remove)
				links.erase(GameData::Systems().Get(value));
			else
				links.insert(GameData::Systems().Get(value));
		}
		else if(key == "asteroids")
		{
			if(remove)
			{
				for(auto it = asteroids.begin(); it != asteroids.end(); ++it)
					if(it->Name() == value)
					{
						asteroids.erase(it);
						break;
					}
			}
			else if(child.Size() > valueIndex + 2)
				asteroids.emplace_back(value, child.Value(valueIndex + 1), child.Value(valueIndex + 2));
			else
				child.PrintTrace("Expected " + to_string(valueIndex + 3)
					+ " tokens. Found " + to_string(child.Size()) + ":");
		}
		else if(key == "minables")
		{
			const Minable *type = GameData::Minables().Get(value);
			if(remove)
			{
				for(auto it = asteroids.begin(); it != asteroids.end(); ++it)
					if(it->Type() == type)
					{
						asteroids.erase(it);
						break;
					}
			}
			else if(child.Size() > valueIndex + 2)
				asteroids.emplace_back(type, child.Value(valueIndex + 1), child.Value(valueIndex + 2));
			else
				child.PrintTrace("Expected " + to_string(valueIndex + 3)
					+ " tokens. Found " + to_string(child.Size()) + ":");
		}
		else if(key == "fleet")
		{
			const Fleet *fleet = GameData::Fleets().Get(value);
			if(remove)
			{
				for(auto it = fleets.begin(); it != fleets.end(); ++it)
					if(it->Get() == fleet)
					{
						fleets.erase(it);
						break;
					}
			}
			else
				fleets.emplace_back(fleet, child.Value(valueIndex + 1), child, playerConditions);
		}
		else if(key == "raid")
			RaidFleet::Load(raidFleets, child, remove, valueIndex);
		else if(key == "hazard")
		{
			const Hazard *hazard = GameData::Hazards().Get(value);
			if(remove)
			{
				for(auto it = hazards.begin(); it != hazards.end(); ++it)
					if(it->Get() == hazard)
					{
						hazards.erase(it);
						break;
					}
			}
			else
			{
				hazards.emplace_back(hazard, child.Value(valueIndex + 1), child, playerConditions);
			}
		}
		else if(key == "belt")
		{
			double radius = child.Value(valueIndex);
			if(remove)
				erase(belts, radius);
			else
			{
				int weight = (child.Size() >= valueIndex + 2) ? max<int>(1, child.Value(valueIndex + 1)) : 1;
				belts.emplace_back(weight, radius);
			}
		}
		else if(key == "object")
		{
			if(remove)
			{
				StellarObject toRemoveTemplate;
				for(const DataNode &grand : child)
					LoadObjectHelper(grand, toRemoveTemplate, true);

				auto removeIt = find_if(objects.begin(), objects.end(),
					[&toRemoveTemplate](const StellarObject &object)
					{
						if(toRemoveTemplate.GetSprite() != object.GetSprite())
							return false;
						if(toRemoveTemplate.distance != object.distance)
							return false;
						if(toRemoveTemplate.speed != object.speed)
							return false;
						if(toRemoveTemplate.offset != object.offset)
							return false;
						return true;
					}
				);

				if(removeIt == objects.end())
				{
					child.PrintTrace("Did not find matching object for specified operation:");
					continue;
				}

				auto index = removeIt - objects.begin();
				auto last = removeIt + 1;
				size_t removed = 1;
				// Remove any child objects too.
				for( ; last != objects.end() && last->parent >= index; ++last, ++removed)
					if(last->planet)
						removedObjectPlanets.insert(last->planet);
				if(removeIt->planet)
					removedObjectPlanets.insert(removeIt->planet);
				last = objects.erase(removeIt, last);

				// Recalculate every parent index.
				for(auto it = last; it != objects.end(); ++it)
					if(it->parent >= index)
						it->parent -= removed;
			}
			else
				LoadObject(child, planets, playerConditions);
		}
		// Handle the attributes which cannot be "removed."
		else if(remove)
		{
			child.PrintTrace("Cannot \"remove\" a specific value from the given key:");
			continue;
		}
		else if(key == "display name" && hasValue)
			displayName = value;
		else if(key == "pos" && child.Size() >= 3)
		{
			position.Set(child.Value(valueIndex), child.Value(valueIndex + 1));
			hasPosition = true;
		}
		else if(key == "government")
			government = GameData::Governments().Get(value);
		else if(key == "music")
			music = value;
		else if(key == "habitable")
			habitable = child.Value(valueIndex);
		else if(key == "jump range")
			jumpRange = max(0., child.Value(valueIndex));
		else if(key == "haze")
			haze = SpriteSet::Get(value);
		else if(key == "starfield density")
			starfieldDensity = child.Value(valueIndex);
		else if(key == "trade" && child.Size() >= 3)
			trade[value].SetBase(child.Value(valueIndex + 1));
		else if(key == "arrival")
		{
			if(hasValue)
			{
				extraHyperArrivalDistance = child.Value(1);
				extraJumpArrivalDistance = fabs(child.Value(1));
			}
			for(const DataNode &grand : child)
			{
				const string &type = grand.Token(0);
				bool grandHasValue = grand.Size() >= 2;
				if(type == "link" && grandHasValue)
					extraHyperArrivalDistance = grand.Value(1);
				else if(type == "jump" && grandHasValue)
					extraJumpArrivalDistance = fabs(grand.Value(1));
				else
					grand.PrintTrace("Skipping unsupported arrival distance limitation:");
			}
		}
		else if(key == "departure")
		{
			if(hasValue)
			{
				jumpDepartureDistance = child.Value(1);
				hyperDepartureDistance = fabs(child.Value(1));
			}
			for(const DataNode &grand : child)
			{
				const string &type = grand.Token(0);
				bool grandHasValue = grand.Size() >= 2;
				if(type == "link" && grandHasValue)
					hyperDepartureDistance = grand.Value(1);
				else if(type == "jump" && grandHasValue)
					jumpDepartureDistance = fabs(grand.Value(1));
				else
					grand.PrintTrace("Skipping unsupported departure distance limitation:");
			}
		}
		else if(key == "invisible fence" && hasValue)
			invisibleFenceRadius = max(0., child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// Set planet messages based on what zone they are in and check if any planets
	// from removed objects are still present on other objects.
	for(StellarObject &object : objects)
	{
		if(object.planet)
		{
			removedObjectPlanets.erase(object.planet);
			continue;
		}

		if(object.message)
			continue;

		const StellarObject *root = &object;
		while(root->parent >= 0)
			root = &objects[root->parent];

		static const string STAR = "You cannot land on a star!";
		static const string HOTPLANET = "This planet is too hot to land on.";
		static const string COLDPLANET = "This planet is too cold to land on.";
		static const string UNINHABITEDPLANET = "This planet doesn't have anywhere you can land.";
		static const string HOTMOON = "This moon is too hot to land on.";
		static const string COLDMOON = "This moon is too cold to land on.";
		static const string UNINHABITEDMOON = "This moon doesn't have anywhere you can land.";
		static const string STATION = "This station cannot be docked with.";

		double fraction = root->distance / habitable;
		if(object.IsStar())
			object.message = &STAR;
		else if(object.IsStation())
			object.message = &STATION;
		else if(object.IsMoon())
		{
			if(fraction < .5)
				object.message = &HOTMOON;
			else if(fraction >= 2.)
				object.message = &COLDMOON;
			else
				object.message = &UNINHABITEDMOON;
		}
		else
		{
			if(fraction < .5)
				object.message = &HOTPLANET;
			else if(fraction >= 2.)
				object.message = &COLDPLANET;
			else
				object.message = &UNINHABITEDPLANET;
		}
	}
	// Tell any planets that were present but are no longer present in this system
	// that they are no longer in this system.
	for(const Planet *planet : removedObjectPlanets)
		planets.Get(planet->TrueName())->RemoveSystem(this);
	// Print a warning if this system wasn't explicitly given a position.
	if(!hasPosition)
		node.PrintTrace("System will be ignored due to missing position:");
	// Systems without an asteroid belt defined default to a radius of 1500.
	if(belts.empty())
		belts.emplace_back(1, 1500.);

	if(displayName.empty())
		displayName = trueName;
}



// Update any information about the system that may have changed due to events,
// or because the game was started, e.g. neighbors, solar wind and power, or
// if the system is inhabited.
void System::UpdateSystem(const Set<System> &systems, const set<double> &neighborDistances)
{
	accessibleLinks.clear();
	neighbors.clear();

	payloads.clear();
	for(const auto &asteroid : asteroids)
		if(asteroid.Type())
			for(const auto &payload : asteroid.Type()->GetPayload())
				payloads.insert(payload.outfit);

	// Some systems in the game may be considered inaccessible. If this system is inaccessible,
	// then it shouldn't have accessible links or jump neighbors.
	if(!IsValid() || inaccessible)
		return;

	// If linked systems are inaccessible, then they shouldn't be a part of the accessible links
	// set that gets used for navigation and other purposes.
	for(const System *link : links)
		if(link->IsValid() && !link->Inaccessible())
			accessibleLinks.insert(link);

	// Neighbors are cached for each system for the purpose of quicker
	// pathfinding. If this system has a static jump range then that
	// is the only range that we need to create jump neighbors for, but
	// otherwise we must create a set of neighbors for every potential
	// jump range that can be encountered.
	if(jumpRange)
	{
		UpdateNeighbors(systems, jumpRange);
		// Systems with a static jump range must also create a set for
		// the DEFAULT_NEIGHBOR_DISTANCE to be returned for those systems
		// which are visible from it.
		UpdateNeighbors(systems, DEFAULT_NEIGHBOR_DISTANCE);
	}
	else
		for(const double distance : neighborDistances)
			UpdateNeighbors(systems, distance);

	// Cache the map star icons.
	mapIcons.clear();
	for(const StellarObject &object : objects)
	{
		const Sprite *starIcon = GameData::StarIcon(object.GetSprite());
		if(starIcon)
			mapIcons.emplace_back(starIcon);
	}

	// Systems only have a single auto-attribute, "uninhabited." It is set if
	// the system has no inhabited planets that are accessible to all ships.
	if(IsInhabited(nullptr))
		attributes.erase("uninhabited");
	else
		attributes.insert("uninhabited");

	// Calculate the smallest arrival period of a fleet (or 0 if no fleets arrive)
	minimumFleetPeriod = numeric_limits<int>::max();
	for(auto &event : fleets)
		minimumFleetPeriod = min<int>(minimumFleetPeriod, event.Period());
	if(minimumFleetPeriod == numeric_limits<int>::max())
		minimumFleetPeriod = 0;
}



// Modify a system's links.
void System::Link(System *other)
{
	links.insert(other);
	other->links.insert(this);
	// accessibleLinks will be updated when UpdateSystem is called.
}



void System::Unlink(System *other)
{
	links.erase(other);
	other->links.erase(this);
	// accessibleLinks will be updated when UpdateSystem is called.
}



// Check that this system has been loaded and given a position.
bool System::IsValid() const
{
	return isDefined && hasPosition;
}



const string &System::TrueName() const
{
	return trueName;
}



void System::SetTrueName(const string &name)
{
	trueName = name;
	if(displayName.empty())
		displayName = trueName;
}



// Get this system's display name.
const string &System::DisplayName() const
{
	return displayName;
}



// Get this system's position in the star map.
const Point &System::Position() const
{
	return position;
}



// Get this system's government.
const Government *System::GetGovernment() const
{
	static const Government empty;
	return government ? government : &empty;
}



// Get this system's map icons.
const vector<const Sprite *> &System::GetMapIcons() const
{
	return mapIcons;
}



// Get the name of the ambient audio to play in this system.
const string &System::MusicName() const
{
	return music;
}



// Get the list of "attributes" of the planet.
const set<string> &System::Attributes() const
{
	return attributes;
}



// Get a list of systems you can travel to through hyperspace from here.
const set<const System *> &System::Links() const
{
	return accessibleLinks;
}



// Get a list of systems that can be jumped to from here with the given
// jump distance, whether or not there is a direct hyperspace link to them.
// If this system has its own jump range, then it will always return the
// systems within that jump range instead of the jump range given.
const set<const System *> &System::JumpNeighbors(double neighborDistance) const
{
	static const set<const System *> EMPTY;
	const auto it = neighbors.find(jumpRange ? jumpRange : neighborDistance);
	return it == neighbors.end() ? EMPTY : it->second;
}



// Defines whether this system can be seen when not linked. A hidden system will
// not appear when in view range, except when linked to a visited system.
bool System::Hidden() const
{
	return hidden;
}



// Defines whether a system can be remembered when out of view.
bool System::Shrouded() const
{
	return shrouded;
}



// Defines whether this system can be accessed or interacted with in any way.
bool System::Inaccessible() const
{
	return inaccessible;
}



// Return how much ramscoop fuel and solar energy/heat is generated by this system
// for a ship with the attributes and position.
System::SolarGeneration System::GetSolarGeneration(const Point &shipPosition,
	double shipRamscoop, double shipCollection, double shipCollectionHeat) const
{
	SolarGeneration generation{ramscoopAddend, 0., 0.};
	for(const auto &stellar : objects)
	{
		double power = GameData::SolarPower(stellar.GetSprite());
		double wind = GameData::SolarWind(stellar.GetSprite());
		double scale = .2 + 1.8 / (.001 * stellar.position.Distance(shipPosition) + 1);
		// Even if a ship has no ramscoop, it can harvest a tiny bit of fuel by flying close to the star,
		// provided the system allows it. Both the system and the gamerule must allow the universal ramscoop
		// in order for it to function.
		double universal = .05 * scale * universalRamscoop * GameData::GetGamerules().UniversalRamscoopActive();
		generation.fuel += wind * .03 * scale * ramscoopMultiplier * (sqrt(shipRamscoop) + universal);
		generation.energy += power * shipCollection * scale;
		generation.heat += power * shipCollectionHeat * scale;
	}
	generation.fuel = max(0., generation.fuel);
	return generation;
}



// Additional travel distance to target for ships entering through hyperspace.
double System::ExtraHyperArrivalDistance() const
{
	const optional<double> arrivalGamerule = GameData::GetGamerules().SystemArrivalMin();
	if(arrivalGamerule.has_value())
		return max(extraHyperArrivalDistance, *arrivalGamerule);
	return extraHyperArrivalDistance;
}



// Additional travel distance to target for ships entering using a jumpdrive.
double System::ExtraJumpArrivalDistance() const
{
	const optional<double> arrivalGamerule = GameData::GetGamerules().SystemArrivalMin();
	if(arrivalGamerule.has_value())
		return max(extraJumpArrivalDistance, *arrivalGamerule);
	return extraJumpArrivalDistance;
}



double System::JumpDepartureDistance() const
{
	return max(jumpDepartureDistance, GameData::GetGamerules().SystemDepartureMin());
}



double System::HyperDepartureDistance() const
{
	return max(hyperDepartureDistance, GameData::GetGamerules().SystemDepartureMin());
}



// Get a list of systems you can "see" from here, whether or not there is a
// direct hyperspace link to them.
const set<const System *> &System::VisibleNeighbors() const
{
	static const set<const System *> EMPTY;
	const auto it = neighbors.find(DEFAULT_NEIGHBOR_DISTANCE);
	return it == neighbors.end() ? EMPTY : it->second;
}



// Move the stellar objects to their positions on the given date.
void System::SetDate(const Date &date)
{
	double now = date.DaysSinceEpoch();

	for(StellarObject &object : objects)
	{
		// "offset" is used to allow binary orbits; the second object is offset
		// by 180 degrees.
		object.angle = Angle(now * object.speed + object.offset);
		object.position = object.angle.Unit() * object.distance;

		// Because of the order of the vector, the parent's position has always
		// been updated before this loop reaches any of its children, so:
		if(object.parent >= 0)
			object.position += objects[object.parent].position;

		if(object.position)
			object.angle = Angle(object.position);

		if(object.planet)
			object.planet->ResetDefense();
	}
}



// Get the stellar object locations on the most recently set date.
const vector<StellarObject> &System::Objects() const
{
	return objects;
}



// Get the stellar object (if any) for the given planet.
const StellarObject *System::FindStellar(const Planet *planet) const
{
	if(planet)
		for(const StellarObject &object : objects)
			if(object.GetPlanet() == planet)
				return &object;

	return nullptr;
}



// Get the habitable zone's center.
double System::HabitableZone() const
{
	return habitable;
}



// Get the radius of an asteroid belt.
double System::AsteroidBeltRadius() const
{
	return belts.Get();
}



// Get the list of asteroid belts.
const WeightedList<double> &System::AsteroidBelts() const
{
	return belts;
}



// Get the system's invisible fence radius.
double System::InvisibleFenceRadius() const
{
	return invisibleFenceRadius;
}



// Get how far ships can jump from this system.
double System::JumpRange() const
{
	return jumpRange;
}



double System::StarfieldDensity() const
{
	return starfieldDensity;
}



// Check if this system is inhabited.
bool System::IsInhabited(const Ship *ship) const
{
	for(const StellarObject &object : objects)
		if(object.HasSprite() && object.HasValidPlanet())
		{
			const Planet &planet = *object.GetPlanet();
			if(!planet.IsWormhole() && planet.IsInhabited() && planet.IsAccessible(ship))
				return true;
		}

	return false;
}



// Check if ships of the given government can refuel in this system.
bool System::HasFuelFor(const Ship &ship) const
{
	for(const StellarObject &object : objects)
		if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->HasFuelFor(ship))
			return true;

	return false;
}



// Check whether you can buy or sell ships in this system.
bool System::HasShipyard() const
{
	for(const StellarObject &object : objects)
		if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->HasShipyard())
			return true;

	return false;
}



// Check whether you can buy or sell ship outfits in this system.
bool System::HasOutfitter() const
{
	for(const StellarObject &object : objects)
		if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->HasOutfitter())
			return true;

	return false;
}



// Get the specification of how many asteroids of each type there are.
const vector<System::Asteroid> &System::Asteroids() const
{
	return asteroids;
}



// Get a list of all unique payload outfits from minables in this system.
const set<const Outfit *> &System::Payloads() const
{
	return payloads;
}



// Get the background haze sprite for this system.
const Sprite *System::Haze() const
{
	return haze;
}



// Get the price of the given commodity in this system.
int System::Trade(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second.price;
}



bool System::HasTrade() const
{
	return !trade.empty();
}



// Update the economy.
void System::StepEconomy()
{
	for(auto &it : trade)
	{
		it.second.exports = EXPORT * it.second.supply;
		it.second.supply *= KEEP;
		it.second.supply += Random::Normal() * VOLUME;
		it.second.Update();
	}
}



void System::SetSupply(const string &commodity, double tons)
{
	auto it = trade.find(commodity);
	if(it == trade.end())
		return;

	it->second.supply = tons;
	it->second.Update();
}



double System::Supply(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second.supply;
}



double System::Exports(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0 : it->second.exports;
}



// Get the probabilities of various fleets entering this system.
const vector<RandomEvent<Fleet>> &System::Fleets() const
{
	return fleets;
}



// Get the probabilities of various hazards in this system.
const vector<RandomEvent<Hazard>> &System::Hazards() const
{
	return hazards;
}



// Check how dangerous this system is (credits worth of enemy ships jumping
// in per frame).
double System::Danger() const
{
	double danger = 0.;
	for(const auto &fleet : fleets)
	{
		auto *gov = fleet.Get()->GetGovernment();
		if(gov && gov->IsEnemy())
			danger += static_cast<double>(fleet.Get()->Strength()) / fleet.Period();
	}
	return danger;
}



int System::MinimumFleetPeriod() const
{
	return minimumFleetPeriod;
}



const vector<RaidFleet> &System::RaidFleets() const
{
	static const vector<RaidFleet> EMPTY;
	// If the system defines its own raid fleets then those are used in lieu of the government's fleets.
	return noRaids ? EMPTY : ((raidFleets.empty() && government) ? government->RaidFleets() : raidFleets);
}



void System::LoadObject(const DataNode &node, Set<Planet> &planets,
		const ConditionsStore *playerConditions, int parent)
{
	int index = objects.size();
	objects.push_back(StellarObject());
	StellarObject &object = objects.back();
	object.parent = parent;

	bool isAdded = (node.Token(0) == "add");
	if(node.Size() >= 2 + isAdded)
	{
		Planet *planet = planets.Get(node.Token(1 + isAdded));
		object.planet = planet;
		planet->SetSystem(this);
	}

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "hazard" && child.Size() >= 3)
			object.hazards.emplace_back(GameData::Hazards().Get(child.Token(1)), child.Value(2),
				child, playerConditions);
		else if(key == "object")
			LoadObject(child, planets, playerConditions, index);
		else
			LoadObjectHelper(child, object);
	}
}



void System::LoadObjectHelper(const DataNode &node, StellarObject &object, bool removing) const
{
	const string &key = node.Token(0);
	bool hasValue = node.Size() >= 2;
	if(key == "sprite" && hasValue)
	{
		object.LoadSprite(node);
		if(removing)
			return;
		object.isStar = node.Token(1).starts_with("star/");
		if(!object.isStar)
		{
			object.isStation = node.Token(1).starts_with("planet/station");
			object.isMoon = (!object.isStation && object.parent >= 0 && !objects[object.parent].IsStar());
		}
	}
	else if(key == "distance" && hasValue)
		object.distance = node.Value(1);
	else if(key == "period" && hasValue)
		object.speed = 360. / node.Value(1);
	else if(key == "offset" && hasValue)
		object.offset = node.Value(1);
	else if(key == "swizzle" && hasValue)
		object.SetSwizzle(GameData::Swizzles().Get(node.Token(1)));
	else if(key == "visibility" && hasValue)
	{
		object.distanceInvisible = node.Value(1);
		if(node.Size() >= 3)
			object.distanceVisible = node.Value(2);
	}
	else if(removing && (key == "hazard" || key == "object"))
		node.PrintTrace("Key \"" + key + "\" cannot be removed from an object:");
	else
		node.PrintTrace("Skipping unrecognized attribute:");
}



// Once the star map is fully loaded or an event has changed systems
// or links, figure out which stars are "neighbors" of this one, i.e.
// close enough to see or to reach via jump drive.
void System::UpdateNeighbors(const Set<System> &systems, double distance)
{
	set<const System *> &neighborSet = neighbors[distance];

	// Every accessible star system that is linked to this one is automatically a neighbor,
	// even if it is farther away than the maximum distance.
	for(const System *system : accessibleLinks)
		neighborSet.insert(system);

	// Any other star system that is within the neighbor distance is also a
	// neighbor.
	for(const auto &it : systems)
	{
		const System &other = it.second;
		// Skip systems that are invalid or inaccessible.
		if(!other.IsValid() || other.Inaccessible())
			continue;

		if(&other != this && other.Position().Distance(position) <= distance)
			neighborSet.insert(&other);
	}
}



void System::Price::SetBase(int base)
{
	this->base = base;
	this->price = base;
}



void System::Price::Update()
{
	price = base + static_cast<int>(-100. * erf(supply / LIMIT));
}
