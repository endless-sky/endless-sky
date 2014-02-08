/* Ship.cpp
Michael Zahniser, 28 Oct 2013

Function definitions for the Ship class.
*/

#include "Ship.h"

#include "Government.h"
#include "Mask.h"
#include "Projectile.h"
#include "SpriteSet.h"
#include "System.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace std;



Ship::Ship()
	: government(nullptr),
	forget(0), isSpecial(false), isOverheated(false), isDisabled(false),
	cargoMass(0),
	shields(0.), hull(0.), energy(0.), fuel(0.), heat(0.),
	currentSystem(nullptr),
	zoom(1.), landingPlanet(nullptr),
	hyperspaceCount(0), hyperspaceSystem(nullptr),
	explosionRate(0), explosionCount(0), explosionTotal(0)
{
}



void Ship::Load(const DataFile::Node &node, const Set<Outfit> &outfits, const Set<Effect> &effects)
{
	assert(node.Size() >= 2 && node.Token(0) == "ship");
	modelName = node.Token(1);
	
	// Note: I do not clear the attributes list here so that it is permissible
	// to override one ship definition with another.
	for(const DataFile::Node &child : node)
	{
		const string &token = child.Token(0);
		if(token == "sprite")
			sprite.Load(child);
		else if(token == "attributes")
			attributes.Load(child, outfits, effects);
		else if(token == "engine" && child.Size() >= 3)
			enginePoints.emplace_back(child.Value(1), child.Value(2));
		else if(token == "gun" && child.Size() >= 3)
		{
			gunPoints.emplace_back(child.Value(1), child.Value(2));
			attributes.Add("gun ports");
		}
		else if(token == "turret" && child.Size() >= 3)
		{
			turretPoints.emplace_back(child.Value(1), child.Value(2));
			attributes.Add("turret mounts");
		}
		else if(token == "explode" && child.Size() >= 2)
		{
			int count = (child.Size() >= 3) ? child.Value(2) : 1;
			explosionEffects[effects.Get(child.Token(1))] += count;
			explosionTotal += count;
		}
		else if(token == "outfits")
		{
			for(const DataFile::Node &grand : child)
			{
				int count = (grand.Size() >= 2) ? grand.Value(1) : 1;
				this->outfits[outfits.Get(grand.Token(0))] += count;
			}
		}
		else if(token == "description" && child.Size() >= 2)
		{
			description += child.Token(1);
			description += '\n';
		}
	}
}



// When loading a ship, some of the outfits it lists may not have been
// loaded yet. So, wait until everything has been loaded, then call this.
void Ship::FinishLoading()
{
	for(const auto &it : outfits)
		attributes.Add(*it.first, it.second);
	
	Recharge();
}



const string &Ship::ModelName() const
{
	return modelName;
}



// Get this ship's description.
const std::string &Ship::Description() const
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
	zoom = 1.;
	
	cargo.clear();
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



