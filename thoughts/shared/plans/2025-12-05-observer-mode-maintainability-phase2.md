# Observer Mode Maintainability - Phase 2 Implementation Plan

## Overview

This plan implements additional maintainability improvements for observer mode, building on the completed Phase 1-6 from the original maintainability plan. The focus is on further reducing merge conflict surface in Engine.cpp through code consolidation and the introduction of a Camera Adapter pattern.

## Current State Analysis

The original maintainability plan (Phases 1-6) has been implemented:
- Helper methods extracted: `PopulateShipStatusBars()`, `PopulateTargetScanInfo()`
- PanelUtils created for shared zoom handling
- Speed control unified in main.cpp
- Tests added for CameraController and PanelUtils
- README updated

However, Engine.cpp still has **21 scattered `cameraController` references** creating merge conflict risk:
- 2 locations create `bool isObserver = (cameraController != nullptr)` instead of using `IsObserverMode()`
- 3 locations have redundant camera snap patterns
- Draw() has multiple observer checks that could be consolidated
- The overall structure could benefit from a Camera Adapter pattern

### Key Discoveries:
- `Engine.cpp:759` and `Engine.cpp:1587` both create local `isObserver` variable
- Camera snap pattern appears at `Engine.cpp:400-403`, `Engine.cpp:512-519`, `Engine.cpp:1722-1725`
- `IsObserverMode()` already exists at `Engine.cpp:1402` but isn't used consistently
- Draw() checks `hideInterface` at line 1192 and `!cameraController` at line 1298

## Desired End State

After completing this plan:
1. All observer checks use `IsObserverMode()` consistently
2. Camera positioning consolidated into a single helper method
3. Draw() observer logic consolidated into early exit pattern
4. New `CameraSource` abstraction dramatically reduces Engine.cpp conditionals from 21 to ~8
5. All existing functionality preserved

**Verification**: `cmake --build build/macos-arm` succeeds, game runs correctly, observer mode works as before.

## What We're NOT Doing

