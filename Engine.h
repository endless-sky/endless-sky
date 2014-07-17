/* Engine.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ENGINE_H_
#define ENGINE_H_

#include "AI.h"
#include "AsteroidField.h"
#include "Date.h"
#include "DrawList.h"
#include "GameData.h"
#include "Information.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Projectile.h"
#include "Radar.h"
#include "Ship.h"
#include "System.h"

#include <list>
#include <map>
#include <memory>
#include <thread>
#include <vector>

class Government;
class Panel;



// Class representing the game engine: its job is to track all of the objects in
// the game, and to move them, step by step.
class Engine {
public:
	Engine(const GameData &data, PlayerInfo &player);
	~Engine();
	
	// Place all the player's ships, and "enter" the system the player is in.
	void Place();
	
	// Begin the next step of calculations.
	void Step(bool isActive);
	
	// Get any ships that we must show boarding dialogs for.
	const std::shared_ptr<Ship> &Boarding() const;
	
	// Draw a frame.
	void Draw() const;
	
	
private:
	void EnterSystem();
	
	void ThreadEntryPoint();
	void CalculateStep();
	
	
private:
	class Target {
	public:
		Point center;
		Angle angle;
		double radius;
		int type;
	};
	
	
private:
	const GameData &data;
	PlayerInfo &player;
	const Government *playerGovernment;
	
	AI ai;
	
	std::thread calcThread;
	std::condition_variable condition;
	std::mutex swapMutex;
	
	bool calcTickTock;
	bool drawTickTock;
	bool terminate;
	DrawList draw[2];
	Radar radar[2];
	// Viewport position and velocity.
	Point position;
	Point velocity;
	// Other information to display.
	Information info;
	std::vector<Target> targets;
	std::vector<std::pair<const Outfit *, int>> ammo;
	
	int step;
	
	std::list<std::shared_ptr<Ship>> ships;
	std::list<Projectile> projectiles;
	std::list<Effect> effects;
	// Keep track of which ships we have not seen for long enough that it is
	// time to stop tracking their movements.
	std::map<std::list<Ship>::iterator, int> forget;
	
	std::list<std::shared_ptr<Ship>> boardingQueue;
	std::shared_ptr<Ship> boarding;
	
	AsteroidField asteroids;
	double flash;
	
	double load;
	int loadCount;
	double loadSum;
};



#endif