// Move this ship. A ship may create effects as it moves, in particular if
// it is in the process of blowing up. If this returns false, the ship
// should be deleted.
bool Ship::Move(std::list<Effect> &effects)
{
	// Check if this ship has been in a different system from the player for so
	// long that it should be "forgotten."
	if(!isSpecial && ++forget >= 1000)
		return false;
	
	
	// When ships recharge, what actually happens is that they can exceed their
	// maximum capacity for the rest of the turn, but must be clamped to the
	// maximum here before they gain more. This is so that, for example, a ship
	// with no batteries but a good generator can still move.
	fuel = min(fuel, attributes.Get("fuel capacity"));
	energy = min(energy, attributes.Get("energy capacity"));
	
	heat *= .999;
	if(heat > Mass() * 100.)
		isOverheated = true;
	
	shields = min(shields, attributes.Get("shields"));
	
	double maximumHull = attributes.Get("hull");
	hull = min(hull, maximumHull);
	// Check if the hull amount is low enough to disable this ship.
	double minimumHull = max(.10 * maximumHull, min(.50 * maximumHull, 100.));
	isDisabled = isOverheated || (hull < minimumHull);
	
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
		fuel += .03 * sqrt(attributes.Get("ramscoop"));
		energy += attributes.Get("energy generation");
		heat += attributes.Get("heat generation");
		shields += attributes.Get("shield generation");
		
		// Reload weapons.
		for(auto &it : outfits)
		{
			int count = it.second;
			if(!count || !it.first->IsWeapon())
				continue;
		
			Weapon &weapon = weapons[it.first];
			if(weapon.reload > 0)
				weapon.reload -= count;
		}
	}
	
	
	if(hull <= 0.)
	{
		// If the ship is dead, it first creates explosions at an increasing
		// rate, then disappears in one big explosion.
		++explosionRate;
		if(rand() % 1024 < explosionRate)
			CreateExplosion(effects);
		
		// Once we've created enough little explosions, die.
		if(explosionCount == explosionTotal)
		{
			for(int i = 0; i < explosionTotal; ++i)
				CreateExplosion(effects);
			return false;
		}
	}
	else if(hyperspaceSystem || hyperspaceCount)
	{
		fuel -= (hyperspaceSystem != nullptr);
		
		// Enter hyperspace.
		double maxVelocity = MaxVelocity();
		double acceleration = Acceleration();
		double turnRate = TurnRate();
		
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
				if(object.GetPlanet())
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
		if(velocity.Length() <= maxVelocity && !hyperspaceSystem)
			hyperspaceCount = 0;
		
		return true;
	}
	else if(landingPlanet || zoom < 1.)
	{
		if(landingPlanet)
		{
			zoom -= .02;
			if(zoom < 0.)
			{
				// If this is not a special ship, it ceases to exist when it
				// lands.
				if(!isSpecial)
					return false;
				
				// Special ships do not disappear forever when they land; they
				// just slowly refuel.
				landingPlanet = nullptr;
				zoom = 0.;
			}
		}
		else if(fuel == attributes.Get("fuel capacity"))
			zoom = min(1., zoom + .02);
		else
			fuel = min(fuel + 1., attributes.Get("fuel capacity"));
		
		// Move the ship at the velocity it had when it began landing, but
		// scaled based on how small it is now.
		position += velocity * zoom;
		
		return true;
	}
	if(HasLandCommand() && CanLand())
		landingPlanet = GetTargetPlanet();
	else if(HasHyperspaceCommand() && CanHyperspace())
		hyperspaceSystem = GetTargetSystem();
	
	// This ship is not landing or entering hyperspace. So, move it. If it is
	// disabled, all it can do is slow down to a stop.
	double mass = Mass();
	if(isDisabled)
		velocity *= 1. - attributes.Get("drag") / mass;
	else
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
	
	// And finally: move the ship!
	position += velocity;
	
	return true;
}



// Launch any ships that are ready to launch.
void Ship::Launch(std::list<std::shared_ptr<Ship>> &ships)
{
	// TODO: allow carrying and launching of ships.
}



