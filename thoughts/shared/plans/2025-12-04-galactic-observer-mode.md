# Galactic Observer Mode Implementation Plan

## Overview

Implement a "Galactic Observer" mode - a screensaver/spectator mode where the player watches the universe simulate itself without direct interaction. The mode launches from the main menu, creates a ghost player (no flagship), and provides multiple camera modes to observe NPC activity in a system.

## Current State Analysis

### Key Discoveries:
- Camera is coupled to flagship at ~6 locations in `Engine.cpp` (lines 270, 399, 507-511, 532, 1568, 1626-1632)
- Game only simulates current system - ships in other systems are frozen (acceptable for MVP)
- Intro missions already handle player without flagship (landed state) - can adapt this pattern
- UI is data-driven via `Interface` class - new HUD can be added via data files
- Two crash points without flagship: `Engine.cpp:399` and `Engine.cpp:1617`
- Fast-forward skips 2/3 frames (`% 3`) - can be modified for faster speeds post-MVP
- Debug slow-mo reduces frame rate to 10 FPS via `FrameTimer::SetFrameRate()`

### Architecture:
- Dual UI stack: `menuPanels` and `gamePanels`
- `MenuPanel` pre-creates `MainPanel` in constructor, "Enter Ship" just pops menu overlay
- Engine uses double-buffered rendering with separate calculation thread
- Source files added to `source/CMakeLists.txt` in `target_sources(EndlessSkyLib PRIVATE ...)`

## Desired End State

A working observer/screensaver mode where:
1. User clicks "Observe" from main menu
2. Game enters a random system (if it turns out to be difficult, the currently default starting system is fine) with no player ship
3. Camera automatically follows ships, orbits planets, or tracks battles
4. Minimal HUD shows camera mode and system name
5. ESC returns to menu
6. Auto-saves periodically (compatible with original game format)

### Verification:
- Launch observer mode from menu without crash
- Camera smoothly follows NPC ships
- Can cycle between camera modes (Tab)
- ESC returns to menu cleanly
- Save file can be loaded in standard game (will start landed somewhere)

## What We're NOT Doing

- Multi-system background simulation (universe only simulates current system)
- Trade flow visualization / heatmaps
- 10x+ time scaling (requires delta-time physics rewrite)
- Player "drop-in" to take control of watched ship
- System selection UI (MVP uses random/default system)
- Informative HUD with statistics (post-MVP)
- Time speed controls (post-MVP)
- System auto-switching (post-MVP)

## Implementation Approach

Create a clean separation via new `ObserverPanel` class that owns its own `Engine` instance with a ghost `PlayerInfo`. Introduce `CameraController` abstraction to decouple camera targeting from flagship. Modify `Engine` minimally to support external camera control and handle null flagship gracefully.

---

## Phase 1: Foundation Infrastructure

### Overview
Create the core infrastructure: ghost player factory, camera controller abstraction, and CMake setup.

### Changes Required:

#### 1.1 Ghost Player Factory

**File**: `source/PlayerInfo.h`
**Changes**: Add static factory method declaration

```cpp
// Add after line ~220 (near other static/factory methods)
public:
	// Create an observer-mode player with no ships in the given system.
	static PlayerInfo CreateObserver(const System *system);
```

**File**: `source/PlayerInfo.cpp`
**Changes**: Implement the factory method

```cpp
// Add at end of file, before closing namespace if any
PlayerInfo PlayerInfo::CreateObserver(const System *system)
{
	PlayerInfo observer;
	observer.Clear();

	// Set minimal identity (required for IsLoaded())
	observer.firstName = "Observer";
	observer.lastName = "Mode";

	// Set location - in space, no planet
	observer.system = system;
	observer.planet = nullptr;

	// Initialize date to current game date
	observer.date = GameData::GetDate();

	// No ships, no missions, no money needed
	// accounts, ships, missions vectors stay empty

	return observer;
}
```

#### 1.2 Camera Controller Interface

**File**: `source/CameraController.h` (NEW)
**Changes**: Create new file

