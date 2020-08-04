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

#include "Body.h"

#include "Angle.h"
#include "Armament.h"
#include "CargoHold.h"
#include "Command.h"
#include "Outfit.h"
#include "Personality.h"
#include "Point.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Effect;
class Flotsam;
class Government;
class Minable;
class Phrase;
class Planet;
class PlayerInfo;
class Projectile;
class StellarObject;
class System;
class Visual;



// Class representing a ship (either a model for sale or an instance of it). A
// ship's information can be saved to a file, so that it can later be read back
// in exactly the same state. The same class is used for the player's ship as
// for all other ships, so their capabilities are exactly the same  within the
// limits of what the AI knows how to command them to do.
class Ship : public Body, public std::enable_shared_from_this<Ship> {
public:
	// These are all the possible category strings for ships.
	static const std::vector<std::string> CATEGORIES;
	// Allow retrieving the available bay types for the current game;
	static const std::set<std::string> BAY_TYPES;
	
	class Bay {
	public:
		Bay(double x, double y, std::string category) : point(x * .5, y * .5), category(category) {}
		// Copying a bay does not copy the ship inside it.
		Bay(const Bay &b) : point(b.point), category(b.category), side(b.side), facing(b.facing), launchEffects(b.launchEffects) {}
		
		Point point;
		std::shared_ptr<Ship> ship;
		std::string category;
		
		uint8_t side = 0;
		static const uint8_t INSIDE = 0;
		static const uint8_t OVER = 1;
		static const uint8_t UNDER = 2;
		
		// The angle at which the carried ship will depart, relative to the carrying ship.
		Angle facing;
		
		// The launch effect(s) to be simultaneously played when the bay's ship launches.
		std::vector<const Effect *> launchEffects;
	};
	
	class EnginePoint : public Point {
	public:
		EnginePoint(double x, double y, double zoom) : Point(x, y), zoom(zoom) {}
		
		uint8_t side = 0;
		static const uint8_t UNDER = 0;
		static const uint8_t OVER = 1;
		
		uint8_t steering = 0;
		static const uint8_t NONE = 0;
		static const uint8_t LEFT = 1;
		static const uint8_t RIGHT = 2;
		
		double zoom;
		Angle facing;
	};
	
	
public:
	/* Functions provided by the Body base class:
	bool HasSprite() const;
	const Sprite *GetSprite() const;
	int Width() const;
	int Height() const;
	int GetSwizzle() const;
	Frame GetFrame(int step = -1) const;
	const Mask &GetMask(int step = -1) const;
	const Point &Position() const;
	const Point &Velocity() const;
	const Angle &Facing() const;
	Point Unit() const;
	double Zoom() const;
	const Government *GetGovernment() const;
	*/

	Ship() = default;
	// Construct and Load() at the same time.
	Ship(const DataNode &node);
	
	// Load data for a type of ship:
	void Load(const DataNode &node);
	// When loading a ship, some of the outfits it lists may not have been
	// loaded yet. So, wait until everything has been loaded, then call this.
	void FinishLoading(bool isNewInstance);
	// Save a full description of this ship, as currently configured.
	void Save(DataWriter &out) const;
	
	// Get the name of this particular ship.
	const std::string &Name() const;
	
	// Get the name of this model of ship.
	const std::string &ModelName() const;
	const std::string &PluralModelName() const;
	// Get the generic noun (e.g. "ship") to be used when describing this ship.
	const std::string &Noun() const;
	// Get this ship's description.
	const std::string &Description() const;
	// Get the shipyard thumbnail for this ship.
	const Sprite *Thumbnail() const;
	// Get this ship's cost.
	int64_t Cost() const;
	int64_t ChassisCost() const;

	// Check if this ship is configured in such a way that it would be difficult
	// or impossible to fly.
	std::vector<std::string> FlightCheck() const;
	
	void SetPosition(Point position);
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
	// The player can selectively deploy their carried ships, rather than just all / none.
	void SetDeployOrder(bool shouldDeploy = true);
	bool HasDeployOrder() const;
	
	// Access the ship's personality, which affects how the AI behaves.
	const Personality &GetPersonality() const;
	void SetPersonality(const Personality &other);
	// Get a random hail message, or set the object used to generate them. If no
	// object is given the government's default will be used.
	void SetHail(const Phrase &phrase);
	std::string GetHail(const PlayerInfo &player) const;
	
