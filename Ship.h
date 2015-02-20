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
#include <set>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Government;
class Planet;
class Projectile;
class Phrase;
class StellarObject;
class System;



// Class representing a ship (either a model for sale or an instance of it). A
// ship's information can be saved to a file, so that it can later be read back
// in exactly the same state. To simplify this code, several aspects of a ship
// have been separated into other classes: Controllable for the AI's access to
// a ship, CargoHold for everything the ship is carrying, and Armament for the
// weapons and weapon hardpoints on a ship. The same class is used for the
// player's ship as for all other ships, so their capabilities are exactly the
// same  within the limits of what the AI knows how to command them to do.
class Ship : public std::enable_shared_from_this<Ship> {
public:
	// Load data for a type of ship:
	void Load(const DataNode &node);
	// When loading a ship, some of the outfits it lists may not have been
	// loaded yet. So, wait until everything has been loaded, then call this.
	void FinishLoading();
	// Save a full description of this ship, as currently configured.
	void Save(DataWriter &out) const;
	
	// Get the name of this model of ship.
	const std::string &ModelName() const;
	// Get this ship's description.
	const std::string &Description() const;
	// Get this ship's cost.
	int Cost() const;
	// Get the licenses needed to buy or operate this ship.
	const std::vector<std::string> &Licenses(const Government *government) const;
	
	// When creating a new ship, you must set the following:
	void Place(Point position = Point(), Point velocity = Point(), Angle angle = Angle());
	void SetName(const std::string &name);
	void SetSystem(const System *system);
	void SetPlanet(const Planet *planet);
	void SetGovernment(const Government *government);
	void SetIsSpecial(bool special = true);
	
	const Personality &GetPersonality() const;
	void SetPersonality(const Personality &other);
	
	std::string GetHail() const;
	void SetHail(const Phrase *friendly, const Phrase *hostile);
	
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
	bool Fire(std::list<Projectile> &projectiles);
	// Fire an anti-missile. Returns true if the missile was killed.
	bool FireAntiMissile(const Projectile &projectile, std::list<Effect> &effects);
	
	// Get the system this ship is in.
	const System *GetSystem() const;
	// If the ship is landed, get the planet it has landed on.
	const Planet *GetPlanet() const;
	
	bool IsTargetable() const;
	bool IsOverheated() const;
	bool IsDisabled() const;
	bool IsLanding() const;
	bool IsEnteringHyperspace() const;
	bool IsHyperspacing() const;
	// Check if this ship is currently able to begin landing on its target.
	bool CanLand() const;
	// Check if this ship is currently able to enter hyperspace to it target.
	bool CanHyperspace() const;
	bool IsBoarding() const;
	// Get the degree to which this ship is cloaked. 1 means invisible and
	// impossible to hit or target; 0 means fully visible.
	double Cloaking() const;
	
	// Get information on this particular ship, for displaying it.
	const Animation &GetSprite() const;
	// Get the ship's government.
	const Government *GetGovernment() const;
	// Get this ship's zoom factor (due to landing on a planet).
	double Zoom() const;
	// Get the name of this particular ship.
	const std::string &Name() const;
	
	// Get the points from which engine flares should be drawn. If the ship is
	// not thrusting right now, this will be empty.
	const std::vector<Point> &EnginePoints() const;
	// Get the sprite to be used for an engine flare.
	const Animation &FlareSprite() const;
	
	// Get the position of this ship.
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	// Get the facing unit vector times the scale factor.
	Point Unit() const;
	
	// Recharge and repair this ship (e.g. because it has landed).
	void Recharge(bool atSpaceport = true);
	// Mark a ship as destroyed.
	void Destroy();
	// Check if this ship has been destroyed.
	bool IsDestroyed() const;
	
	// Get characteristics of this ship, as a fraction between 0 and 1.
	double Shields() const;
	double Hull() const;
	double Fuel() const;
	int JumpsRemaining() const;
	double Energy() const;
	double Heat() const;
	int Crew() const;
	int RequiredCrew() const;
	void AddCrew(int count);
	bool CanRefuel(const Ship &other) const;
	double TransferFuel(double amount, Ship *to);
	void WasCaptured(const std::shared_ptr<Ship> &capturer);
	// Check if this ship should be deleted.
	bool ShouldDelete() const;
	
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
	
	int FighterBaysFree() const;
	int DroneBaysFree() const;
	bool AddFighter(const std::shared_ptr<Ship> &ship);
	void UnloadFighters();
	bool IsFighter() const;
	bool HasBays() const;
	std::vector<std::shared_ptr<Ship>> CarriedShips() const;
	
	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	
	// Get outfit information.
	const std::map<const Outfit *, int> &Outfits() const;
	int OutfitCount(const Outfit *outfit) const;
	const Outfit &Attributes() const;
	// Get the attributes of this ship chassis before any outfits were added.
	const Outfit &BaseAttributes() const;
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
	void SetTargetShip(const std::weak_ptr<Ship> &ship);
	void SetShipToAssist(const std::weak_ptr<Ship> &ship);
	void SetTargetPlanet(const StellarObject *object);
	void SetTargetSystem(const System *system);
	void SetDestination(const Planet *planet);
	
	// Add escorts to this ship. Escorts look to the parent ship for movement
	// cues and try to stay with it when it lands or goes into hyperspace.
	void AddEscort(const std::weak_ptr<Ship> &ship);
	void SetParent(const std::weak_ptr<Ship> &ship);
	void RemoveEscort(const Ship *ship);
	void ClearEscorts();
	
	const std::vector<std::weak_ptr<Ship>> &GetEscorts() const;
	std::shared_ptr<Ship> GetParent() const;
	
	
private:
	bool CannotAct() const;
	double MinimumHull() const;
	void CreateExplosion(std::list<Effect> &effects);
	
	
private:
	class Bay {
	public:
		Bay() = default;
		Bay(double x, double y) : point(x * .5, y * .5) {}
		
		Point point;
		std::shared_ptr<Ship> ship;
	};
	
	
private:
	// Characteristics of the chassis:
	std::string modelName;
	std::string description;
	Animation sprite;
	// Characteristics of this particular ship:
	std::string name;
	const Government *government = nullptr;
	
	// Licenses needed to operate this ship.
	std::map<const Government *, std::vector<std::string>> licenses;
	
	int forget = 0;
	bool isInSystem = true;
	// "Special" ships cannot be forgotten, and if they land on a planet, they
	// continue to exist and refuel instead of being deleted.
	bool isSpecial = false;
	bool isOverheated = false;
	bool isDisabled = false;
	bool isBoarding = false;
	bool hasBoarded = false;
	double cloak = 0.;
	
	Command commands;
	
	Personality personality;
	const Phrase *hail[2] = {nullptr, nullptr};
	
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
	std::vector<std::weak_ptr<Ship>> escorts;
	std::weak_ptr<Ship> parent;
};



#endif
