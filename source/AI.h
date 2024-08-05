/* AI.h
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

#ifndef ES_AI_H_
#define ES_AI_H_

#include "Command.h"
#include "FireCommand.h"
#include "FormationPositioner.h"
#include "Orders.h"
#include "Point.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

class Angle;
class AsteroidField;
class Body;
class Flotsam;
class Government;
class Minable;
class PlayerInfo;
class Ship;
class ShipEvent;
class StellarObject;
class System;



// This class is responsible for controlling all the ships in the game,
// including the player's "flagship" - which is usually controlled via the
// keyboard but can also make use of the AI's autopilot for landing, aiming,
// or hyperspace travel between systems. The AI also tracks which actions
// ships have performed, to avoid having the same ship try to scan or board
// the same target over and over.
class AI {
public:
	// Any object that can be a ship's target is in a list of this type:
template <class Type>
	using List = std::list<std::shared_ptr<Type>>;
	// Constructor, giving the AI access to the player and various object lists.
	AI(const PlayerInfo &player, const std::vector<std::shared_ptr<Ship>> &ships,
			const List<Minable> &minables, const List<Flotsam> &flotsam);

	// Fleet commands from the player.
	void IssueFormationChange(PlayerInfo &player);
	void IssueShipTarget(const std::shared_ptr<Ship> &target);
	void IssueAsteroidTarget(const std::shared_ptr<Minable> &targetAsteroid);
	void IssueMoveTarget(const Point &target, const System *moveToSystem);

	// Commands issued via the keyboard (mostly, to the flagship).
	void UpdateKeys(PlayerInfo &player, Command &clickCommands);

	// Allow the AI to track any events it is interested in.
	void UpdateEvents(const std::list<ShipEvent> &events);
	// Reset the AI's memory of events.
	void Clean();
	// Clear ship orders. This should be done when the player lands on a planet,
	// but not when they jump from one system to another.
	void ClearOrders();
	// Issue AI commands to all ships for one game step.
	void Step(Command &activeCommands);

	// Set the mouse position for turning the player's flagship.
	void SetMousePosition(Point position);

	// Get the in-system strength of each government's allies and enemies.
	int64_t AllyStrength(const Government *government);
	int64_t EnemyStrength(const Government *government);

	// Find nearest landing location.
	static const StellarObject *FindLandingLocation(const Ship &ship, const bool refuel = true);


private:
	// Check if a ship can pursue its target (i.e. beyond the "fence").
	bool CanPursue(const Ship &ship, const Ship &target) const;
	// Disabled or stranded ships coordinate with other ships to get assistance.
	void AskForHelp(Ship &ship, bool &isStranded, const Ship *flagship);
	bool CanHelp(const Ship &ship, const Ship &helper, const bool needsFuel, const bool needsEnergy) const;
	bool HasHelper(const Ship &ship, const bool needsFuel, const bool needsEnergy);
	// Pick a new target for the given ship.
	std::shared_ptr<Ship> FindTarget(const Ship &ship) const;
	std::shared_ptr<Ship> FindNonHostileTarget(const Ship &ship) const;
	// Obtain a list of ships matching the desired hostility.
	std::vector<Ship *> GetShipsList(const Ship &ship, bool targetEnemies, double maxRange = -1.) const;

	bool FollowOrders(Ship &ship, Command &command);
	void MoveInFormation(Ship &ship, Command &command);
	void MoveIndependent(Ship &ship, Command &command) const;
	void MoveEscort(Ship &ship, Command &command);
	static void Refuel(Ship &ship, Command &command);
	static bool CanRefuel(const Ship &ship, const StellarObject *target);
	// Set the ship's target system or planet in order to reach the
	// next desired system. Will target a landable planet to refuel.
	// If the ship is an escort it will only use routes known to the player.
	void SelectRoute(Ship &ship, const System *targetSystem) const;
	bool ShouldDock(const Ship &ship, const Ship &parent, const System *playerSystem) const;

	// Methods of moving from the current position to a desired position / orientation.
	static double TurnBackward(const Ship &ship);
	// Determine the value to use in Command::SetTurn() to turn the ship towards the desired facing.
	// "precision" is an optional argument corresponding to a value of the dot product of the current and target facing
	// vectors above which no turning should be attempting, to reduce constant, minute corrections.
	static double TurnToward(const Ship &ship, const Point &vector, const double precision = 0.9999);
	static bool MoveToPlanet(Ship &ship, Command &command);
	static bool MoveTo(Ship &ship, Command &command, const Point &targetPosition,
		const Point &targetVelocity, double radius, double slow);
	static bool Stop(Ship &ship, Command &command, double maxSpeed = 0., const Point direction = Point());
	static void PrepareForHyperspace(Ship &ship, Command &command);
	static void CircleAround(Ship &ship, Command &command, const Body &target);
	static void Swarm(Ship &ship, Command &command, const Body &target);
	static void KeepStation(Ship &ship, Command &command, const Body &target);
	static void Attack(Ship &ship, Command &command, const Ship &target);
	static void AimToAttack(Ship &ship, Command &command, const Body &target);
	static void MoveToAttack(Ship &ship, Command &command, const Body &target);
	static void PickUp(Ship &ship, Command &command, const Body &target);
	// Special decisions a ship might make.
	static bool ShouldUseAfterburner(Ship &ship);
	// Special personality behaviors.
	void DoAppeasing(const std::shared_ptr<Ship> &ship, double *threshold) const;
	void DoSwarming(Ship &ship, Command &command, std::shared_ptr<Ship> &target);
	void DoSurveillance(Ship &ship, Command &command, std::shared_ptr<Ship> &target) const;
	void DoMining(Ship &ship, Command &command);
	bool DoHarvesting(Ship &ship, Command &command) const;
	bool DoCloak(Ship &ship, Command &command);
	void DoPatrol(Ship &ship, Command &command) const;
	// Prevent ships from stacking on each other when many are moving in sync.
	void DoScatter(Ship &ship, Command &command);
	bool DoSecretive(Ship &ship, Command &command);

	static Point StoppingPoint(const Ship &ship, const Point &targetVelocity, bool &shouldReverse);
	// Get a vector giving the direction this ship should aim in in order to do
	// maximum damage to a target at the given position with its non-turret,
	// non-homing weapons. If the ship has no non-homing weapons, this just
	// returns the direction to the target.
	static Point TargetAim(const Ship &ship);
	static Point TargetAim(const Ship &ship, const Body &target);
	// Aim the given ship's turrets.
	void AimTurrets(const Ship &ship, FireCommand &command, bool opportunistic = false) const;
	// Fire whichever of the given ship's weapons can hit a hostile target.
	// Return a bitmask giving the weapons to fire.
	void AutoFire(const Ship &ship, FireCommand &command, bool secondary = true, bool isFlagship = false) const;
	void AutoFire(const Ship &ship, FireCommand &command, const Body &target) const;

	// Calculate how long it will take a projectile to reach a target given the
	// target's relative position and velocity and the velocity of the
	// projectile. If it cannot hit the target, this returns NaN.
	static double RendezvousTime(const Point &p, const Point &v, double vp);

	void MovePlayer(Ship &ship, Command &activeCommands);

	// True if found asteroid.
	bool TargetMinable(Ship &ship) const;
	// True if the ship performed the indicated event to the other ship.
	bool Has(const Ship &ship, const std::weak_ptr<const Ship> &other, int type) const;
	// True if the government performed the indicated event to the other ship.
	bool Has(const Government *government, const std::weak_ptr<const Ship> &other, int type) const;
	// True if the ship has performed the indicated event against any member of the government.
	bool Has(const Ship &ship, const Government *government, int type) const;

	// Functions to classify ships based on government and system.
	void UpdateStrengths(std::map<const Government *, int64_t> &strength, const System *playerSystem);
	void CacheShipLists();


private:
	void IssueOrders(const Orders &newOrders, const std::string &description);
	// Convert order types based on fulfillment status.
	void UpdateOrders(const Ship &ship);


private:
	// TODO: Figure out a way to remove the player dependency.
	const PlayerInfo &player;
	// Data from the game engine.
	const std::vector<std::shared_ptr<Ship>> &ships;
	const List<Minable> &minables;
	const List<Flotsam> &flotsam;

	// The current step count for the AI, ranging from 0 to 30. Its value
	// helps limit how often certain actions occur (such as changing targets).
	int step = 0;

	// Command applied by the player's "autopilot."
	Command autoPilot;
	// Position of the cursor, for when the player is using mouse turning.
	Point mousePosition;
	// General firing command for ships. This is a data member to avoid
	// thrashing the heap, since we can reuse the storage for
	// each ship.
	FireCommand firingCommands;

	bool isCloaking = false;

	bool escortsAreFrugal = true;
	bool escortsUseAmmo = true;

	// The minimum speed before landing will consider non-landable objects.
	const float MIN_LANDING_VELOCITY = 80.;

	// Current orders for the player's ships. Because this map only applies to
	// player ships, which are never deleted except when landed, it can use
	// ordinary pointers instead of weak pointers.
	std::map<const Ship *, Orders> orders;

	// Records of what various AI ships and factions have done.
	typedef std::owner_less<std::weak_ptr<const Ship>> Comp;
	std::map<std::weak_ptr<const Ship>, std::map<std::weak_ptr<const Ship>, int, Comp>, Comp> actions;
	std::map<std::weak_ptr<const Ship>, std::map<const Government *, int>, Comp> notoriety;
	std::map<const Government *, std::map<std::weak_ptr<const Ship>, int, Comp>> governmentActions;
	std::map<const Government *, bool> scanPermissions;
	std::map<std::weak_ptr<const Ship>, int, Comp> playerActions;
	std::map<const Ship *, std::weak_ptr<Ship>> helperList;
	std::map<const Ship *, int> swarmCount;
	std::map<const Ship *, int> fenceCount;
	std::map<const Ship *, std::set<const Ship *>> cargoScans;
	std::map<const Ship *, std::set<const Ship *>> outfitScans;
	std::map<const Ship *, int> scanTime;
	std::map<const Ship *, Angle> miningAngle;
	std::map<const Ship *, double> miningRadius;
	std::map<const Ship *, int> miningTime;
	std::map<const Ship *, double> appeasementThreshold;

	// Records for formations flying around leadships and other objects.
	std::map<const Body *, std::map<const FormationPattern *, FormationPositioner>> formations;

	// Records that affect the combat behavior of various governments.
	std::map<const Ship *, int64_t> shipStrength;
	std::map<const Government *, int64_t> enemyStrength;
	std::map<const Government *, int64_t> allyStrength;
	std::map<const Government *, std::vector<Ship *>> governmentRosters;
	std::map<const Government *, std::vector<Ship *>> enemyLists;
	std::map<const Government *, std::vector<Ship *>> allyLists;
};



#endif
