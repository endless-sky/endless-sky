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

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Government.h"
#include "Mask.h"
#include "Messages.h"
#include "Projectile.h"
#include "Random.h"
#include "SpriteSet.h"
#include "System.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace std;



Ship::Ship()
	: government(nullptr), isInSystem(true),
	forget(0), isSpecial(false), isOverheated(false), isDisabled(false),
	isBoarding(false), hasBoarded(false),
	explosionWeapon(nullptr),
	shields(0.), hull(0.), fuel(0.), energy(0.), heat(0.),
	currentSystem(nullptr),
	zoom(1.), landingPlanet(nullptr),
	hyperspaceCount(0), hyperspaceSystem(nullptr),
	explosionRate(0), explosionCount(0), explosionTotal(0)
{
}



void Ship::Load(const DataNode &node)
{
	assert(node.Size() >= 2 && node.Token(0) == "ship");
	modelName = node.Token(1);
	
	government = GameData::Governments().Get("Escort");
	crew = 0;
	equipped.clear();
	
	// Note: I do not clear the attributes list here so that it is permissible
	// to override one ship definition with another.
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "sprite")
			sprite.Load(child);
		else if(child.Token(0) == "name" && child.Size() >= 2)
			name = child.Token(1);
		else if(child.Token(0) == "attributes")
			baseAttributes.Load(child);
		else if(child.Token(0) == "engine" && child.Size() >= 3)
			enginePoints.emplace_back(child.Value(1), child.Value(2));
		else if((child.Token(0) == "gun" || child.Token(0) == "turret") && child.Size() >= 3)
		{
			Point hardpoint(child.Value(1), child.Value(2));
			const Outfit *outfit = nullptr;
			if(child.Size() >= 4)
			{
				outfit = GameData::Outfits().Get(child.Token(3));
				++equipped[outfit];
			}
			if(child.Token(0) == "gun")
				armament.AddGunPort(hardpoint, outfit);
			else
				armament.AddTurret(hardpoint, outfit);
		}
		else if(child.Token(0) == "fighter" && child.Size() >= 3)
			fighterBays.emplace_back(child.Value(1), child.Value(2));
		else if(child.Token(0) == "drone" && child.Size() >= 3)
			droneBays.emplace_back(child.Value(1), child.Value(2));
		else if(child.Token(0) == "explode" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			explosionEffects[GameData::Effects().Get(child.Token(1))] += count;
			explosionTotal += count;
		}
		else if(child.Token(0) == "outfits")
		{
			for(const DataNode &grand : child)
			{
				int count = (grand.Size() >= 2) ? grand.Value(1) : 1;
				this->outfits[GameData::Outfits().Get(grand.Token(0))] += count;
			}
		}
		else if(child.Token(0) == "cargo")
			cargo.Load(child);
		else if(child.Token(0) == "crew" && child.Size() >= 2)
			crew = static_cast<int>(child.Value(1));
		else if(child.Token(0) == "system" && child.Size() >= 2)
			currentSystem = GameData::Systems().Get(child.Token(1));
		else if(child.Token(0) == "planet" && child.Size() >= 2)
		{
			zoom = 0.;
			landingPlanet = GameData::Planets().Get(child.Token(1));
		}
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			description += child.Token(1);
			description += '\n';
		}
	}
	baseAttributes.Reset("gun ports", armament.GunCount());
	baseAttributes.Reset("turret mounts", armament.TurretCount());
	attributes = baseAttributes;
	cargo.SetSize(attributes.Get("cargo space"));
	// All copies of this ship should save pointers to the "explosion" weapon
	// definition stored safely in the ship model, which will not be destroyed
	// until GameData is when the program quits.
	explosionWeapon = &baseAttributes;
}



