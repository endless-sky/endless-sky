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
#include "Point.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
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

	// Constructor, giving the AI access to various object lists.
	// Called on Engine Constructor.
	AI(const List<Ship> &ships, const List<Minable> &minables, const List<Flotsam> &flotsam);

	// Fleet commands from the player. //
	// Fuction that sets a ship as the target of the players fleet.
	// If the targeted ship is an enemy the fleet will be given the
	// command to attack.
	// Called on mouse click events handling.
	void IssueShipTarget(const PlayerInfo &player, const std::shared_ptr<Ship> &target);
	// Function that sets a position in the system as target. The fleet
	// will be given the command to move to this location.
	// Called on mouse click events handling.
	void IssueMoveTarget(const PlayerInfo &player, const Point &target, const System *moveToSystem);
	// Function to update all commands the player gives to the flagship
	// and the fleet via keyboard.
	// Called in an Engine Step.
	void UpdateKeys(PlayerInfo &player, Command &clickCommands);

	// Allow the AI to track any events it is interested in. //
	// Function used to keep track of provoking the AI and to update
	// the notoriety of certain actors.
	// Called in an Engine Step.
	void UpdateEvents(const std::list<ShipEvent> &events);
	// Function to reset the AI's memory of events.
	// Called on entering a system.
	void Clean();
	// Function to clear ship orders. This should be done when the player
	// lands on a planet, but not when they jump from one system to another.
	// Called when ships are "placed" in a system for example on enter.
	void ClearOrders();
	// Function to issue AI commands to all ships for one game step.
	// Called when the Engine calculates a Step.
	void Step(const PlayerInfo &player, Command &activeCommands);

	// Functions to get the in-system strength of each government's allies and enemies.
	// Called on fleet spawn.
	int64_t AllyStrength(const Government *government);
	int64_t EnemyStrength(const Government *government);


private:
	// Reminder: Functions in the private section are only called inside the AI class.

	// Function to check if a ship can pursue its target (i.e. beyond the "fence").
	// Called during an AI step and when finding a target.
	bool CanPursue(const Ship &ship, const Ship &target) const;

	// Disabled or stranded ships coordinate with other ships to get assistance. //
	// Function to check if the ship is being helped, and if not, ask for help.
	// Called during an AI step if the ship is stranded or disabled.
	void AskForHelp(Ship &ship, bool &isStranded, const Ship *flagship);
	// Function to check if the given ship can be helped by the helper ship.
	// Called when asking for help and when checking if the ship has a helper.
	static bool CanHelp(const Ship &ship, const Ship &helper, const bool needsFuel);
	// Function to check if the given ship already has a helper ship.
	// Called when asking for help.
	bool HasHelper(const Ship &ship, const bool needsFuel);

	// Function to pick a new target for the given ship.
	// Called in an AI step.
	std::shared_ptr<Ship> FindTarget(const Ship &ship) const;
	// Function to obtain a list of ships matching the desired hostility.
	// Used in combat target finding and firing functions.
	std::vector<Ship *> GetShipsList(const Ship &ship, bool targetEnemies, double maxRange = -1.) const;

	// Function to execute orders calculated by the AI earlier.
	// Called in an AI step.
	bool FollowOrders(Ship &ship, Command &command) const;
	// Function to move a ship independent from its fleet.
	// Called mainly in an AI step but also when following orders or doing surveilance.
	void MoveIndependent(Ship &ship, Command &command) const;
	// Function to move an escort depending on what commands its parent has.
	// Called in an AI step.
	void MoveEscort(Ship &ship, Command &command) const;
	// Function to make a ship refuel.
	// Called when moving escorts or during swarming.
	static void Refuel(Ship &ship, Command &command);
	// Function to check if the given ship can refuel on the given stellar object.
	// Called when making a ship refueling.
	static bool CanRefuel(const Ship &ship, const StellarObject *target);
	// Function to determine if a carried ship meets any of the criteria for returning to its parent.
	// Called in an AI step.
	bool ShouldDock(const Ship &ship, const Ship &parent, const System *playerSystem) const;

	// Methods of moving from the current position to a desired position / orientation.
	// Called whenever a ship has to move.
	static double TurnBackward(const Ship &ship);
	static double TurnToward(const Ship &ship, const Point &vector);
	static bool MoveToPlanet(Ship &ship, Command &command);
	static bool MoveTo(Ship &ship, Command &command, const Point &targetPosition,
		const Point &targetVelocity, double radius, double slow);
	static bool Stop(Ship &ship, Command &command, double maxSpeed = 0., const Point direction = Point());
	static void PrepareForHyperspace(Ship &ship, Command &command);
	static void CircleAround(Ship &ship, Command &command, const Body &target);
	static void Swarm(Ship &ship, Command &command, const Body &target);
	static void KeepStation(Ship &ship, Command &command, const Body &target);
	static void Attack(Ship &ship, Command &command, const Ship &target);
	static void MoveToAttack(Ship &ship, Command &command, const Body &target);
	static void PickUp(Ship &ship, Command &command, const Body &target);
	// Special decisions a ship might make.
	static bool ShouldUseAfterburner(Ship &ship);
	// Special personality behaviors.
	void DoAppeasing(const std::shared_ptr<Ship> &ship, double *threshold) const;
	void DoSwarming(Ship &ship, Command &command, std::shared_ptr<Ship> &target);
	void DoSurveillance(Ship &ship, Command &command, std::shared_ptr<Ship> &target) const;
	void DoMining(Ship &ship, Command &command);
	bool DoHarvesting(Ship &ship, Command &command);
	bool DoCloak(Ship &ship, Command &command);
	// Prevent ships from stacking on each other when many are moving in sync.
	void DoScatter(Ship &ship, Command &command);

	// Function to calculate a point so that instead of coming to a full stop,
	// the ship adjusts to a target velocity vector.
	// Called when a ship moves to a Point or when orders are issued.
	static Point StoppingPoint(const Ship &ship, const Point &targetVelocity, bool &shouldReverse);
	// Function to get a vector giving the direction this ship should aim in
	// in order to do maximum damage to a target at the given position with
	// its non-turret, non-homing weapons. If the ship has no non-homing weapons,
	// this just returns the direction to the target.
	// Called in an AI step, when following orders and when moving the player.
	static Point TargetAim(const Ship &ship);
	static Point TargetAim(const Ship &ship, const Body &target);
	// Function to aim the given ship's turrets.
	// Called in an AI step and when moving the player.
	void AimTurrets(const Ship &ship, FireCommand &command, bool opportunistic = false) const;
	// Function to fire whichever of the given ship's weapons can hit a hostile target.
	// Return a bitmask giving the weapons to fire.
	// Called in an AI step and when moving the player.
	void AutoFire(const Ship &ship, FireCommand &command, bool secondary = true) const;
	void AutoFire(const Ship &ship, FireCommand &command, const Body &target) const;

	// Function to calculate how long it will take a projectile to reach a target given the
	// target's relative position and velocity and the velocity of the
	// projectile. If it cannot hit the target, this returns NaN.
	// Called in all kinds of movement functions.
	static double RendezvousTime(const Point &p, const Point &v, double vp);

	// Move the players ship on basis of the given commands.
	// Called in an AI step.
	void MovePlayer(Ship &ship, const PlayerInfo &player, Command &activeCommands);

	// These functions are called in multiple places. //
	// Function that returns true if the ship performed the indicated event to the other ship.
	bool Has(const Ship &ship, const std::weak_ptr<const Ship> &other, int type) const;
	// Function that returns true if the government performed the indicated event to the other ship.
	bool Has(const Government *government, const std::weak_ptr<const Ship> &other, int type) const;
	// Function that returns true if the ship has performed the indicated event against any member of the government.
	bool Has(const Ship &ship, const Government *government, int type) const;

	// Functions to classify ships based on government and system.
	// Both called in an AI step.
	void UpdateStrengths(std::map<const Government *, int64_t> &strength, const System *playerSystem);
	void CacheShipLists();


