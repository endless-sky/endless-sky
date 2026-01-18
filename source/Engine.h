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

#pragma once

#include "AI.h"
#include "AlertLabel.h"
#include "AmmoDisplay.h"
#include "AsteroidField.h"
#include "shader/BatchDrawList.h"
#include "Camera.h"
#include "CollisionSet.h"
#include "Color.h"
#include "Command.h"
#include "shader/DrawList.h"
#include "EscortDisplay.h"
#include "Information.h"
#include "MiniMap.h"
#include "MouseButton.h"
#include "PlanetLabel.h"
#include "Point.h"
#include "Preferences.h"
#include "Projectile.h"
#include "Radar.h"
#include "Rectangle.h"
#include "TaskQueue.h"

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <utility>
#include <vector>

class Flotsam;
class Government;
class NPC;
class Outfit;
class PlayerInfo;
class Ship;
class ShipEvent;
class Sprite;
class Swizzle;
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
	void Place(const std::list<NPC> &npcs, const std::shared_ptr<Ship> &flagship = nullptr);

	// Wait for the previous calculations (if any) to be done.
	void Wait();
	// Perform all the work that can only be done while the calculation thread
	// is paused (for thread safety reasons).
	void Step(bool isActive);
	// Begin the next step of calculations.
	void Go();
	// Whether the player has the game paused.
	bool IsPaused() const;

	// Give a command on behalf of the player, used for integration tests.
	void GiveCommand(const Command &command);

	// Get any special events that happened in this step.
	// MainPanel::Step will clear this list.
	std::list<ShipEvent> &Events();

	// Draw a frame.
	void Draw() const;

	// Select the object the player clicked on.
	void Click(const Point &from, const Point &to, bool hasShift, bool hasControl);
	void RightOrMiddleClick(const Point &point, MouseButton button);
	void SelectGroup(int group, bool hasShift, bool hasControl);

	// Break targeting on all projectiles between the player and the given
	// government; gov projectiles stop targeting the player and player's
	// projectiles stop targeting gov.
	void BreakTargeting(const Government *gov);


private:
	class Outline {
	public:
		Outline(const Sprite *sprite, const Point &position, const Point &unit,
			const float frame, const Color &color)
			: sprite(sprite), position(position), unit(unit), frame(frame), color(color)
		{
		}

		const Sprite *sprite;
		const Point position;
		const Point unit;
		const float frame;
		const Color color;
	};

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
		enum class Type {
			FLAGSHIP,
			FRIENDLY,
			HOSTILE,
			NEUTRAL,
			SCAN,
			SCAN_OUT_OF_RANGE,
			COUNT // This item should always be the last in this list.
		};

	public:
		constexpr Status(const Point &position, double outer, double inner,
			double disabled, double radius, Type type, float alpha, double angle = 0.)
			: position(position), outer(outer), inner(inner),
				disabled(disabled), radius(radius), type(type), alpha(alpha), angle(angle) {}

		Point position;
		double outer;
		double inner;
		double disabled;
		double radius;
		Type type;
		float alpha;
		double angle;
	};

	class TurretOverlay {
	public:
		Point position;
		Point angle;
		double scale;
		bool isBlind;
	};

	class Zoom {
	public:
		constexpr Zoom() : base(0.) {}
		explicit constexpr Zoom(double zoom) : base(zoom) {}

		constexpr operator double() const { return base * modifier; }

		double base;
		double modifier = 1.;
	};


private:
	void EnterSystem();

	void CalculateStep();
	// Calculate things that require the engine not to be paused.
	void CalculateUnpaused(const Ship *flagship, const System *playerSystem);

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

	void DrawShipSprites(const Ship &ship);

	void DoGrudge(const std::shared_ptr<Ship> &target, const Government *attacker);

	void CreateStatusOverlays();
	void EmplaceStatusOverlay(const std::shared_ptr<Ship> &ship, Preferences::OverlayState overlaySetting,
		Status::Type type, double cloak);


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

	// Track which ships currently have anti-missiles or
	// tractor beams ready to fire.
	std::vector<Ship *> hasAntiMissile;
	std::vector<Ship *> hasTractorBeam;

	AI ai;

	TaskQueue queue;

	// ES uses a technique called double buffering to calculate the next frame and render the current one simultaneously.
	// To facilitate this, it uses two buffers for each list of things to draw - one for the next frame's calculations and
	// one for rendering the current frame. A little synchronization is required to prevent mutable references to the
	// currently rendering buffer.
	size_t currentCalcBuffer = 0;
	size_t currentDrawBuffer = 0;
	DrawList draw[2];
	BatchDrawList batchDraw[2];
	Radar radar[2];

	bool wasActive = false;
	bool isMouseHoldEnabled = false;
	bool isMouseTurningEnabled = false;

	// Viewport camera.
	Camera camera;
	// Other information to display.
	Information info;
	std::vector<Target> targets;
	Point targetVector;
	Point targetUnit;
	const Swizzle *targetSwizzle = nullptr;
	// Represents the state of the currently targeted ship when it was last seen,
	// so the target display does not show updates to its state the player should not be aware of.
	int lastTargetType = 0;
	EscortDisplay escorts;
	AmmoDisplay ammoDisplay;
	std::vector<Outline> outlines;
	std::vector<Status> statuses;
	std::vector<PlanetLabel> labels;
	std::vector<AlertLabel> missileLabels;
	std::vector<TurretOverlay> turretOverlays;
	std::vector<std::pair<const Outfit *, int>> ammo;
	// Flagship's hyperspace percentage converted to a [0, 1] double.
	double hyperspacePercentage = 0.;

	MiniMap minimap;

	int step = 0;
	// Count steps for UI elements separately, because they shouldn't be affected by pausing.
	mutable int uiStep = 0;
	bool timePaused = false;

	std::list<ShipEvent> eventQueue;
	std::list<ShipEvent> events;
	// Keep track of who has asked for help in fighting whom.
	std::map<const Government *, std::weak_ptr<const Ship>> grudge;
	int grudgeTime = 0;

	CollisionSet shipCollisions;

	int alarmTime = 0;
	double flash = 0.;
	bool doFlash = false;
	bool doEnterLabels = false;
	bool doEnter = false;
	bool hadHostiles = false;

	// A timer preventing out-of-ammo sounds from triggering constantly every frame when the fire key is held.
	std::vector<int> emptySoundsTimer;

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
	MouseButton mouseButton = MouseButton::NONE;
	bool isRadarClick = false;
	Point clickPoint;
	Rectangle uiClickBox;
	Rectangle clickBox;
	int groupSelect = -1;

	// Set of asteroids scanned in the current system.
	std::set<std::string> asteroidsScanned;
	bool isAsteroidCatalogComplete = false;

	Zoom zoom;
	// Tracks the next zoom change so that objects aren't drawn at different zooms in a single frame.
	Zoom nextZoom;
};