```cpp
// CameraController.h - Abstract interface for camera targeting

#ifndef CAMERA_CONTROLLER_H_
#define CAMERA_CONTROLLER_H_

#include "Point.h"

#include <list>
#include <memory>

class Ship;
class StellarObject;
class System;

// Abstract base class for camera control strategies.
// Implementations provide different ways to position the camera:
// following ships, orbiting planets, free movement, or tracking battles.
class CameraController {
public:
	virtual ~CameraController() = default;

	// Get the current target position for the camera to follow.
	virtual Point GetTarget() const = 0;

	// Get the velocity of the target (for motion blur calculation).
	virtual Point GetVelocity() const = 0;

	// Update internal state. Called each frame.
	virtual void Step() = 0;

	// Provide the list of ships for modes that need to select targets.
	virtual void SetShips(const std::list<std::shared_ptr<Ship>> &ships);

	// Provide stellar objects for orbit mode.
	virtual void SetStellarObjects(const std::vector<StellarObject> &objects);

	// Get a display name for the current mode (for HUD).
	virtual const std::string &ModeName() const = 0;

	// Get info about current target (for HUD). Empty if no specific target.
	virtual std::string TargetName() const;
};

#endif
```

**File**: `source/CameraController.cpp` (NEW)
**Changes**: Create new file with base implementations

```cpp
// CameraController.cpp

#include "CameraController.h"

void CameraController::SetShips(const std::list<std::shared_ptr<Ship>> &ships)
{
	// Default: do nothing. Subclasses override if needed.
}

void CameraController::SetStellarObjects(const std::vector<StellarObject> &objects)
{
	// Default: do nothing. Subclasses override if needed.
}

std::string CameraController::TargetName() const
{
	return "";
}
```

#### 1.3 Follow Ship Camera

**File**: `source/FollowShipCamera.h` (NEW)
**Changes**: Create new file

```cpp
// FollowShipCamera.h - Camera that follows a randomly selected ship

#ifndef FOLLOW_SHIP_CAMERA_H_
#define FOLLOW_SHIP_CAMERA_H_

#include "CameraController.h"

#include <list>
#include <memory>
#include <string>

class Ship;

class FollowShipCamera : public CameraController {
public:
	FollowShipCamera();

	Point GetTarget() const override;
	Point GetVelocity() const override;
	void Step() override;
	void SetShips(const std::list<std::shared_ptr<Ship>> &ships) override;
	const std::string &ModeName() const override;
	std::string TargetName() const override;

	// Select the next ship in the list.
	void CycleTarget();
	// Select a random ship.
	void SelectRandom();

private:
	std::list<std::shared_ptr<Ship>> ships;
	std::weak_ptr<Ship> target;
	Point lastPosition;
	Point lastVelocity;
	static const std::string name;
};

#endif
```

**File**: `source/FollowShipCamera.cpp` (NEW)
**Changes**: Create new file

```cpp
// FollowShipCamera.cpp

#include "FollowShipCamera.h"

#include "Ship.h"
#include "Random.h"

const std::string FollowShipCamera::name = "Follow Ship";

FollowShipCamera::FollowShipCamera()
	: lastPosition(0., 0.), lastVelocity(0., 0.)
{
}

Point FollowShipCamera::GetTarget() const
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
		return ship->Center();
	return lastPosition;
}

Point FollowShipCamera::GetVelocity() const
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
		return ship->Velocity();
	return lastVelocity;
}

void FollowShipCamera::Step()
{
	auto ship = target.lock();
	if(ship && ship->GetSystem())
	{
		lastPosition = ship->Center();
		lastVelocity = ship->Velocity();
	}
	else if(!ships.empty())
	{
		// Target lost, select a new one
		SelectRandom();
	}
}

void FollowShipCamera::SetShips(const std::list<std::shared_ptr<Ship>> &newShips)
{
	ships = newShips;

	// If we have no target, select one
	if(target.expired() && !ships.empty())
		SelectRandom();
}

const std::string &FollowShipCamera::ModeName() const
{
	return name;
}

std::string FollowShipCamera::TargetName() const
{
	auto ship = target.lock();
	if(ship)
		return ship->Name();
	return "";
}

void FollowShipCamera::CycleTarget()
{
	if(ships.empty())
		return;

	auto current = target.lock();
	bool foundCurrent = false;

	for(auto it = ships.begin(); it != ships.end(); ++it)
	{
		if(foundCurrent && (*it)->IsTargetable())
		{
			target = *it;
			return;
		}
		if(*it == current)
			foundCurrent = true;
	}

	// Wrap around to first targetable ship
	for(auto &ship : ships)
	{
		if(ship->IsTargetable())
		{
			target = ship;
			return;
		}
	}
}

void FollowShipCamera::SelectRandom()
{
	if(ships.empty())
		return;

	// Build list of targetable ships
	std::vector<std::shared_ptr<Ship>> targetable;
	for(auto &ship : ships)
		if(ship->IsTargetable())
			targetable.push_back(ship);

	if(!targetable.empty())
		target = targetable[Random::Int(targetable.size())];
}
```

