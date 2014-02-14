/* Controllable.h
Michael Zahniser, 27 January 2014

Class representing all aspects of a ship which an AI has access to, either to
modify them or just to view them.
*/

#ifndef CONTROLLABLE_H_INCLUDED
#define CONTROLLABLE_H_INCLUDED

#include <memory>
#include <vector>

class Ship;
class StellarObject;
class System;



class Controllable {
public:
	Controllable();
	
	// Find out what commands this ship has been given.
	double GetThrustCommand() const;
	double GetTurnCommand() const;
	bool HasLandCommand() const;
	bool HasHyperspaceCommand() const;
	bool HasFireCommand(int index) const;
	
	// Set the commands.
	void ResetCommands();
	void SetThrustCommand(double direction);
	void SetTurnCommand(double direction);
	void SetLandCommand();
	void SetHyperspaceCommand();
	void SetFireCommand(int index);
	void FireAll();
	
	// Each ship can have a target system (to travel to), a target planet (to
	// land on) and a target ship (to move to, and attack if hostile).
	const std::weak_ptr<const Ship> &GetTargetShip() const;
	const StellarObject *GetTargetPlanet() const;
	const System *GetTargetSystem() const;
	
	// Set this ship's targets.
	void SetTargetShip(const std::weak_ptr<const Ship> &ship);
	void SetTargetPlanet(const StellarObject *object);
	void SetTargetSystem(const System *system);
	
	// Add escorts to this ship. Escorts look to the parent ship for movement
	// cues and try to stay with it when it lands or goes into hyperspace.
	void AddEscort(const std::weak_ptr<const Ship> &ship);
	void SetParent(const std::weak_ptr<const Ship> &ship);
	
	const std::vector<std::weak_ptr<const Ship>> &GetEscorts() const;
	const std::weak_ptr<const Ship> &GetParent() const;
	
	
private:
	int commands;
	
	std::weak_ptr<const Ship> targetShip;
	const StellarObject *targetPlanet;
	const System *targetSystem;
	
	std::vector<std::weak_ptr<const Ship>> escorts;
	std::weak_ptr<const Ship> parent;
};



#endif
