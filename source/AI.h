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

#include <cstdint>
#include <list>
#include <map>
#include <memory>

class Government;
class Point;
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
	AI();
	
	void UpdateKeys(PlayerInfo &player, bool isActive);
	void UpdateEvents(const std::list<ShipEvent> &events);
	void Clean();
	void Step(const std::list<std::shared_ptr<Ship>> &ships, const PlayerInfo &player);
	
	
private:
	// Pick a new target for the given ship.
	std::shared_ptr<Ship> FindTarget(const Ship &ship, const std::list<std::shared_ptr<Ship>> &ships) const;
	
	void MoveIndependent(Ship &ship, Command &command) const;
	static void MoveEscort(Ship &ship, Command &command);
	static void Refuel(Ship &ship, Command &command);
	
	static double TurnBackward(const Ship &ship);
	static double TurnToward(const Ship &ship, const Point &vector);
	static bool MoveToPlanet(Ship &ship, Command &command);
	static bool MoveTo(Ship &ship, Command &command, const Point &target, double radius, double slow);
	static bool Stop(Ship &ship, Command &command, double slow = .2);
	static void PrepareForHyperspace(Ship &ship, Command &command);
	static void CircleAround(Ship &ship, Command &command, const Ship &target);
	static void Attack(Ship &ship, Command &command, const Ship &target);
	void DoSurveillance(Ship &ship, Command &command, const std::list<std::shared_ptr<Ship>> &ships) const;
	static void DoCloak(Ship &ship, Command &command, const std::list<std::shared_ptr<Ship>> &ships);
	static void DoScatter(Ship &ship, Command &command, const std::list<std::shared_ptr<Ship>> &ships);
	
	static Point StoppingPoint(const Ship &ship, bool &shouldReverse);
	// Get a vector giving the direction this ship should aim in in order to do
	// maximum damaged to a target at the given position with its non-turret,
	// non-homing weapons. If the ship has no non-homing weapons, this just
	// returns the direction to the target.
	static Point TargetAim(const Ship &ship);
	// Fire whichever of the given ship's weapons can hit a hostile target.
	// Return a bitmask giving the weapons to fire.
	Command AutoFire(const Ship &ship, const std::list<std::shared_ptr<Ship>> &ships, bool secondary = true) const;
	
	void MovePlayer(Ship &ship, const PlayerInfo &player, const std::list<std::shared_ptr<Ship>> &ships);
	
	bool Has(const Ship &ship, const std::weak_ptr<const Ship> &other, int type) const;
	
	
private:
	int step;
	
	Command keyDown;
	Command keyHeld;
	Command keyStuck;
	bool isLaunching;
	bool isCloaking;
	bool shift;
	
	bool holdPosition;
	bool moveToMe;
	std::weak_ptr<Ship> sharedTarget;
	// Pressing "land" rapidly toggles targets; pressing it once re-engages landing.
	int landKeyInterval;
	
	typedef std::owner_less<std::weak_ptr<const Ship>> Comp;
	std::map<std::weak_ptr<const Ship>, std::map<std::weak_ptr<const Ship>, int, Comp>, Comp> actions;
	std::map<std::weak_ptr<const Ship>, int, Comp> playerActions;
	
	std::map<const Government *, int64_t> enemyStrength;
	std::map<const Government *, int64_t> allyStrength;
};



#endif