#### 1.4 Orbit Planet Camera

**File**: `source/OrbitPlanetCamera.h` (NEW)
**Changes**: Create new file

```cpp
// OrbitPlanetCamera.h - Camera that orbits around a stellar object

#ifndef ORBIT_PLANET_CAMERA_H_
#define ORBIT_PLANET_CAMERA_H_

#include "CameraController.h"
#include "Angle.h"
#include "Point.h"

#include <string>
#include <vector>

class StellarObject;

class OrbitPlanetCamera : public CameraController {
public:
	OrbitPlanetCamera();

	Point GetTarget() const override;
	Point GetVelocity() const override;
	void Step() override;
	void SetStellarObjects(const std::vector<StellarObject> &objects) override;
	const std::string &ModeName() const override;
	std::string TargetName() const override;

	// Select the next stellar object.
	void CycleTarget();

private:
	std::vector<const StellarObject *> objects;
	size_t targetIndex = 0;
	Angle orbitAngle;
	double orbitDistance = 400.;
	Point currentPosition;
	Point velocity;
	static const std::string name;
};

#endif
```

**File**: `source/OrbitPlanetCamera.cpp` (NEW)
**Changes**: Create new file

```cpp
// OrbitPlanetCamera.cpp

#include "OrbitPlanetCamera.h"

#include "StellarObject.h"
#include "Planet.h"

const std::string OrbitPlanetCamera::name = "Orbit Planet";

OrbitPlanetCamera::OrbitPlanetCamera()
	: orbitAngle(0.), currentPosition(0., 0.), velocity(0., 0.)
{
}

Point OrbitPlanetCamera::GetTarget() const
{
	return currentPosition;
}

Point OrbitPlanetCamera::GetVelocity() const
{
	return velocity;
}

void OrbitPlanetCamera::Step()
{
	// Rotate slowly around the object
	orbitAngle += Angle(0.2);

	Point oldPosition = currentPosition;

	if(targetIndex < objects.size())
	{
		const StellarObject *obj = objects[targetIndex];
		// Orbit distance based on object size
		double distance = orbitDistance + obj->Radius() * 1.5;
		currentPosition = obj->Position() + orbitAngle.Unit() * distance;
	}

	velocity = currentPosition - oldPosition;
}

void OrbitPlanetCamera::SetStellarObjects(const std::vector<StellarObject> &newObjects)
{
	objects.clear();

	// Only include objects with sprites (visible planets/stations)
	for(const StellarObject &obj : newObjects)
		if(obj.HasSprite())
			objects.push_back(&obj);

	if(!objects.empty() && targetIndex >= objects.size())
		targetIndex = 0;
}

const std::string &OrbitPlanetCamera::ModeName() const
{
	return name;
}

std::string OrbitPlanetCamera::TargetName() const
{
	if(targetIndex < objects.size())
	{
		const StellarObject *obj = objects[targetIndex];
		if(obj->HasValidPlanet())
			return obj->GetPlanet()->Name();
		return obj->Name();
	}
	return "";
}

void OrbitPlanetCamera::CycleTarget()
{
	if(!objects.empty())
		targetIndex = (targetIndex + 1) % objects.size();
}
```

#### 1.5 Free Camera

**File**: `source/FreeCamera.h` (NEW)
**Changes**: Create new file

```cpp
// FreeCamera.h - Free-roaming camera controlled by keyboard

#ifndef FREE_CAMERA_H_
#define FREE_CAMERA_H_

#include "CameraController.h"
#include "Point.h"

#include <string>

class FreeCamera : public CameraController {
public:
	FreeCamera();

	Point GetTarget() const override;
	Point GetVelocity() const override;
	void Step() override;
	const std::string &ModeName() const override;

	// Set movement direction from input (-1 to 1 for each axis)
	void SetMovement(double x, double y);

	// Set position directly (e.g., when switching to this mode)
	void SetPosition(const Point &pos);

private:
	Point position;
	Point velocity;
	Point inputDirection;
	double speed = 8.;
	double friction = 0.92;
	static const std::string name;
};

#endif
```

**File**: `source/FreeCamera.cpp` (NEW)
**Changes**: Create new file

