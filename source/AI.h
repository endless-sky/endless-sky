/* AI.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef AI_H_
#define AI_H_

#include "Command.h"
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
	AI(const List<Ship> &ships, const List<Minable> &minables, const List<Flotsam> &flotsam);
	
	// Fleet commands from the player.
	void IssueShipTarget(const PlayerInfo &player, const std::shared_ptr<Ship> &target);
	void IssueMoveTarget(const PlayerInfo &player, const Point &target, const System *moveToSystem);
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
	void Step(const PlayerInfo &player, Command &activeCommands);
	
	// Get the in-system strength of each government's allies and enemies.
	int64_t AllyStrength(const Government *government);
	int64_t EnemyStrength(const Government *government);
	
	
private:
	// Check if a ship can pursue its target (i.e. beyond the "fence").
	bool CanPursue(const Ship &ship, const Ship &target) const;
	// Disabled or stranded ships coordinate with other ships to get assistance.
	void AskForHelp(Ship &ship, bool &isStranded, const Ship *flagship);
	static bool CanHelp(const Ship &ship, const Ship &helper, const bool needsFuel);
	bool HasHelper(const Ship &ship, const bool needsFuel);
	// Pick a new target for the given ship.
	std::shared_ptr<Ship> FindTarget(const Ship &ship) const;
	// Obtain a list of ships matching the desired hostility.
	std::vector<std::shared_ptr<Ship>> GetShipsList(const Ship &ship, bool targetEnemies, double maxRange = -1.) const;
	
	bool FollowOrders(Ship &ship, Command &command) const;
	void MoveIndependent(Ship &ship, Command &command) const;
	void MoveEscort(Ship &ship, Command &command) const;
	static void Refuel(Ship &ship, Command &command);
	static bool CanRefuel(const Ship &ship, const StellarObject *target);
	bool ShouldDock(const Ship &ship, const Ship &parent, const System *playerSystem) const;
	
	// Methods of moving from the current position to a desired position / orientation.
	static double TurnBackward(const Ship &ship);
	static double TurnToward(const Ship &ship, const Point &vector);
	static bool MoveToPlanet(Ship &ship, Command &command);
	static bool MoveTo(Ship &ship, Command &command, const Point &targetPosition, const Point &targetVelocity, double radius, double slow);
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
	void DoSwarming(Ship &ship, Command &command, std::shared_ptr<Ship> &target);
	void DoSurveillance(Ship &ship, Command &command, std::shared_ptr<Ship> &target) const;
	void DoMining(Ship &ship, Command &command);
	bool DoHarvesting(Ship &ship, Command &command);
	bool DoCloak(Ship &ship, Command &command);
	// Prevent ships from stacking on each other when many are moving in sync.
	void DoScatter(Ship &ship, Command &command);
	
	static Point StoppingPoint(const Ship &ship, const Point &targetVelocity, bool &shouldReverse);
	// Get a vector giving the direction this ship should aim in in order to do
	// maximum damage to a target at the given position with its non-turret,
	// non-homing weapons. If the ship has no non-homing weapons, this just
	// returns the direction to the target.
	static Point TargetAim(const Ship &ship);
	static Point TargetAim(const Ship &ship, const Body &target);
	// Aim the given ship's turrets.
	void AimTurrets(const Ship &ship, Command &command, bool opportunistic = false) const;
	// Fire whichever of the given ship's weapons can hit a hostile target.
	// Return a bitmask giving the weapons to fire.
	void AutoFire(const Ship &ship, Command &command, bool secondary = true) const;
	void AutoFire(const Ship &ship, Command &command, const Body &target) const;
	
	// Calculate how long it will take a projectile to reach a target given the
	// target's relative position and velocity and the velocity of the
	// projectile. If it cannot hit the target, this returns NaN.
	static double RendezvousTime(const Point &p, const Point &v, double vp);
	
	void MovePlayer(Ship &ship, const PlayerInfo &player, Command &activeCommands);
	
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
	
	bool isCloaking = false;
	
	bool escortsAreFrugal = true;
	bool escortsUseAmmo = true;
	
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
	std::map<const Ship *, int> miningTime;
	std::map<const Ship *, double> appeasmentThreshold;
	
	std::map<const Ship *, int64_t> shipStrength;
	
	std::map<const Government *, int64_t> enemyStrength;
	std::map<const Government *, int64_t> allyStrength;
	std::map<const Government *, std::vector<std::shared_ptr<Ship>>> governmentRosters;
	std::map<const Government *, std::vector<std::shared_ptr<Ship>>> enemyLists;
	std::map<const Government *, std::vector<std::shared_ptr<Ship>>> allyLists;
};



#endif
