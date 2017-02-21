/* Ship.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Ship.h"

#include "Audio.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Effect.h"
#include "GameData.h"
#include "Government.h"
#include "Mask.h"
#include "Messages.h"
#include "Phrase.h"
#include "Planet.h"
#include "Projectile.h"
#include "Random.h"
#include "ShipEvent.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

const vector<string> Ship::CATEGORIES = {
	"Transport",
	"Light Freighter",
	"Heavy Freighter",
	"Interceptor",
	"Light Warship",
	"Medium Warship",
	"Heavy Warship",
	"Fighter",
	"Drone"
};

namespace {
	const string BAY_TYPE[2] = {"drone", "fighter"};
	const string BAY_SIDE[3] = {"inside", "over", "under"};
	const string BAY_FACING[4] = {"forward", "left", "right", "back"};
	const Angle BAY_ANGLE[4] = {Angle(0.), Angle(-90.), Angle(90.), Angle(180.)};
}



void Ship::Load(const DataNode &node)
{
	if(node.Size() >= 2)
	{
		modelName = node.Token(1);
		pluralModelName = modelName + 's';
	}
	if(node.Size() >= 3)
		base = GameData::Ships().Get(modelName);
	
	government = GameData::PlayerGovernment();
	equipped.clear();
	
	// Note: I do not clear the attributes list here so that it is permissible
	// to override one ship definition with another.
	bool hasEngine = false;
	bool hasArmament = false;
	bool hasLicenses = false;
	bool hasBays = false;
	bool hasExplode = false;
	bool hasFinalExplode = false;
	bool hasOutfits = false;
	bool hasDescription = false;
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sprite")
			LoadSprite(child);
		else if(child.Token(0) == "name" && child.Size() >= 2)
			name = child.Token(1);
		else if(child.Token(0) == "plural" && child.Size() >= 2)
			pluralModelName = child.Token(1);
		else if(child.Token(0) == "attributes")
			baseAttributes.Load(child);
		else if(child.Token(0) == "engine" && child.Size() >= 3)
		{
			if(!hasEngine)
			{
				enginePoints.clear();
				hasEngine = true;
			}
			enginePoints.emplace_back(.5 * child.Value(1), .5 * child.Value(2),
				(child.Size() > 3 ? child.Value(3) : 1.));
		}
		else if(child.Token(0) == "gun" || child.Token(0) == "turret")
		{
			if(!hasArmament)
			{
				armament = Armament();
				hasArmament = true;
			}
			const Outfit *outfit = nullptr;
			Point hardpoint;
			if(child.Size() >= 3)
			{
				hardpoint = Point(child.Value(1), child.Value(2));
				if(child.Size() >= 4)
					outfit = GameData::Outfits().Get(child.Token(3));
			}
			else
			{
				if(child.Size() >= 2)
					outfit = GameData::Outfits().Get(child.Token(1));
			}
			if(outfit)
				++equipped[outfit];
			if(child.Token(0) == "gun")
				armament.AddGunPort(hardpoint, outfit);
			else
				armament.AddTurret(hardpoint, outfit);
		}
		else if(child.Token(0) == "licenses")
		{
			if(!hasLicenses)
			{
				licenses.clear();
				hasLicenses = true;
			}
			for(const DataNode &grand : child)
				licenses.push_back(grand.Token(0));
		}
		else if(child.Token(0) == "never disabled")
			neverDisabled = true;
		else if(child.Token(0) == "uncapturable")
			isCapturable = false;
		else if((child.Token(0) == "fighter" || child.Token(0) == "drone") && child.Size() >= 3)
		{
			if(!hasBays)
			{
				bays.clear();
				hasBays = true;
			}
			bays.emplace_back(child.Value(1), child.Value(2), child.Token(0) == "fighter");
			for(int i = 3; i < child.Size(); ++i)
			{
				for(unsigned j = 1; j < sizeof(BAY_SIDE) / sizeof(BAY_SIDE[0]); ++j)
					if(child.Token(i) == BAY_SIDE[j])
						bays.back().side = j;
				for(unsigned j = 1; j < sizeof(BAY_FACING) / sizeof(BAY_FACING[0]); ++j)
					if(child.Token(i) == BAY_FACING[j])
						bays.back().facing = j;
			}
		}
		else if(child.Token(0) == "explode" && child.Size() >= 2)
		{
			if(!hasExplode)
			{
				explosionEffects.clear();
				explosionTotal = 0;
				hasExplode = true;
			}
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			explosionEffects[GameData::Effects().Get(child.Token(1))] += count;
			explosionTotal += count;
		}
		else if(child.Token(0) == "final explode" && child.Size() >= 2)
		{
			if(!hasFinalExplode)
			{
				finalExplosions.clear();
				hasFinalExplode = true;
			}
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			finalExplosions[GameData::Effects().Get(child.Token(1))] += count;
		}
		else if(child.Token(0) == "outfits")
		{
			if(!hasOutfits)
			{
				outfits.clear();
				hasOutfits = true;
			}
			for(const DataNode &grand : child)
			{
				int count = (grand.Size() >= 2) ? grand.Value(1) : 1;
				outfits[GameData::Outfits().Get(grand.Token(0))] += count;
			}
		}
		else if(child.Token(0) == "cargo")
			cargo.Load(child);
		else if(child.Token(0) == "crew" && child.Size() >= 2)
			crew = static_cast<int>(child.Value(1));
		else if(child.Token(0) == "fuel" && child.Size() >= 2)
			fuel = child.Value(1);
		else if(child.Token(0) == "shields" && child.Size() >= 2)
			shields = child.Value(1);
		else if(child.Token(0) == "hull" && child.Size() >= 2)
			hull = child.Value(1);
		else if(child.Token(0) == "position" && child.Size() >= 3)
			position = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "system" && child.Size() >= 2)
			currentSystem = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
		{
			zoom = 0.;
			landingPlanet = GameData::Planets().Get(child.Token(1));
		}
		else if(child.Token(0) == "destination system" && child.Size() >= 2)
			targetSystem = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "parked")
			isParked = true;
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			if(!hasDescription)
			{
				description.clear();
				hasDescription = true;
			}
			description += child.Token(1);
			description += '\n';
		}
		else if(child.Token(0) != "actions")
			child.PrintTrace("Skipping unrecognized attribute:");
	}
	
	// Check that all the "equipped" outfits actually match what your ship has.
	if(!outfits.empty())
		for(auto &it : equipped)
		{
			int excess = it.second - outfits[it.first];
			if(excess > 0)
			{
				// If there are more hardpoints specifying this outfit than there
				// are instances of this outfit installed, remove some of them.
				armament.Add(it.first, -excess);
				it.second -= excess;
			}
		}
}



// When loading a ship, some of the outfits it lists may not have been
// loaded yet. So, wait until everything has been loaded, then call this.
void Ship::FinishLoading()
{
	// All copies of this ship should save pointers to the "explosion" weapon
	// definition stored safely in the ship model, which will not be destroyed
	// until GameData is when the program quits.
	if(GameData::Ships().Has(modelName))
		explosionWeapon = &GameData::Ships().Get(modelName)->BaseAttributes();
	
	// If this ship has a base class, copy any attributes not defined here.
	// Exception: uncapturable and "never disabled" flags don't carry over.
	if(base && base != this)
	{
		pluralModelName = base->pluralModelName;
		if(!GetSprite())
			reinterpret_cast<Body &>(*this) = *base;
		if(baseAttributes.Attributes().empty())
			baseAttributes = base->baseAttributes;
		if(bays.empty() && !base->bays.empty())
			bays = base->bays;
		if(enginePoints.empty())
			enginePoints = base->enginePoints;
		if(explosionEffects.empty())
		{
			explosionEffects = base->explosionEffects;
			explosionTotal = base->explosionTotal;
		}
		if(finalExplosions.empty())
			finalExplosions = base->finalExplosions;
		if(outfits.empty())
			outfits = base->outfits;
		if(description.empty())
			description = base->description;
		
		bool hasHardpoints = false;
		for(const Hardpoint &weapon : armament.Get())
			if(weapon.GetPoint())
				hasHardpoints = true;
		
		if(!hasHardpoints)
		{
			// Check if any hardpoint locations were not specified.
			auto bit = base->Weapons().begin();
			auto bend = base->Weapons().end();
			auto nextGun = armament.Get().begin();
			auto nextTurret = armament.Get().begin();
			auto end = armament.Get().end();
			Armament merged;
			for( ; bit != bend; ++bit)
			{
				if(!bit->IsTurret())
				{
					while(nextGun != end && nextGun->IsTurret())
						++nextGun;
					merged.AddGunPort(bit->GetPoint() * 2.,
						(nextGun == end) ? nullptr : nextGun->GetOutfit());
					if(nextGun != end)
						++nextGun;
				}
				else
				{
					while(nextTurret != end && !nextTurret->IsTurret())
						++nextTurret;
					merged.AddTurret(bit->GetPoint() * 2.,
						(nextTurret == end) ? nullptr : nextTurret->GetOutfit());
					if(nextTurret != end)
						++nextTurret;
				}
			}
			armament = merged;
		}
	}
	
	// Mark any drone that has no "automaton" value as an automaton, to
	// grandfather in the drones from before that attribute existed.
	if(baseAttributes.Category() == "Drone" && !baseAttributes.Attributes().count("automaton"))
		baseAttributes.Add("automaton", 1.);
	
	// Different ships dissipate heat at different rates.
	heatDissipation = baseAttributes.Get("heat dissipation");
	if(!heatDissipation)
		heatDissipation = .999;
	else
		heatDissipation = 1. - .001 * heatDissipation;
	
	baseAttributes.Reset("gun ports", armament.GunCount());
	baseAttributes.Reset("turret mounts", armament.TurretCount());
	
	// Add the attributes of all your outfits to the ship's base attributes.
	attributes = baseAttributes;
	for(const auto &it : outfits)
	{
		if(it.first->Name().empty())
		{
			cerr << "Unrecognized outfit in " << modelName << " \"" << name << "\"" << endl;
			continue;
		}
		attributes.Add(*it.first, it.second);
		if(it.first->IsWeapon())
		{
			int count = it.second;
			auto eit = equipped.find(it.first);
			if(eit != equipped.end())
				count -= eit->second;
			
			if(count)
				armament.Add(it.first, count);
		}
	}
	cargo.SetSize(attributes.Get("cargo space"));
	equipped.clear();
	armament.FinishLoading();
	
	// Figure out how far from center the farthest weapon it.
	weaponRadius = 0.;
	for(const Hardpoint &weapon : armament.Get())
		weaponRadius = max(weaponRadius, weapon.GetPoint().Length());
	
	// Recharge, but don't recharge crew or fuel if not in the parent's system.
	// Do not recharge if this ship's starting state was saved.
	if(!hull)
	{
		shared_ptr<const Ship> parent = GetParent();
		Recharge(!parent || currentSystem == parent->currentSystem);
	}
	else
	{
		isDisabled = true;
		isDisabled = IsDisabled();
	}
}



// Save a full description of this ship, as currently configured.
void Ship::Save(DataWriter &out) const
{
	out.Write("ship", modelName);
	out.BeginChild();
	{
		out.Write("name", name);
		if(pluralModelName != modelName + 's')
			out.Write("plural", pluralModelName);
		SaveSprite(out);
		
		if(neverDisabled)
			out.Write("never disabled");
		if(!isCapturable)
			out.Write("uncapturable");
		
		out.Write("attributes");
		out.BeginChild();
		{
			out.Write("category", baseAttributes.Category());
			out.Write("cost", baseAttributes.Cost());
			for(const auto &it : baseAttributes.Attributes())
				if(it.second)
					out.Write(it.first, it.second);
		}
		out.EndChild();
		
		out.Write("outfits");
		out.BeginChild();
		{
			for(const auto &it : outfits)
				if(it.first && it.second)
				{
					if(it.second == 1)
						out.Write(it.first->Name());
					else
						out.Write(it.first->Name(), it.second);
				}
		}
		out.EndChild();
		
		cargo.Save(out);
		out.Write("crew", crew);
		out.Write("fuel", fuel);
		out.Write("shields", shields);
		out.Write("hull", hull);
		out.Write("position", position.X(), position.Y());
		
		for(const EnginePoint &point : enginePoints)
			out.Write("engine", 2. * point.X(), 2. * point.Y(), point.Zoom());
		for(const Hardpoint &weapon : armament.Get())
		{
			const char *type = (weapon.IsTurret() ? "turret" : "gun");
			if(weapon.GetOutfit())
				out.Write(type, 2. * weapon.GetPoint().X(), 2. * weapon.GetPoint().Y(),
					weapon.GetOutfit()->Name());
			else
				out.Write(type, 2. * weapon.GetPoint().X(), 2. * weapon.GetPoint().Y());
		}
		for(const Bay &bay : bays)
		{
			double x = 2. * bay.point.X();
			double y = 2. * bay.point.Y();
			if(bay.side && bay.facing)
				out.Write(BAY_TYPE[bay.isFighter], x, y, BAY_SIDE[bay.side], BAY_FACING[bay.facing]);
			else if(bay.side)
				out.Write(BAY_TYPE[bay.isFighter], x, y, BAY_SIDE[bay.side]);
			else if(bay.facing)
				out.Write(BAY_TYPE[bay.isFighter], x, y, BAY_FACING[bay.facing]);
			else
				out.Write(BAY_TYPE[bay.isFighter], x, y);
		}
		for(const auto &it : explosionEffects)
			if(it.first && it.second)
				out.Write("explode", it.first->Name(), it.second);
		for(const auto &it : finalExplosions)
			if(it.first && it.second)
				out.Write("final explode", it.first->Name(), it.second);
		
		if(currentSystem)
			out.Write("system", currentSystem->Name());
		else
		{
			shared_ptr<const Ship> parent = GetParent();
			if(parent && parent->currentSystem)
				out.Write("system", parent->currentSystem->Name());
		}
		if(landingPlanet)
			out.Write("planet", landingPlanet->Name());
		if(targetSystem && !targetSystem->Name().empty())
			out.Write("destination system", targetSystem->Name());
		if(isParked)
			out.Write("parked");
	}
	out.EndChild();
}



const string &Ship::Name() const
{
	return name;
}



const string &Ship::ModelName() const
{
	return modelName;
}



const string &Ship::PluralModelName() const
{
	return pluralModelName;
}



// Get this ship's description.
const string &Ship::Description() const
{
	return description;
}



// Get this ship's cost.
int64_t Ship::Cost() const
{
	return attributes.Cost();
}



// Get the cost of this ship's chassis, with no outfits installed.
int64_t Ship::ChassisCost() const
{
	return baseAttributes.Cost();
}



// Get the licenses needed to buy or operate this ship.
const vector<string> &Ship::Licenses() const
{
	return licenses;
}



void Ship::Place(Point position, Point velocity, Angle angle)
{
	this->position = position;
	this->velocity = velocity;
	this->angle = angle;
	// If landed, place the ship right above the planet.
	if(landingPlanet)
	{
		landingPlanet = nullptr;
		zoom = parent.lock() ? (-.2 + -.8 * Random::Real()) : 0.;
	}
	else
		zoom = 1.;
	// Make sure various special status values are reset.
	heat = IdleHeat();
	ionization = 0.;
	disruption = 0.;
	slowness = 0.;
	isInvisible = !HasSprite();
	jettisoned.clear();
	hyperspaceCount = 0;
	hyperspaceType = 0;
	forget = 1;
	targetShip.reset();
	shipToAssist.reset();
	if(government)
		SetSwizzle(government->GetSwizzle());
}



// Set the name of this particular ship.
void Ship::SetName(const string &name)
{
	this->name = name;
}



// Set which system this ship is in.
void Ship::SetSystem(const System *system)
{
	currentSystem = system;
}



void Ship::SetPlanet(const Planet *planet)
{
	// Escorts should take off a bit behind their flagships.
	zoom = !planet;
	landingPlanet = planet;
}



void Ship::SetGovernment(const Government *government)
{
	if(government)
		SetSwizzle(government->GetSwizzle());
	this->government = government;
}



void Ship::SetIsSpecial(bool special)
{
	isSpecial = special;
}



bool Ship::IsSpecial() const
{
	return isSpecial;
}



void Ship::SetIsYours(bool yours)
{
	isYours = yours;
}



bool Ship::IsYours() const
{
	return isYours;
}



void Ship::SetIsParked(bool parked)
{
	isParked = parked;
}



bool Ship::IsParked() const
{
	return isParked;
}



const Personality &Ship::GetPersonality() const
{
	return personality;
}



void Ship::SetPersonality(const Personality &other)
{
	personality = other;
	if(personality.IsDerelict())
	{
		shields = 0.;
		hull = .5 * MinimumHull();
		isDisabled = true;
	}
}



void Ship::SetHail(const Phrase &phrase)
{
	hail = &phrase;
}



string Ship::GetHail() const
{
	return hail ? hail->Get() : government ? government->GetHail() : "";
}



// Set the commands for this ship to follow this timestep.
void Ship::SetCommands(const Command &command)
{
	commands = command;
}



const Command &Ship::Commands() const
{
	return commands;
}



// Move this ship. A ship may create effects as it moves, in particular if
// it is in the process of blowing up. If this returns false, the ship
// should be deleted.
bool Ship::Move(list<Effect> &effects, list<shared_ptr<Flotsam>> &flotsam)
{
	// Check if this ship has been in a different system from the player for so
	// long that it should be "forgotten." Also eliminate ships that have no
	// system set because they just entered a fighter bay.
	forget += !isInSystem;
	isThrusting = false;
	if((!isSpecial && forget >= 1000) || !currentSystem)
		return false;
	isInSystem = false;
	if(!fuel || !(attributes.Get("hyperdrive") || attributes.Get("jump drive")))
		hyperspaceSystem = nullptr;
	
	// Adjust the error in the pilot's targeting.
	personality.UpdateConfusion(commands.IsFiring());
	
	// Handle ionization effects, etc.
	if(ionization)
	{
		ionization *= .99;
		CreateSparks(effects, "ion spark", ionization * .1);
	}
	if(disruption)
	{
		disruption *= .99;
		CreateSparks(effects, "disruption spark", disruption * .1);
	}
	if(slowness)
	{
		slowness *= .99;
		CreateSparks(effects, "slowing spark", slowness * .1);
	}
	double slowMultiplier = 1. / (1. + slowness * .05);
	// Jettisoned cargo effects (only for ships in the current system).
	if(!jettisoned.empty() && !forget)
	{
		jettisoned.front()->Place(*this);
		flotsam.splice(flotsam.end(), jettisoned, jettisoned.begin());
	}
	
	// When ships recharge, what actually happens is that they can exceed their
	// maximum capacity for the rest of the turn, but must be clamped to the
	// maximum here before they gain more. This is so that, for example, a ship
	// with no batteries but a good generator can still move.
	energy = min(energy, attributes.Get("energy capacity"));
	
	heat *= heatDissipation;
	if(heat > Mass() * 100.)
		isOverheated = true;
	else if(heat < Mass() * 90.)
		isOverheated = false;
	
	double maxShields = attributes.Get("shields");
	shields = min(shields, maxShields);
	double maxHull = attributes.Get("hull");
	hull = min(hull, maxHull);
	
	int requiredCrew = RequiredCrew();
	isDisabled = isOverheated || hull < MinimumHull() || (!crew && requiredCrew);
	
	// Update ship supply levels.
	if(!isDisabled)
	{
		// Ramscoops work much better when close to the system center. Even if a
		// ship has no ramscoop, it can harvest a tiny bit of fuel by flying
		// close to the star.
		double scale = .2 + 1.8 / (.001 * position.Length() + 1);
		fuel += .03 * scale * (sqrt(attributes.Get("ramscoop")) + .05 * scale);
		fuel = min(fuel, attributes.Get("fuel capacity"));
		
		energy += scale * attributes.Get("solar collection");
		
		energy += attributes.Get("energy generation") - ionization;
		energy = max(0., energy);
		heat += attributes.Get("heat generation");
		heat -= attributes.Get("cooling");
		heat = max(0., heat);
		
		// Apply active cooling. The fraction of full cooling to apply equals
		// your ship's current fraction of its maximum temperature.
		double activeCooling = attributes.Get("active cooling");
		if(activeCooling > 0.)
		{
			// Although it's a misuse of this feature, handle the case where
			// "active cooling" does not require any energy.
			double coolingEnergy = attributes.Get("cooling energy");
			if(coolingEnergy)
			{
				double spentEnergy = min(energy, coolingEnergy * min(1., Heat()));
				heat -= activeCooling * spentEnergy / coolingEnergy;
				energy -= spentEnergy;
			}
			else
				heat -= activeCooling;
			
			heat = max(0., heat);
		}
	}
	
	if(!isInvisible)
	{
		double cloakingSpeed = attributes.Get("cloak");
		bool canCloak = (!isDisabled && cloakingSpeed > 0.
			&& fuel >= attributes.Get("cloaking fuel")
			&& energy >= attributes.Get("cloaking energy"));
		if(commands.Has(Command::CLOAK) && canCloak)
		{
			cloak = min(1., cloak + cloakingSpeed);
			fuel -= attributes.Get("cloaking fuel");
			energy -= attributes.Get("cloaking energy");
		}
		else if(cloakingSpeed)
			cloak = max(0., cloak - cloakingSpeed);
		else
			cloak = 0.;
	}
	
	if(IsDestroyed())
	{
		// Make sure the shields are zero, as well as the hull.
		shields = 0.;
		
		// Once we've created enough little explosions, die.
		if(explosionCount == explosionTotal || forget)
		{
			if(!forget)
			{
				const Effect *effect = GameData::Effects().Get("smoke");
				double size = Width() + Height();
				double scale = .03 * size + .5;
				double radius = .2 * size;
				int debrisCount = attributes.Get("mass") * .07;
				for(int i = 0; i < debrisCount; ++i)
				{
					effects.push_back(*effect);
					
					Angle angle = Angle::Random();
					Point effectVelocity = velocity + angle.Unit() * (scale * Random::Real());
					Point effectPosition = position + radius * angle.Unit();
					effects.back().Place(effectPosition, effectVelocity, angle);
				}
					
				for(unsigned i = 0; i < explosionTotal / 2; ++i)
					CreateExplosion(effects, true);
				for(const auto &it : finalExplosions)
				{
					effects.push_back(*it.first);
					effects.back().Place(position, velocity, angle);
				}
				// For everything in this ship's cargo hold there is a 25% chance
				// that it will survive as flotsam.
				for(const auto &it : cargo.Commodities())
					Jettison(it.first, Random::Binomial(it.second, .25));
				for(const auto &it : cargo.Outfits())
					Jettison(it.first, Random::Binomial(it.second, .25));
				for(shared_ptr<Flotsam> &it : jettisoned)
					it->Place(*this);
				flotsam.splice(flotsam.end(), jettisoned);
			}
			energy = 0.;
			heat = 0.;
			ionization = 0.;
			fuel = 0.;
			return false;
		}
		
		// If the ship is dead, it first creates explosions at an increasing
		// rate, then disappears in one big explosion.
		++explosionRate;
		if(Random::Int(1024) < explosionRate)
			CreateExplosion(effects);
	}
	else if(hyperspaceSystem || hyperspaceCount)
	{
		// Don't apply external acceleration while jumping.
		acceleration = Point();
		
		fuel -= (hyperspaceSystem != nullptr) * hyperspaceType * .01;
		
		// Enter hyperspace.
		int direction = (hyperspaceSystem != nullptr) - (hyperspaceSystem == nullptr);
		hyperspaceCount += direction;
		static const int HYPER_C = 100;
		static const double HYPER_A = 2.;
		static const double HYPER_D = 1000.;
		bool hasJumpDrive = (hyperspaceType == 200);
		
		// Create the particle effects for the jump drive. This may create 100
		// or more particles per ship per turn at the peak of the jump.
		if(hasJumpDrive && !forget)
			CreateSparks(effects, "jump drive", hyperspaceCount * Width() * Height() * .000006);
		
		if(hyperspaceCount == HYPER_C)
		{
			currentSystem = hyperspaceSystem;
			hyperspaceSystem = nullptr;
			targetSystem = nullptr;
			// Check if the target planet is in the destination system or not.
			const Planet *planet = (targetPlanet ? targetPlanet->GetPlanet() : nullptr);
			if(!planet || planet->GetSystem() != currentSystem)
				targetPlanet = nullptr;
			direction = -1;
			
			// If you have a target planet in the destination system, exit
			// hyperpace aimed at it. Otherwise, target the first planet that
			// has a spaceport.
			Point target;
			if(targetPlanet)
				target = targetPlanet->Position();
			else
			{
				for(const StellarObject &object : currentSystem->Objects())
					if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
					{
						target = object.Position();
						break;
					}
			}
			
			if(hasJumpDrive)
			{
				position = target + Angle::Random().Unit() * 300. * (Random::Real() + 1.);
				return true;
			}
			
			// Have all ships exit hyperspace at the same distance so that
			// your escorts always stay with you.
			double distance = (HYPER_C * HYPER_C) * .5 * HYPER_A + HYPER_D;
			position = (target - distance * angle.Unit());
			position += hyperspaceOffset;
			// Make sure your velocity is in exactly the direction you are
			// traveling in, so that when you decelerate there will not be a
			// sudden shift in direction at the end.
			velocity = velocity.Length() * angle.Unit();
		}
		if(!hasJumpDrive)
		{
			velocity += (HYPER_A * direction) * angle.Unit();
			if(!hyperspaceSystem)
			{
				// Exit hyperspace far enough from the planet to be able to land.
				// This does not take drag into account, so it is always an over-
				// estimate of how long it will take to stop.
				// We start decellerating after rotating about 150 degrees (that
				// is, about acos(.8) from the proper angle). So:
				// Stopping distance = .5*a*(v/a)^2 + (150/turn)*v.
				// Exit distance = HYPER_D + .25 * v^2 = stopping distance.
				double exitV = MaxVelocity();
				double a = (.5 / Acceleration() - .25);
				double b = 150. / TurnRate();
				double discriminant = b * b - 4. * a * -HYPER_D;
				if(discriminant > 0.)
				{
					double altV = (-b + sqrt(discriminant)) / (2. * a);
					if(altV > 0. && altV < exitV)
						exitV = altV;
				}
				if(velocity.Length() <= exitV)
				{
					velocity = angle.Unit() * exitV;
					hyperspaceCount = 0;
				}
			}
		}
		position += velocity;
		if(GetParent() && GetParent()->currentSystem == currentSystem)
		{
			hyperspaceOffset = position - GetParent()->position;
			double length = hyperspaceOffset.Length();
			if(length > 1000.)
				hyperspaceOffset *= 1000. / length;
		}
		
		return true;
	}
	else if(landingPlanet || zoom < 1.)
	{
		// Don't apply external acceleration while landing.
		acceleration = Point();
		
		// If a ship was disabled at the very moment it began landing, do not
		// allow it to continue landing.
		if(isDisabled)
			landingPlanet = nullptr;
		
		// Special ships do not disappear forever when they land; they
		// just slowly refuel.
		if(landingPlanet && zoom)
		{
			// Move the ship toward the center of the planet while landing.
			if(GetTargetPlanet())
				position = .97 * position + .03 * GetTargetPlanet()->Position();
			zoom -= .02;
			if(zoom < 0.)
			{
				// If this is not a special ship, it ceases to exist when it
				// lands on a true planet. If this is a wormhole, the ship is
				// instantly transported.
				if(landingPlanet->IsWormhole())
				{
					currentSystem = landingPlanet->WormholeDestination(currentSystem);
					for(const StellarObject &object : currentSystem->Objects())
						if(object.GetPlanet() == landingPlanet)
							position = object.Position();
					SetTargetPlanet(nullptr);
					landingPlanet = nullptr;
				}
				else if(!isSpecial || personality.IsFleeing())
					return false;
				
				zoom = 0.;
			}
		}
		// Only refuel if this planet has a spaceport.
		else if(fuel == attributes.Get("fuel capacity")
				|| !landingPlanet || !landingPlanet->HasSpaceport())
		{
			zoom = min(1., zoom + .02);
			SetTargetPlanet(nullptr);
			landingPlanet = nullptr;
		}
		else
			fuel = min(fuel + 1., attributes.Get("fuel capacity"));
		
		// Move the ship at the velocity it had when it began landing, but
		// scaled based on how small it is now.
		if(zoom > 0.)
			position += velocity * zoom;
		
		return true;
	}
	if(isDisabled)
	{
		// If you're disabled, you can't initiate landing or jumping.
	}
	else if(commands.Has(Command::LAND) && CanLand())
		landingPlanet = GetTargetPlanet()->GetPlanet();
	else if(commands.Has(Command::JUMP))
	{
		hyperspaceType = CheckHyperspace();
		if(hyperspaceType)
			hyperspaceSystem = GetTargetSystem();
	}
	
	if(pilotError)
		--pilotError;
	else if(pilotOkay)
		--pilotOkay;
	else if(isDisabled)
	{
		// If the ship is disabled, don't show a warning message due to missing crew.
	}
	else if(requiredCrew && static_cast<int>(Random::Int(requiredCrew)) >= Crew())
	{
		pilotError = 30;
		if(parent.lock() || !government->IsPlayer())
			Messages::Add(name + " is moving erratically because there are not enough crew to pilot it.");
		else
			Messages::Add("Your ship is moving erratically because you do not have enough crew to pilot it.");
	}
	else
		pilotOkay = 30;
	
	// This ship is not landing or entering hyperspace. So, move it. If it is
	// disabled, all it can do is slow down to a stop.
	double mass = Mass();
	if(isDisabled)
		velocity *= 1. - attributes.Get("drag") / mass;
	else if(!pilotError)
	{
		double thrustCommand = commands.Has(Command::FORWARD) - commands.Has(Command::BACK);
		if(thrustCommand)
		{
			// Check if we are able to apply this thrust.
			double cost = attributes.Get((thrustCommand > 0.) ?
				"thrusting energy" : "reverse thrusting energy");
			if(energy < cost)
				thrustCommand = 0.;
			else
			{
				// If a reverse thrust is commanded and the capability does not
				// exist, ignore it (do not even slow under drag).
				isThrusting = (thrustCommand > 0.);
				double thrust = attributes.Get(isThrusting ? "thrust" : "reverse thrust");
				if(!thrust)
					thrustCommand = 0.;
				else
				{
					energy -= cost;
					heat += attributes.Get(isThrusting ? "thrusting heat" : "reverse thrusting heat");
					acceleration += angle.Unit() * (thrustCommand * thrust / mass);
				}
			}
		}
		bool applyAfterburner = commands.Has(Command::AFTERBURNER) && !CannotAct();
		if(applyAfterburner)
		{
			double thrust = attributes.Get("afterburner thrust");
			double cost = attributes.Get("afterburner fuel");
			double energyCost = attributes.Get("afterburner energy");
			if(!thrust || fuel < cost || energy < energyCost)
				applyAfterburner = false;
			else
			{
				heat += attributes.Get("afterburner heat");
				fuel -= cost;
				energy -= energyCost;
				acceleration += angle.Unit() * thrust / mass;
				
				if(!forget)
					for(const EnginePoint &point : enginePoints)
					{
						Point pos = angle.Rotate(point) * Zoom() + position;
						for(const auto &it : attributes.AfterburnerEffects())
							for(int i = 0; i < it.second; ++i)
							{
								effects.push_back(*it.first);
								effects.back().Place(pos + velocity, velocity - 6. * angle.Unit(), angle);
							}
					}
			}
		}
		if(commands.Turn())
		{
			// Check if we are able to turn.
			double cost = attributes.Get("turning energy");
			if(energy < cost)
				commands.SetTurn(0.);
			else
			{
				energy -= cost;
				heat += attributes.Get("turning heat");
				angle += commands.Turn() * TurnRate() * slowMultiplier;
			}
		}
	}
	if(acceleration)
	{
		acceleration *= slowMultiplier;
		Point dragAcceleration = acceleration - velocity * (attributes.Get("drag") / mass);
		// Make sure dragAcceleration has nonzero length, to avoid divide by zero.
		if(dragAcceleration)
		{
			// What direction will the net acceleration be if this drag is applied?
			// If the net acceleration will be opposite the thrust, do not apply drag.
			dragAcceleration *= .5 * (acceleration.Unit().Dot(dragAcceleration.Unit()) + 1.);
			
			// A ship can only "cheat" to stop if it is moving slow enough that
			// it could stop completely this frame. This is to avoid overshooting
			// when trying to stop and ending up headed in the other direction.
			if(commands.Has(Command::STOP))
			{
				// How much acceleration would it take to come to a stop in the
				// direction normal to the ship's current facing? This is only
				// possible if the acceleration plus drag vector is in the
				// opposite direction from the velocity vector when both are
				// projected onto the current facing vector, and the acceleration
				// vector is the larger of the two.
				double vNormal = velocity.Dot(angle.Unit());
				double aNormal = dragAcceleration.Dot(angle.Unit());
				if((aNormal > 0.) != (vNormal > 0.) && fabs(aNormal) > fabs(vNormal))
					dragAcceleration = -vNormal * angle.Unit();
			}
			velocity += dragAcceleration;
		}
		acceleration = Point();
	}
	
	// Boarding:
	if(isBoarding && (commands.Has(Command::FORWARD | Command::BACK) || commands.Turn()))
		isBoarding = false;
	shared_ptr<const Ship> target = GetTargetShip();
	// If this is a fighter or drone and it is not assisting someone at the
	// moment, its boarding target should be its parent ship.
	if(CanBeCarried() && !(target && target == GetShipToAssist()))
		target = GetParent();
	if(target && !isDisabled)
	{
		Point dp = (target->position - position);
		double distance = dp.Length();
		Point dv = (target->velocity - velocity);
		double speed = dv.Length();
		isBoarding |= (distance < 50. && speed < 1. && commands.Has(Command::BOARD));
		if(isBoarding && !CanBeCarried())
		{
			if(!target->IsDisabled() && government->IsEnemy(target->government))
				isBoarding = false;
			else if(target->IsDestroyed() || target->IsLanding() || target->IsHyperspacing()
					|| target->GetSystem() != GetSystem())
				isBoarding = false;
		}
		if(isBoarding && !pilotError)
		{
			Angle facing = angle;
			bool left = target->Unit().Cross(facing.Unit()) < 0.;
			double turn = left - !left;
			
			// Check if the ship will still be pointing to the same side of the target
			// angle if it turns by this amount.
			facing += TurnRate() * turn;
			bool stillLeft = target->Unit().Cross(facing.Unit()) < 0.;
			if(left != stillLeft)
				turn = 0.;
			angle += TurnRate() * turn;
			
			velocity += dv.Unit() * .1;
			position += dp.Unit() * .5;
			
			if(distance < 10. && speed < 1. && (CanBeCarried() || !turn))
			{
				if(cloak)
				{
					// Allow the player to get all the way to the end of the
					// boarding sequence (including locking on to the ship) but
					// not to actually board, if they are cloaked.
					if(government->IsPlayer())
						Messages::Add("You cannot board a ship while cloaked.");
				}
				else
				{
					isBoarding = false;
					bool isEnemy = government->IsEnemy(target->government);
					if(isEnemy && Random::Real() < target->Attributes().Get("self destruct"))
					{
						Messages::Add("The " + target->ModelName() + " \"" + target->Name()
							+ "\" has activated its self-destruct mechanism.");
						targetShip.lock()->SelfDestruct();
					}
					else
						hasBoarded = true;
				}
			}
		}
	}
	
	// Shield and hull recharge. This comes after movement so that engines take
	// priority over shield recharge.
	if(!isDisabled)
	{
		// Recharge is limited by available energy. Extra recharge capacity can
		// be used on fighters this ship is carrying.
		double hullRate = attributes.Get("hull repair rate");
		if(hullRate > 0.)
		{
			double hullEnergy = attributes.Get("hull energy");
			double hullHeat = attributes.Get("hull heat");
			double hullAdded = AddHull(hullRate * min(1., hullEnergy ? energy / hullEnergy : 1.));
			energy -= hullEnergy * hullAdded / hullRate;
			heat += hullHeat * hullAdded / hullRate;
		}
		
		double shieldRate = attributes.Get("shield generation");
		if(shieldRate > 0.)
		{
			double shieldEnergy = attributes.Get("shield energy");
			double shieldHeat = attributes.Get("shield heat");
			double shieldsAdded = AddShields(shieldRate * min(1., shieldEnergy ? energy / shieldEnergy : 1.));
			energy -= shieldEnergy * shieldsAdded / shieldRate;
			heat += shieldHeat * shieldsAdded / shieldRate;
		}
	}
	
	// Clear your target if it is destroyed. This is only important for NPCs,
	// because ordinary ships cease to exist once they are destroyed.
	target = targetShip.lock();
	if(target && target->IsDestroyed() && target->explosionCount >= target->explosionTotal)
		targetShip.reset();
	
	// And finally: move the ship!
	position += velocity;
	
	return true;
}



// Launch any ships that are ready to launch.
void Ship::Launch(list<shared_ptr<Ship>> &ships)
{
	if(!IsDestroyed() && (!commands.Has(Command::DEPLOY) || CannotAct()))
		return;
	
	for(Bay &bay : bays)
		if(bay.ship && !Random::Int(40 + 20 * bay.isFighter))
		{
			ships.push_back(bay.ship);
			double maxV = bay.ship->MaxVelocity();
			Angle launchAngle = angle + BAY_ANGLE[bay.facing];
			Point v = velocity + (.3 * maxV) * launchAngle.Unit() + (.2 * maxV) * Angle::Random().Unit();
			bay.ship->Place(position + angle.Rotate(bay.point), v, launchAngle);
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetParent(shared_from_this());
			// Fighters in your ship have the same temperature as your ship
			// itself, so when they launch they should take their sahre of heat
			// with them, so that the fighter and the mothership remain at the
			// same temperature.
			bay.ship->heat = heat * bay.ship->Mass() / Mass();
			heat -= bay.ship->heat;
			
			bay.ship.reset();
		}
}



// Check if this ship is boarding another ship.
shared_ptr<Ship> Ship::Board(bool autoPlunder)
{
	if(!hasBoarded)
		return shared_ptr<Ship>();
	hasBoarded = false;
	
	shared_ptr<Ship> victim = GetTargetShip();
	if(CannotAct() || !victim || victim->IsDestroyed() || victim->GetSystem() != GetSystem())
		return shared_ptr<Ship>();
	
	// For a fighter, "board" means "return to ship."
	if(CanBeCarried() && !victim->IsDisabled())
	{
		victim->Carry(shared_from_this());
		return shared_ptr<Ship>();
	}
	
	// Board a ship of your own government to repair/refuel it.
	if(!government->IsEnemy(victim->GetGovernment()))
	{
		SetShipToAssist(shared_ptr<Ship>());
		bool helped = victim->isDisabled;
		victim->hull = max(victim->hull, victim->MinimumHull());
		victim->isDisabled = false;
		// Transfer some fuel if needed.
		if(!victim->JumpsRemaining() && CanRefuel(*victim))
		{
			helped = true;
			TransferFuel(victim->JumpFuel(), victim.get());
		}
		if(helped)
		{
			pilotError = 120;
			victim->pilotError = 120;
		}
		return autoPlunder ? shared_ptr<Ship>() : victim;
	}
	if(!victim->IsDisabled())
		return shared_ptr<Ship>();
	
	// If the boarding ship is the player, they will choose what to plunder.
	// Always take fuel if you can.
	victim->TransferFuel(victim->fuel, this);
	if(autoPlunder)
	{
		// Take any commodities that fit.
		victim->cargo.TransferAll(&cargo);
		// Stop targeting this ship.
		SetTargetShip(shared_ptr<Ship>());
		
		// Pause for two seconds before moving on.
		pilotError = 120;
	}
	
	// Stop targeting this ship (so you will not board it again right away).
	SetTargetShip(shared_ptr<Ship>());
	return victim;
}



// Scan the target, if able and commanded to. Return a ShipEvent bitmask
// giving the types of scan that succeeded.
int Ship::Scan()
{
	// Whenever not actively scanning, the amount of scan information the ship
	// has "decays" over time. For a scanner with a speed of 1, one second of
	// uninterrupted scanning is required to successfully scan its target.
	// Only apply the decay if not already done scanning the target.
	static const double SCAN_TIME = 60.;
	if(cargoScan < SCAN_TIME)
		cargoScan = max(0., cargoScan - 1.);
	if(outfitScan < SCAN_TIME)
		outfitScan = max(0., outfitScan - 1.);
	if(!commands.Has(Command::SCAN) || CannotAct())
		return 0;
	
	shared_ptr<const Ship> target = GetTargetShip();
	if(!target)
		return 0;
	
	// The range of a scanner is proportional to the square root of its power.
	double cargoPower = attributes.Get("cargo scan power");
	double cargoDistance = cargoPower ? 100. * sqrt(cargoPower) : attributes.Get("cargo scan");
	double outfitPower = attributes.Get("outfit scan power");
	double outfitDistance = outfitPower ? 100. * sqrt(outfitPower) : attributes.Get("outfit scan");
	
	// Bail out if this ship has no scanners.
	if(!cargoDistance && !outfitDistance)
		return 0;
	
	// Scanning speed also uses a square root, so you need four scanners to get
	// twice the speed out of them.
	double cargoSpeed = sqrt(attributes.Get("cargo scan speed"));
	if(!cargoSpeed)
		cargoSpeed = 1.;
	double outfitSpeed = sqrt(attributes.Get("outfit scan speed"));
	if(!outfitSpeed)
		outfitSpeed = 1.;
	
	// Check how close this ship is to the target it is trying to scan.
	double distance = (target->position - position).Length();
	
	// Check if either scanner has finished scanning.
	bool startedScanning = false;
	bool activeScanning = false;
	int result = 0;
	if(cargoScan < SCAN_TIME)
	{
		if(distance < cargoDistance)
		{
			startedScanning |= !cargoScan;
			activeScanning = true;
			// To make up for the scan decay above:
			cargoScan += cargoSpeed + 1.;
			if(cargoScan >= SCAN_TIME)
				result |= ShipEvent::SCAN_CARGO;
		}
	}
	if(outfitScan < SCAN_TIME)
	{
		if(distance < outfitDistance)
		{
			startedScanning |= !outfitScan;
			activeScanning = true;
			// To make up for the scan decay above:
			outfitScan += outfitSpeed + 1.;
			if(outfitScan >= SCAN_TIME)
				result |= ShipEvent::SCAN_OUTFITS;
		}
	}
	
	// Play the scanning sound if the actor or the target is the player's ship.
	if(government->IsPlayer() || (target->GetGovernment()->IsPlayer() && activeScanning))
		Audio::Play(Audio::Get("scan"), Position());
	
	if(startedScanning && government->IsPlayer())
		Messages::Add("Attempting to scan the ship \"" + target->Name() + "\".", false);
	else if(startedScanning && target->GetGovernment()->IsPlayer())
		Messages::Add("The " + government->GetName() + " ship \""
			+ Name() + "\" is attempting to scan you.", false);
	
	if(target->GetGovernment()->IsPlayer() && (result & ShipEvent::SCAN_CARGO))
	{
		Messages::Add("The " + government->GetName() + " ship \""
			+ Name() + "\" succeeded in scanning your cargo.");
	}
	if(target->GetGovernment()->IsPlayer() && (result & ShipEvent::SCAN_OUTFITS))
	{
		Messages::Add("The " + government->GetName() + " ship \""
			+ Name() + "\" succeeded in scanning your outfits.");
	}
	
	return result;
}



// Fire any weapons that are ready to fire. If an anti-missile is ready,
// instead of firing here this function returns true and it can be fired if
// collision detection finds a missile in range.
bool Ship::Fire(list<Projectile> &projectiles, list<Effect> &effects)
{
	isInSystem = true;
	forget = 0;
	
	// A ship that is about to die creates a special single-turn "projectile"
	// representing its death explosion.
	if(IsDestroyed() && explosionCount == explosionTotal && explosionWeapon)
		projectiles.emplace_back(position, explosionWeapon);
	
	if(CannotAct())
		return false;
	
	antiMissileRange = 0.;
	
	const vector<Hardpoint> &weapons = armament.Get();
	for(unsigned i = 0; i < weapons.size(); ++i)
	{
		const Outfit *outfit = weapons[i].GetOutfit();
		if(outfit && CanFire(outfit))
		{
			if(outfit->AntiMissile())
				antiMissileRange = max(antiMissileRange, outfit->Velocity() + weaponRadius);
			else if(commands.HasFire(i))
				armament.Fire(i, *this, projectiles, effects);
		}
	}
	
	armament.Step(*this);
	
	return antiMissileRange;
}



// Fire an anti-missile.
bool Ship::FireAntiMissile(const Projectile &projectile, list<Effect> &effects)
{
	if(projectile.Position().Distance(position) > antiMissileRange)
		return false;
	if(CannotAct())
		return false;
	
	const vector<Hardpoint> &weapons = armament.Get();
	for(unsigned i = 0; i < weapons.size(); ++i)
	{
		const Outfit *outfit = weapons[i].GetOutfit();
		if(outfit && CanFire(outfit))
			if(armament.FireAntiMissile(i, *this, projectile, effects))
				return true;
	}
	
	return false;
}



const System *Ship::GetSystem() const
{
	return currentSystem;
}



// If the ship is landed, get the planet it has landed on.
const Planet *Ship::GetPlanet() const
{
	return zoom ? nullptr : landingPlanet;
}



bool Ship::IsCapturable() const
{
	return isCapturable;
}



bool Ship::IsTargetable() const
{
	return (zoom == 1. && !explosionRate && !forget && !isInvisible && cloak < 1. && hull >= 0. && hyperspaceCount < 70);
}



bool Ship::IsOverheated() const
{
	return isOverheated;
}



bool Ship::IsDisabled() const
{
	if(!isDisabled)
		return false;
	
	double minimumHull = MinimumHull();
	bool needsCrew = RequiredCrew() != 0;
	return (hull < minimumHull || (!crew && needsCrew));
}



bool Ship::IsBoarding() const
{
	return isBoarding;
}



bool Ship::IsLanding() const
{
	return landingPlanet;
}



// Check if this ship is currently able to begin landing on its target.
bool Ship::CanLand() const
{
	if(!GetTargetPlanet() || !GetTargetPlanet()->GetPlanet() || isDisabled || IsDestroyed())
		return false;
	
	if(!GetTargetPlanet()->GetPlanet()->CanLand(*this))
		return false;
	
	Point distance = GetTargetPlanet()->Position() - position;
	double speed = velocity.Length();
	
	return (speed < 1. && distance.Length() < GetTargetPlanet()->Radius());
}



bool Ship::CannotAct() const
{
	return (zoom != 1. || isDisabled || hyperspaceCount || pilotError || cloak);
}



double Ship::Cloaking() const
{
	return isInvisible ? 1. : cloak;
}



// Get the system that the ship is currently jumping to. This returns null
// if the ship has already jumped, i.e. it is leaving hyperspace.
const System *Ship::HyperspaceSystem() const
{
	return hyperspaceSystem;
}



bool Ship::IsEnteringHyperspace() const
{
	return hyperspaceSystem;
}



bool Ship::IsHyperspacing() const
{
	return hyperspaceCount != 0;
}



// Check if this ship is currently able to enter hyperspace to it target.
int Ship::CheckHyperspace() const
{
	// You can't jump if you're waiting for someone else or are already jumping.
	if(commands.Has(Command::WAIT) || hyperspaceCount)
		return 0;
	
	// Find out where we're going and how we're getting there,
	const System *destination = GetTargetSystem();
	int type = HyperspaceType();
	
	// You can't jump to a system with no link to it.
	if(!type)
		return 0;
	
	if(fuel < type)
		return 0;
	
	Point direction = destination->Position() - currentSystem->Position();
	
	// The ship can only enter hyperspace if it is traveling slowly enough
	// and pointed in the right direction.
	if(type == 150)
	{
		double deviation = fabs(direction.Unit().Cross(velocity));
		if(deviation > attributes.Get("scram drive"))
			return 0;
	}
	else if(velocity.Length() > attributes.Get("jump speed"))
		return 0;
	
	if(type != 200)
	{
		// Figure out if we're within one turn step of facing this system.
		bool left = direction.Cross(angle.Unit()) < 0.;
		Angle turned = angle + TurnRate() * (left - !left);
		bool stillLeft = direction.Cross(turned.Unit()) < 0.;
	
		if(left == stillLeft)
			return 0;
	}
	
	return type;
}



// Check what type of hyperspce jump this ship is making (0 = not allowed,
// 100 = hyperdrive, 150 = scram drive, 200 = jump drive).
int Ship::HyperspaceType() const
{
	if(IsDisabled())
		return 0;
	const System *destination = GetTargetSystem();
	if(!destination)
		return 0;
	
	// Check what equipment this ship has.
	bool hasHyperdrive = attributes.Get("hyperdrive");
	bool hasScramDrive = attributes.Get("scram drive");
	bool hasJumpDrive = attributes.Get("jump drive");
	
	// Figure out what sort of jump we're making. 100 = normal hyperspace,
	// 150 = scram drive, 200 = jump drive.
	if(hasHyperdrive || hasScramDrive)
		for(const System *link : currentSystem->Links())
			if(link == destination)
				return hasScramDrive ? 150 : 100;
	
	if(hasJumpDrive)
		for(const System *link : currentSystem->Neighbors())
			if(link == destination)
				return 200;
	
	return 0;
}



// Check if the ship is thrusting. If so, the engine sound should be played.
bool Ship::IsThrusting() const
{
	return isThrusting;
}



// Get the points from which engine flares should be drawn.
const vector<Ship::EnginePoint> &Ship::EnginePoints() const
{
	return enginePoints;
}



// Mark a ship as destroyed.
void Ship::Destroy()
{
	hull = -1.;
}



void Ship::SelfDestruct()
{
	Destroy();
	explosionRate = 1024;
}



void Ship::Restore()
{
	hull = 0;
	Recharge(true);
}



// Check if this ship has been destroyed.
bool Ship::IsDestroyed() const
{
	return (hull < 0.);
}



// Recharge and repair this ship (e.g. because it has landed).
void Ship::Recharge(bool atSpaceport)
{
	if(IsDestroyed())
		return;
	
	if(atSpaceport)
	{
		crew = min(max(crew, RequiredCrew()), static_cast<int>(attributes.Get("bunks")));
		fuel = attributes.Get("fuel capacity");
	}
	pilotError = 0;
	pilotOkay = 0;
	
	if(!personality.IsDerelict())
	{
		if(atSpaceport || attributes.Get("shield generation"))
			shields = attributes.Get("shields");
		if(atSpaceport || attributes.Get("hull repair rate"))
			hull = attributes.Get("hull");
		if(atSpaceport || attributes.Get("energy generation"))
			energy = attributes.Get("energy capacity");
	}
	heat = IdleHeat();
	ionization = 0.;
	disruption = 0.;
	slowness = 0.;
}



bool Ship::CanRefuel(const Ship &other) const
{
	return (fuel - other.JumpFuel() >= JumpFuel());
}



double Ship::TransferFuel(double amount, Ship *to)
{
	amount = max(fuel - attributes.Get("fuel capacity"), amount);
	if(to)
	{
		amount = min(to->attributes.Get("fuel capacity") - to->fuel, amount);
		to->fuel += amount;
	}
	fuel -= amount;
	return amount;
}



void Ship::WasCaptured(const shared_ptr<Ship> &capturer)
{
	// Repair up to the point where it is just barely not disabled.
	hull = max(hull, MinimumHull());
	
	// Set the new government.
	government = capturer->GetGovernment();
	
	// Transfer some crew over. Only transfer the bare minimum unless even that
	// is not possible, in which case, share evenly.
	int totalRequired = capturer->RequiredCrew() + RequiredCrew();
	int transfer = RequiredCrew();
	if(totalRequired > capturer->Crew())
		transfer = max(1, (capturer->Crew() * RequiredCrew()) / totalRequired);
	capturer->AddCrew(-transfer);
	AddCrew(transfer);
	
	// Set the capturer as this ship's parent.
	SetParent(capturer);
	SetTargetShip(shared_ptr<Ship>());
	SetTargetPlanet(nullptr);
	SetTargetSystem(nullptr);
	shipToAssist.reset();
	commands.Clear();
	isDisabled = false;
	hyperspaceSystem = nullptr;
	landingPlanet = nullptr;
	
	isSpecial = capturer->isSpecial;
	personality = capturer->personality;
}



// Get characteristics of this ship, as a fraction between 0 and 1.
double Ship::Shields() const
{
	double maximum = attributes.Get("shields");
	return maximum ? min(1., shields / maximum) : 0.;
}



double Ship::Hull() const
{
	double maximum = attributes.Get("hull");
	return maximum ? min(1., hull / maximum) : 1.;
}



double Ship::Energy() const
{
	double maximum = attributes.Get("energy capacity");
	return maximum ? min(1., energy / maximum) : (hull > 0.) ? 1. : 0.;
}



double Ship::Heat() const
{
	double maximum = Mass() * 100.;
	return maximum ? min(1., heat / maximum) : 1.;
}



double Ship::Fuel() const
{
	double maximum = attributes.Get("fuel capacity");
	return maximum ? min(1., fuel / maximum) : 0.;
}



int Ship::JumpsRemaining() const
{
	// Make sure this ship has some sort of hyperdrive, and if so return how
	// many jumps it can make.
	double jumpFuel = JumpFuel();
	return jumpFuel ? fuel / jumpFuel : 0.;
}



double Ship::JumpFuel() const
{
	int type = HyperspaceType();
	if(type)
		return type;
	return attributes.Get("jump drive") ? 200. :
		attributes.Get("scram drive") ? 150. : 
		attributes.Get("hyperdrive") ? 100. : 0.;
}



// Get the heat level at idle.
double Ship::IdleHeat() const
{
	// Idle heat is the heat level where:
	// heat = heat * diss + heatGen - cool - activeCool * heat / (100 * mass)
	// heat = heat * (diss - activeCool / (100 * mass)) + (heatGen - cool)
	// heat * (1 - diss + activeCool / (100 * mass)) = (heatGen - cool)
	double production = max(0., attributes.Get("heat generation") - attributes.Get("cooling"));
	double dissipation = 1. - heatDissipation + attributes.Get("active cooling") / (100. * Mass());
	return production / dissipation;
}



int Ship::Crew() const
{
	return crew;
}



int Ship::RequiredCrew() const
{
	if(attributes.Get("automaton"))
		return 0;
	
	// Drones do not need crew, but all other ships need at least one.
	return max(1, static_cast<int>(attributes.Get("required crew")));
}



void Ship::AddCrew(int count)
{
	crew = min(crew + count, static_cast<int>(attributes.Get("bunks")));
}



// Check if this is a ship that can be used as a flagship.
bool Ship::CanBeFlagship() const
{
	return !CanBeCarried() && RequiredCrew() && Crew() && !IsDisabled();
}



double Ship::Mass() const
{
	double carried = 0.;
	for(const Bay &bay : bays)
		if(bay.ship)
			carried += bay.ship->Mass();
	return carried + cargo.Used() + attributes.Get("mass");
}



double Ship::TurnRate() const
{
	return attributes.Get("turn") / Mass();
}



double Ship::Acceleration() const
{
	return attributes.Get("thrust") / Mass();
}



double Ship::MaxVelocity() const
{
	// v * drag / mass == thrust / mass
	// v * drag == thrust
	// v = thrust / drag
	return attributes.Get("thrust") / attributes.Get("drag");
}



// This ship just got hit by the given projectile. Take damage according to
// what sort of weapon the projectile it.
int Ship::TakeDamage(const Projectile &projectile, bool isBlast)
{
	int type = 0;
	
	const Outfit &weapon = projectile.GetWeapon();
	double shieldDamage = weapon.ShieldDamage();
	double hullDamage = weapon.HullDamage();
	double hitForce = weapon.HitForce();
	double heatDamage = weapon.HeatDamage();
	double ionDamage = weapon.IonDamage();
	double disruptionDamage = weapon.DisruptionDamage();
	double slowingDamage = weapon.SlowingDamage();
	bool wasDisabled = IsDisabled();
	bool wasDestroyed = IsDestroyed();
	
	double shieldFraction = 1. - weapon.Piercing();
	shieldFraction *= 1. / (1. + disruption * .01);
	if(shields <= 0.)
		shieldFraction = 0.;
	else if(shieldDamage > shields)
		shieldFraction = min(shieldFraction, shields / shieldDamage);
	shields -= shieldDamage * shieldFraction;
	hull -= hullDamage * (1. - shieldFraction);
	heat += heatDamage * (1. - .5 * shieldFraction);
	ionization += ionDamage * (1. - .5 * shieldFraction);
	disruption += disruptionDamage * (1. - .5 * shieldFraction);
	slowness += slowingDamage * (1. - .5 * shieldFraction);
	
	if(hitForce)
	{
		Point d = position - projectile.Position();
		double distance = d.Length();
		if(distance)
			ApplyForce((hitForce / distance) * d);
	}
	
	// Recalculate the disabled ship check.
	isDisabled = true;
	isDisabled = IsDisabled();
	if(!wasDisabled && isDisabled)
		type |= ShipEvent::DISABLE;
	if(!wasDestroyed && IsDestroyed())
		type |= ShipEvent::DESTROY;
	// If this ship was hit directly and did not consider itself an enemy of the
	// ship that hit it, it is now "provoked" against that government.
	if(!isBlast && projectile.GetGovernment() && !projectile.GetGovernment()->IsEnemy(government)
			&& (Shields() < .9 || Hull() < .9 || !personality.IsForbearing())
			&& !personality.IsPacifist() && (shieldDamage > 0. || hullDamage > 0.))
		type |= ShipEvent::PROVOKE;
	
	return type;
}



// Apply a force to this ship, accelerating it. This might be from a weapon
// impact, or from firing a weapon, for example.
void Ship::ApplyForce(const Point &force)
{
	double currentMass = Mass();
	if(!currentMass)
		return;
	
	acceleration += force / currentMass;
}



bool Ship::HasBays() const
{
	return !bays.empty();
}



int Ship::BaysFree(bool isFighter) const
{
	int count = 0;
	for(const Bay &bay : bays)
		count += (bay.isFighter == isFighter) && !bay.ship;
	return count;
}



// Check if this ship has a bay free for the given fighter, and the bay is
// not reserved for one of its existing escorts.
bool Ship::CanCarry(const Ship &ship) const
{
	bool isFighter = (ship.attributes.Category() == "Fighter");
	if(!isFighter && ship.attributes.Category() != "Drone")
		return false;
	
	int free = BaysFree(isFighter);
	if(!free)
		return false;
	
	for(const auto &it : escorts)
	{
		auto escort = it.lock();
		if(escort && escort->attributes.Category() == ship.attributes.Category())
			--free;
	}
	return (free > 0);
}



bool Ship::CanBeCarried() const
{
	const string &category = attributes.Category();
	return (category == "Fighter" || category == "Drone");
}



bool Ship::Carry(const shared_ptr<Ship> &ship)
{
	if(!ship)
		return false;
	
	bool isFighter = ship->attributes.Category() == "Fighter";
	bool isDrone = ship->attributes.Category() == "Drone";
	if(!(isFighter || isDrone))
		return false;
	
	for(Bay &bay : bays)
		if((bay.isFighter == isFighter) && !bay.ship)
		{
			bay.ship = ship;
			ship->SetSystem(nullptr);
			ship->SetPlanet(nullptr);
			ship->SetParent(shared_from_this());
			ship->isThrusting = false;
			// When a fighter rejoins its mothership, its mass is added to the
			// mothership but so is its accumulated heat.
			heat += ship->heat;
			return true;
		}
	return false;
}



void Ship::UnloadBays()
{
	for(Bay &bay : bays)
		if(bay.ship)
		{
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetPlanet(landingPlanet);
			bay.ship.reset();
		}
}



const vector<Ship::Bay> &Ship::Bays() const
{
	return bays;
}



// Adjust the positions and velocities of any visible carried fighters or
// drones. If any are visible, return true.
bool Ship::PositionFighters() const
{
	bool hasVisible = false;
	for(const Bay &bay : bays)
		if(bay.ship && bay.side)
		{
			hasVisible = true;
			bay.ship->position = angle.Rotate(bay.point) * Zoom() + position;
			bay.ship->velocity = velocity;
			bay.ship->angle = angle + BAY_ANGLE[bay.facing];
			bay.ship->zoom = zoom;
		}
	return hasVisible;
}



CargoHold &Ship::Cargo()
{
	return cargo;
}



const CargoHold &Ship::Cargo() const
{
	return cargo;
}



// Display box effects from jettisoning this much cargo.
void Ship::Jettison(const string &commodity, int tons)
{
	cargo.Remove(commodity, tons);
	
	// Jettisoned cargo must carry some of the ship's heat with it. Otherwise
	// jettisoning cargo would increase the ship's temperature.
	double shipMass = Mass();
	heat *= shipMass / (shipMass + tons);
	
	static const int perBox = 5;
	for( ; tons >= perBox; tons -= perBox)
		jettisoned.emplace_back(new Flotsam(commodity, perBox));
}



void Ship::Jettison(const Outfit *outfit, int count)
{
	if(count < 0)
		return;

	cargo.Remove(outfit, count);
	
	// Jettisoned cargo must carry some of the ship's heat with it. Otherwise
	// jettisoning cargo would increase the ship's temperature.
	double mass = outfit->Get("mass");
	double shipMass = Mass();
	heat *= shipMass / (shipMass + count * mass);
	
	const int perBox = (mass <= 0.) ? count : (mass > 5.) ? 1 : static_cast<int>(5. / mass);
	while(count > 0)
	{
		jettisoned.emplace_back(new Flotsam(outfit, (perBox < count) ? perBox : count));
		count -= perBox;
	}
}



const Outfit &Ship::Attributes() const
{
	return attributes;
}



const Outfit &Ship::BaseAttributes() const
{
	return baseAttributes;
}



// Get outfit information.
const map<const Outfit *, int> &Ship::Outfits() const
{
	return outfits;
}



int Ship::OutfitCount(const Outfit *outfit) const
{
	auto it = outfits.find(outfit);
	return (it == outfits.end()) ? 0 : it->second;
}



// Add or remove outfits. (To remove, pass a negative number.)
void Ship::AddOutfit(const Outfit *outfit, int count)
{
	if(outfit && count)
	{
		auto it = outfits.find(outfit);
		if(it == outfits.end())
			outfits[outfit] = count;
		else
		{
			it->second += count;
			if(!it->second)
				outfits.erase(it);
		}
		attributes.Add(*outfit, count);
		if(outfit->IsWeapon())
			armament.Add(outfit, count);
		
		if(outfit->Get("cargo space"))
			cargo.SetSize(attributes.Get("cargo space"));
		if(outfit->Get("hull"))
			hull += outfit->Get("hull") * count;
	}
}



// Get the list of weapons.
Armament &Ship::GetArmament()
{
	return armament;
}



const vector<Hardpoint> &Ship::Weapons() const
{
	return armament.Get();
}



// Check if we are able to fire the given weapon (i.e. there is enough
// energy, ammo, and fuel to fire it).
bool Ship::CanFire(const Outfit *outfit) const
{
	if(!outfit || !outfit->IsWeapon())
		return false;
	
	if(outfit->Ammo())
	{
		auto it = outfits.find(outfit->Ammo());
		if(it == outfits.end() || it->second <= 0)
			return false;
	}
	
	if(energy < outfit->FiringEnergy())
		return false;
	if(fuel < outfit->FiringFuel())
		return false;
	
	return true;
}



// Fire the given weapon (i.e. deduct whatever energy, ammo, or fuel it uses
// and add whatever heat it generates. Assume that CanFire() is true.
void Ship::ExpendAmmo(const Outfit *outfit)
{
	if(!outfit)
		return;
	if(outfit->Ammo())
		AddOutfit(outfit->Ammo(), -1);
	
	energy -= outfit->FiringEnergy();
	fuel -= outfit->FiringFuel();
	heat += outfit->FiringHeat();
}



// Each ship can have a target system (to travel to), a target planet (to
// land on) and a target ship (to move to, and attack if hostile).
shared_ptr<Ship> Ship::GetTargetShip() const
{
	return targetShip.lock();
}



shared_ptr<Ship> Ship::GetShipToAssist() const
{
	return shipToAssist.lock();
}



const StellarObject *Ship::GetTargetPlanet() const
{
	return targetPlanet;
}



const System *Ship::GetTargetSystem() const
{
	return (targetSystem == currentSystem) ? nullptr : targetSystem;
}



// Mining target.
shared_ptr<Minable> Ship::GetTargetAsteroid() const
{
	return targetAsteroid.lock();
}



shared_ptr<Flotsam> Ship::GetTargetFlotsam() const
{
	return targetFlotsam.lock();
}



// Set this ship's targets.
void Ship::SetTargetShip(const shared_ptr<Ship> &ship)
{
	if(ship != targetShip.lock())
	{
		targetShip = ship;
		// When you change targets, clear your scanning records.
		cargoScan = 0.;
		outfitScan = 0.;
	}
}



void Ship::SetShipToAssist(const shared_ptr<Ship> &ship)
{
	shipToAssist = ship;
}



void Ship::SetTargetPlanet(const StellarObject *object)
{
	targetPlanet = object;
}



void Ship::SetTargetSystem(const System *system)
{
	targetSystem = system;
}



// Mining target.
void Ship::SetTargetAsteroid(const shared_ptr<Minable> &asteroid)
{
	targetAsteroid = asteroid;
}



void Ship::SetTargetFlotsam(const shared_ptr<Flotsam> &flotsam)
{
	targetFlotsam = flotsam;
}



void Ship::SetParent(const shared_ptr<Ship> &ship)
{
	shared_ptr<Ship> oldParent = parent.lock();
	if(oldParent)
		oldParent->RemoveEscort(*this);
	
	parent = ship;
	if(ship)
		ship->AddEscort(*this);
}



shared_ptr<Ship> Ship::GetParent() const
{
	return parent.lock();
}



const vector<weak_ptr<const Ship>> &Ship::GetEscorts() const
{
	return escorts;
}



// Add escorts to this ship. Escorts look to the parent ship for movement
// cues and try to stay with it when it lands or goes into hyperspace.
void Ship::AddEscort(const Ship &ship)
{
	escorts.push_back(ship.shared_from_this());
}



void Ship::RemoveEscort(const Ship &ship)
{
	auto it = escorts.begin();
	for( ; it != escorts.end(); ++it)
		if(it->lock().get() == &ship)
		{
			escorts.erase(it);
			return;
		}
}



double Ship::MinimumHull() const
{
	if(neverDisabled)
		return 0.;
	
	double maximumHull = attributes.Get("hull");
	return max(.20 * maximumHull, min(.50 * maximumHull, 400.));
}



// Add to this ship's hull or shields, and return the amount added. If the
// ship is carrying fighters, add to them as well.
double Ship::AddHull(double rate)
{
	double added = min(rate, attributes.Get("hull") - hull);
	hull += added;
	rate -= added;
	
	for(Bay &bay : bays)
	{
		if(!bay.ship)
			continue;
		
		double myGen = bay.ship->Attributes().Get("hull repair rate");
		double myMax = bay.ship->Attributes().Get("hull");
		bay.ship->hull = min(myMax, bay.ship->hull + myGen);
		if(rate > 0. && bay.ship->hull < myMax)
		{
			double extra = min(myMax - bay.ship->hull, rate);
			bay.ship->hull += extra;
			rate -= extra;
			added += extra;
		}
	}
	return added;
}



double Ship::AddShields(double rate)
{
	double added = min(rate, attributes.Get("shields") - shields);
	shields += added;
	rate -= added;
	
	for(Bay &bay : bays)
	{
		if(!bay.ship)
			continue;
		
		double myGen = bay.ship->Attributes().Get("shield generation");
		double myMax = bay.ship->Attributes().Get("shields");
		bay.ship->shields = min(myMax, bay.ship->shields + myGen);
		if(rate > 0. && bay.ship->shields < myMax)
		{
			double extra = min(myMax - bay.ship->shields, rate);
			bay.ship->shields += extra;
			rate -= extra;
			added += extra;
		}
	}
	return added;
}



void Ship::CreateExplosion(list<Effect> &effects, bool spread)
{
	if(!HasSprite() || !GetMask().IsLoaded() || explosionEffects.empty())
		return;
	
	// Bail out if this loops enough times, just in case.
	for(int i = 0; i < 10; ++i)
	{
		Point point((Random::Real() - .5) * Width(),
			(Random::Real() - .5) * Height());
		if(GetMask().Contains(point, Angle()))
		{
			// Pick an explosion.
			int type = Random::Int(explosionTotal);
			auto it = explosionEffects.begin();
			for( ; it != explosionEffects.end(); ++it)
			{
				type -= it->second;
				if(type < 0)
					break;
			}
			effects.push_back(*it->first);
			Point effectVelocity = velocity;
			if(spread)
			{
				double scale = .04 * (Width() + Height());
				effectVelocity += Angle::Random().Unit() * (scale * Random::Real());
			}
			effects.back().Place(angle.Rotate(point) + position, effectVelocity, angle);
			++explosionCount;
			return;
		}
	}
}



// Place a "spark" effect, like ionization or disruption.
void Ship::CreateSparks(list<Effect> &effects, const string &name, double amount)
{
	if(forget)
		return;
	
	// Limit the number of sparks, depending on the size of the sprite.
	amount = min(amount, Width() * Height() * .0006);
	
	const Effect *effect = GameData::Effects().Get(name);
	while(true)
	{
		amount -= Random::Real();
		if(amount <= 0.)
			break;
		
		Point point((Random::Real() - .5) * Width(),
			(Random::Real() - .5) * Height());
		if(GetMask().Contains(point, Angle()))
		{
			effects.push_back(*effect);
			effects.back().Place(angle.Rotate(point) + position, velocity, angle);
		}
	}
}
