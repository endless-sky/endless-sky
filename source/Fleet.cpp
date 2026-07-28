/* Fleet.cpp
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

#include "Fleet.h"

#include "DataNode.h"
#include "FormationPattern.h"
#include "GameData.h"
#include "Government.h"
#include "Logger.h"
#include "Phrase.h"
#include "Planet.h"
#include "Random.h"
#include "Ship.h"
#include "ShipJumpNavigation.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>

using namespace std;

namespace {
	// Generate an offset magnitude that will sample from an annulus (planets)
	// or a circle (systems without inhabited planets).
	double OffsetFrom(const pair<Point, double> &center)
	{
		// If the center has a radius, then position ships further away.
		double minimumOffset = center.second ? 1. : 0.;
		// Since it is sensible that ships would be nearer to the object of
		// interest on average, do not apply the sqrt(rand) correction.
		return (Random::Real() + minimumOffset) * 400. + 2. * center.second;
	}
}



// Construct and Load() at the same time.
Fleet::Fleet(const DataNode &node)
{
	Load(node);
}



void Fleet::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		fleetName = node.Token(1);

	// If Load() has already been called once on this fleet, any subsequent
	// calls will replace the variants instead of adding to them.
	bool resetVariants = !variants.empty();

	for(const DataNode &child : node)
	{
		// The "add" and "remove" keywords should never be alone on a line, and
		// are only valid with "variant" or "personality" definitions.
		bool add = (child.Token(0) == "add");
		bool remove = (child.Token(0) == "remove");
		bool hasValue = child.Size() >= 2;
		if((add || remove) && (!hasValue || (child.Token(1) != "variant" && child.Token(1) != "personality")))
		{
			child.PrintTrace("Skipping invalid \"" + child.Token(0) + "\" tag:");
			continue;
		}

		// If this line is an add or remove, the key is the token at index 1.
		const string &key = child.Token(add || remove);

		if(key == "government" && hasValue)
			government = GameData::Governments().Get(child.Token(1));
		else if(key == "names" && hasValue)
			names = GameData::Phrases().Get(child.Token(1));
		else if(key == "fighters" && (hasValue || child.HasChildren()))
		{
			if(hasValue)
				fighterNames = GameData::Phrases().Get(child.Token(1));
			for(const DataNode &grand : child)
			{
				const string &fighterKey = grand.Token(0);
				if(fighterKey == "names" && grand.Size() >= 2)
					fighterNames = GameData::Phrases().Get(grand.Token(1));
				else if(fighterKey == "personality")
					fighterPersonality.Load(grand);
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else if(key == "cargo settings" && child.HasChildren())
			cargo.Load(child);
		// Allow certain individual cargo settings to be direct children
		// of Fleet for backwards compatibility.
		else if(key == "cargo" || key == "commodities" || key == "outfitters")
			cargo.LoadSingle(child);
		else if(key == "personality")
			personality.Load(child);
		else if(key == "formation" && hasValue)
			formation = GameData::Formations().Get(child.Token(1));
		else if(key == "variant" && !remove)
		{
			if(resetVariants && !add)
			{
				resetVariants = false;
				variants.clear();
			}
			int weight = (child.Size() >= add + 2) ? max<int>(1, child.Value(add + 1)) : 1;
			variants.emplace_back(weight, child);
		}
		else if(key == "variant")
		{
			// If given a full definition of one of this fleet's variant members, remove the variant.
			Variant toRemove(child);
			int count = erase(variants, toRemove);
			if(!count)
				child.PrintTrace("Did not find matching variant for specified operation:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	if(variants.empty())
		node.PrintTrace((fleetName.empty() ? "Unnamed fleet" : "Fleet \"" + fleetName + "\"")
			+ " contains no variants:");
}



bool Fleet::IsValid(bool requireGovernment) const
{
	// Generally, a government is required for a fleet to be valid.
	if(requireGovernment && !government)
		return false;

	if(names && names->IsEmpty())
		return false;

	if(fighterNames && fighterNames->IsEmpty())
		return false;

	// Any variant a fleet could choose should be valid.
	if(any_of(variants.begin(), variants.end(),
			[](const Variant &v) noexcept -> bool { return !v.IsValid(); }))
		return false;

	return true;
}



void Fleet::RemoveInvalidVariants()
{
	int total = variants.TotalWeight();
	int count = erase_if(variants, [](const Variant &v) noexcept -> bool { return !v.IsValid(); });
	if(!count)
		return;

	Logger::Log((fleetName.empty() ? "Unnamed fleet" : "Fleet \"" + fleetName + "\"")
		+ ": Removing " + to_string(count) + " invalid " + (count > 1 ? "variants" : "variant")
		+ " (" + to_string(total - variants.TotalWeight()) + " of " + to_string(total) + " weight).",
		Logger::Level::WARNING);
}



// Get the government of this fleet.
const Government *Fleet::GetGovernment() const
{
	return government;
}



// Choose a fleet to be created during flight, and have it enter the system via jump or planetary departure.
void Fleet::Enter(const System &system, list<shared_ptr<Ship>> &ships, const Planet *planet) const
{
	if(variants.empty() || personality.IsDerelict())
		return;

	// Pick a fleet variant to instantiate.
	const vector<const Ship *> &variantShips = variants.Get().Ships();
	if(variantShips.empty())
		return;

	// Figure out what system the fleet is starting in, where it is going, and
	// what position it should start from in the system.
	const System *source = &system;
	const System *target = &system;
	Point position;
	double radius = 1000.;

	// The chosen stellar object the fleet will depart from, if any.
	const StellarObject *object = nullptr;
	// Only pick a random entry point for this fleet if a source planet was not specified.
	if(!planet)
	{
		// Where this fleet can come from depends on whether it is friendly to any
		// planets in this system and whether it has jump drives.
		vector<const System *> linkVector;
		// Find out what the "best" jump method the fleet has is. Assume that if the
		// others don't have that jump method, they are being carried as fighters.
		// That is, content creators should avoid creating fleets with a mix of jump
		// drives and hyperdrives.
		bool hasJump = false;
		bool hasHyper = false;
		double jumpDistance = System::DEFAULT_NEIGHBOR_DISTANCE;
		for(const Ship *ship : variantShips)
		{
			if(ship->JumpNavigation().HasJumpDrive())
			{
				hasJump = true;
				jumpDistance = ship->JumpNavigation().JumpRange();
				break;
			}
			if(ship->JumpNavigation().HasHyperdrive())
				hasHyper = true;
		}
		const bool unrestricted = personality.IsUnrestricted();
		// Don't try to make a fleet "enter" from another system if none of the
		// ships have jump drives.
		if(hasJump || hasHyper)
		{
			bool isWelcomeHere = !system.GetGovernment()->IsEnemy(government);
			for(const System *neighbor : (hasJump ? system.JumpNeighbors(jumpDistance) : system.Links()))
			{
				if(!unrestricted && government->IsRestrictedFrom(*neighbor))
					continue;
				// If this ship is not "welcome" in the current system, prefer to have
				// it enter from a system that is friendly to it. (This is for realism,
				// so attack fleets don't come from what ought to be a safe direction.)
				if(isWelcomeHere || neighbor->GetGovernment()->IsEnemy(government))
					linkVector.push_back(neighbor);
				else
					linkVector.insert(linkVector.end(), 8, neighbor);
			}
		}

		// Find all the inhabited planets this fleet could take off from.
		vector<const StellarObject *> stellarVector;
		if(!personality.IsSurveillance())
			for(const StellarObject &object : system.Objects())
				if(object.HasValidPlanet() && object.GetPlanet()->IsInhabited()
						&& (unrestricted || !government->IsRestrictedFrom(*object.GetPlanet()))
						&& !object.GetPlanet()->GetGovernment()->IsEnemy(government))
					stellarVector.push_back(&object);

		// If there is nowhere for this fleet to come from, don't create it.
		size_t options = linkVector.size() + stellarVector.size();
		if(!options)
		{
			// Prefer to launch from inhabited planets, but launch from
			// uninhabited ones if there is no other option.
			for(const StellarObject &object : system.Objects())
				if(object.HasValidPlanet()
						&& (unrestricted || !government->IsRestrictedFrom(*object.GetPlanet()))
						&& !object.GetPlanet()->GetGovernment()->IsEnemy(government))
					stellarVector.push_back(&object);
			options = stellarVector.size();
			if(!options)
				return;
		}

		// Choose a random planet or star system to come from.
		size_t choice = Random::Int(options);

		// If a planet is chosen, also pick a system to travel to after taking off.
		if(choice >= linkVector.size())
		{
			object = stellarVector[choice - linkVector.size()];
			planet = object->GetPlanet();
			if(!linkVector.empty())
				target = linkVector[Random::Int(linkVector.size())];
		}
		// We are entering this system via hyperspace, not taking off from a planet.
		else
			source = linkVector[choice];
	}

	auto placed = Instantiate(variantShips);
	// Carry all ships that can be carried, as they don't need to be positioned
	// or checked to see if they can access a particular planet.
	for(auto &ship : placed)
		PlaceFighter(ship, placed);

	// Find the stellar object for this planet if necessary, and place the ships there.
	if(planet)
	{
		if(!object)
		{
			// Search the stellar object associated with the given planet.
			// If there are many possible candidates (for example for ringworlds),
			// then choose a random one.
			vector<const StellarObject *> stellarObjects;
			for(const auto &object : system.Objects())
				if(object.GetPlanet() == planet)
					stellarObjects.push_back(&object);

			// If the source planet isn't in the source for some reason, bail out.
			if(stellarObjects.empty())
			{
				// Log this error.
				Logger::Log("Fleet::Enter: Unable to find valid stellar object for planet \""
					+ planet->TrueName() + "\" in system \"" + system.TrueName() + "\".",
					Logger::Level::WARNING);
				return;
			}

			object = stellarObjects[Random::Int(stellarObjects.size())];
		}


		// To take off from the planet, all non-carried ships must be able to access it.
		if(planet->IsUnrestricted() || all_of(placed.cbegin(), placed.cend(), [&](const shared_ptr<Ship> &ship)
				{ return ship->GetParent() || planet->IsAccessible(ship.get()); }))
		{
			position = object->Position();
			radius = object->Radius();
		}
		// The chosen planet could not be departed from by all ships in the variant.
		else
		{
			// If there are no departure paths, then there are no arrival paths either.
			if(source == target)
				return;
			// Otherwise, have the fleet arrive here from the target system.
			swap(source, target);
			planet = nullptr;
		}
	}

	// Place all the ships in the chosen fleet variant.
	shared_ptr<Ship> flagship;
	for(shared_ptr<Ship> &ship : placed)
	{
		// If this is a carried fighter, no need to position it.
		if(ship->GetParent())
			continue;

		Angle angle = Angle::Random(360.);
		Point pos = position + angle.Unit() * (Random::Real() * radius);

		ships.push_front(ship);
		ship->SetSystem(source);
		ship->SetPlanet(planet);
		if(source == &system)
			ship->Place(pos, angle.Unit(), angle);
		else
		{
			// Place the ship stationary and pointed in the right direction.
			angle = Angle(system.Position() - source->Position());
			ship->Place(pos, Point(), angle);
		}
		if(target != source)
			ship->SetTargetSystem(target);

		if(flagship)
			ship->SetParent(flagship);
		else
			flagship = ship;

		cargo.SetCargo(&*ship);
	}
}



// Place one of the variants in the given system, already "in action." If the carried flag is set,
// only uncarried ships will be added to the list (as any carriables will be stored in bays).
void Fleet::Place(const System &system, list<shared_ptr<Ship>> &ships, bool carried, bool addCargo) const
{
	if(variants.empty())
		return;

	// Pick a fleet variant to instantiate.
	const vector<const Ship *> &variantShips = variants.Get().Ships();
	if(variantShips.empty())
		return;

	// Determine where the fleet is going to or coming from.
	auto center = ChooseCenter(system);

	// Place all the ships in the chosen fleet variant.
	shared_ptr<Ship> flagship;
	vector<shared_ptr<Ship>> placed = Instantiate(variantShips);
	for(shared_ptr<Ship> &ship : placed)
	{
		// If this is a fighter and someone can carry it, no need to position it.
		if(carried && PlaceFighter(ship, placed))
			continue;

		Angle angle = Angle::Random();
		Point pos = center.first + Angle::Random().Unit() * OffsetFrom(center);
		double velocity = 0;
		if(!ship->GetPersonality().IsDerelict())
			velocity = Random::Real() * ship->MaxVelocity();
		else
			ship->Disable();

		ships.push_front(ship);
		ship->SetSystem(&system);
		ship->Place(pos, velocity * angle.Unit(), angle);

		if(flagship)
			ship->SetParent(flagship);
		else
			flagship = ship;

		if(addCargo)
			cargo.SetCargo(&*ship);
	}
}



// Do the randomization to make a ship enter or be in the given system.
const System *Fleet::Enter(const System &system, Ship &ship, const System *source)
{
	bool canEnter = (source != nullptr || any_of(system.Links().begin(), system.Links().end(),
		[&ship](const System *link) noexcept -> bool
		{
			return !ship.IsRestrictedFrom(*link);
		}
	));

	if(!canEnter || system.Links().empty() || (source && !system.Links().contains(source)))
	{
		Place(system, ship);
		return &system;
	}

	// Choose which system this ship is coming from.
	if(!source)
	{
		vector<const System *> validSystems;
		for(const System *link : system.Links())
			if(!ship.IsRestrictedFrom(*link))
				validSystems.emplace_back(link);
		auto it = validSystems.cbegin();
		advance(it, Random::Int(validSystems.size()));
		source = *it;
	}

	Angle angle = Angle::Random();
	Point pos = angle.Unit() * Random::Real() * 1000.;

	ship.Place(pos, angle.Unit(), angle);
	ship.SetSystem(source);
	ship.SetTargetSystem(&system);

	return source;
}



void Fleet::Place(const System &system, Ship &ship)
{
	// Choose a random inhabited object in the system to spawn around.
	auto center = ChooseCenter(system);
	Point pos = center.first + Angle::Random().Unit() * OffsetFrom(center);

	double velocity = ship.IsDisabled() ? 0. : Random::Real() * ship.MaxVelocity();

	ship.SetSystem(&system);
	Angle angle = Angle::Random();
	ship.Place(pos, velocity * angle.Unit(), angle);
}



int64_t Fleet::Strength() const
{
	return variants.Average(mem_fn(&Variant::Strength));
}



// Obtain a positional reference and the radius of the object at that position (e.g. a planet).
// Spaceport status can be modified during normal gameplay, so this information is not cached.
pair<Point, double> Fleet::ChooseCenter(const System &system)
{
	auto centers = vector<pair<Point, double>>();
	for(const StellarObject &object : system.Objects())
		if(object.HasValidPlanet() && object.GetPlanet()->IsInhabited())
			centers.emplace_back(object.Position(), object.Radius());

	if(centers.empty())
		return {Point(), 0.};
	return centers[Random::Int(centers.size())];
}



vector<shared_ptr<Ship>> Fleet::Instantiate(const vector<const Ship *> &ships) const
{
	vector<shared_ptr<Ship>> placed;
	for(const Ship *model : ships)
	{
		// At least one of this variant's ships is valid, but we should avoid spawning any that are not defined.
		if(!model->IsValid())
		{
			Logger::Log("Skipping invalid ship model \"" + model->TrueModelName()
				+ "\" in fleet \"" + fleetName + "\".", Logger::Level::WARNING);
			continue;
		}

		// Copy the model instance into a new instance.
		auto ship = make_shared<Ship>(*model);

		bool canBeCarried = ship->CanBeCarried();
		const Phrase *phrase = ((canBeCarried && fighterNames) ? fighterNames : names);
		if(phrase)
			ship->SetGivenName(phrase->Get());
		ship->SetGovernment(government);
		if(canBeCarried && fighterPersonality.IsDefined())
			ship->SetPersonality(fighterPersonality);
		else
			ship->SetPersonality(personality);
		ship->SetFormationPattern(formation);

		placed.push_back(ship);
	}
	return placed;
}



bool Fleet::PlaceFighter(const shared_ptr<Ship> &fighter, vector<shared_ptr<Ship>> &placed) const
{
	if(!fighter->CanBeCarried())
		return false;

	for(const shared_ptr<Ship> &parent : placed)
		if(parent->Carry(fighter))
			return true;

	return false;
}