```cpp
// FreeCamera.cpp

#include "FreeCamera.h"

const std::string FreeCamera::name = "Free Camera";

FreeCamera::FreeCamera()
	: position(0., 0.), velocity(0., 0.), inputDirection(0., 0.)
{
}

Point FreeCamera::GetTarget() const
{
	return position;
}

Point FreeCamera::GetVelocity() const
{
	return velocity;
}

void FreeCamera::Step()
{
	// Apply input as acceleration
	velocity += inputDirection * speed;

	// Apply friction
	velocity *= friction;

	// Update position
	position += velocity;

	// Clear input for next frame
	inputDirection = Point(0., 0.);
}

const std::string &FreeCamera::ModeName() const
{
	return name;
}

void FreeCamera::SetMovement(double x, double y)
{
	inputDirection = Point(x, y);
}

void FreeCamera::SetPosition(const Point &pos)
{
	position = pos;
	velocity = Point(0., 0.);
}
```

#### 1.6 CMakeLists Update

**File**: `source/CMakeLists.txt`
**Changes**: Add new source files in alphabetical order within `target_sources()`

After `Camera.h` (around line 42), add:
```cmake
	CameraController.cpp
	CameraController.h
```

After `Flotsam.h` (around line 110), add:
```cmake
	FollowShipCamera.cpp
	FollowShipCamera.h
	FreeCamera.cpp
	FreeCamera.h
```

After `OrderedList.h` (around line 216), add:
```cmake
	ObserverPanel.cpp
	ObserverPanel.h
	OrbitPlanetCamera.cpp
	OrbitPlanetCamera.h
```

### Success Criteria:

#### Automated Verification:
- [x] Project compiles: `cmake --build build`
- [x] No new compiler warnings in added files

#### Manual Verification:
- [ ] New files appear in IDE project structure
- [ ] Code review: interfaces are clean and follow project conventions

---

## Phase 2: Engine Integration

### Overview
Modify Engine to support external camera control and handle observer mode gracefully (no flagship).

### Changes Required:

#### 2.1 Engine Header Changes

**File**: `source/Engine.h`
**Changes**: Add observer mode support

```cpp
// Add include at top (around line 30)
#include <memory>

// Forward declaration (around line 45)
class CameraController;

// Add in public section (around line 85, after IsPaused())
public:
	// Set external camera controller for observer mode.
	// Pass nullptr to return to normal flagship-following behavior.
	void SetCameraController(CameraController *controller);

	// Check if engine is in observer mode (no flagship camera tracking).
	bool IsObserverMode() const;

// Add in private section (around line 260, near other member variables)
private:
	// External camera controller for observer mode. Not owned.
	CameraController *cameraController = nullptr;
```

#### 2.2 Engine Implementation Changes

**File**: `source/Engine.cpp`
**Changes**: Implement camera controller support and fix crash points

Add include at top:
```cpp
#include "CameraController.h"
```

Add new methods (after existing methods, before destructor or at end):
```cpp
void Engine::SetCameraController(CameraController *controller)
{
	cameraController = controller;
}

bool Engine::IsObserverMode() const
{
	return cameraController != nullptr;
}
```

**Modify `Engine::Place()` around line 399:**

Change from:
```cpp
	camera.SnapTo(flagship->Center());
```
To:
```cpp
	if(cameraController)
		camera.SnapTo(cameraController->GetTarget());
	else if(flagship)
		camera.SnapTo(flagship->Center());
	// else: camera stays at current position
```

**Modify `Engine::Step()` around lines 507-532:**

Change from:
```cpp
	if(object)
		camera.SnapTo(object->Position());
	else if(flagship)
	{
		if(isActive && !timePaused)
			camera.MoveTo(flagship->Center(), hyperspacePercentage);
	}
	// ... (line 530-532)
	else
		camera.SnapTo(camera.Center());
```
To:
```cpp
	if(object)
		camera.SnapTo(object->Position());
	else if(cameraController)
	{
		if(isActive && !timePaused)
		{
			cameraController->Step();
			camera.MoveTo(cameraController->GetTarget(), 0.);
		}
	}
	else if(flagship)
	{
		if(isActive && !timePaused)
			camera.MoveTo(flagship->Center(), hyperspacePercentage);
	}
	else
		camera.SnapTo(camera.Center());
```

**Modify `Engine::CalculateStep()` around line 1617:**