// Fire any weapons that are ready to fire. If an anti-missile is ready,
// instead of firing here this function returns true and it can be fired if
// collision detection finds a missile in range.
bool Ship::Fire(std::list<Projectile> &projectiles)
{
	forget = 0;
	
	if(zoom != 1. || isDisabled)
		return false;
	
	bool hasAntiMissile = false;
	int gun = 0;
	int turret = 0;
	shared_ptr<const Ship> target = GetTargetShip().lock();
	for(auto &it : outfits)
	{
		int count = it.second;
		if(!count || !it.first->IsWeapon())
			continue;
		
		Weapon &weapon = weapons[it.first];
		bool isGun = it.first->Get("gun ports");
		bool isTurret = it.first->Get("turret mounts");
		while(weapon.reload <= 0)
		{
			if(it.first->WeaponGet("anti-missile"))
			{
				hasAntiMissile = true;
				break;
			}
			// TODO: match this to whatever fire command this particular
			// weapon is tied to.
			if(!HasFireCommand(0))
				break;
			
			int cluster = it.first->WeaponGet("missile strength") ? count : 1;
			
			// Find out how many projectiles we are able to fire.
			if(it.first->Ammo())
				cluster = min(cluster, outfits[it.first->Ammo()]);
			if(it.first->WeaponGet("firing energy"))
				cluster = min(cluster, static_cast<int>(
					energy / it.first->WeaponGet("firing energy")));
			if(it.first->WeaponGet("firing fuel"))
				cluster = min(cluster, static_cast<int>(
					fuel / it.first->WeaponGet("firing fuel")));
			
			if(!cluster)
				break;
			
			// Subtract whatever it cost to fire this projectile.
			if(it.first->Ammo())
			{
				outfits[it.first->Ammo()] -= cluster;
				attributes.Add(*it.first->Ammo(), -cluster);
			}
			energy -= it.first->WeaponGet("firing energy") * cluster;
			fuel -= it.first->WeaponGet("firing fuel") * cluster;
			heat += it.first->WeaponGet("firing heat") * cluster;
			weapon.reload += cluster * it.first->WeaponGet("reload");
			
			// Create the projectile(s).
			for(int i = 0; i < cluster; ++i)
			{
				Angle aim = angle;
				
				Point start = position;
				
				if(isGun)
				{
					int thisGun = gun + weapon.nextPort;
					if(thisGun < gunPoints.size())
					{
						start += angle.Rotate(gunPoints[thisGun]) * .5 * zoom;
						// Find the point of convergence of shots fired from
						// this gun. The shots should converge at distance d:
						double d = it.first->WeaponGet("range") * .9;
						// The angle is therefore:
						aim += Angle(asin(gunPoints[thisGun].X() * .5 / d) * (180. / M_PI));
					}
				}
				else if(isTurret)
				{
					int thisTurret = turret + weapon.nextPort;
					if(thisTurret < turretPoints.size())
					{
						start += angle.Rotate(turretPoints[thisTurret]) * .5 * zoom;
						
						if(target)
						{
							Point p = target->position - start;
							Point v = target->velocity - velocity;
							double vp = it.first->WeaponGet("velocity");
							
							// How many steps will it take this projectile
							// to intersect the target?
							// (p.x + v.x*t)^2 + (p.y + v.y*t)^2 = vp^2*t^2
							// p.x^2 + 2*p.x*v.x*t + v.x^2*t^2
							//    + p.y^2 + 2*p.y*v.y*t + v.y^2t^2
							//    - vp^2*t^2 = 0
							// (v.x^2 + v.y^2 - vp^2) * t^2
							//    + (2 * (p.x * v.x + p.y * v.y)) * t
							//    + (p.x^2 + p.y^2) = 0
							double a = v.Dot(v) - vp * vp;
							double b = 2. * p.Dot(v);
							double c = p.Dot(p);
							double discriminant = b * b - 4 * a * c;
							double steps = 0.;
							if(discriminant > 0.)
							{
								discriminant = sqrt(discriminant);
								
								// The solutions are b +- discriminant.
								// But it's not a solution if it's negative.
								double r1 = (-b + discriminant) / (2. * a);
								double r2 = (-b - discriminant) / (2. * a);
								if(r1 > 0. && r2 > 0.)
									steps = min(r1, r2);
								else if(r1 > 0. || r2 > 0.)
									steps = max(r1, r2);
								
								// If it is not possible to reach the
								// rendevous spot within this projectile's
								// lifetime, just fire straight at the
								// target.
								steps = min(steps, it.first->WeaponGet("lifetime"));
								// Figure out where the target will be after
								// the calculated time has elapsed.
								p += steps * v;
							}
							
							aim = Angle((180. / M_PI) * atan2(p.X(), -p.Y()));
						}
						else
						{
							// Find the point of convergence of shots fired from
							// this gun. The shots should converge at distance d:
							double d = it.first->WeaponGet("range") * .9;
							// The angle is therefore:
							aim += Angle((180. / M_PI) * asin(
								turretPoints[thisTurret].X() * .5 / d));
						}
					}
				}
				
				projectiles.emplace_back(*this, start, aim, it.first);
				double force = it.first->WeaponGet("firing force");
				if(force)
				{
					double currentMass = Mass();
					velocity -= aim.Unit() * (force / currentMass);
					velocity *= 1. - attributes.Get("drag") / currentMass;
				}
					
				if(++weapon.nextPort == count)
					weapon.nextPort = 0;
			}
		}
		if(isGun)
			gun += count;
		else if(isTurret)
			turret += count;
	}
	
	return hasAntiMissile;
}



