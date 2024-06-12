/* Ship.h
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

#ifndef SHIP_H_
#define SHIP_H_

#include "Body.h"

#include "Angle.h"
#include "Armament.h"
#include "CargoHold.h"
#include "Command.h"
#include "EsUuid.h"
#include "FireCommand.h"
#include "Outfit.h"
#include "Personality.h"
#include "Point.h"
#include "Port.h"
#include "ship/ShipAICache.h"
#include "ShipJumpNavigation.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class DamageDealt;
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
	class Bay {
	public:
		Bay(double x, double y, std::string category) : point(x * .5, y * .5), category(std::move(category)) {}
		Bay(Bay &&) = default;
		Bay &operator=(Bay &&) = default;
		~Bay() = default;

		// Copying a bay does not copy the ship inside it.
		Bay(const Bay &b) : point(b.point), category(b.category), side(b.side),
			facing(b.facing), launchEffects(b.launchEffects) {}
		Bay &operator=(const Bay &b) { return *this = Bay(b); }

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
		Angle gimbal;
	};


public:
	// Functions provided by the Body base class:
	// bool HasSprite() const;
	// const Sprite *GetSprite() const;
	// int Width() const;
	// int Height() const;
	// int GetSwizzle() const;
	// Frame GetFrame(int step = -1) const;
	// const Mask &GetMask(int step = -1) const;
	// const Point &Position() const;
	// const Point &Velocity() const;
	// const Angle &Facing() const;
	// Point Unit() const;
	// double Zoom() const;
	// const Government *GetGovernment() const;

	Ship() = default;
	// Construct and Load() at the same time.
	Ship(const DataNode &node);

	// Load data for a type of ship:
	void Load(const DataNode &node);
	// When loading a ship, some of the outfits it lists may not have been
	// loaded yet. So, wait until everything has been loaded, then call this.
	void FinishLoading(bool isNewInstance);
	// Check that this ship model and all its outfits have been loaded.
	bool IsValid() const;
	// Save a full description of this ship, as currently configured.
	void Save(DataWriter &out) const;

	const EsUuid &UUID() const noexcept;
	// Explicitly set this ship's ID.
	void SetUUID(const EsUuid &id);

	// Get the name of this particular ship.
	const std::string &Name() const;

	// Set / Get the name of this model of ship.
	void SetTrueModelName(const std::string &model);
	const std::string &TrueModelName() const;
	const std::string &DisplayModelName() const;
	const std::string &PluralModelName() const;
	// Get the name of this ship as a variant.
	const std::string &VariantName() const;
	// Get the generic noun (e.g. "ship") to be used when describing this ship.
	const std::string &Noun() const;
	// Get this ship's description.
	const std::string &Description() const;
	// Get the shipyard thumbnail for this ship.
	const Sprite *Thumbnail() const;
	// Get this ship's cost.
	int64_t Cost() const;
	int64_t ChassisCost() const;
	int64_t Strength() const;
	// Get the attraction and deterrence of this ship, for pirate raids.
	// This is only useful for the player's ships.
	double Attraction() const;
	double Deterrence() const;

	// Check if this ship is configured in such a way that it would be difficult
	// or impossible to fly.
	std::vector<std::string> FlightCheck() const;

	void SetPosition(Point position);
	// When creating a new ship, you must set the following:
	void Place(Point position = Point(), Point velocity = Point(), Angle angle = Angle(), bool isDeparting = true);
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
	const Phrase *GetHailPhrase() const;
	void SetHailPhrase(const Phrase &phrase);
	std::string GetHail(std::map<std::string, std::string> &&subs) const;
	bool CanSendHail(const PlayerInfo &player, bool allowUntranslated = false) const;

	// Access the ship's AI cache, containing the range and expected AI behavior for this ship.
	ShipAICache &GetAICache();
	void UpdateCaches();

	// Set the commands for this ship to follow this timestep.
	void SetCommands(const Command &command);
	void SetCommands(const FireCommand &firingCommand);
	const Command &Commands() const;
	const FireCommand &FiringCommands() const noexcept;
	// Move this ship. A ship may create effects as it moves, in particular if
	// it is in the process of blowing up.
	void Move(std::vector<Visual> &visuals, std::list<std::shared_ptr<Flotsam>> &flotsam);

	// Launch any ships that are ready to launch.
	void Launch(std::list<std::shared_ptr<Ship>> &ships, std::vector<Visual> &visuals);
	// Check if this ship is boarding another ship. If it is, it either plunders
	// it or, if this is a player ship, returns the ship it is plundering so a
	// plunder dialog can be displayed.
	std::shared_ptr<Ship> Board(bool autoPlunder, bool nonDocking);
	// Scan the target, if able and commanded to. Return a ShipEvent bitmask
	// giving the types of scan that succeeded.
	int Scan(const PlayerInfo &player);
	// Find out what fraction of the scan is complete.
	double CargoScanFraction() const;
	double OutfitScanFraction() const;

	// Fire any primary or secondary weapons that are ready to fire. Determines
	// if any special weapons (e.g. anti-missile, tractor beam) are ready to fire.
	// The firing of special weapons is handled separately.
	void Fire(std::vector<Projectile> &projectiles, std::vector<Visual> &visuals);
	// Return true if any anti-missile or tractor beam systems are ready to fire.
	bool HasAntiMissile() const;
	bool HasTractorBeam() const;
	// Fire an anti-missile at the given missile. Returns true if the missile was killed.
	bool FireAntiMissile(const Projectile &projectile, std::vector<Visual> &visuals);
	// Fire tractor beams at the given flotsam. Returns a Point representing the net
	// pull on the flotsam from this ship's tractor beams.
	Point FireTractorBeam(const Flotsam &flotsam, std::vector<Visual> &visuals);

	// Get the system this ship is in. Set to nullptr if the ship is being carried.
	const System *GetSystem() const;
	// Get the system this ship is in. If being carried, gets the parent's system.
	const System *GetActualSystem() const;
	// If the ship is landed, get the planet it has landed on.
	const Planet *GetPlanet() const;

	// Check the status of this ship.
	bool IsCapturable() const;
	bool IsTargetable() const;
	bool IsOverheated() const;
	bool IsDisabled() const;
	bool IsBoarding() const;
	bool IsLanding() const;
	bool IsFleeing() const;
	// Check if this ship is currently able to begin landing on its target.
	bool CanLand() const;
	// What kind of action this is we are trying to do.
	enum class ActionType {AFTERBURNER, BOARD, COMMUNICATION, FIRE, PICKUP, SCAN};
	// Check if some condition is keeping this ship from acting. (That is, it is
	// landing, hyperspacing, cloaking without "cloaked ActionType", disabled, or under-crewed.)
	bool CannotAct(ActionType actionType) const;
	// Get the degree to which this ship is cloaked. 1 means fully cloaked; 0 means fully visible.
	// Depending on its "cloaking ..." attributes the ship will be unable to shoot, will not be seen on radar...
	double Cloaking() const;
	bool IsCloaked() const;
	// The amount of cloaking this ship can do, per frame.
	double CloakingSpeed() const;
	// If this ship should be immune to the next damage caused.
	bool Phases(Projectile &projectile) const;
	// Check if this ship is entering (rather than leaving) hyperspace.
	bool IsEnteringHyperspace() const;
	// Check if this ship is entering or leaving hyperspace.
	bool IsHyperspacing() const;
	int GetHyperspacePercentage() const;
	// Check if this ship is hyperspacing, specifically via a jump drive.
	bool IsUsingJumpDrive() const;
	// Check if this ship is currently able to enter hyperspace to it target.
	bool IsReadyToJump(bool waitingIsReady = false) const;
	// Check if this ship is allowed to land on this planet, accounting for its personality.
	bool IsRestrictedFrom(const Planet &planet) const;
	// Check if this ship is allowed to enter this system, accounting for its personality.
	bool IsRestrictedFrom(const System &system) const;
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
	bool IsDamaged() const;
	// Check if this ship has been destroyed.
	bool IsDestroyed() const;
	// Recharge and repair this ship (e.g. because it has landed).
	void Recharge(int rechargeType = Port::RechargeType::All, bool hireCrew = true);
	// Check if this ship is able to give the given ship enough fuel to jump.
	bool CanRefuel(const Ship &other) const;
	// Give the other ship enough fuel for it to jump.
	double TransferFuel(double amount, Ship *to);
	// Mark this ship as property of the given ship. Returns the number of crew transferred from the capturer.
	int WasCaptured(const std::shared_ptr<Ship> &capturer);
	// Clear all orders and targets this ship has (after capture or transfer of control).
	void ClearTargetsAndOrders();

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
	// Get the maximum shield and hull values of the ship, accounting for multipliers.
	double MaxShields() const;
	double MaxHull() const;
	// Get the actual shield level of the ship.
	double ShieldLevel() const;
	// Get how disrupted this ship's shields are.
	double DisruptionLevel() const;
	// Get the (absolute) amount of hull that needs to be damaged until the
	// ship becomes disabled. Returns 0 if the ships hull is already below the
	// disabled threshold.
	double HullUntilDisabled() const;
	// Returns the remaining damage timer, for the damage overlay.
	int DamageOverlayTimer() const;
	// Get this ship's jump navigation, which contains information about how
	// much it costs for this ship to jump, how far it can jump, and its possible
	// jump methods.
	const ShipJumpNavigation &JumpNavigation() const;
	// Get the number of jumps this ship can make before running out of fuel.
	// This depends on how much fuel it has and what sort of hyperdrive it uses.
	// This does not show accurate number of jumps remaining beyond 1.
	// If followParent is false, this ship will not follow the parent.
	int JumpsRemaining(bool followParent = true) const;
	bool NeedsFuel(bool followParent = true) const;
	// Get the amount of fuel missing for the next jump (smart refueling)
	double JumpFuelMissing() const;
	// Get the heat level at idle.
	double IdleHeat() const;
	// Get the heat dissipation, in heat units per heat unit per frame.
	double HeatDissipation() const;
	// Get the maximum heat level, in heat units (not temperature).
	double MaximumHeat() const;
	// Calculate the multiplier for cooling efficiency.
	double CoolingEfficiency() const;
	// Calculate the drag on this ship. The drag can be no greater than the mass.
	double Drag() const;
	// Calculate the drag force that this ship experiences. The drag force is the drag
	// divided by the mass, up to a value of 1.
	double DragForce() const;

	// Access how many crew members this ship has or needs.
	int Crew() const;
	int RequiredCrew() const;
	// Get the reputational value of this ship's crew, which depends
	// on its crew size and "crew equivalent" attribute.
	int CrewValue() const;
	void AddCrew(int count);
	// Check if this is a ship that can be used as a flagship.
	bool CanBeFlagship() const;

	// Get this ship's movement characteristics.
	double Mass() const;
	double InertialMass() const;
	double TurnRate() const;
	double Acceleration() const;
	double MaxVelocity(bool withAfterburner = false) const;
	double ReverseAcceleration() const;
	double MaxReverseVelocity() const;

	// This ship just got hit by a weapon. Take damage according to the
	// DamageDealt from that weapon. The return value is a ShipEvent type,
	// which may be a combination of PROVOKED, DISABLED, and DESTROYED.
	// Create any target effects as sparks.
	int TakeDamage(std::vector<Visual> &visuals, const DamageDealt &damage, const Government *sourceGovernment);
	// Apply a force to this ship, accelerating it. This might be from a weapon
	// impact, or from firing a weapon, for example.
	void ApplyForce(const Point &force, bool gravitational = false);

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
	void Jettison(const std::string &commodity, int tons, bool wasAppeasing = false);
	void Jettison(const Outfit *outfit, int count, bool wasAppeasing = false);

	// Get the current attributes of this ship.
	const Outfit &Attributes() const;
	// Get the attributes of this ship chassis before any outfits were added.
	const Outfit &BaseAttributes() const;
	// Get the computed attributes of this ship.
	const Dictionary &DerivedAttributes() const;
	// Get the list of all outfits installed in this ship.
	const std::map<const Outfit *, int> &Outfits() const;
	// Find out how many outfits of the given type this ship contains.
	int OutfitCount(const Outfit *outfit) const;
	// Add or remove outfits. (To remove, pass a negative number.)
	void AddOutfit(const Outfit *outfit, int count);

	// Get the list of weapons.
	Armament &GetArmament();
	const std::vector<Hardpoint> &Weapons() const;
	// Check if we are able to fire the given weapon (i.e. there is enough
	// energy, ammo, and fuel to fire it).
	bool CanFire(const Weapon *weapon) const;
	// Fire the given weapon (i.e. deduct whatever energy, ammo, or fuel it uses
	// and add whatever heat it generates). Assume that CanFire() is true.
	void ExpendAmmo(const Weapon &weapon);

	// Each ship can have a target system (to travel to), a target planet (to
	// land on) and a target ship (to move to, and attack if hostile).
	std::shared_ptr<Ship> GetTargetShip() const;
	std::shared_ptr<Ship> GetShipToAssist() const;
	const StellarObject *GetTargetStellar() const;
	// Get ship's target system (it should always be one jump / wormhole pass away).
	const System *GetTargetSystem() const;
	// Mining target.
	std::shared_ptr<Minable> GetTargetAsteroid() const;
	std::shared_ptr<Flotsam> GetTargetFlotsam() const;
	const std::set<const Flotsam *> &GetTractorFlotsam() const;

	// Mark this ship as fleeing.
	void SetFleeing(bool fleeing = true);

	// Set this ship's targets.
	void SetTargetShip(const std::shared_ptr<Ship> &ship);
	void SetShipToAssist(const std::shared_ptr<Ship> &ship);
	void SetTargetStellar(const StellarObject *object);
	// Set ship's target system (it should always be one jump / wormhole pass away).
	void SetTargetSystem(const System *system);
	// Mining target.
	void SetTargetAsteroid(const std::shared_ptr<Minable> &asteroid);
	void SetTargetFlotsam(const std::shared_ptr<Flotsam> &flotsam);

	bool CanPickUp(const Flotsam &flotsam) const;

	// Manage escorts. When you set this ship's parent, it will automatically
	// register itself as an escort of that ship, and unregister itself from any
	// previous parent it had.
	void SetParent(const std::shared_ptr<Ship> &ship);
	std::shared_ptr<Ship> GetParent() const;
	const std::vector<std::weak_ptr<Ship>> &GetEscorts() const;

	// How many AI steps has this ship been lingering?
	int GetLingerSteps() const;
	// The AI wants the ship to linger for one AI step.
	void Linger();


private:
	void RecomputeDerivedAttributes();

	// Various steps of Ship::Move:

	// Check if this ship has been in a different system from the player for so
	// long that it should be "forgotten." Also eliminate ships that have no
	// system set because they just entered a fighter bay. Clear the hyperspace
	// targets of ships that can't enter hyperspace.
	bool StepFlags();
	// Step ship destruction logic. Returns 1 if the ship has been destroyed, -1 if it is being
	// destroyed, or 0 otherwise.
	int StepDestroyed(std::vector<Visual> &visuals, std::list<std::shared_ptr<Flotsam>> &flotsam);
	void DoGeneration();
	void DoPassiveEffects(std::vector<Visual> &visuals, std::list<std::shared_ptr<Flotsam>> &flotsam);
	void DoJettison(std::list<std::shared_ptr<Flotsam>> &flotsam);
	void DoCloakDecision();
	// Step hyperspace enter/exit logic. Returns true if ship is hyperspacing in or out.
	bool DoHyperspaceLogic(std::vector<Visual> &visuals);
	// Step landing logic. Returns true if the ship is landing or departing.
	bool DoLandingLogic();
	void DoInitializeMovement();
	void StepPilot();
	void DoMovement(bool &isUsingAfterburner);
	void StepTargeting();
	void DoEngineVisuals(std::vector<Visual> &visuals, bool isUsingAfterburner);


	// Add or remove a ship from this ship's list of escorts.
	void AddEscort(Ship &ship);
	void RemoveEscort(const Ship &ship);
	// Get the hull amount at which this ship is disabled.
	double MinimumHull() const;
	// Create one of this ship's explosions, within its mask. The explosions can
	// either stay over the ship, or spread out if this is the final explosion.
	void CreateExplosion(std::vector<Visual> &visuals, bool spread = false);
	// Place a "spark" effect, like ionization or disruption.
	void CreateSparks(std::vector<Visual> &visuals, const std::string &name, double amount);
	void CreateSparks(std::vector<Visual> &visuals, const Effect *effect, double amount);

	// Calculate the attraction and deterrence of this ship, for pirate raids.
	// This is only useful for the player's ships.
	double CalculateAttraction() const;
	double CalculateDeterrence() const;


private:
	// Protected member variables of the Body class:
	// Point position;
	// Point velocity;
	// Angle angle;
	// double zoom;
	// int swizzle;
	// const Government *government;

	// Characteristics of the chassis:
	bool isDefined = false;
	const Ship *base = nullptr;
	std::string trueModelName;
	std::string displayModelName;
	std::string pluralModelName;
	std::string variantName;
	std::string noun;
	std::string description;
	const Sprite *thumbnail = nullptr;
	// Characteristics of this particular ship:
	EsUuid uuid;
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
	bool isFleeing = false;
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
	// Cached values for figuring out when anti-missiles or tractor beams are in range.
	double antiMissileRange = 0.;
	double tractorBeamRange = 0.;
	double weaponRadius = 0.;
	// Cargo and outfit scanning takes time.
	double cargoScan = 0.;
	double outfitScan = 0.;

	double attraction = 0.;
	double deterrence = 0.;

	// Number of AI steps this ship has spent lingering
	int lingerSteps = 0;

	Command commands;
	FireCommand firingCommands;

	Personality personality;
	const Phrase *hail = nullptr;
	ShipAICache aiCache;

	// Installed outfits, cargo, etc.:
	Outfit attributes;
	Outfit baseAttributes;
	Dictionary derivedAttributes;
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

	// Various energy levels:
	double shields = 0.;
	double hull = 0.;
	double fuel = 0.;
	double energy = 0.;
	double heat = 0.;
	// Accrued "ion damage" that will affect this ship's energy over time.
	double ionization = 0.;
	// Accrued "scrambling damage" that will affect this ship's weaponry over time.
	double scrambling = 0.;
	// Accrued "disruption damage" that will affect this ship's shield effectiveness over time.
	double disruption = 0.;
	// Accrued "slowing damage" that will affect this ship's movement over time.
	double slowness = 0.;
	// Accrued "discharge damage" that will affect this ship's shields over time.
	double discharge = 0.;
	// Accrued "corrosion damage" that will affect this ship's hull over time.
	double corrosion = 0.;
	// Accrued "leak damage" that will affect this ship's fuel over time.
	double leakage = 0.;
	// Accrued "burn damage" that will affect this ship's heat over time.
	double burning = 0.;
	// Delays for shield generation and hull repair.
	int shieldDelay = 0;
	int hullDelay = 0;
	// Timer for a disabled ship to repair itself.
	int disabledRecoveryCounter = 0;
	// Number of frames the damage overlay should be displayed, if any.
	int damageOverlayTimer = 0;
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

	ShipJumpNavigation navigation;
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
	std::set<const Flotsam *> tractorFlotsam;

	// Links between escorts and parents.
	std::vector<std::weak_ptr<Ship>> escorts;
	std::weak_ptr<Ship> parent;

	bool removeBays = false;
};



#endif