Change from:
```cpp
	if(timePaused)
	{
		ai.MovePlayer(*player.Flagship(), activeCommands);
```
To:
```cpp
	if(timePaused)
	{
		if(player.Flagship() && !cameraController)
			ai.MovePlayer(*player.Flagship(), activeCommands);
```

**Modify `Engine::CalculateStep()` around lines 1626-1632:**

Change from:
```cpp
	if(flagship && !timePaused)
	{
		if(flagship->IsHyperspacing())
			hyperspacePercentage = flagship->GetHyperspacePercentage() / 100.;
		else
			hyperspacePercentage = 0.;
		newCamera.MoveTo(flagship->Center(), hyperspacePercentage);
	}
```
To:
```cpp
	if(cameraController && !timePaused)
	{
		newCamera.MoveTo(cameraController->GetTarget(), 0.);
	}
	else if(flagship && !timePaused)
	{
		if(flagship->IsHyperspacing())
			hyperspacePercentage = flagship->GetHyperspacePercentage() / 100.;
		else
			hyperspacePercentage = 0.;
		newCamera.MoveTo(flagship->Center(), hyperspacePercentage);
	}
```

**Modify `Engine::EnterSystem()` around line 1439-1441:**

Change from:
```cpp
	Ship *flagship = player.Flagship();
	if(!flagship)
		return;
```
To:
```cpp
	Ship *flagship = player.Flagship();
	bool isObserver = (cameraController != nullptr);

	// In observer mode, we don't need a flagship to enter a system
	if(!flagship && !isObserver)
		return;
```

Then, throughout `EnterSystem()`, guard flagship usages. Change line ~1568:
```cpp
	camera.SnapTo(flagship->Center(), true);
```
To:
```cpp
	if(flagship)
		camera.SnapTo(flagship->Center(), true);
	else if(cameraController)
		camera.SnapTo(cameraController->GetTarget());
```

#### 2.3 Provide Ships to Camera Controller

**File**: `source/Engine.cpp`
**Changes**: Update camera controller with ship list

In `Engine::CalculateStep()`, after ships are processed (around line 1700, before `SpawnFleets()`):
```cpp
	// Update camera controller with current ships
	if(cameraController)
	{
		cameraController->SetShips(ships);
		cameraController->SetStellarObjects(player.GetSystem()->Objects());
	}
```

### Success Criteria:

#### Automated Verification:
- [ ] Project compiles: `cmake --build build`
- [ ] No crashes when running game normally (regression test)

#### Manual Verification:
- [ ] Start a normal game, fly around - camera still works correctly
- [ ] Engine changes don't affect normal gameplay

---

## Phase 3: ObserverPanel & Menu Integration

### Overview
Create the ObserverPanel class and integrate it into the main menu.

### Changes Required:

#### 3.1 ObserverPanel Header

**File**: `source/ObserverPanel.h` (NEW)
**Changes**: Create new file

```cpp
// ObserverPanel.h - Panel for observer/screensaver mode

#ifndef OBSERVER_PANEL_H_
#define OBSERVER_PANEL_H_

#include "Panel.h"

#include "Engine.h"
#include "PlayerInfo.h"

#include <memory>

class CameraController;

class ObserverPanel : public Panel {
public:
	// Create an observer panel for the given system.
	ObserverPanel();

	void Step() override;
	void Draw() override;

	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;

	// Allow fast-forward in observer mode.
	bool AllowsFastForward() const override { return true; }

private:
	void CycleCamera();
	void InitializeSystem();

private:
	PlayerInfo player;
	Engine engine;

	std::unique_ptr<CameraController> cameraController;
	int cameraMode = 0;  // 0=follow, 1=orbit, 2=free

	// For free camera movement
	bool movingUp = false;
	bool movingDown = false;
	bool movingLeft = false;
	bool movingRight = false;

	// Auto-save timer
	int saveTimer = 0;
	static const int SAVE_INTERVAL = 60 * 60 * 5;  // 5 minutes at 60 FPS
};

#endif
```

#### 3.2 ObserverPanel Implementation

**File**: `source/ObserverPanel.cpp` (NEW)
**Changes**: Create new file