	// Set the commands for this ship to follow this timestep.
	void SetCommands(const Command &command);
	const Command &Commands() const;
	// Move this ship. A ship may create effects as it moves, in particular if
	// it is in the process of blowing up.
	void Move(std::vector<Visual> &visuals, std::list<std::shared_ptr<Flotsam>> &flotsam);
	// Generate energy, heat, etc. (This is called by Move().)
	void DoGeneration();
	// Launch any ships that are ready to launch.
	void Launch(std::list<std::shared_ptr<Ship>> &ships, std::vector<Visual> &visuals);
	// Check if this ship is boarding another ship. If it is, it either plunders
	// it or, if this is a player ship, returns the ship it is plundering so a
	// plunder dialog can be displayed.
	std::shared_ptr<Ship> Board(bool autoPlunder = true);
	// Scan the target, if able and commanded to. Return a ShipEvent bitmask
	// giving the types of scan that succeeded.
	int Scan();
	// Find out what fraction of the scan is complete.
	double CargoScanFraction() const;
	double OutfitScanFraction() const;
	
	// Fire any weapons that are ready to fire. If an anti-missile is ready,
	// instead of firing here this function returns true and it can be fired if
	// collision detection finds a missile in range.
	bool Fire(std::vector<Projectile> &projectiles, std::vector<Visual> &visuals);
	// Fire an anti-missile. Returns true if the missile was killed.
	bool FireAntiMissile(const Projectile &projectile, std::vector<Visual> &visuals);
	
	// Get the system this ship is in.
	const System *GetSystem() const;
	// If the ship is landed, get the planet it has landed on.
	const Planet *GetPlanet() const;
	
	// Check the status of this ship.
	bool IsCapturable() const;
	bool IsTargetable() const;
	bool IsOverheated() const;
	bool IsDisabled() const;
	bool IsBoarding() const;
	bool IsLanding() const;
	// Check if this ship is currently able to begin landing on its target.
	bool CanLand() const;
	// Check if some condition is keeping this ship from acting. (That is, it is
	// landing, hyperspacing, cloaking, disabled, or under-crewed.)
	bool CannotAct() const;
	// Get the degree to which this ship is cloaked. 1 means invisible and
	// impossible to hit or target; 0 means fully visible.
	double Cloaking() const;
	// Check if this ship is entering (rather than leaving) hyperspace.
	bool IsEnteringHyperspace() const;
	// Check if this ship is entering or leaving hyperspace.
	bool IsHyperspacing() const;
	// Check if this ship is hyperspacing, specifically via a jump drive.
	bool IsUsingJumpDrive() const;
	// Check if this ship is currently able to enter hyperspace to it target.
	bool IsReadyToJump(bool waitingIsReady = false) const;
	// Get this ship's custom swizzle.
	int CustomSwizzle() const;
	
	// Check if the ship is thrusting. If so, the engine sound should be played.
	bool IsThrusting() const;
	bool IsReversing() const;
	bool IsSteering() const;
	// The direction that the ship is steering. If positive, the ship is steering right. 
	// If negative, the ship is steering left.
	double SteeringDirection() const;
	// Get the points from which engine flares should be drawn.
	const std::vector<EnginePoint> &EnginePoints() const;
	const std::vector<EnginePoint> &ReverseEnginePoints() const;
	const std::vector<EnginePoint> &SteeringEnginePoints() const;
	
	// Make a ship disabled or destroyed, or bring back a destroyed ship.
	void Disable();
	void Destroy();
	void SelfDestruct();
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
	double Fuel() const;
	double Energy() const;
	// A ship's heat is generally between 0 and 1, but if it receives
	// heat damage the value can increase above 1.
	double Heat() const;
	// Get the ship's "health," where <=0 is disabled and 1 means full health.
	double Health() const;
	// Get the hull fraction at which this ship is disabled.
	double DisabledHull() const;
	// Get the number of jumps this ship can make before running out of fuel.
	// This depends on how much fuel it has and what sort of hyperdrive it uses.
	// If followParent is false, this ship will not follow the parent.
	int JumpsRemaining(bool followParent = true) const;
	// Get the amount of fuel expended per jump.
	double JumpFuel(const System *destination = nullptr) const;
	// Get the cost of making a jump of the given type (if possible).
	double HyperdriveFuel() const;
	double JumpDriveFuel() const;
	// Get the amount of fuel missing for the next jump (smart refuelling)
	double JumpFuelMissing() const;
	// Get the heat level at idle.
	double IdleHeat() const;
	// Get the heat dissipation, in heat units per heat unit per frame.
	double HeatDissipation() const;
	// Get the maximum heat level, in heat units (not temperature).
	double MaximumHeat() const;
	// Calculate the multiplier for cooling efficiency.
	double CoolingEfficiency() const;
	