// When loading a ship, some of the outfits it lists may not have been
// loaded yet. So, wait until everything has been loaded, then call this.
void Ship::FinishLoading()
{
	// TODO: any way to do this through lazy evaluation, instead of having to
	// call this function explicitly?
	for(const auto &it : outfits)
	{
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
	
	Recharge();
}



// Save a full description of this ship, as currently configured.
void Ship::Save(DataWriter &out) const
{
	out.Write("ship", modelName);
	out.BeginChild();
		out.Write("name", name);
		sprite.Save(out);
		
		out.Write("attributes");
		out.BeginChild();
			out.Write("category", baseAttributes.Category());
			for(const auto &it : baseAttributes.Attributes())
				if(it.second)
					out.Write(it.first, it.second);
		out.EndChild();
		
		out.Write("outfits");
		out.BeginChild();
			for(const auto &it : outfits)
				if(it.first && it.second)
				{
					if(it.second == 1)
						out.Write(it.first->Name());
					else
						out.Write(it.first->Name(), it.second);
				}
		out.EndChild();
		
		cargo.Save(out);
		out.Write("crew", crew);
		
		for(const Point &point : enginePoints)
			out.Write("engine", point.X(), point.Y());
		for(const Armament::Weapon &weapon : armament.Get())
		{
			const char *type = (weapon.IsTurret() ? "turret" : "gun");
			if(weapon.GetOutfit())
				out.Write(type, 2. * weapon.GetPoint().X(), 2. * weapon.GetPoint().Y(),
					weapon.GetOutfit()->Name());
			else
				out.Write(type, 2. * weapon.GetPoint().X(), 2. * weapon.GetPoint().Y());
		}
		for(const Bay &bay : fighterBays)
			out.Write("fighter", 2. * bay.point.X(), 2. * bay.point.Y());
		for(const Bay &bay : droneBays)
			out.Write("drone", 2. * bay.point.X(), 2. * bay.point.Y());
		for(const auto &it : explosionEffects)
			if(it.first && it.second)
				out.Write("explode", it.first->Name(), it.second);
		
		if(currentSystem)
			out.Write("system", currentSystem->Name());
		if(landingPlanet)
			out.Write("planet", landingPlanet->Name());
	out.EndChild();
}



const string &Ship::ModelName() const
{
	return modelName;
}



// Get this ship's description.
const string &Ship::Description() const
{
	return description;
}



// Get this ship's cost.
int Ship::Cost() const
{
	return attributes.Cost();
}



void Ship::Place(Point position, Point velocity, Angle angle)
{
	this->position = position;
	this->velocity = velocity;
	this->angle = angle;
	// If landed, place the ship right above the planet.
	if(landingPlanet)
		landingPlanet = nullptr;
	else
		zoom = 1.;
	forget = 1;
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
	zoom = 0.;
	landingPlanet = planet;
}



void Ship::SetGovernment(const Government *government)
{
	if(government)
		sprite.SetSwizzle(government->GetSwizzle());
	this->government = government;
}



void Ship::SetIsSpecial(bool special)
{
	isSpecial = special;
}



const Personality &Ship::GetPersonality() const
{
	return personality;
}



void Ship::SetPersonality(const Personality &other)
{
	personality = other;
}



// Move this ship. A ship may create effects as it moves, in particular if
// it is in the process of blowing up. If this returns false, the ship
// should be deleted.
bool Ship::Move(list<Effect> &effects)
{
	// Check if this ship has been in a different system from the player for so
	// long that it should be "forgotten."
	forget += !isInSystem;
	if((!isSpecial && forget >= 1000) || !currentSystem)
		return false;
	isInSystem = false;
	if(!fuel || !(attributes.Get("hyperdrive") || attributes.Get("jump drive")))
		hyperspaceSystem = nullptr;
	
	
	// When ships recharge, what actually happens is that they can exceed their
	// maximum capacity for the rest of the turn, but must be clamped to the
	// maximum here before they gain more. This is so that, for example, a ship
	// with no batteries but a good generator can still move.
	energy = min(energy, attributes.Get("energy capacity"));
	
	heat *= .999;
	if(heat > Mass() * 100.)
		isOverheated = true;
	else if(heat < Mass() * 90.)
		isOverheated = false;
	
	double maxShields = attributes.Get("shields");
	shields = min(shields, attributes.Get("shields"));
	hull = min(hull, attributes.Get("hull"));
	isDisabled = isOverheated || IsDisabled();
	
	// Update ship supply levels.
	if(!isOverheated)
	{
		// Note: If the ship is disabled because of low hull percent, _and_ it
		// has the capability of repairing its hull, it can repair enough to
		// cease to be disabled.
		hull += attributes.Get("hull repair rate");
	}
	if(!isDisabled)
	{
		// If you have a ramscoop, you recharge enough fuel to make one jump in
		// a little less than a minute - enough to be an inconvenience without
		// being totally aggravating.
		if(attributes.Get("ramscoop"))
		{
			fuel += .03 * sqrt(attributes.Get("ramscoop"));
			fuel = min(fuel, attributes.Get("fuel capacity"));
		}
		
		energy += attributes.Get("energy generation");
		heat += attributes.Get("heat generation");
		
		// Recharge shields, but only up to the max. If there is extra shield
		// energy, use it to recharge fighters and drones.
		shields += attributes.Get("shield generation");
		double excessShields = max(0., shields - maxShields);
		shields -= excessShields;
		
		for(Bay &bay : fighterBays)
		{
			if(!bay.ship)
				continue;
			
			double myGen = bay.ship->Attributes().Get("shield generation");
			double myMax = bay.ship->Attributes().Get("shields");
			bay.ship->shields = min(myMax, bay.ship->shields + myGen);
			if(excessShields > 0. && bay.ship->shields < myMax)
			{
				double extra = min(myMax - bay.ship->shields, excessShields);
				bay.ship->shields += extra;
				excessShields -= extra;
			}
		}
		for(Bay &bay : droneBays)
		{
			if(!bay.ship)
				continue;
			
			double myGen = bay.ship->Attributes().Get("shield generation");
			double myMax = bay.ship->Attributes().Get("shields");
			bay.ship->shields = min(myMax, bay.ship->shields + myGen);
			if(excessShields > 0. && bay.ship->shields < myMax)
			{
				double extra = min(myMax - bay.ship->shields, excessShields);
				bay.ship->shields += extra;
				excessShields -= extra;
			}
		}
	}
	
	
	if(hull <= 0.)
	{
		// Once we've created enough little explosions, die.
		if(explosionCount == explosionTotal || forget)
		{
			if(!forget)
				for(unsigned i = 0; i < explosionTotal; ++i)
					CreateExplosion(effects);
			energy = 0.;
			heat = 0.;
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
		fuel -= (hyperspaceSystem != nullptr);
		
		// Enter hyperspace.
		int direction = (hyperspaceSystem != nullptr) - (hyperspaceSystem == nullptr);
		hyperspaceCount += direction;
		static const int HYPER_C = 100;
		static const double HYPER_A = 2.;
		if(hyperspaceCount == HYPER_C)
		{
			currentSystem = hyperspaceSystem;
			hyperspaceSystem = nullptr;
			SetTargetSystem(nullptr);
			SetTargetPlanet(nullptr);
			direction = -1;
			
			Point target;
			for(const StellarObject &object : currentSystem->Objects())
				if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
				{
					target = object.Position();
					break;
				}
			
			// Have all ships exit hyperspace at the same distance so that
			// your escorts always stay with you.
			double distance = (HYPER_C * HYPER_C) * .5 * HYPER_A + 1000.;
			position = (target - distance * angle.Unit());
		}
		velocity += (HYPER_A * direction) * angle.Unit();
		position += velocity;
		if(velocity.Length() <= MaxVelocity() && !hyperspaceSystem)
		{
			velocity = angle.Unit() * MaxVelocity();
			hyperspaceCount = 0;
		}
		
		return true;
	}
	else if(landingPlanet || zoom < 1.)
	{
		// Special ships do not disappear forever when they land; they
		// just slowly refuel.
		if(landingPlanet && zoom)
		{
			zoom -= .02;
			if(zoom < 0.)
			{
				// If this is not a special ship, it ceases to exist when it
				// lands.
				if(!isSpecial)
					return false;
				
				zoom = 0.;
			}
		}
		// Only refuel if this planet has a spaceport.
		else if(fuel == attributes.Get("fuel capacity")
				|| !landingPlanet || !landingPlanet->HasSpaceport())
		{
			zoom = min(1., zoom + .02);
			landingPlanet = nullptr;
		}
		else
			fuel = min(fuel + 1., attributes.Get("fuel capacity"));
		
		// Move the ship at the velocity it had when it began landing, but
		// scaled based on how small it is now.
		position += velocity * zoom;
		
		return true;
	}
	if(HasLandCommand() && CanLand())
		landingPlanet = GetTargetPlanet()->GetPlanet();
	else if(HasHyperspaceCommand() && CanHyperspace())
		hyperspaceSystem = GetTargetSystem();
	
	int requiredCrew = RequiredCrew();
	if(pilotError)
		--pilotError;
	else if(requiredCrew && static_cast<int>(Random::Int(requiredCrew)) >= Crew())
	{
		pilotError = 30;
		Messages::Add("Your ship is moving erratically because you do not have enough crew to pilot it.");
	}
	
	// This ship is not landing or entering hyperspace. So, move it. If it is
	// disabled, all it can do is slow down to a stop.
	double mass = Mass();
	if(isDisabled)
		velocity *= 1. - attributes.Get("drag") / mass;
	else if(!pilotError)
	{
		if(GetThrustCommand())
		{
			// Check if we are able to apply this thrust.
			double cost = attributes.Get((GetThrustCommand() > 0.) ?
				"thrusting energy" : "reverse thrusting energy");
			if(energy < cost)
				SetThrustCommand(0.);
			else
			{
				// If a reverse thrust is commanded and the capability does not
				// exist, ignore it (do not even slow under drag).
				double thrust = attributes.Get((GetThrustCommand() > 0.) ?
					"thrust" : "reverse thrust");
				if(!thrust)
					SetThrustCommand(0.);
				else
				{
					energy -= cost;
					heat += attributes.Get((GetThrustCommand() > 0.) ?
						"thrusting heat" : "reverse thrusting heat");
					velocity += angle.Unit() * (GetThrustCommand() * thrust / mass);
					velocity *= 1. - attributes.Get("drag") / mass;
				}
			}
		}
		if(GetTurnCommand())
		{
			// Check if we are able to turn.
			double cost = attributes.Get("turning energy");
			if(energy < cost)
				SetTurnCommand(0.);
			else
			{
				energy -= cost;
				heat += attributes.Get("turning heat");
				angle += GetTurnCommand() * TurnRate();
			}
		}
	}
	
	// Boarding:
	if(isBoarding && (GetThrustCommand() || GetTurnCommand()))
		isBoarding = false;
	shared_ptr<const Ship> target = (IsFighter() ? GetParent() : GetTargetShip()).lock();
	if(target && !isDisabled)
	{
		Point dp = (target->position - position);
		double distance = dp.Length();
		Point dv = (target->velocity - velocity);
		double speed = dv.Length();
		isBoarding |= (distance < 50. && speed < 1. && HasBoardCommand());
		if(isBoarding && !IsFighter() && (!target->IsDisabled() || target->Hull() < 0.))
			isBoarding = false;
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
			
			if(distance < 10. && speed < 1. && (IsFighter() || !turn))
			{
				isBoarding = false;
				hasBoarded = true;
			}
		}
	}
	
	// And finally: move the ship!
	position += velocity;
	
	return true;
}



// Launch any ships that are ready to launch.
void Ship::Launch(list<shared_ptr<Ship>> &ships)
{
	if(!HasLaunchCommand() || zoom != 1. || isDisabled || hyperspaceCount || pilotError)
		return;
	
	for(Bay &bay : fighterBays)
		if(bay.ship && !Random::Int(60))
		{
			ships.push_back(bay.ship);
			double maxV = bay.ship->MaxVelocity();
			Point v = velocity + (.3 * maxV) * angle.Unit() + (.2 * maxV) * Angle::Random().Unit();
			bay.ship->Place(position + angle.Rotate(bay.point), v, angle);
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetParent(shared_from_this());
			AddEscort(bay.ship);
			bay.ship.reset();
		}
	for(Bay &bay : droneBays)
		if(bay.ship && !Random::Int(40))
		{
			ships.push_back(bay.ship);
			double maxV = bay.ship->MaxVelocity();
			Point v = velocity + (.3 * maxV) * angle.Unit() + (.2 * maxV) * Angle::Random().Unit();
			bay.ship->Place(position + angle.Rotate(bay.point), v, angle);
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetParent(shared_from_this());
			AddEscort(bay.ship);
			bay.ship.reset();
		}
}



// Check if this ship is boarding another ship.
shared_ptr<Ship> Ship::Board(list<shared_ptr<Ship>> &ships, bool autoPlunder)
{
	if(!hasBoarded)
		return shared_ptr<Ship>();
	hasBoarded = false;
	
	// Get a non-const pointer to the ship.
	shared_ptr<const Ship> target = (IsFighter() ? GetParent() : GetTargetShip()).lock();
	shared_ptr<Ship> victim;
	for(shared_ptr<Ship> &ship : ships)
		if(ship == target)
			victim = ship;
	if(!victim)
		return shared_ptr<Ship>();
	
	// For a fighter, "board" means "return to ship."
	if(IsFighter())
	{
		if(victim->AddFighter(shared_from_this()))
		{
			currentSystem = nullptr;
			victim->RemoveEscort(this);
		}
		return shared_ptr<Ship>();
	}
	if(!target->IsDisabled() || target->Hull() <= 0.)
		return shared_ptr<Ship>();
	
	// If the boarding ship is the player, they will choose what to plunder.
	if(victim && autoPlunder)
	{
		boardedShips.insert(victim.get());
		
		// Take any outfits that fit.
		for(auto &it : victim->outfits)
			while(it.second && cargo.Transfer(it.first, -1))
				--it.second;
		// Take any commodities that fit.
		victim->cargo.TransferAll(&cargo);
		victim.reset();
		// Stop targeting this ship.
		SetTargetShip(victim);
	}
	
	return victim;
}



// Fire any weapons that are ready to fire. If an anti-missile is ready,
// instead of firing here this function returns true and it can be fired if
// collision detection finds a missile in range.
bool Ship::Fire(list<Projectile> &projectiles)
{
	isInSystem = true;
	forget = 0;
	
	// A ship that is about to die creates a special single-turn "projectile"
	// representing its death explosion.
	if(explosionCount == explosionTotal && explosionWeapon)
		projectiles.emplace_back(position, explosionWeapon);
	
	if(zoom != 1. || isDisabled || hyperspaceCount || pilotError)
		return false;
	
	bool hasAntiMissile = false;
	
	const vector<Armament::Weapon> &weapons = armament.Get();
	for(unsigned i = 0; i < weapons.size(); ++i)
	{
		const Outfit *outfit = weapons[i].GetOutfit();
		if(outfit && CanFire(outfit))
		{
			if(outfit->WeaponGet("anti-missile"))
				hasAntiMissile = true;
			else if(HasFireCommand(i))
				armament.Fire(i, *this, projectiles);
		}
	}
	
	armament.Step(*this);
	
	return hasAntiMissile;
}



// Fire an anti-missile.
bool Ship::FireAntiMissile(const Projectile &projectile, list<Effect> &effects)
{
	const vector<Armament::Weapon> &weapons = armament.Get();
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



bool Ship::IsTargetable() const
{
	return (zoom == 1. && !explosionRate && !forget);
}



bool Ship::IsOverheated() const
{
	return isOverheated;
}



bool Ship::IsDisabled() const
{
	double maximumHull = attributes.Get("hull");
	double minimumHull = max(.10 * maximumHull, min(.50 * maximumHull, 400.));
	bool needsCrew = RequiredCrew() != 0;
	return (hull < minimumHull || (!crew && needsCrew));
}



bool Ship::IsLanding() const
{
	return landingPlanet;
}



bool Ship::IsHyperspacing() const
{
	return hyperspaceSystem;
}



// Check if this ship is currently able to begin landing on its target.
bool Ship::CanLand() const
{
	if(!GetTargetPlanet() || isDisabled || Hull() <= 0.)
		return false;
	
	Point distance = GetTargetPlanet()->Position() - position;
	double speed = velocity.Length();
	
	return (speed < 1. && distance.Length() < GetTargetPlanet()->Radius());
}



// Check if this ship is currently able to enter hyperspace to it target.
bool Ship::CanHyperspace() const
{
	if(!GetTargetSystem())
		return false;
	
	// The ship can only enter hyperspace if it is traveling slowly enough
	// and pointed in the right direction.
	double speed = velocity.Length();
	if(speed > .2)
		return false;
	
	Point direction = GetTargetSystem()->Position() - currentSystem->Position();
	
	// Figure out if we're within one turn step of facing this system.
	bool left = direction.Cross(angle.Unit()) < 0.;
	Angle turned = angle + TurnRate() * (left - !left);
	bool stillLeft = direction.Cross(turned.Unit()) < 0.;
	
	return (left != stillLeft);
}



bool Ship::IsBoarding() const
{
	return isBoarding;
}



// Check if this ship has boarded another ship. This is just so the AI knows
// to move on and not keep boarding the same ship forever.
bool Ship::HasBoarded(const Ship *ship) const
{
	return (boardedShips.find(ship) != boardedShips.end());
}



const Animation &Ship::GetSprite() const
{
	return sprite;
}



// Get the ship's government.
const Government *Ship::GetGovernment() const
{
	return government;
}



double Ship::Zoom() const
{
	return zoom;
}



const string &Ship::Name() const
{
	return name;
}



// Get the points from which engine flares should be drawn. If the ship is
// not thrusting right now, this will be empty.
const vector<Point> &Ship::EnginePoints() const
{
	static const vector<Point> empty;
	if(GetThrustCommand() <= 0. || isDisabled || attributes.FlareSprite().IsEmpty())
		return empty;
	
	return enginePoints;
}



// Get the sprite to be used for an engine flare, if the engines are firing
// at the moment. Otherwise, this returns null.
const Animation &Ship::FlareSprite() const
{
	return attributes.FlareSprite();
}



const Point &Ship::Position() const
{
	return position;
}



const Point &Ship::Velocity() const
{
	return velocity;
}



const Angle &Ship::Facing() const
{
	return angle;
}



// Get the facing unit vector times the scale factor.
Point Ship::Unit() const
{
	return angle.Unit() * (zoom * .5);
}



// Recharge and repair this ship (e.g. because it has landed).
void Ship::Recharge()
{
	crew = max(crew, RequiredCrew());
	pilotError = 0;
	
	shields = attributes.Get("shields");
	hull = attributes.Get("hull");
	energy = attributes.Get("energy capacity");
	fuel = attributes.Get("fuel capacity");
	heat = attributes.Get("heat generation") / .001;
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



double Ship::Fuel() const
{
	double maximum = attributes.Get("fuel capacity");
	return maximum ? min(1., fuel / maximum) : 0.;
}



int Ship::JumpsRemaining() const
{
	return fuel / 100;
}



double Ship::Energy() const
{
	double maximum = attributes.Get("energy capacity");
	return maximum ? min(1., energy / maximum) : 1.;
}



double Ship::Heat() const
{
	double maximum = Mass() * 100.;
	return maximum ? min(1., heat / maximum) : 1.;
}



int Ship::Crew() const
{
	return crew;
}



int Ship::RequiredCrew() const
{
	// Drones do not need crew, but all other ships need at least one.
	return max(attributes.Category() == "Drone" ? 0 : 1,
		static_cast<int>(attributes.Get("required crew")));
}



void Ship::AddCrew(int count)
{
	crew += count;
}



void Ship::WasCaptured(const shared_ptr<Ship> &capturer)
{
	// Repair up to the point where it is just barely not disabled.
	double maximumHull = attributes.Get("hull");
	hull = max(.10 * maximumHull, min(.50 * maximumHull, 400.));
	
	// Set the new government.
	government = capturer->GetGovernment();
	
	// Transfer some crew over.
	int totalRequired = capturer->RequiredCrew() + RequiredCrew();
	int transfer = max(1, (capturer->Crew() * RequiredCrew()) / totalRequired);
	capturer->AddCrew(-transfer);
	AddCrew(transfer);
	
	// Set the capturer as this ship's parent.
	SetParent(capturer);
	// TODO: add as an "escort" to this ship.
	
	isSpecial = capturer->isSpecial;
	personality = capturer->personality;
}



// Check if this ship should be deleted.
bool Ship::ShouldDelete() const
{
	return (!zoom && !isSpecial) || (hull <= 0. && explosionCount >= explosionTotal);
}



double Ship::Mass() const
{
	double carried = 0.;
	for(const Bay &bay : droneBays)
		if(bay.ship)
			carried += bay.ship->Mass();
	for(const Bay &bay : fighterBays)
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
void Ship::TakeDamage(const Projectile &projectile)
{
	const Outfit &weapon = projectile.GetWeapon();
	double shieldDamage = weapon.WeaponGet("shield damage");
	double hullDamage = weapon.WeaponGet("hull damage");
	double hitForce =  weapon.WeaponGet("hit force");
	double heatDamage = weapon.WeaponGet("heat damage");
	
	isBoarding = false;
	
	if(shields > shieldDamage)
	{
		shields -= shieldDamage;
		heat += .25 * heatDamage;
	}
	else
	{
		hull -= hullDamage * (1. - (shields / shieldDamage));
		shields = 0.;
		heat += heatDamage;
	}
	
	if(hitForce)
		ApplyForce(hitForce * (position - projectile.Position()).Unit());
}



// Apply a force to this ship, accelerating it. This might be from a weapon
// impact, or from firing a weapon, for example.
void Ship::ApplyForce(const Point &force)
{
	double currentMass = Mass();
	velocity += force / currentMass;
	double maxVelocity = MaxVelocity();
	double currentVelocity = velocity.Length();
	if(currentVelocity > maxVelocity)
		velocity *= maxVelocity / currentVelocity;
}



int Ship::FighterBaysFree() const
{
	int count = 0;
	for(const Bay &bay : fighterBays)
		count += !bay.ship;
	return count;
}



int Ship::DroneBaysFree() const
{
	int count = 0;
	for(const Bay &bay : droneBays)
		count += !bay.ship;
	return count;
}



bool Ship::AddFighter(const shared_ptr<Ship> &ship)
{
	if(!ship)
		return false;
	
	bool isFighter = ship->attributes.Category() == "Fighter";
	bool isDrone = ship->attributes.Category() == "Drone";
	if(!(isFighter || isDrone))
		return false;
	
	vector<Bay> &bays = isFighter ? fighterBays : droneBays;
	for(Bay &bay : bays)
		if(!bay.ship)
		{
			bay.ship = ship;
			ship->SetSystem(nullptr);
			ship->SetPlanet(nullptr);
			return true;
		}
	return false;
}



void Ship::UnloadFighters()
{
	for(Bay &bay : fighterBays)
		if(bay.ship)
		{
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetPlanet(landingPlanet);
			bay.ship.reset();
		}
	for(Bay &bay : droneBays)
		if(bay.ship)
		{
			bay.ship->SetSystem(currentSystem);
			bay.ship->SetPlanet(landingPlanet);
			bay.ship.reset();
		}
}



bool Ship::IsFighter() const
{
	const string &category = attributes.Category();
	return (category == "Fighter" || category == "Drone");
}



CargoHold &Ship::Cargo()
{
	return cargo;
}




const CargoHold &Ship::Cargo() const
{
	return cargo;
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



const Outfit &Ship::Attributes() const
{
	return attributes;
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
	}
}



// Get the list of weapons.
const vector<Armament::Weapon> &Ship::Weapons() const
{
	return armament.Get();
}



// Check if we are able to fire the given weapon (i.e. there is enough
// energy, ammo, and fuel to fire it).
bool Ship::CanFire(const Outfit *outfit)
{
	if(!outfit || !outfit->IsWeapon())
		return false;
	
	if(outfit->Ammo())
	{
		auto it = outfits.find(outfit->Ammo());
		if(it == outfits.end() || it->second <= 0)
			return false;
	}
	
	if(energy < outfit->WeaponGet("firing energy"))
		return false;
	if(fuel < outfit->WeaponGet("firing fuel"))
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
	
	energy -= outfit->WeaponGet("firing energy");
	fuel -= outfit->WeaponGet("firing fuel");
	heat += outfit->WeaponGet("firing heat");
}



void Ship::CreateExplosion(list<Effect> &effects)
{
	if(sprite.IsEmpty() || !sprite.GetMask(0).IsLoaded() || explosionEffects.empty())
		return;
	
	// Bail out if this loops enough times, just in case.
	for(int i = 0; i < 10; ++i)
	{
		Point point((Random::Real() - .5) * .5 * sprite.Width(),
			(Random::Real() - .5) * .5 * sprite.Height());
		if(sprite.GetMask(0).Contains(point, Angle()))
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
			effects.back().Place(angle.Rotate(point) + position, velocity, angle);
			++explosionCount;
			return;
		}
	}
}