```cpp
// ObserverPanel.cpp

#include "ObserverPanel.h"

#include "CameraController.h"
#include "FollowShipCamera.h"
#include "OrbitPlanetCamera.h"
#include "FreeCamera.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "Random.h"
#include "Screen.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "text/FontSet.h"

#include <vector>

using namespace std;

ObserverPanel::ObserverPanel()
	: player(PlayerInfo::CreateObserver(nullptr)), engine(player)
{
	SetIsFullScreen(true);

	InitializeSystem();

	// Start with follow ship camera
	cameraController = make_unique<FollowShipCamera>();
	engine.SetCameraController(cameraController.get());
}

void ObserverPanel::InitializeSystem()
{
	// Pick a random inhabited system with fleets
	vector<const System *> candidates;
	for(const auto &it : GameData::Systems())
	{
		const System &system = it.second;
		if(system.IsValid() && !system.Fleets().empty() && system.IsInhabited(nullptr))
			candidates.push_back(&system);
	}

	const System *system = nullptr;
	if(!candidates.empty())
		system = candidates[Random::Int(candidates.size())];
	else
	{
		// Fallback: any valid system
		for(const auto &it : GameData::Systems())
			if(it.second.IsValid())
			{
				system = &it.second;
				break;
			}
	}

	if(system)
	{
		player.SetSystem(*system);
		engine.EnterSystem();

		Messages::Add({"Observing the " + system->DisplayName() + " system.",
			GameData::MessageCategories().Get("info")});
	}
}

void ObserverPanel::Step()
{
	// Handle free camera movement
	if(cameraMode == 2)  // Free camera
	{
		FreeCamera *freeCam = dynamic_cast<FreeCamera *>(cameraController.get());
		if(freeCam)
		{
			double dx = (movingRight ? 1. : 0.) - (movingLeft ? 1. : 0.);
			double dy = (movingDown ? 1. : 0.) - (movingUp ? 1. : 0.);
			freeCam->SetMovement(dx, dy);
		}
	}

	engine.Wait();
	engine.Step(true);  // Always active
	engine.Go();

	// Auto-save periodically
	++saveTimer;
	if(saveTimer >= SAVE_INTERVAL)
	{
		saveTimer = 0;
		player.Save();
	}
}

void ObserverPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	engine.Draw();

	// Draw minimal observer HUD
	const Font &font = FontSet::Get(14);
	Color dimColor(0.5f, 0.5f, 0.5f, 1.f);
	Color brightColor(0.8f, 0.8f, 0.8f, 1.f);

	// Camera mode in top-left
	Point hudPos = Screen::TopLeft() + Point(20., 20.);
	string modeText = "Mode: " + cameraController->ModeName();
	font.Draw(modeText, hudPos, dimColor);

	// Target name below mode
	string targetName = cameraController->TargetName();
	if(!targetName.empty())
	{
		hudPos.Y() += 20.;
		font.Draw("Target: " + targetName, hudPos, brightColor);
	}

	// System name
	if(player.GetSystem())
	{
		hudPos.Y() += 20.;
		font.Draw("System: " + player.GetSystem()->DisplayName(), hudPos, dimColor);
	}

	// Controls hint at bottom
	Point hintPos = Screen::BottomLeft() + Point(20., -30.);
	font.Draw("Tab: cycle camera | Space: new target | ESC: exit", hintPos, dimColor);
}

bool ObserverPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_ESCAPE || command.Has(Command::MENU))
	{
		GetUI()->Pop(this);
		return true;
	}

	if(key == SDLK_TAB)
	{
		CycleCamera();
		return true;
	}

	if(key == ' ')
	{
		// Select new target in current mode
		if(cameraMode == 0)
		{
			FollowShipCamera *follow = dynamic_cast<FollowShipCamera *>(cameraController.get());
			if(follow)
				follow->CycleTarget();
		}
		else if(cameraMode == 1)
		{
			OrbitPlanetCamera *orbit = dynamic_cast<OrbitPlanetCamera *>(cameraController.get());
			if(orbit)
				orbit->CycleTarget();
		}
		return true;
	}

	// WASD / Arrow keys for free camera
	if(key == 'w' || key == SDLK_UP)
	{
		movingUp = true;
		return true;
	}
	if(key == 's' || key == SDLK_DOWN)
	{
		movingDown = true;
		return true;
	}
	if(key == 'a' || key == SDLK_LEFT)
	{
		movingLeft = true;
		return true;
	}
	if(key == 'd' || key == SDLK_RIGHT)
	{
		movingRight = true;
		return true;
	}

	return false;
}

void ObserverPanel::CycleCamera()
{
	cameraMode = (cameraMode + 1) % 3;

	// Get current position to pass to new camera
	Point currentPos = cameraController->GetTarget();

	switch(cameraMode)
	{
	case 0:
		cameraController = make_unique<FollowShipCamera>();
		break;
	case 1:
		cameraController = make_unique<OrbitPlanetCamera>();
		break;
	case 2:
		{
			auto freeCam = make_unique<FreeCamera>();
			freeCam->SetPosition(currentPos);
			cameraController = std::move(freeCam);
		}
		break;
	}

	engine.SetCameraController(cameraController.get());

	// Provide current data to new controller
	if(player.GetSystem())
		cameraController->SetStellarObjects(player.GetSystem()->Objects());
}
```