// Fire an anti-missile.
bool Ship::FireAntiMissile(Projectile &projectile, std::list<Effect> &effects)
{
	int turret = 0;
	for(auto &it : outfits)
	{
		int count = it.second;
		if(!count || !it.first->IsWeapon() || !it.first->Get("turret mounts"))
			continue;
		
		Weapon &weapon = weapons[it.first];
		int strength = it.first->WeaponGet("anti-missile");
		double energyCost = it.first->WeaponGet("firing energy");
		if(strength && weapon.reload <= 0 && energy >= energyCost)
		{
			double range = it.first->WeaponGet("velocity");
			Point start = position;
			int thisTurret = turret + weapon.nextPort;
			if(++weapon.nextPort == count)
				weapon.nextPort = 0;
			
			if(thisTurret < turretPoints.size())
			{
				start += angle.Rotate(turretPoints[thisTurret]) * .5 * zoom;
				Point offset = projectile.Position() - start;
				if(offset.Length() <= range)
				{
					energy -= energyCost;
					weapon.reload += it.first->WeaponGet("reload");
					
					start += (.5 * range) * offset.Unit();
					Angle a = (180. / M_PI) * atan2(offset.X(), -offset.Y());
					for(const auto &eit : it.first->HitEffects())
						for(int i = 0; i < eit.second; ++i)
						{
							effects.push_back(*eit.first);
							effects.back().Place(start, velocity, a);
						}
					
					if(rand() % strength > rand() % projectile.MissileStrength())
						projectile.Kill();
					return true;
				}
			}
		}
		turret += count;
	}
	
	return false;
}



const System *Ship::GetSystem() const
{
	return currentSystem;
}



bool Ship::IsTargetable() const
{
	return (zoom == 1. && !explosionRate);
}



bool Ship::IsDisabled() const
{
	return isDisabled;
}



bool Ship::HasLanded() const
{
	// Special ships do not cease to exist when they land.
	return (zoom == 0.);
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
	if(!GetTargetPlanet())
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
	if(speed > .1)
		return false;
	
	Point direction = GetTargetSystem()->Position() - currentSystem->Position();
	
	// Figure out if we're within one turn step of facing this system.
	bool left = direction.Cross(angle.Unit()) < 0.;
	Angle turned = angle + TurnRate() * (left - !left);
	bool stillLeft = direction.Cross(turned.Unit()) < 0.;
	
	return (left != stillLeft);
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
	return maximum ? min(1., shields / maximum) : 1.;
}



double Ship::Hull() const
{
	double maximum = attributes.Get("hull");
	return maximum ? min(1., hull / maximum) : 1.;
}



double Ship::Fuel() const
{
	double maximum = attributes.Get("fuel capacity");
	return maximum ? min(1., fuel / maximum) : 1.;
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
	return 1;
}



// Check if this ship should be deleted.
bool Ship::ShouldDelete() const
{
	return (HasLanded() && !isSpecial) || (hull <= 0. && explosionCount >= explosionTotal);
}



double Ship::Mass() const
{
	return cargoMass + attributes.Get("mass");
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



void Ship::TakeDamage(double shieldDamage, double hullDamage, Point force) const
{
	if(shields > shieldDamage)
		shields -= shieldDamage;
	else
	{
		hull -= hullDamage * (1. - (shields / shieldDamage));
		shields = 0.;
	}
	
	if(force)
	{
		double currentMass = Mass();
		velocity += force / currentMass;
		velocity *= 1. - attributes.Get("drag") / currentMass;
	}
}



int Ship::Cargo(const string &type) const
{
	auto it = cargo.find(type);
	if(it == cargo.end())
		return 0;
	
	return it->second;
}



int Ship::FreeCargo() const
{
	int total = attributes.Get("cargo space");
	for(const auto &it : cargo)
		total -= it.second;
	
	return total;
}



int Ship::AddCargo(int tons, const string &type)
{
	int free = FreeCargo();
	if(tons > free)
		tons = free;
	
	int &value = cargo[type];
	value += tons;
	if(value < 0)
	{
		tons -= value;
		value = 0;
	}
	cargoMass += tons;
	return tons;
}



// Get outfit information.
const map<const Outfit *, int> &Ship::Outfits() const
{
	return outfits;
}



const Outfit &Ship::Attributes() const
{
	return attributes;
}



void Ship::CreateExplosion(std::list<Effect> &effects)
{
	if(sprite.IsEmpty() || !sprite.GetMask(0).IsLoaded() || explosionEffects.empty())
		return;
	
	// Bail out if this loops enough times, just in case.
	for(int i = 0; i < 10; ++i)
	{
		Point point((rand() % sprite.Width() - .5 * sprite.Width()) * .5,
			(rand() % sprite.Height() - .5 * sprite.Height()) * .5);
		if(sprite.GetMask(0).Contains(point, Angle()))
		{
			// Pick an explosion.
			int type = rand() % explosionTotal;
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



Ship::Weapon::Weapon()
	: reload(0), nextPort(0)
{
}
