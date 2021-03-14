/* Fleet.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Fleet.h"

#include "DataNode.h"
#include "Files.h"
#include "GameData.h"
#include "Government.h"
#include "Phrase.h"
#include "pi.h"
#include "Planet.h"
#include "Random.h"
#include "Ship.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <iterator>

using namespace std;

namespace {
	// Generate an offset magnitude that will sample from an annulus (planets)
	// or a circle (systems without inhabited planets).
	double OffsetFrom(pair<Point, double> &center)
	{
		// If the center has a radius, then position ships further away.
		double minimumOffset = center.second ? 1. : 0.;
		// Since it is sensible that ships would be nearer to the object of
		// interest on average, do not apply the sqrt(rand) correction.
		return (Random::Real() + minimumOffset) * 400. + 2. * center.second;
	}
	
	// Construct a list of all outfits for sale in this system and its linked neighbors.
	Sale<Outfit> GetOutfitsForSale(const System *here)
	{
		auto outfits = Sale<Outfit>();
		if(here)
		{
			for(const StellarObject &object : here->Objects())
			{
				const Planet *planet = object.GetPlanet();
				if(planet && planet->HasOutfitter())
					outfits.Add(planet->Outfitter());
			}
		}
		return outfits;
	}
	
	// Construct a list of varying numbers of outfits that were either specified for
	// this fleet directly, or are sold in this system or its linked neighbors.
	vector<const Outfit *> OutfitChoices(const set<const Sale<Outfit> *> &outfitters, const System *hub, int maxSize)
	{
		auto outfits = vector<const Outfit *>();
		if(maxSize > 0)
		{
			auto choices = Sale<Outfit>();
			// If no outfits were directly specified, choose from those sold nearby.
			if(outfitters.empty() && hub)
			{
				choices = GetOutfitsForSale(hub);
				for(const System *other : hub->Links())
					choices.Add(GetOutfitsForSale(other));
			}
			else
				for(const auto outfitter : outfitters)
					choices.Add(*outfitter);
			
			if(!choices.empty())
			{
				for(const auto outfit : choices)
				{
					double mass = outfit->Mass();
					// Avoid free outfits, massless outfits, and those too large to fit.
					if(mass > 0. && mass < maxSize && outfit->Cost() > 0)
					{
						// Also avoid outfits that add space (such as Outfits / Cargo Expansions)
						// or modify bunks.
						// TODO: Specify rejection criteria in datafiles as ConditionSets or similar.
						const auto &attributes = outfit->Attributes();
						if(attributes.Get("outfit space") > 0.
								|| attributes.Get("cargo space") > 0.
								|| attributes.Get("bunks"))
							continue;
						
						outfits.push_back(outfit);
					}
				}
			}
		}
		// Sort this list of choices ascending by mass, so it can be easily trimmed to just
		// the outfits that fit as the ship's free space decreases.
		sort(outfits.begin(), outfits.end(), [](const Outfit *a, const Outfit *b)
			{ return a->Mass() < b->Mass(); });
		return outfits;
	}
	
	// Add a random commodity from the list to the ship's cargo.
	void AddRandomCommodity(Ship &ship, int freeSpace, const vector<string> &commodities)
	{
		int index = Random::Int(GameData::Commodities().size());
		if(!commodities.empty())
		{
			// If a list of possible commodities was given, pick one of them at
			// random and then double-check that it's a valid commodity name.
			const string &name = commodities[Random::Int(commodities.size())];
			for(const auto &it : GameData::Commodities())
				if(it.name == name)
				{
					index = &it - &GameData::Commodities().front();
					break;
				}
		}
		
		const Trade::Commodity &commodity = GameData::Commodities()[index];
		int amount = Random::Int(freeSpace) + 1;
		ship.Cargo().Add(commodity.name, amount);
	}
	
	// Add a random outfit from the list to the ship's cargo.
	void AddRandomOutfit(Ship &ship, int freeSpace, const vector<const Outfit *> &outfits)
	{
		if(outfits.empty())
			return;
		int index = Random::Int(outfits.size());
		const Outfit *picked = outfits[index];
		int maxQuantity = floor(static_cast<double>(freeSpace) / picked->Mass());
		int amount = Random::Int(maxQuantity) + 1;
		ship.Cargo().Add(picked, amount);
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
		bool hasValue = (child.Size() >= 2);
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
		else if(key == "fighters" && hasValue)
			fighterNames = GameData::Phrases().Get(child.Token(1));
		else if(key == "cargo" && hasValue)
			cargo = static_cast<int>(child.Value(1));
		else if(key == "commodities" && hasValue)
		{
			commodities.clear();
			for(int i = 1; i < child.Size(); ++i)
				commodities.push_back(child.Token(i));
		}
		else if(key == "outfitters" && hasValue)
		{
			outfitters.clear();
			for(int i = 1; i < child.Size(); ++i)
				outfitters.insert(GameData::Outfitters().Get(child.Token(i)));
		}
		else if(key == "personality")
			personality.Load(child);
		else if(key == "variant" && !remove)
		{
			if(resetVariants && !add)
			{
				resetVariants = false;
				variants.clear();
				total = 0;
			}
			variants.emplace_back(child);
			total += variants.back().weight;
		}
		else if(key == "variant")
		{
			// If given a full ship definition of one of this fleet's variant members, remove the variant.
			bool didRemove = false;
			Variant toRemove(child);
			for(auto it = variants.begin(); it != variants.end(); ++it)
				if(toRemove.ships.size() == it->ships.size() &&
					is_permutation(it->ships.begin(), it->ships.end(), toRemove.ships.begin()))
				{
					total -= it->weight;
					variants.erase(it);
					didRemove = true;
					break;
				}
			
			if(!didRemove)
				child.PrintTrace("Did not find matching variant for specified operation:");
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	if(variants.empty())
		node.PrintTrace("Warning: " + (fleetName.empty() ? "unnamed fleet" : "Fleet \"" + fleetName + "\"") + " contains no variants:");
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
	
	// A fleet's variants should reference at least one valid ship.
	for(auto &&v : variants)
		if(none_of(v.ships.begin(), v.ships.end(),
				[](const Ship *const s) noexcept -> bool { return s->IsValid(); }))
			return false;
	
	return true;
}



// Get the government of this fleet.
const Government *Fleet::GetGovernment() const
{
	return government;
}


// Choose a fleet to be created during flight, and have it enter the system via jump or planetary departure.
void Fleet::Enter(const System &system, list<shared_ptr<Ship>> &ships, const Planet *planet) const
{
	if(!total || variants.empty())
		return;
	
	// Pick a fleet variant to instantiate.
	const Variant &variant = ChooseVariant();
	if(variant.ships.empty())
		return;
	
	// Figure out what system the fleet is starting in, where it is going, and
	// what position it should start from in the system.
	const System *source = &system;
	const System *target = &system;
	Point position;
	double radius = 1000.;
	
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
		for(const Ship *ship : variant.ships)
		{
			if(ship->Attributes().Get("jump drive"))
			{
				hasJump = true;
				jumpDistance = ship->JumpRange();
				break;
			}
			if(ship->Attributes().Get("hyperdrive"))
				hasHyper = true;
		}
		// Don't try to make a fleet "enter" from another system if none of the
		// ships have jump drives.
		if(hasJump || hasHyper)
		{
			bool isWelcomeHere = !system.GetGovernment()->IsEnemy(government);
			for(const System *neighbor : (hasJump ? system.JumpNeighbors(jumpDistance) : system.Links()))
			{
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
		vector<const Planet *> planetVector;
		if(!personality.IsSurveillance())
			for(const StellarObject &object : system.Objects())
				if(object.GetPlanet() && object.GetPlanet()->HasSpaceport()
						&& !object.GetPlanet()->GetGovernment()->IsEnemy(government))
					planetVector.push_back(object.GetPlanet());
	
		// If there is nowhere for this fleet to come from, don't create it.
		size_t options = linkVector.size() + planetVector.size();
		if(!options)
		{
			// Prefer to launch from inhabited planets, but launch from
			// uninhabited ones if there is no other option.
			for(const StellarObject &object : system.Objects())
				if(object.GetPlanet() && !object.GetPlanet()->GetGovernment()->IsEnemy(government))
					planetVector.push_back(object.GetPlanet());
			options = planetVector.size();
			if(!options)
				return;
		}
		
		// Choose a random planet or star system to come from.
		size_t choice = Random::Int(options);
	
		// If a planet is chosen, also pick a system to travel to after taking off.
		if(choice >= linkVector.size())
		{
			planet = planetVector[choice - linkVector.size()];
			if(!linkVector.empty())
				target = linkVector[Random::Int(linkVector.size())];
		}
		// We are entering this system via hyperspace, not taking off from a planet.
		else
			source = linkVector[choice];
	}
	
	auto placed = Instantiate(variant);
	// Carry all ships that can be carried, as they don't need to be positioned
	// or checked to see if they can access a particular planet.
	for(auto &ship : placed)
		PlaceFighter(ship, placed);
	
	// Find the stellar object for this planet, and place the ships there.
	if(planet)
	{
		const StellarObject *object = system.FindStellar(planet);
		if(!object)
		{
			// Log this error.
			Files::LogError("Fleet::Enter: Unable to find valid stellar object for planet \""
				+ planet->TrueName() + "\" in system \"" + system.Name() + "\"");
			return;
		}
		// To take off from the planet, all non-carried ships must be able to access it.
		else if(planet->IsUnrestricted() || all_of(placed.cbegin(), placed.cend(), [&](const shared_ptr<Ship> &ship)
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
			std::swap(source, target);
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
		
		SetCargo(&*ship);
	}
}



// Place one of the variants in the given system, already "in action." If the carried flag is set,
// only uncarried ships will be added to the list (as any carriables will be stored in bays).
void Fleet::Place(const System &system, list<shared_ptr<Ship>> &ships, bool carried) const
{
	if(!total || variants.empty())
		return;
	
	// Pick a fleet variant to instantiate.
	const Variant &variant = ChooseVariant();
	if(variant.ships.empty())
		return;
	
	// Determine where the fleet is going to or coming from.
	auto center = ChooseCenter(system);
	
	// Place all the ships in the chosen fleet variant.
	shared_ptr<Ship> flagship;
	vector<shared_ptr<Ship>> placed = Instantiate(variant);
	for(shared_ptr<Ship> &ship : placed)
	{
		// If this is a fighter and someone can carry it, no need to position it.
		if(carried && PlaceFighter(ship, placed))
			continue;
		
		Angle angle = Angle::Random();
		Point pos = center.first + Angle::Random().Unit() * OffsetFrom(center);
		double velocity = Random::Real() * ship->MaxVelocity();
		
		ships.push_front(ship);
		ship->SetSystem(&system);
		ship->Place(pos, velocity * angle.Unit(), angle);
		
		if(flagship)
			ship->SetParent(flagship);
		else
			flagship = ship;
		
		SetCargo(&*ship);
	}
}



// Do the randomization to make a ship enter or be in the given system.
const System *Fleet::Enter(const System &system, Ship &ship, const System *source)
{
	if(system.Links().empty() || (source && !system.Links().count(source)))
	{
		Place(system, ship);
		return &system;
	}
	
	// Choose which system this ship is coming from.
	if(!source)
	{
		auto it = system.Links().cbegin();
		advance(it, Random::Int(system.Links().size()));
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
	if(!total || variants.empty())
		return 0;
	
	int64_t sum = 0;
	for(const Variant &variant : variants)
	{
		int64_t thisSum = 0;
		for(const Ship *ship : variant.ships)
			thisSum += ship->Cost();
		sum += thisSum * variant.weight;
	}
	return sum / total;
}



Fleet::Variant::Variant(const DataNode &node)
{
	weight = 1;
	if(node.Token(0) == "variant" && node.Size() >= 2)
		weight = node.Value(1);
	else if(node.Token(0) == "add" && node.Size() >= 3)
		weight = node.Value(2);
	
	for(const DataNode &child : node)
	{
		int n = 1;
		if(child.Size() >= 2 && child.Value(1) >= 1.)
			n = child.Value(1);
		ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
	}
}



const Fleet::Variant &Fleet::ChooseVariant() const
{
	// Pick a random variant based on the weights.
	unsigned index = 0;
	for(int choice = Random::Int(total); choice >= variants[index].weight; ++index)
		choice -= variants[index].weight;
	
	return variants[index];
}



// Obtain a positional reference and the radius of the object at that position (e.g. a planet).
// Spaceport status can be modified during normal gameplay, so this information is not cached.
pair<Point, double> Fleet::ChooseCenter(const System &system)
{
	auto centers = vector<pair<Point, double>>();
	for(const StellarObject &object : system.Objects())
		if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
			centers.emplace_back(object.Position(), object.Radius());
	
	if(centers.empty())
		return {Point(), 0.};
	return centers[Random::Int(centers.size())];
}



vector<shared_ptr<Ship>> Fleet::Instantiate(const Variant &variant) const
{
	vector<shared_ptr<Ship>> placed;
	for(const Ship *model : variant.ships)
	{
		// At least one of this variant's ships is valid, but we should avoid spawning any that are not defined.
		if(!model->IsValid())
		{
			Files::LogError("Skipping invalid ship model \"" + model->ModelName() + "\" in fleet \"" + fleetName + "\".");
			continue;
		}
		
		auto ship = make_shared<Ship>(*model);
		
		const Phrase *phrase = ((ship->CanBeCarried() && fighterNames) ? fighterNames : names);
		if(phrase)
			ship->SetName(phrase->Get());
		ship->SetGovernment(government);
		ship->SetPersonality(personality);
		
		placed.push_back(ship);
	}
	return placed;
}



bool Fleet::PlaceFighter(shared_ptr<Ship> fighter, vector<shared_ptr<Ship>> &placed) const
{
	if(!fighter->CanBeCarried())
		return false;
	
	for(const shared_ptr<Ship> &parent : placed)
		if(parent->Carry(fighter))
			return true;
	
	return false;
}



// Choose the cargo associated with this ship in the fleet.
// If outfits were specified, but not commodities, do not pick commodities.
// If commodities were specified, but not outfits, do not pick outfits.
// If neither or both were specified, choose commodities more often..
void Fleet::SetCargo(Ship *ship) const
{
	const bool canChooseOutfits = commodities.empty() || !outfitters.empty();
	const bool canChooseCommodities = outfitters.empty() || !commodities.empty();
	// Populate the possible outfits that may be chosen.
	int free = ship->Cargo().Free();
	auto outfits = OutfitChoices(outfitters, ship->GetSystem(), free);
	
	// Choose random outfits or commodities to transport.
	for(int i = 0; i < cargo; ++i)
	{
		if(free <= 0)
			break;
		// Remove any outfits that do not fit into remaining cargo.
		if(canChooseOutfits && !outfits.empty())
			outfits.erase(remove_if(outfits.begin(), outfits.end(),
					[&free](const Outfit *a) { return a->Mass() > free; }),
				outfits.end());
		
		if(canChooseCommodities && canChooseOutfits)
		{
			if(Random::Real() < .8)
				AddRandomCommodity(*ship, free, commodities);
			else
				AddRandomOutfit(*ship, free, outfits);
		}
		else if(canChooseCommodities)
			AddRandomCommodity(*ship, free, commodities);
		else
			AddRandomOutfit(*ship, free, outfits);
		
		free = ship->Cargo().Free();
	}
	int extraCrew = ship->Attributes().Get("bunks") - ship->RequiredCrew();
	if(extraCrew > 0)
		ship->AddCrew(Random::Int(extraCrew + 1));
}