Note: We also need to handle key release for free camera. Add to header:
```cpp
	bool KeyUp(SDL_Keycode key) override;
```

Add to implementation:
```cpp
bool ObserverPanel::KeyUp(SDL_Keycode key)
{
	if(key == 'w' || key == SDLK_UP)
	{
		movingUp = false;
		return true;
	}
	if(key == 's' || key == SDLK_DOWN)
	{
		movingDown = false;
		return true;
	}
	if(key == 'a' || key == SDLK_LEFT)
	{
		movingLeft = false;
		return true;
	}
	if(key == 'd' || key == SDLK_RIGHT)
	{
		movingRight = false;
		return true;
	}
	return false;
}
```

#### 3.3 Menu Panel Integration

**File**: `source/MenuPanel.cpp`
**Changes**: Add Observe button handler

Add include at top:
```cpp
#include "ObserverPanel.h"
```

In `MenuPanel::KeyDown()` (around line 200, in the else-if chain), add:
```cpp
	else if(key == 'o')
	{
		// Launch observer mode
		GetUI()->Push(new ObserverPanel());
	}
```

**File**: `data/_ui/interfaces.txt`
**Changes**: Add Observe button to main menu

In the `interface "main menu"` section (around line 290), add a new button:
```
	button o "_Observe Galaxy"
		center -285 185
		dimensions 120 30
```

Position it below the Quit button (adjust coordinates as needed for layout).

### Success Criteria:

#### Automated Verification:
- [ ] Project compiles: `cmake --build build`
- [ ] Game launches without crash

#### Manual Verification:
- [ ] "Observe Galaxy" button appears in main menu
- [ ] Clicking button enters observer mode
- [ ] Camera follows ships
- [ ] Tab cycles between camera modes
- [ ] Space selects new target
- [ ] ESC returns to menu
- [ ] No crashes during 5+ minutes of observation

---

## Phase 4: Polish & Auto-Save

### Overview
Ensure save files are compatible with original game and add finishing touches.

### Changes Required:

#### 4.1 Save Compatibility

**File**: `source/PlayerInfo.cpp`
**Changes**: Ensure observer saves are loadable

In `PlayerInfo::CreateObserver()`, ensure we set enough state to be saveable:
```cpp
PlayerInfo PlayerInfo::CreateObserver(const System *system)
{
	PlayerInfo observer;
	observer.Clear();

	observer.firstName = "Observer";
	observer.lastName = "Mode";
	observer.system = system;

	// Must have a planet to save (CanBeSaved checks for planet)
	// Pick first inhabited planet in system, or first planet
	if(system)
	{
		for(const StellarObject &object : system->Objects())
		{
			if(object.HasValidPlanet() && object.GetPlanet()->IsInhabited())
			{
				observer.planet = object.GetPlanet();
				break;
			}
		}
		// Fallback: any planet
		if(!observer.planet)
		{
			for(const StellarObject &object : system->Objects())
			{
				if(object.HasValidPlanet())
				{
					observer.planet = object.GetPlanet();
					break;
				}
			}
		}
	}

	observer.date = GameData::GetDate();

	// Give minimal credits so player can buy a ship when loading
	observer.accounts.SetCredits(1000000);

	return observer;
}
```

#### 4.2 Fix Engine EnterSystem for Observer

**File**: `source/Engine.cpp`
**Changes**: More guards for observer mode in EnterSystem

Throughout `EnterSystem()`, add guards where flagship is dereferenced. Search for `flagship->` and add null checks or `if(flagship)` guards:

Around line 1482-1497 (wormhole handling):
```cpp
	if(flagship && usedWormhole)
	{
		// ... existing code
	}
```

Around line 1579-1586 (tutorial messages):
```cpp
	if(flagship && today <= player.StartData().GetDate() + 4)
	{
		// ... existing code
	}
```