	// Access how many crew members this ship has or needs.
	int Crew() const;
	int RequiredCrew() const;
	void AddCrew(int count);
	// Check if this is a ship that can be used as a flagship.
	bool CanBeFlagship() const;
	
	// Get this ship's movement characteristics.
	double Mass() const;
	double TurnRate() const;
	double Acceleration() const;
	double MaxVelocity() const;
	double MaxReverseVelocity() const;
	
	// This ship just got hit by the given projectile. Take damage according to
	// what sort of weapon the projectile it. The return value is a ShipEvent
	// type, which may be a combination of PROVOKED, DISABLED, and DESTROYED.
	// If isBlast, this ship was caught in the blast radius of a weapon but was
	// not necessarily its primary target.
	// Blast damage is dependent on the distance to the damage source.
	int TakeDamage(const Projectile &projectile, bool isBlast = false);
	// Apply a force to this ship, accelerating it. This might be from a weapon
	// impact, or from firing a weapon, for example.
	void ApplyForce(const Point &force);
	
	// Check if this ship has bays to carry other ships.
	bool HasBays() const;
	// Check how many bays are not occupied at present. This does not check
	// whether one of your escorts plans to use that bay.
	int BaysFree(const std::string &category) const;
	// Check how many bays this ship has of a given category.
	int BaysTotal(const std::string &category) const;
	// Check if this ship has a bay free for the given other ship, and the
	// bay is not reserved for one of its existing escorts.
	bool CanCarry(const Ship &ship) const;
	// Check if this is a ship of a type that can be carried.
	bool CanBeCarried() const;
	// Move the given ship into one of the bays, if possible.
	bool Carry(const std::shared_ptr<Ship> &ship);
	// Empty the bays. If the carried ships are not special ships that are
	// saved in the player data, they will be deleted. Otherwise, they become
	// visible as ships landed on the same planet as their parent.
	void UnloadBays();
	// Get a list of any ships this ship is carrying.
	const std::vector<Bay> &Bays() const;
	// Adjust the positions and velocities of any visible carried fighters or
	// drones. If any are visible, return true.
	bool PositionFighters() const;
	
	// Get cargo information.
	CargoHold &Cargo();
	const CargoHold &Cargo() const;
	// Display box effects from jettisoning this much cargo.
	void Jettison(const std::string &commodity, int tons);
	void Jettison(const Outfit *outfit, int count);
	
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
	// Determine if the ship has an outfit with the specified type and optionally subtype
	bool HasOutfit(const std::string &type, const std::string &subtype) const;
	
	// Get the list of weapons.
	Armament &GetArmament();
	const std::vector<Hardpoint> &Weapons() const;
	// Check if we are able to fire the given weapon (i.e. there is enough
	// energy, ammo, and fuel to fire it).
	bool CanFire(const Weapon *weapon) const;
	// Fire the given weapon (i.e. deduct whatever energy, ammo, or fuel it uses
	// and add whatever heat it generates. Assume that CanFire() is true.
	void ExpendAmmo(const Weapon *weapon);
	
	// Each ship can have a target system (to travel to), a target planet (to
	// land on) and a target ship (to move to, and attack if hostile).
	std::shared_ptr<Ship> GetTargetShip() const;
	std::shared_ptr<Ship> GetShipToAssist() const;
	const StellarObject *GetTargetStellar() const;
	const System *GetTargetSystem() const;
	// Mining target.
	std::shared_ptr<Minable> GetTargetAsteroid() const;
	std::shared_ptr<Flotsam> GetTargetFlotsam() const;
	
	// Set this ship's targets.
	void SetTargetShip(const std::shared_ptr<Ship> &ship);
	void SetShipToAssist(const std::shared_ptr<Ship> &ship);
	void SetTargetStellar(const StellarObject *object);
	void SetTargetSystem(const System *system);
	// Mining target.
	void SetTargetAsteroid(const std::shared_ptr<Minable> &asteroid);
	void SetTargetFlotsam(const std::shared_ptr<Flotsam> &flotsam);
	
	// Manage escorts. When you set this ship's parent, it will automatically
	// register itself as an escort of that ship, and unregister itself from any
	// previous parent it had.
	void SetParent(const std::shared_ptr<Ship> &ship);
	std::shared_ptr<Ship> GetParent() const;
	const std::vector<std::weak_ptr<Ship>> &GetEscorts() const;
	
	
private:
	// Add or remove a ship from this ship's list of escorts.
	void AddEscort(Ship &ship);
	void RemoveEscort(const Ship &ship);
	// Get the hull amount at which this ship is disabled.
	double MinimumHull() const;
	// Find out how much fuel is consumed by the hyperdrive of the given type.
	double BestFuel(const std::string &type, const std::string &subtype, double defaultFuel) const;
	// Create one of this ship's explosions, within its mask. The explosions can
	// either stay over the ship, or spread out if this is the final explosion.
	void CreateExplosion(std::vector<Visual> &visuals, bool spread = false);
	// Place a "spark" effect, like ionization or disruption.
	void CreateSparks(std::vector<Visual> &visuals, const std::string &name, double amount);
	void CreateSparks(std::vector<Visual> &visuals, const Effect *effect, double amount);
	
	
private:
	/* Protected member variables of the Body class:
	Point position;
	Point velocity;
	Angle angle;
	double zoom;
	int swizzle;
	const Government *government;
	*/
	
