/* Engine.h
Michael Zahniser, 3 Jan 2014

Class representing the game engine: its job is to track all of the objects in
the game, and to move them, step by step.
*/

#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

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



class Engine {
public:
	Engine(const GameData &data, PlayerInfo &playerInfo);
	~Engine();
	
	// Begin the next step of calculations.
	void Step(bool isActive);
	// Check if there's a new panel to show as a result of something that
	// happened in this step (such as landing on a planet).
	Panel *PanelToShow();
	
	// Draw a frame.
	void Draw() const;
	
	// Get a map of where the player is, currently.
	Panel *Map();
	
	
private:
	void EnterSystem();
	// Place all the player's ships, and "enter" the system the player is in.
	void Place();
	
	void ThreadEntryPoint();
	void CalculateStep();
	void AddMessage(const std::string &message);
	
	
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
	PlayerInfo &playerInfo;
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
	
	mutable std::vector<std::pair<int, std::string>> messages;
	int step;
	bool shouldLand;
	bool hasLanded;
	
	std::list<std::shared_ptr<Ship>> ships;
	std::list<Projectile> projectiles;
	std::list<Effect> effects;
	// Keep track of which ships we have not seen for long enough that it is
	// time to stop tracking their movements.
	std::map<std::list<Ship>::iterator, int> forget;
	
	AsteroidField asteroids;
	
	double load;
	int loadCount;
	double loadSum;
};



#endif
