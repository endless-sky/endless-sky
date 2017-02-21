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

class Angle;
class AsteroidField;
class Body;
class Flotsam;
class Government;
class Minable;
class Ship;
class ShipEvent;
class PlayerInfo;



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
	void IssueMoveTarget(const PlayerInfo &player, const Point &target);
	// Commands issued via the keyboard (mostly, to the flagship).
	void UpdateKeys(PlayerInfo &player, Command &clickCommands, bool isActive);
	
	// Allow the AI to track any events it is interested in.
	void UpdateEvents(const std::list<ShipEvent> &events);
	// Reset the AI's memory of events.
	void Clean();
	// Issue AI commands to all ships for one game step.
	void Step(const PlayerInfo &player);
	
	
private:
	// Pick a new target for the given ship.
	std::shared_ptr<Ship> FindTarget(const Ship &ship) const;
	
	bool FollowOrders(Ship &ship, Command &command) const;
	void MoveIndependent(Ship &ship, Command &command) const;
	void MoveEscort(Ship &ship, Command &command) const;
	static void Refuel(Ship &ship, Command &command);
	
	static double TurnBackward(const Ship &ship);
	static double TurnToward(const Ship &ship, const Point &vector);
	static bool MoveToPlanet(Ship &ship, Command &command);
	static bool MoveTo(Ship &ship, Command &command, const Point &target, double radius, double slow);
	static bool Stop(Ship &ship, Command &command, double maxSpeed = 0.);
	static void PrepareForHyperspace(Ship &ship, Command &command);
	static void CircleAround(Ship &ship, Command &command, const Ship &target);
	static void Swarm(Ship &ship, Command &command, const Ship &target);
	static void KeepStation(Ship &ship, Command &command, const Ship &target);
	static void Attack(Ship &ship, Command &command, const Ship &target);
	static void MoveToAttack(Ship &ship, Command &command, const Body &target);
	static void PickUp(Ship &ship, Command &command, const Body &target);
	void DoSurveillance(Ship &ship, Command &command) const;
	void DoMining(Ship &ship, Command &command);
	bool DoHarvesting(Ship &ship, Command &command);
	void DoCloak(Ship &ship, Command &command);
	void DoScatter(Ship &ship, Command &command);
	
	static Point StoppingPoint(const Ship &ship, bool &shouldReverse);
	// Get a vector giving the direction this ship should aim in in order to do
	// maximum damaged to a target at the given position with its non-turret,
	// non-homing weapons. If the ship has no non-homing weapons, this just
	// returns the direction to the target.
	static Point TargetAim(const Ship &ship);
	static Point TargetAim(const Ship &ship, const Body &target);
	// Fire whichever of the given ship's weapons can hit a hostile target.
	// Return a bitmask giving the weapons to fire.
	Command AutoFire(const Ship &ship, bool secondary = true) const;
	Command AutoFire(const Ship &ship, const Body &target) const;
	
	void MovePlayer(Ship &ship, const PlayerInfo &player);
	
	bool Has(const Ship &ship, const std::weak_ptr<const Ship> &other, int type) const;
	bool Has(const Government *government, const std::weak_ptr<const Ship> &other, int type) const;
	
	
private:
	class Orders {
	public:
		static const int HOLD_POSITION = 0x000;
		static const int MOVE_TO = 0x001;
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
	};


private:
	void IssueOrders(const PlayerInfo &player, const Orders &newOrders, const std::string &description);
	
	
private:
	// Data from the game engine.
	const List<Ship> &ships;
	const List<Minable> &minables;
	const List<Flotsam> &flotsam;
	
	int step = 0;
	
	Command keyDown;
	Command keyHeld;
	Command keyStuck;
	bool wasHyperspacing = false;
	bool isLaunching = false;
	bool isCloaking = false;
	bool shift = false;
	
	bool escortsAreFrugal = true;
	bool escortsUseAmmo = true;
	// Pressing "land" rapidly toggles targets; pressing it once re-engages landing.
	int landKeyInterval = 0;
	
	// Current orders for the player's ships. Because this map only applies to
	// player ships, which are never deleted except when landed, it can use
	// ordinary pointers instead of weak pointers.
	std::map<const Ship *, Orders> orders;
	
	// Records of what various AI ships and factions have done.
	typedef std::owner_less<std::weak_ptr<const Ship>> Comp;
	std::map<std::weak_ptr<const Ship>, std::map<std::weak_ptr<const Ship>, int, Comp>, Comp> actions;
	std::map<const Government *, std::map<std::weak_ptr<const Ship>, int, Comp>> governmentActions;
	std::map<std::weak_ptr<const Ship>, int, Comp> playerActions;
	std::map<const Ship *, int> swarmCount;
	std::map<const Ship *, Angle> miningAngle;
	std::map<const Ship *, int> miningTime;
	std::map<const Ship *, double> appeasmentThreshold;
	
	std::map<const Ship *, int64_t> shipStrength;
	
	std::map<const Government *, int64_t> enemyStrength;
	std::map<const Government *, int64_t> allyStrength;
};



#endif