	// Characteristics of the chassis:
	const Ship *base = nullptr;
	std::string modelName;
	std::string pluralModelName;
	std::string noun;
	std::string description;
	const Sprite *thumbnail = nullptr;
	// Characteristics of this particular ship:
	std::string name;
	bool canBeCarried = false;
	
	int forget = 0;
	bool isInSystem = true;
	// "Special" ships cannot be forgotten, and if they land on a planet, they
	// continue to exist and refuel instead of being deleted.
	bool isSpecial = false;
	bool isYours = false;
	bool isParked = false;
	bool shouldDeploy = false;
	bool isOverheated = false;
	bool isDisabled = false;
	bool isBoarding = false;
	bool hasBoarded = false;
	bool isThrusting = false;
	bool isReversing = false;
	bool isSteering = false;
	double steeringDirection = 0.;
	bool neverDisabled = false;
	bool isCapturable = true;
	bool isInvisible = false;
	int customSwizzle = -1;
	double cloak = 0.;
	double cloakDisruption = 0.;
	// Cached values for figuring out when anti-missile is in range.
	double antiMissileRange = 0.;
	double weaponRadius = 0.;
	// Cargo and outfit scanning takes time.
	double cargoScan = 0.;
	double outfitScan = 0.;
	
	Command commands;
	
	Personality personality;
	const Phrase *hail = nullptr;
	
	// Installed outfits, cargo, etc.:
	Outfit attributes;
	Outfit baseAttributes;
	bool addAttributes = false;
	const Outfit *explosionWeapon = nullptr;
	std::map<const Outfit *, int> outfits;
	CargoHold cargo;
	std::list<std::shared_ptr<Flotsam>> jettisoned;
	
	std::vector<Bay> bays;
	// Cache the mass of carried ships to avoid repeatedly recomputing it.
	double carriedMass = 0.;
	
	std::vector<EnginePoint> enginePoints;
	std::vector<EnginePoint> reverseEnginePoints;
	std::vector<EnginePoint> steeringEnginePoints;
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
	double ionization = 0.;
	double disruption = 0.;
	double slowness = 0.;
	// Acceleration can be created by engines, firing weapons, or weapon impacts.
	Point acceleration;
	
	int crew = 0;
	int pilotError = 0;
	int pilotOkay = 0;
	
	// Current status of this particular ship:
	const System *currentSystem = nullptr;
	// A Ship can be locked into one of three special states: landing,
	// hyperspacing, and exploding. Each one must track some special counters:
	const Planet *landingPlanet = nullptr;
	
	int hyperspaceCount = 0;
	const System *hyperspaceSystem = nullptr;
	bool isUsingJumpDrive = false;
	double hyperspaceFuelCost = 0.;
	Point hyperspaceOffset;
	
	// The hull may spring a "leak" (venting atmosphere, flames, blood, etc.)
	// when the ship is dying.
	class Leak {
	public:
		Leak(const Effect *effect = nullptr) : effect(effect) {}
		
		const Effect *effect = nullptr;
		Point location;
		Angle angle;
		int openPeriod = 60;
		int closePeriod = 60;
	};
	std::vector<Leak> leaks;
	std::vector<Leak> activeLeaks;
	
	// Explosions that happen when the ship is dying:
	std::map<const Effect *, int> explosionEffects;
	unsigned explosionRate = 0;
	unsigned explosionCount = 0;
	unsigned explosionTotal = 0;
	std::map<const Effect *, int> finalExplosions;
	
	// Target ships, planets, systems, etc.
	std::weak_ptr<Ship> targetShip;
	std::weak_ptr<Ship> shipToAssist;
	const StellarObject *targetPlanet = nullptr;
	const System *targetSystem = nullptr;
	std::weak_ptr<Minable> targetAsteroid;
	std::weak_ptr<Flotsam> targetFlotsam;
	
	// Links between escorts and parents.
	std::vector<std::weak_ptr<Ship>> escorts;
	std::weak_ptr<Ship> parent;
};



#endif
