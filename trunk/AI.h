/* AI.h
Michael Zahniser, 3 Jan 2014

Interface for ship AIs.
*/

#ifndef AI_H_INCLUDED
#define AI_H_INCLUDED

// deprecated
#include "KeyStatus.h"

#include <list>
#include <memory>
#include <string>

class Controllable;
class Point;
class Ship;
class PlayerInfo;



class AI {
public:
	void UpdateKeys();
	void Step(const std::list<std::shared_ptr<Ship>> &ships, const PlayerInfo &info);
	
	// Get any messages (such as "you cannot land here!") to display.
	const std::string &Message() const;
	
	
private:
	void MoveIndependent(Controllable &control, const Ship &ship);
	void MoveEscort(Controllable &control, const Ship &ship);
	
	double TurnBackward(const Ship &ship);
	double TurnToward(const Ship &ship, const Point &vector);
	void MoveToPlanet(Controllable &control, const Ship &ship);
	void PrepareForHyperspace(Controllable &control, const Ship &ship);
	void CircleAround(Controllable &control, const Ship &ship, const Ship &target);
	
	Point StoppingPoint(const Ship &ship);
	
	void MovePlayer(Controllable &control, const PlayerInfo &info, const std::list<std::shared_ptr<Ship>> &ships);
	
	
private:
	KeyStatus keys;
	mutable KeyStatus sticky;
	
	std::string message;
};



#endif
