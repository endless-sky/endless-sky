/* Ship.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIP_H_
#define SHIP_H_

#include "Angle.h"
#include "Animation.h"
#include "Armament.h"
#include "CargoHold.h"
#include "Command.h"
#include "Outfit.h"
#include "Personality.h"
#include "Point.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Government;
class Phrase;
class Planet;
class Projectile;
class StellarObject;
class System;



// Class representing a ship (either a model for sale or an instance of it). A
// ship's information can be saved to a file, so that it can later be read back
// in exactly the same state. The same class is used for the player's ship as
// for all other ships, so their capabilities are exactly the same  within the
// limits of what the AI knows how to command them to do.
class Ship : public std::enable_shared_from_this<Ship> {
public:
	// Load data for a type of ship:
	void Load(const DataNode &node);
	// When loading a ship, some of the outfits it lists may not have been
	// loaded yet. So, wait until everything has been loaded, then call this.
	void FinishLoading();
	// Save a full description of this ship, as currently configured.
	void Save(DataWriter &out) const;
	
	// Get information on this particular ship, for displaying it.
	const Animation &GetSprite() const;
	// Get the ship's government.
	const Government *GetGovernment() const;
	// Get this ship's zoom factor (due to landing on a planet).
	double Zoom() const;
	// Get the name of this particular ship.
	const std::string &Name() const;
	
	// Get the name of this model of ship.
	const std::string &ModelName() const;
	// Get this ship's description.
	const std::string &Description() const;
	// Get this ship's cost.
	int64_t Cost() const;
	// Get the licenses needed to buy or operate this ship.
	const std::vector<std::string> &Licenses() const;
	
	// When creating a new ship, you must set the following:
	void Place(Point position = Point(), Point velocity = Point(), Angle angle = Angle());
	void SetName(const std::string &name);
	void SetSystem(const System *system);
	void SetPlanet(const Planet *planet);
	void SetGovernment(const Government *government);
	void SetIsSpecial(bool special = true);
	bool IsSpecial() const;
	
	// If a ship belongs to the player, the player can give it commands.
	void SetIsYours(bool yours = true);
	bool IsYours() const;
	// A parked ship stays on a planet and requires no daily salary payments.
	void SetIsParked(bool parked = true);
	bool IsParked() const;
	
	// Access the ship's personality, which affects how the AI behaves.
	const Personality &GetPersonality() const;
	void SetPersonality(const Personality &other);
	// Get a random hail message, or set the object used to generate them. If no
	// object is given the government's default will be used.
	void SetHail(const Phrase &phrase);
	std::string GetHail() const;
	
	// Set the commands for this ship to follow this timestep.
	void SetCommands(const Command &command);
	const Command &Commands() const;
	// Move this ship. A ship may create effects as it moves, in particular if
	// it is in the process of blowing up. If this returns false, the ship
	// should be deleted.
	bool Move(std::list<Effect> &effects);
	// Launch any ships that are ready to launch.
	void Launch(std::list<std::shared_ptr<Ship>> &ships);
	// Check if this ship is boarding another ship. If it is, it either plunders
	// it or, if this is a player ship, returns the ship it is plundering so a
	// plunder dialog can be displayed.
	std::shared_ptr<Ship> Board(bool autoPlunder = true);
	// Scan the target, if able and commanded to. Return a ShipEvent bitmask
	// giving the types of scan that succeeded.
	int Scan() const;
	
	// Fire any weapons that are ready to fire. If an anti-missile is ready,
	// instead of firing here this function returns true and it can be fired if
	// collision detection finds a missile in range.
	bool Fire(std::list<Projectile> &projectiles, std::list<Effect> &effects);
	// Fire an anti-missile. Returns true if the missile was killed.
	bool FireAntiMissile(const Projectile &projectile, std::list<Effect> &effects);
	
	// Get the system this ship is in.
	const System *GetSystem() const;
	// If the ship is landed, get the planet it has landed on.
	const Planet *GetPlanet() const;
	
	// Check the status of this ship.
	bool IsTargetable() const;
	bool IsOverheated() const;
	bool IsDisabled() const;
	bool IsBoarding() const;
	bool IsLanding() const;
	// Check if this ship is currently able to begin landing on its target.
	bool CanLand() const;
	// Get the degree to which this ship is cloaked. 1 means invisible and
	// impossible to hit or target; 0 means fully visible.
	double Cloaking() const;
	// Check if this ship is entering (rather than leaving) hyperspace.
	bool IsEnteringHyperspace() const;
	// Check if this ship is entering or leaving hyperspace.
	bool IsHyperspacing() const;
	// Check if this ship is currently able to enter hyperspace to it target.
	int CheckHyperspace() const;
	// Check what type of hyperspce jump this ship is making (0 = not allowed,
	// 100 = hyperdrive, 150 = scram drive, 200 = jump drive).
	int HyperspaceType() const;
	
	// Check if the ship is thrusting. If so, the engine sound should be played.
	bool IsThrusting() const;
	// Get the points from which engine flares should be drawn.
	const std::vector<Point> &EnginePoints() const;
	
	// Get the position, velocity, and heading of this ship.
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	// Get the facing unit vector times the zoom factor.
	Point Unit() const;
	
	// Mark a ship as destroyed, or bring back a destroyed ship.
	void Destroy();
	void Restore();
	// Check if this ship has been destroyed.
	bool IsDestroyed() const;
	// Recharge and repair this ship (e.g. because it has landed).
	void Recharge(bool atSpaceport = true);
	// Check if this ship is able to give the given ship enough fuel to jump.
	bool CanRefuel(const Ship &other) const;
	// Give the other ship enough fuel for it to jump.
	double TransferFuel(double amount, Ship *to);
	// Mark this ship as property of the given ship.
	void WasCaptured(const std::shared_ptr<Ship> &capturer);
	
	// Get characteristics of this ship, as a fraction between 0 and 1.
	double Shields() const;
	double Hull() const;
	double Energy() const;
	double Heat() const;
	double Fuel() const;
	// Get the number of jumps this ship can make before running out of fuel.
	// This depends on how much fuel it has and what sort of hyperdrive it uses.
	int JumpsRemaining() const;
	// Get the amount of fuel expended per jump.
	double JumpFuel() const;
	
	// Access how many crew members this ship has or needs.
	int Crew() const;
	int RequiredCrew() const;
	void AddCrew(int count);
	
	// Get this ship's movement characteristics.
	double Mass() const;
	double TurnRate() const;
	double Acceleration() const;
	double MaxVelocity() const;
	
	// This ship just got hit by the given projectile. Take damage according to
	// what sort of weapon the projectile it. The return value is a ShipEvent
	// type, which may be a combination of PROVOKED, DISABLED, and DESTROYED.
	// If isBlast, this ship was caught in the blast radius of a weapon but was
	// not necessarily its primary target.
	int TakeDamage(const Projectile &projectile, bool isBlast = false);
	// Apply a force to this ship, accelerating it. This might be from a weapon
	// impact, or from firing a weapon, for example.
	void ApplyForce(const Point &force);
	
	// Check if this ship has fighter or drone bays.
	bool HasBays() const;
	// Check how many fighter and drone bays are not occupied at present. This
	// does not check whether one of your escorts plans to use that bay.
	int FighterBaysFree() const;
	int DroneBaysFree() const;
	// Check if this ship has a bay free for the given fighter, and the bay is
	// not reserved for one of its existing escorts.
	bool CanHoldFighter(const Ship &ship) const;
	// Check if this is a ship of a type that can be carried (fighter or drone).
	bool CanBeCarried() const;
	// Move the given ship into one of the fighter or drone bays, if possible.
	bool AddFighter(const std::shared_ptr<Ship> &ship);
	// Empty the fighter bays. If the fighters are not special ships that are
	// saved in the player data, they will be deleted. Otherwise, they become
	// visible as ships landed on the same planet as their parent.
	void UnloadFighters();
	// Get a list of any ships this ship is carrying.
	std::vector<std::shared_ptr<Ship>> CarriedShips() const;
	
	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	// Display box effects from jettisoning this much cargo.
	void Jettison(int tons);
	
	// Get the current attributes of this ship.
	const Outfit &Attributes() const;
	// Get the attributes of this ship chassis before any outfits were added.
	const Outfit &BaseAttributes() const;
	// Get the list of all outfits installed in this ship.
	const std::map<const Outfit *, int> &Outfits() const;
	// Find out how many outfits of the given type this ship contains.
	int OutfitCount(const Outfit *outfit) const;
	// Add or remove outfits. (To remove, pass a negative number.)
	void AddOutfit(const Outfit *outfit, int count);
	
	// Get the list of weapons.
	Armament &GetArmament();
	const std::vector<Armament::Weapon> &Weapons() const;
	// Check if we are able to fire the given weapon (i.e. there is enough
	// energy, ammo, and fuel to fire it).
	bool CanFire(const Outfit *outfit) const;
	// Fire the given weapon (i.e. deduct whatever energy, ammo, or fuel it uses
	// and add whatever heat it generates. Assume that CanFire() is true.
	void ExpendAmmo(const Outfit *outfit);
	
	// Each ship can have a target system (to travel to), a target planet (to
	// land on) and a target ship (to move to, and attack if hostile).
	std::shared_ptr<Ship> GetTargetShip() const;
	std::shared_ptr<Ship> GetShipToAssist() const;
	const StellarObject *GetTargetPlanet() const;
	const System *GetTargetSystem() const;
	const Planet *GetDestination() const;
	
	// Set this ship's targets.
	void SetTargetShip(const std::shared_ptr<Ship> &ship);
	void SetShipToAssist(const std::shared_ptr<Ship> &ship);
	void SetTargetPlanet(const StellarObject *object);
	void SetTargetSystem(const System *system);
	void SetDestination(const Planet *planet);
	
	// Manage escorts. When you set this ship's parent, it will automatically
	// register itself as an escort of that ship, and unregister itself from any
	// previous parent it had.
	void SetParent(const std::shared_ptr<Ship> &ship);
	std::shared_ptr<Ship> GetParent() const;
	const std::vector<std::weak_ptr<const Ship>> &GetEscorts() const;
	
	
private:
	// Add or remove a ship from this ship's list of escorts.
	void AddEscort(const Ship &ship);
	void RemoveEscort(const Ship &ship);
	// Check if some condition is keeping this ship from acting. (That is, it is
	// landing, hyperspacing, cloaking, disabled, or under-crewed.)
	bool CannotAct() const;
	// Get the hull amount at which this ship is disabled.
	double MinimumHull() const;
	// Create one of this ship's explosions, within its mask. The explosions can
	// either stay over the ship, or spread out if this is the final explosion.
	void CreateExplosion(std::list<Effect> &effects, bool spread = false);
	
	
private:
	class Bay {
	public:
		Bay() = default;
		Bay(double x, double y) : point(x * .5, y * .5) {}
		Bay(const Point &point) : point(point) {}
		
		Point point;
		std::shared_ptr<Ship> ship;
	};
	
	
private:
	// Characteristics of the chassis:
	const Ship *base = nullptr;
	std::string modelName;
	std::string description;
	Animation sprite;
	// Characteristics of this particular ship:
	std::string name;
	const Government *government = nullptr;
	
	// Licenses needed to operate this ship.
	std::vector<std::string> licenses;
	
	int forget = 0;
	bool isInSystem = true;
	// "Special" ships cannot be forgotten, and if they land on a planet, they
	// continue to exist and refuel instead of being deleted.
	bool isSpecial = false;
	bool isYours = false;
	bool isParked = false;
	bool isOverheated = false;
	bool isDisabled = false;
	bool isBoarding = false;
	bool hasBoarded = false;
	bool neverDisabled = false;
	double cloak = 0.;
	int jettisoned = 0;
	
	Command commands;
	
	Personality personality;
	const Phrase *hail = nullptr;
	
	// Installed outfits, cargo, etc.:
	Outfit attributes;
	Outfit baseAttributes;
	const Outfit *explosionWeapon = nullptr;
	std::map<const Outfit *, int> outfits;
	CargoHold cargo;
	
	std::vector<Bay> droneBays;
	std::vector<Bay> fighterBays;
	
	std::vector<Point> enginePoints;
	Armament armament;
	// While loading, keep track of which outfits already have been equipped.
	// (That is, they were specified as linked to a given gun or turret point.)
	std::map<const Outfit *, int> equipped;
	
	// Various energy levels:
	double shields = 0.;
	double hull = 0.;
	double fuel = 0.;
	double energy = 0.;
	double heat = 0.;
	double heatDissipation = .999;
	double ionization = 0.;
	
	int crew = 0;
	int pilotError = 0;
	int pilotOkay = 0;
	
	// Current status of this particular ship:
	const System *currentSystem = nullptr;
	Point position;
	Point velocity;
	Angle angle;
	
	// A Ship can be locked into one of three special states: landing,
	// hyperspacing, and exploding. Each one must track some special counters:
	double zoom = 1.;
	const Planet *landingPlanet = nullptr;
	
	int hyperspaceCount = 0;
	const System *hyperspaceSystem = nullptr;
	int hyperspaceType = 0;
	Point hyperspaceOffset;
	
	std::map<const Effect *, int> explosionEffects;
	unsigned explosionRate = 0;
	unsigned explosionCount = 0;
	unsigned explosionTotal = 0;
	
	// Target ships, planets, systems, etc.
	std::weak_ptr<Ship> targetShip;
	std::weak_ptr<Ship> shipToAssist;
	const StellarObject *targetPlanet = nullptr;
	const System *targetSystem = nullptr;
	const Planet *destination = nullptr;
	
	// Links between escorts and parents.
	std::vector<std::weak_ptr<const Ship>> escorts;
	std::weak_ptr<Ship> parent;
};



#endif