private:
	class Orders {
	public:
		static const int HOLD_POSITION = 0x000;
		// Hold active is the same command as hold position, but it is given when a ship
		// actively needs to move back to the position it was holding.
		static const int HOLD_ACTIVE = 0x001;
		static const int MOVE_TO = 0x002;
		static const int KEEP_STATION = 0x100;
		static const int GATHER = 0x101;
		static const int ATTACK = 0x102;
		static const int FINISH_OFF = 0x103;
		// Bit mask to figure out which orders are canceled if their target
		// ceases to be targetable or present.
		static const int REQUIRES_TARGET = 0x100;

		int type = 0;
		std::weak_ptr<Ship> target;
		Point point;
		const System *targetSystem = nullptr;
	};


private:
	void IssueOrders(const PlayerInfo &player, const Orders &newOrders, const std::string &description);
	// Convert order types based on fulfillment status.
	void UpdateOrders(const Ship &ship);


private:
	// Data from the game engine.
	const List<Ship> &ships;
	const List<Minable> &minables;
	const List<Flotsam> &flotsam;

	// The current step count for the AI, ranging from 0 to 30. Its value
	// helps limit how often certain actions occur (such as changing targets).
	int step = 0;

	// Command applied by the player's "autopilot."
	Command autoPilot;
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
	std::map<const Ship *, Angle> miningAngle;
	std::map<const Ship *, double> miningRadius;
	std::map<const Ship *, int> miningTime;
	std::map<const Ship *, double> appeasementThreshold;

	std::map<const Ship *, int64_t> shipStrength;

	std::map<const Government *, int64_t> enemyStrength;
	std::map<const Government *, int64_t> allyStrength;
	std::map<const Government *, std::vector<Ship *>> governmentRosters;
	std::map<const Government *, std::vector<Ship *>> enemyLists;
	std::map<const Government *, std::vector<Ship *>> allyLists;
};



#endif