- Changing observer mode features or behavior
- Modifying the CameraController interface (it's well-designed)
- Adding new dependencies
- Changing save file format
- Modifying ObserverPanel logic

## Implementation Approach

We'll use **incremental refactoring** with three phases of increasing complexity:
1. Quick cleanups (trivial, immediate value)
2. Helper extraction (moderate, reduces repetition)
3. Camera Adapter pattern (significant, but dramatically reduces scatter)

Each phase produces a working build and can be committed independently.

## Notes for Implementing Agent

### Critical Context

1. **Line numbers are approximate** - Use grep patterns to find actual locations:
   - Search for `bool isObserver = (cameraController` for observer checks
   - Search for `camera.SnapTo(cameraController` for camera snap patterns
   - Search for `IsObserverMode` to find the existing method

2. **Phase 2 → Phase 3 overlap**: The `UpdateCameraPosition()` helper from Phase 2 becomes partially redundant after Phase 3. You have two options:
   - **Option A**: Keep it as a lower-level helper that `CameraSource` implementations can use
   - **Option B**: Remove it during Phase 3 refactoring

   Either approach is fine - use your judgment based on code clarity.

3. **ObserverPanel ownership**: In Phase 3, `ObserverPanel` needs to own both:
   - The `CameraController` (which it already does via `unique_ptr`)
   - The new `ObserverCameraSource` (add as another `unique_ptr`)

   The destructor already waits for Engine's background thread before cleanup - this pattern must continue to prevent use-after-free.

4. **API migration**: Phase 3 replaces `SetCameraController()` with `SetCameraSource()`. A quick grep confirms only `ObserverPanel` calls `SetCameraController()`, so no other callers need updating.

5. **Code style** - Follow Endless Sky conventions:
   - Tabs for indentation
   - Copyright header format matches existing files (see any existing .cpp/.h)
   - Check existing CameraController files for patterns

---

## Phase 1: Quick Cleanups

### Overview
Replace inline observer checks with the existing `IsObserverMode()` method and consolidate Draw() observer logic.

### Changes Required:

#### 1.1 Use IsObserverMode() in Step()

**File**: `source/Engine.cpp`
**Changes**: Replace inline check with method call

Find (around line 759):
```cpp
bool isObserver = (cameraController != nullptr);
```

Replace with:
```cpp
bool isObserver = IsObserverMode();
```

#### 1.2 Use IsObserverMode() in EnterSystem()

**File**: `source/Engine.cpp`
**Changes**: Replace inline check with method call

Find (around line 1587):
```cpp
bool isObserver = (cameraController != nullptr);
```

Replace with:
```cpp
bool isObserver = IsObserverMode();
```

#### 1.3 Consolidate Draw() Observer Checks

**File**: `source/Engine.cpp`
**Changes**: Combine the two observer-related checks in Draw() into a cleaner pattern

Current code has separate checks:
- Line 1192: `if(hideInterface) return;`
- Line 1298: `if(!cameraController)` for ammo/escort

Refactor Draw() to use a more explicit pattern. Find (around line 1297-1307):
```cpp
	// Skip ammo and escort display in observer mode (no player ship)
	if(!cameraController)
	{
		// Draw ammo status.
		double ammoIconWidth = hud->GetValue("ammo icon width");
		double ammoIconHeight = hud->GetValue("ammo icon height");
		ammoDisplay.Draw(hud->GetBox("ammo"), Point(ammoIconWidth, ammoIconHeight));

		// Draw escort status.
		escorts.Draw(hud->GetBox("escorts"));
	}
```

Replace with:
```cpp
	// Draw ammo and escort status (only in normal gameplay, not observer mode)
	if(!IsObserverMode())
	{
		// Draw ammo status.
		double ammoIconWidth = hud->GetValue("ammo icon width");
		double ammoIconHeight = hud->GetValue("ammo icon height");
		ammoDisplay.Draw(hud->GetBox("ammo"), Point(ammoIconWidth, ammoIconHeight));

		// Draw escort status.
		escorts.Draw(hud->GetBox("escorts"));
	}
```

### Success Criteria:

#### Automated Verification:
- [ ] Code compiles: `cmake --build build/macos-arm`
- [ ] Tests pass: `ctest --test-dir build/macos-arm`

#### Manual Verification:
- [ ] Observer mode HUD displays correctly
- [ ] Normal gameplay HUD unchanged

**Implementation Note**: Commit after this phase with message: `refactor: use IsObserverMode() consistently in Engine.cpp`

---

## Phase 2: Extract Camera Position Helper

### Overview
Consolidate the repeated camera snap pattern into a single helper method.

### Changes Required:

#### 2.1 Add Helper Method Declaration

**File**: `source/Engine.h`
**Changes**: Add private helper method declaration

Find the private section (around line 225, near other helper methods):
```cpp
	void PopulateShipStatusBars(const Ship &ship, bool showOverheatBlink);
	void PopulateTargetScanInfo(const Ship &target, bool perfectSensors, const Ship *scanner);
```

Add after:
```cpp
	// Update camera position based on observer mode, landed state, or flagship
	void UpdateCameraPosition(const shared_ptr<Ship> &flagship, const StellarObject *landedObject, bool snap);
```

#### 2.2 Add Helper Method Implementation

**File**: `source/Engine.cpp`
**Changes**: Add implementation after the existing helper methods (around line 1578, after PopulateTargetScanInfo)

```cpp
void Engine::UpdateCameraPosition(const shared_ptr<Ship> &flagship, const StellarObject *landedObject, bool snap)
{
	if(IsObserverMode())
	{
		// Observer mode: always use camera controller's target
		camera.SnapTo(cameraController->GetTarget());
	}
	else if(landedObject)
	{
		// Landed on a planet: center on the stellar object
		camera.SnapTo(landedObject->Position());
	}
	else if(flagship)
	{
		// In flight: center on flagship
		if(snap)
			camera.SnapTo(flagship->Center(), true);
		else
			camera.MoveTo(flagship->Center(), hyperspacePercentage);
	}
	// else: camera stays at current position
}
```

#### 2.3 Refactor Place() to Use Helper

**File**: `source/Engine.cpp`
**Changes**: Replace camera positioning in Place()

Find (around line 400-404):
```cpp
	if(cameraController)
		camera.SnapTo(cameraController->GetTarget());
	else if(flagship)
		camera.SnapTo(flagship->Center());
	// else: camera stays at current position
```

Replace with:
```cpp
	UpdateCameraPosition(flagship, nullptr, true);
```

#### 2.4 Refactor Step() Camera Update

**File**: `source/Engine.cpp`
**Changes**: Replace camera positioning in Step()

Find (around line 512-548):
```cpp
	// In observer mode, use the camera controller even if landed on a planet.
	if(cameraController)
	{
		if(isActive && !timePaused)
		{
			cameraController->Step();
			// Use SnapTo to prevent camera lag/motion blur in observer mode.
			// The camera controller handles its own smoothing if desired.
			camera.SnapTo(cameraController->GetTarget());
		}
	}
	else if(object)
		camera.SnapTo(object->Position());
	else if(flagship)
	{
		if(isActive && !timePaused)
			camera.MoveTo(flagship->Center(), hyperspacePercentage);
		// ... rest of flagship handling
```

This is more complex because the flagship branch has additional logic. Refactor to:
```cpp
	// Step camera controller if in observer mode
	if(IsObserverMode() && isActive && !timePaused)
		cameraController->Step();

	// Update camera position
	if(IsObserverMode())
	{
		if(isActive && !timePaused)
			UpdateCameraPosition(nullptr, nullptr, true);
	}
	else if(object)
		camera.SnapTo(object->Position());
	else if(flagship)
	{
		if(isActive && !timePaused)
			UpdateCameraPosition(flagship, nullptr, false);

		if(doEnterLabels)
		{
			// ... existing label creation code unchanged
		}
		// ... rest of flagship handling unchanged
```

Note: This refactor preserves the existing logic but consolidates camera positioning. The flagship branch retains its additional label/enter handling.

#### 2.5 Refactor EnterSystem() Camera Update

**File**: `source/Engine.cpp`
**Changes**: Replace camera positioning in EnterSystem()

Find (around line 1722-1725):
```cpp
	if(flagship)
		camera.SnapTo(flagship->Center(), true);
	else if(cameraController)
		camera.SnapTo(cameraController->GetTarget());
```

Replace with:
```cpp
	UpdateCameraPosition(flagship ? player.FlagshipPtr() : nullptr, nullptr, true);
```

Note: Need to use `player.FlagshipPtr()` here since we need a shared_ptr and the local `flagship` is a raw pointer.

### Success Criteria:

#### Automated Verification:
- [ ] Code compiles: `cmake --build build/macos-arm`
- [ ] Tests pass: `ctest --test-dir build/macos-arm`

#### Manual Verification:
- [ ] Camera follows ships correctly in observer mode
- [ ] Camera follows flagship correctly in normal gameplay
- [ ] Landing on planets centers camera correctly
- [ ] System entry positions camera correctly

**Implementation Note**: Commit after this phase with message: `refactor: extract UpdateCameraPosition helper in Engine.cpp`

---

## Phase 3: Camera Adapter Pattern

### Overview
Introduce a `CameraSource` abstraction that provides a unified interface for camera positioning, whether from a flagship or an observer camera controller. This dramatically reduces the number of conditionals in Engine.cpp.

### Design

The `CameraSource` class provides:
- `GetTarget()` - Camera center position
- `GetVelocity()` - For motion blur and background scrolling
- `GetShipForHUD()` - Ship to display in HUD (flagship or observed ship)
- `Step()` - Per-frame update
- `IsObserver()` - Quick check for observer-specific behavior

Two implementations:
1. `FlagshipCameraSource` - Wraps the player's flagship
2. `ObserverCameraSource` - Wraps the CameraController

Engine.cpp will use `CameraSource*` instead of checking `cameraController` everywhere.

### Changes Required:

#### 3.1 Create CameraSource Header

**File**: `source/CameraSource.h` (new file)
```cpp
/* CameraSource.h
Copyright (c) 2024 by the Endless Sky developers

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

#include "Point.h"

#include <memory>

class Ship;
class StellarObject;

// Abstract interface for providing camera position and related info to Engine.
// This allows Engine to work with either a flagship or an observer camera controller
// without scattered conditionals throughout the code.
class CameraSource {
public:
	virtual ~CameraSource() = default;

	// Get the position the camera should center on.
	virtual Point GetTarget() const = 0;

	// Get velocity for motion blur and background scrolling.
	virtual Point GetVelocity() const = 0;

	// Get the ship to display in the HUD (flagship or observed ship).
	// Returns nullptr if no ship should be displayed.
	virtual std::shared_ptr<Ship> GetShipForHUD() const = 0;

	// Per-frame update. Called when the game is active and not paused.
	virtual void Step() = 0;

	// Returns true if this is observer mode (affects HUD display, messages, etc.)
	virtual bool IsObserver() const = 0;

	// Returns true if the camera should snap (no interpolation) vs move smoothly.
	virtual bool ShouldSnap() const = 0;
};
```

#### 3.2 Create FlagshipCameraSource

**File**: `source/FlagshipCameraSource.h` (new file)
```cpp
/* FlagshipCameraSource.h
Copyright (c) 2024 by the Endless Sky developers

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

#include "CameraSource.h"

class PlayerInfo;

// CameraSource implementation that follows the player's flagship.
class FlagshipCameraSource : public CameraSource {
public:
	explicit FlagshipCameraSource(PlayerInfo &player);

	Point GetTarget() const override;
	Point GetVelocity() const override;
	std::shared_ptr<Ship> GetShipForHUD() const override;
	void Step() override;
	bool IsObserver() const override { return false; }
	bool ShouldSnap() const override { return false; }

private:
	PlayerInfo &player;
};
```

**File**: `source/FlagshipCameraSource.cpp` (new file)
```cpp
/* FlagshipCameraSource.cpp
Copyright (c) 2024 by the Endless Sky developers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FlagshipCameraSource.h"

#include "PlayerInfo.h"
#include "Ship.h"

FlagshipCameraSource::FlagshipCameraSource(PlayerInfo &player)
	: player(player)
{
}

Point FlagshipCameraSource::GetTarget() const
{
	auto flagship = player.FlagshipPtr();
	return flagship ? flagship->Center() : Point();
}

Point FlagshipCameraSource::GetVelocity() const
{
	auto flagship = player.FlagshipPtr();
	return flagship ? flagship->Velocity() : Point();
}

std::shared_ptr<Ship> FlagshipCameraSource::GetShipForHUD() const
{
	return player.FlagshipPtr();
}

void FlagshipCameraSource::Step()
{
	// Flagship camera doesn't need per-frame updates - the ship handles its own movement
}
```

#### 3.3 Create ObserverCameraSource

**File**: `source/ObserverCameraSource.h` (new file)
```cpp
/* ObserverCameraSource.h
Copyright (c) 2024 by the Endless Sky developers

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

#include "CameraSource.h"

class CameraController;

// CameraSource implementation that wraps a CameraController for observer mode.
class ObserverCameraSource : public CameraSource {
public:
	explicit ObserverCameraSource(CameraController *controller);

	Point GetTarget() const override;
	Point GetVelocity() const override;
	std::shared_ptr<Ship> GetShipForHUD() const override;
	void Step() override;
	bool IsObserver() const override { return true; }
	bool ShouldSnap() const override { return true; }

	// Access the underlying controller for observer-specific operations
	CameraController *GetController() const { return controller; }

private:
	CameraController *controller;
};
```

**File**: `source/ObserverCameraSource.cpp` (new file)
```cpp
/* ObserverCameraSource.cpp
Copyright (c) 2024 by the Endless Sky developers

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ObserverCameraSource.h"

#include "CameraController.h"

ObserverCameraSource::ObserverCameraSource(CameraController *controller)
	: controller(controller)
{
}

Point ObserverCameraSource::GetTarget() const
{
	return controller ? controller->GetTarget() : Point();
}

Point ObserverCameraSource::GetVelocity() const
{
	return controller ? controller->GetVelocity() : Point();
}

std::shared_ptr<Ship> ObserverCameraSource::GetShipForHUD() const
{
	return controller ? controller->GetObservedShip() : nullptr;
}

void ObserverCameraSource::Step()
{
	if(controller)
		controller->Step();
}
```

#### 3.4 Update CMakeLists.txt

**File**: `source/CMakeLists.txt`
**Changes**: Add new files to build

Find the alphabetically appropriate location and add:
```cmake
	CameraSource.h
	FlagshipCameraSource.cpp
	FlagshipCameraSource.h
	ObserverCameraSource.cpp
	ObserverCameraSource.h
```

#### 3.5 Update Engine.h

**File**: `source/Engine.h`
**Changes**: Replace cameraController with cameraSource

Add forward declaration (around line 35):
```cpp
class CameraSource;
```

Update the observer mode section (around line 110-118):
```cpp
	// === Observer Mode Support ===
	// These methods support running the engine without a flagship (screensaver mode).
	// The camera source provides position/velocity/ship info in a unified way.
	void SetCameraSource(CameraSource *source);
	CameraSource *GetCameraSource() const;
	bool IsObserverMode() const;
	void EnterSystem();
	size_t ShipCount() const;
	void SetHideInterface(bool hide);
	bool HideInterface() const;
```

Update private member (around line 351):
```cpp
	// Camera source for unified camera positioning (observer or flagship).
	CameraSource *cameraSource = nullptr;
	// Hide interface for clean screenshot mode.
	bool hideInterface = false;
```

Remove the `UpdateCameraPosition` helper added in Phase 2 if desired (optional - it can remain as a lower-level helper).

#### 3.6 Update Engine.cpp - Replace cameraController References

**File**: `source/Engine.cpp`
**Changes**: Update all cameraController references to use cameraSource

This is the main refactoring. Key changes:

1. **SetCameraController -> SetCameraSource** (around line 1395):
```cpp
void Engine::SetCameraSource(CameraSource *source)
{
	cameraSource = source;
}

CameraSource *Engine::GetCameraSource() const
{
	return cameraSource;
}

bool Engine::IsObserverMode() const
{
	return cameraSource && cameraSource->IsObserver();
}
```

2. **Step() camera handling** (around line 512-520):
```cpp
	// Update camera source and position
	if(cameraSource && isActive && !timePaused)
		cameraSource->Step();

	if(cameraSource && cameraSource->IsObserver())
	{
		// Observer mode: snap to target
		camera.SnapTo(cameraSource->GetTarget());
	}
	else if(object)
		camera.SnapTo(object->Position());
	else if(cameraSource)
	{
		// Flagship mode: smooth movement
		if(isActive && !timePaused)
			camera.MoveTo(cameraSource->GetTarget(), hyperspacePercentage);
		// ... rest of flagship handling
```

3. **Background velocity** (around line 595-597):
```cpp
	Point bgVelocity = timePaused ? Point() :
		(cameraSource ? cameraSource->GetVelocity() : camera.Velocity());
```

4. **HUD population** (around line 759-770):
Replace the observedShip logic:
```cpp
	bool isObserver = IsObserverMode();
	shared_ptr<Ship> hudShip = cameraSource ? cameraSource->GetShipForHUD() : nullptr;

	if(isObserver && hudShip)
	{
		// Observer mode: show observed ship in HUD
		Point shipFacingUnit = hudShip->Facing().Unit();
		info.SetSprite("player sprite", hudShip->GetSprite(), shipFacingUnit,
			hudShip->GetFrame(step), hudShip->GetSwizzle());
		PopulateShipStatusBars(*hudShip, false);
	}
	else if(hudShip && hudShip->Hull())
	{
		// Normal mode: show flagship in HUD
		// ... existing flagship HUD code
```

5. **CalculateUnpaused()** (around line 2012-2016):
```cpp
	// Update camera source with current ships
	if(cameraSource && cameraSource->IsObserver())
	{
		auto *observer = static_cast<ObserverCameraSource *>(cameraSource);
		if(observer->GetController())
		{
			observer->GetController()->SetShips(ships);
			if(playerSystem)
				observer->GetController()->SetStellarObjects(playerSystem->Objects());
		}
	}
```

#### 3.7 Update ObserverPanel.cpp

**File**: `source/ObserverPanel.cpp`
**Changes**: Use ObserverCameraSource instead of setting CameraController directly

Add include:
```cpp
#include "ObserverCameraSource.h"
```

Add member variable to ObserverPanel.h:
```cpp
	std::unique_ptr<ObserverCameraSource> cameraSource;
```

Update constructor to create ObserverCameraSource:
```cpp
	cameraSource = make_unique<ObserverCameraSource>(cameraController.get());
	engine.SetCameraSource(cameraSource.get());
```

Update destructor and any place that sets the camera controller to also update the source.

### Success Criteria:

#### Automated Verification:
- [ ] Code compiles: `cmake --build build/macos-arm`
- [ ] Tests pass: `ctest --test-dir build/macos-arm`
- [ ] New files added to build

#### Manual Verification:
- [ ] Observer mode works: camera follows ships, HUD displays correctly
- [ ] Normal gameplay works: flagship control, HUD displays correctly
- [ ] System switching works in observer mode
- [ ] Camera modes cycle correctly (Tab key)

**Implementation Note**: This is the largest change. Test thoroughly before committing.

Commit after this phase with message: `refactor: introduce CameraSource adapter pattern for observer mode`

---

## Testing Strategy

### Automated Tests:
- Existing CameraController tests continue to pass
- PanelUtils tests continue to pass
- Build succeeds without warnings

### Manual Testing Steps:
1. Launch game, click "Observe" on main menu
2. Verify HUD displays ship stats correctly
3. Test camera modes (Tab key)
4. Test speed controls (1-5 keys)
5. Test system switching (N/P keys)
6. Exit to menu (Esc)
7. Start normal game, verify no regressions
8. Land on planet, verify camera centers correctly
9. Take off, verify camera follows flagship

## Performance Considerations

- Virtual dispatch overhead is negligible (called once per frame)
- No new allocations in hot paths
- CameraSource pointers are non-owning, no ownership overhead

## Migration Notes

- `Engine::SetCameraController()` is replaced by `Engine::SetCameraSource()`
- ObserverPanel must create and own an `ObserverCameraSource`
- MainPanel does not need changes (Engine creates FlagshipCameraSource internally, or we can leave it null for backward compatibility)

## References

- Research document: `thoughts/shared/research/2025-12-05-observer-mode-maintainability-analysis.md`
- Original maintainability plan: `thoughts/shared/plans/2025-12-05-observer-mode-maintainability.md`
- Engine.cpp observer integration: `source/Engine.cpp`
- CameraController interface: `source/CameraController.h`

## Implementation Order Summary

Execute phases in order:

1. **Phase 1**: Quick cleanups - use `IsObserverMode()` consistently
2. **Phase 2**: Extract `UpdateCameraPosition()` helper
3. **Phase 3**: Introduce CameraSource adapter pattern

Each phase produces a working build. Commit after each phase.

**Expected Impact**:
- Phase 1: Reduces risk of inconsistent behavior, minor cleanup
- Phase 2: Reduces 3 code blocks to 3 single-line calls
- Phase 3: Reduces Engine.cpp cameraController references from 21 to ~8, dramatically improving merge conflict surface