#### 4.3 Ensure Radar Works Without Flagship

**File**: `source/Engine.cpp`
**Changes**: In radar setup, handle null flagship

In constructor and `EnterSystem()`, where radar is set up, the code already handles null flagship for radar type determination. Verify no crashes occur.

### Success Criteria:

#### Automated Verification:
- [ ] Project compiles: `cmake --build build`

#### Manual Verification:
- [ ] Observer mode runs for 5+ minutes without crash
- [ ] Auto-save creates file after 5 minutes
- [ ] Save file can be loaded in observer mode again
- [ ] Save file can be loaded as normal game (player starts landed with credits)
- [ ] No error messages about missing data when loading save

---

## Phase 5: Testing & Edge Cases

### Overview
Test edge cases and ensure robustness.

### Test Cases:

1. **Empty System**: What if selected system has no ships?
   - Camera should orbit planets or stay at origin

2. **All Ships Destroyed**: What if all ships in system are destroyed?
   - Camera should switch to orbit mode or stay at last position

3. **Quick Exit**: Enter observer, immediately press ESC
   - Should exit cleanly without crash

4. **Fast Forward**: Use fast-forward in observer mode
   - Should work, ships move faster

5. **Long Duration**: Run for 30+ minutes
   - Should not crash, memory should be stable

### Success Criteria:

#### Manual Verification:
- [ ] All test cases pass
- [ ] No memory leaks (check with profiler if available)
- [ ] Performance is acceptable (60 FPS maintained)

---

## Post-MVP Features (IMPLEMENTED)

### Time Controls ✓

**Implemented in:** `ObserverPanel.cpp`, `Panel.h/cpp`, `main.cpp`

- Added `GetSpeedMultiplier()` virtual method to Panel class
- ObserverPanel overrides with 5 speed levels: 1x, 2x, 3x, 5x, 10x
- Keys: F cycles speeds, 1-5 for direct selection
- main.cpp checks `panelSpeed` and skips frames accordingly
- HUD shows current speed with color highlighting

### System Auto-Switching ✓

**Implemented in:** `ObserverPanel.cpp/h`

- Tracks `systemTimer` (time in current system) and `quietTimer` (time since last activity)
- Monitors ShipEvents for DESTROY, DISABLE, PROVOKE to detect combat activity
- Auto-switches when:
  - 3 minutes max in one system (MAX_SYSTEM_TIME)
  - 45 seconds of quiet with no recent activity (QUIET_THRESHOLD)
- Manual switch: N key
- Resets camera to follow mode on switch

### Informative HUD ✓ (Redesigned)

**Implemented in:** `ObserverPanel.cpp`, `Engine.h/cpp`, `Engine.cpp`

**Architecture Changes:**
- Engine::Draw() now skips full HUD (`hud->Draw(info)`) when in observer mode (`cameraController != nullptr`)
- Radar, minimap, and messages still draw (useful for observer)
- Player-specific elements (status bars, credits, fuel/energy/heat) are completely hidden

**Clean Observer Panel (top-right):**
- Semi-transparent dark panel background using `FillShader::Fill()`
- "OBSERVER" title in larger font
- System name
- Activity indicator (COMBAT in red, Active in green, Quiet in dim)
- Ship count, Destroyed count, Disabled count
- Session time (hh:mm:ss format for long sessions)

**Camera Info (below radar, top-left):**
- Camera mode name
- Target name (when following a ship)
- Speed indicator (only shown when > 1x, in accent color)

**Controls hint (bottom):**
- Clean, minimal hint text

**Bug Fixes:**
- Speed controls now work (checks menuPanels stack, not just gamePanels)
- Auto-save now works (added filePath to NewObserver)

### Enhanced AI Activity (Future)

Use existing AI commands to spawn more interesting behavior:
- Mining fleets in asteroid fields
- Patrol routes between planets
- Trade convoys between systems (if multi-system isn't needed for it)

---

## References

- Research document: `thoughts/shared/research/2025-12-04-galactic-observer-mode-feasibility.md`
- Camera system: `source/Camera.h:36-41`, `source/Engine.cpp:507-511`
- Menu panel: `source/MenuPanel.cpp:184-219`
- Interface system: `source/Interface.h`, `data/_ui/interfaces.txt`
- Time controls: `source/main.cpp:405-426`
- CMake structure: `source/CMakeLists.txt:14-449`
