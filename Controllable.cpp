/* Controllable.cpp
Michael Zahniser, 27 Jan 2014

Function definitions for the Controllable class.
*/

#include "Controllable.h"

using namespace std;

namespace {
	static const unsigned FORWARD = 1;
	static const unsigned REVERSE = 2;
	static const unsigned LEFT = 4;
	static const unsigned RIGHT = 8;
	static const unsigned LAND = 16;
	static const unsigned HYPERSPACE = 32;
	static const unsigned LAUNCH = 64;
	static const unsigned BOARD = 128;
	
	static const unsigned WEAPON_SHIFT = 16;
}



Controllable::Controllable()
	: commands(0), targetPlanet(nullptr), targetSystem(nullptr)
{
}



// Find out what commands this ship has been given.
double Controllable::GetThrustCommand() const
{
	return ((commands & FORWARD) != 0) - ((commands & REVERSE) != 0);
}



double Controllable::GetTurnCommand() const
{
	return ((commands & RIGHT) != 0) - ((commands & LEFT) != 0);
}



bool Controllable::HasLandCommand() const
{
	return commands & LAND;
}



bool Controllable::HasHyperspaceCommand() const
{
	return commands & HYPERSPACE;
}



bool Controllable::HasFireCommand(int index) const
{
	return commands & (1 << (index + WEAPON_SHIFT));
}



// Set the commands.
void Controllable::ResetCommands()
{
	commands = 0;
}



void Controllable::SetThrustCommand(double direction)
{
	commands ^= (commands & (FORWARD | REVERSE));
	if(direction > 0.)
		commands |= FORWARD;
	if(direction < 0.)
		commands |= REVERSE;
}



void Controllable::SetTurnCommand(double direction)
{
	commands ^= (commands & (RIGHT | LEFT));
	if(direction > 0.)
		commands |= RIGHT;
	if(direction < 0.)
		commands |= LEFT;
}



void Controllable::SetLandCommand()
{
	commands |= LAND;
}



void Controllable::SetHyperspaceCommand()
{
	commands |= HYPERSPACE;
}



void Controllable::SetFireCommand(int index)
{
	commands |= (1 << (index + WEAPON_SHIFT));
}



void Controllable::SetFireCommands(int bitmask)
{
	commands |= bitmask << WEAPON_SHIFT;
}



// Each ship can have a target system (to travel to), a target planet (to
// land on) and a target ship (to move to, and attack if hostile).
const std::weak_ptr<const Ship> &Controllable::GetTargetShip() const
{
	return targetShip;
}



const StellarObject *Controllable::GetTargetPlanet() const
{
	return targetPlanet;
}



const System *Controllable::GetTargetSystem() const
{
	return targetSystem;
}



// Set this ship's targets.
void Controllable::SetTargetShip(const std::weak_ptr<const Ship> &ship)
{
	targetShip = ship;
}



void Controllable::SetTargetPlanet(const StellarObject *object)
{
	targetPlanet = object;
}



void Controllable::SetTargetSystem(const System *system)
{
	targetSystem = system;
}



// Add escorts to this ship. Escorts look to the parent ship for movement
// cues and try to stay with it when it lands or goes into hyperspace.
void Controllable::AddEscort(const std::weak_ptr<const Ship> &ship)
{
	escorts.push_back(ship);
}



void Controllable::SetParent(const std::weak_ptr<const Ship> &ship)
{
	parent = ship;
}



const vector<std::weak_ptr<const Ship>> &Controllable::GetEscorts() const
{
	return escorts;
}



const std::weak_ptr<const Ship> &Controllable::GetParent() const
{
	return parent;
}
