/* Controllable.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CONTROLLABLE_H_
#define CONTROLLABLE_H_

#include "Point.h"

#include <memory>
#include <vector>

class Planet;
class Ship;
class StellarObject;
class System;



// Class representing all aspects of a ship which an AI has access to, either to
// modify them or just to view them. This is separated out so that the AI can
// have read-only access to ship characteristics, but write access to their
// actions. This also includes the various targets each ship can have.
class Controllable {
public:
	Controllable();
	
	// Find out what commands this ship has been given.
	double GetThrustCommand() const;
	double GetTurnCommand() const;
	bool HasLandCommand() const;
	bool HasHyperspaceCommand() const;
	bool HasLaunchCommand() const;
	bool HasBoardCommand() const;
	bool HasScanCommand() const;
	bool HasAfterburnerCommand() const;
	bool HasCloakCommand() const;
	bool HasFireCommand(int index) const;
	
	// Set the commands.
	void ResetCommands();
	void SetThrustCommand(double direction);
	void SetTurnCommand(double direction);
	void SetLandCommand();
	void SetHyperspaceCommand();
	void SetLaunchCommand();
	void SetBoardCommand();
	void SetScanCommand();
	void SetAfterburnerCommand();
	void SetCloakCommand();
	void SetFireCommand(int index);
	void SetFireCommands(int bitmask = 0xFFFFFFFF);
	
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
	int commands;
	double thrust;
	double turn;
	
	std::weak_ptr<Ship> targetShip;
	std::weak_ptr<Ship> shipToAssist;
	const StellarObject *targetPlanet;
	const System *targetSystem;
	const Planet *destination;
	
	std::vector<std::weak_ptr<Ship>> escorts;
	std::weak_ptr<Ship> parent;
};



#endif
