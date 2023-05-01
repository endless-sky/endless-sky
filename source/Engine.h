/* Engine.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ENGINE_H_
#define ENGINE_H_

#include "AI.h"
#include "AmmoDisplay.h"
#include "AsteroidField.h"
#include "BatchDrawList.h"
#include "CollisionSet.h"
#include "Command.h"
#include "DrawList.h"
#include "EscortDisplay.h"
#include "Information.h"
#include "Point.h"
#include "Preferences.h"
#include "Radar.h"
#include "Rectangle.h"

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

class AlertLabel;
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
class TestContext;
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

	// Set the given TestContext in the next step of the Engine.
	void SetTestContext(TestContext &newTestContext);

	// Select the object the player clicked on.
	void Click(const Point &from, const Point &to, bool hasShift, bool hasControl);
	void RClick(const Point &point);
	void SelectGroup(int group, bool hasShift, bool hasControl);

	// Break targeting on all projectiles between the player and the given
	// government; gov projectiles stop targeting the player and player's
	// projectiles stop targeting gov.
	void BreakTargeting(const Government *gov);


private:
	class Target {
	public:
		Point center;
		Angle angle;
		double radius;
		const Color &color;
		int count;
	};

	class Status {
	public:
		Status(const Point &position, double outer, double inner,
			double disabled, double radius, int type, double angle = 0.)
			: position(position), outer(outer), inner(inner),
				disabled(disabled), radius(radius), type(type), angle(angle) {}

		Point position;
		double outer;
		double inner;
		double disabled;
		double radius;
		int type;
		double angle;
	};


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
	void HandleMouseInput(Command &activeCommands);

	void FillCollisionSets();

	void DoCollisions(Projectile &projectile);
	void DoWeather(Weather &weather);
	void DoCollection(Flotsam &flotsam);
	void DoScanning(const std::shared_ptr<Ship> &ship);

	void FillRadar();

	void AddSprites(const Ship &ship);

	void DoGrudge(const std::shared_ptr<Ship> &target, const Government *attacker);

	void CreateStatusOverlays();
	void EmplaceStatusOverlay(const std::shared_ptr<Ship> &ship, Preferences::OverlayState overlaySetting, int value);


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
	bool hasFinishedCalculating = true;
	bool terminate = false;
	bool wasActive = false;
	bool isMouseHoldEnabled = false;
	bool isMouseTurningEnabled = false;
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
	AmmoDisplay ammoDisplay;
	std::vector<Status> statuses;
	std::vector<PlanetLabel> labels;
	std::vector<AlertLabel> missileLabels;
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
	// Pressing "land" or "board" rapidly toggles targets; pressing it once re-engages landing or boarding.
	int keyInterval = 0;

	// Inputs received from a mouse or other pointer device.
	bool doClickNextStep = false;
	bool doClick = false;
	bool hasShift = false;
	bool hasControl = false;
	bool isRightClick = false;
	bool isRadarClick = false;
	Point clickPoint;
	Rectangle uiClickBox;
	Rectangle clickBox;
	int groupSelect = -1;

	// Set of asteroids scanned in the current system.
	std::set<std::string> asteroidsScanned;
	bool isAsteroidCatalogComplete = false;

	// Input, Output and State handling for automated tests.
	TestContext *testContext = nullptr;

	double zoom = 1.;
	// Tracks the next zoom change so that objects aren't drawn at different zooms in a single frame.
	double nextZoom = 0.;

	double load = 0.;
	int loadCount = 0;
	double loadSum = 0.;
};



#endif
