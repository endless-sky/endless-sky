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
#include "BatchDrawList.h"
#include "CollisionSet.h"
#include "Command.h"
#include "DrawList.h"
#include "EscortDisplay.h"
#include "Information.h"
#include "Point.h"
#include "Radar.h"
#include "Rectangle.h"

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

class Flotsam;
class Government;
class NPC;
class Outfit;
class PlanetLabel;
class PlayerInfo;
class Projectile;
class Ship;
class ShipEvent;
class Sprite;
class Visual;
class Weather;



// Class representing the game engine: its job is to track all of the objects in
// the game, and to move them, step by step. All the motion and collision
// calculations are handled in a separate thread so that the graphics thread is
// free to just work on drawing things; this means that the drawn state of the
// game is always one step (1/60 second) behind what is being calculated. This
// lag is too small to be detectable and means that the game can better handle
// situations where there are many objects on screen at once.
class Engine {
public:
	explicit Engine(PlayerInfo &player);
	~Engine();
	
	// Place all the player's ships, and "enter" the system the player is in.
	void Place();
	// Place NPCs spawned by a mission that offers when the player is not landed.
	void Place(const std::list<NPC> &npcs, std::shared_ptr<Ship> flagship = nullptr);
	
	// Wait for the previous calculations (if any) to be done.
	void Wait();
	// Perform all the work that can only be done while the calculation thread
	// is paused (for thread safety reasons).
	void Step(bool isActive);
	// Begin the next step of calculations.
	void Go();
	
	// Get any special events that happened in this step.
	// MainPanel::Step will clear this list.
	std::list<ShipEvent> &Events();
	
	// Draw a frame.
	void Draw() const;
	
	// Give an (automated/scripted) command on behalf of the player.
	void GiveCommand(const Command &command);
	
	// Select the object the player clicked on.
	void Click(const Point &from, const Point &to, bool hasShift);
	void RClick(const Point &point);
	void SelectGroup(int group, bool hasShift, bool hasControl);
	
	
private:
	void EnterSystem();
	
	void ThreadEntryPoint();
	void CalculateStep();
	
	void MoveShip(const std::shared_ptr<Ship> &ship);
	
	void SpawnFleets();
	void SpawnPersons();
	void GenerateWeather();
	void SendHails();
	void HandleKeyboardInputs();
	void HandleMouseClicks();
	
	void FillCollisionSets();
	
	void DoCollisions(Projectile &projectile);
	void DoWeather(Weather &weather);
	void DoCollection(Flotsam &flotsam);
	void DoScanning(const std::shared_ptr<Ship> &ship);
	
	void FillRadar();
	
	void AddSprites(const Ship &ship);
	
	void DoGrudge(const std::shared_ptr<Ship> &target, const Government *attacker);
	
	
private:
	class Target {
	public:
		Point center;
		Angle angle;
		double radius;
		int type;
		int count;
	};
	
	class Status {
	public:
		Status(const Point &position, double outer, double inner, double disabled, double radius, int type, double angle = 0.);
		
		Point position;
		double outer;
		double inner;
		double disabled;
		double radius;
		int type;
		double angle;
	};
	
	
private:
	PlayerInfo &player;
	
	std::list<std::shared_ptr<Ship>> ships;
	std::vector<Projectile> projectiles;
	std::vector<Weather> activeWeather;
	std::list<std::shared_ptr<Flotsam>> flotsam;
	std::vector<Visual> visuals;
	AsteroidField asteroids;
	
	// New objects created within the latest step:
	std::list<std::shared_ptr<Ship>> newShips;
	std::vector<Projectile> newProjectiles;
	std::list<std::shared_ptr<Flotsam>> newFlotsam;
	std::vector<Visual> newVisuals;
	
	// Track which ships currently have anti-missiles ready to fire.
	std::vector<Ship *> hasAntiMissile;
	
	AI ai;
	
	std::thread calcThread;
	std::condition_variable condition;
	std::mutex swapMutex;
	
	bool calcTickTock = false;
	bool drawTickTock = false;
	bool terminate = false;
	bool wasActive = false;
	DrawList draw[2];
	BatchDrawList batchDraw[2];
	Radar radar[2];
	// Viewport position and velocity.
	Point center;
	Point centerVelocity;
	// Other information to display.
	Information info;
	std::vector<Target> targets;
	Point targetVector;
	Point targetUnit;
	int targetSwizzle = -1;
	EscortDisplay escorts;
	std::vector<Status> statuses;
	std::vector<PlanetLabel> labels;
	std::vector<std::pair<const Outfit *, int>> ammo;
	int jumpCount = 0;
	const System *jumpInProgress[2] = {nullptr, nullptr};
	const Sprite *highlightSprite = nullptr;
	Point highlightUnit;
	float highlightFrame = 0.f;
	
	int step = 0;
	
	std::list<ShipEvent> eventQueue;
	std::list<ShipEvent> events;
	// Keep track of who has asked for help in fighting whom.
	std::map<const Government *, std::weak_ptr<const Ship>> grudge;
	int grudgeTime = 0;
	
	CollisionSet shipCollisions;
	
	int alarmTime = 0;
	double flash = 0.;
	bool doFlash = false;
	bool doEnter = false;
	bool hadHostiles = false;
	
	// Commands that are currently active (and not yet handled). This is a combination
	// of keyboard and mouse commands (and any other available input device).
	Command activeCommands;
	// Keyboard commands that were active in the previous step.
	Command keyHeld;
	// Pressing "land" rapidly toggles targets; pressing it once re-engages landing.
	int landKeyInterval = 0;
	
	// Inputs received from a mouse or other pointer device.
	bool doClickNextStep = false;
	bool doClick = false;
	bool hasShift = false;
	bool hasControl = false;
	bool isRightClick = false;
	bool isRadarClick = false;
	Point clickPoint;
	Rectangle clickBox;
	int groupSelect = -1;
	
	double zoom = 1.;
	
	double load = 0.;
	int loadCount = 0;
	double loadSum = 0.;
};



#endif
