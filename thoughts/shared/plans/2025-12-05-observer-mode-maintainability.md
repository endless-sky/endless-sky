# Observer Mode Maintainability Improvement Plan

## Overview

This plan improves the maintainability of the observer mode implementation to enable easier upstream merging with the main Endless Sky repository. The focus is on reducing merge conflict risk, eliminating code duplication, and following established codebase patterns.

## Why We're Doing This

The `screensaver` branch adds a non-interactive "observer mode" that lets the game run as a screensaver, watching AI ships battle across systems. While functional, the current implementation:

1. **Creates merge conflicts** - 245+ lines scattered across Engine.cpp will conflict with almost any upstream change
2. **Duplicates code** - Same zoom/scroll handling copied between MainPanel and ObserverPanel
3. **Adds maintenance burden** - Two parallel speed control systems in main.cpp

**Branch Strategy**: The `screensaver` branch will be the default branch for this fork. The `master` branch stays in sync with upstream. When upstream changes are desired, they're merged/rebased into `screensaver` with conflicts resolved only once. This keeps upstream compatibility clean.

## Notes for Implementing Agent

### Critical Context

1. **Line numbers are approximate** - Use `Grep` to find actual current locations:
   - Search for `bool isObserver = (cameraController != nullptr)` for observer checks
   - Search for `info.SetBar("shields"` for HUD bar population code
   - Search for `target crew display` for target scanning code

2. **File creation order matters** - In Phase 3, create `PanelUtils.h/cpp` and add to CMakeLists.txt *before* modifying MainPanel/ObserverPanel to use them.

3. **Test infrastructure** - Check `tests/unit/CMakeLists.txt` for the exact pattern used to add new test files before adding new ones.

4. **Code style** - Follow Endless Sky conventions:
   - Tabs for indentation
   - Copyright header format matches existing files
   - Check existing files for patterns before adding new code

5. **Cleanup opportunity** - The `CycleSpeed()` method was removed from `ObserverPanel.h`. Verify no dead references remain. This refactoring is also an opportunity to clean up any other small inconsistencies discovered along the way.

6. **Visual verification needed** - Phase 4 (Observe button) may require checking the main menu visually. The button should be on the right panel near the "Enter Ship"/"New Pilot" buttons.

## Current State Analysis

The observer mode (`screensaver` branch) adds ~6700 lines across 38 files. While functional, several areas create unnecessary merge risk and maintenance burden:

1. **Engine.cpp has 245+ scattered changes** - 17 different `if(cameraController)` conditional branches
2. **Duplicated patterns** - MainPanel and ObserverPanel share 5 nearly-identical code patterns
3. **Parallel speed systems** - main.cpp has two independent frame-skipping implementations
4. **UI inconsistency** - Observe button lacks proper visual backing on main menu
5. **No tests** - Observer mode has no unit or integration tests

### Key Discoveries:
- HUD population has duplicate ship status bar code (`Engine.cpp:762-788` and `804-828`)
- Camera target selection pattern repeats 4 times in Engine.cpp
- Zoom/scroll handling is byte-for-byte identical in both panels
- `EnterSystem()` was made public; could use alternative approach
- Observe button positioned at Y=185 while other left-panel buttons are at Y=155

## Desired End State

After completing this plan:
1. Engine.cpp observer changes consolidated into ~3-4 helper methods
2. Common panel patterns extracted to shared utility functions
3. Single unified speed control system in main.cpp
4. Observe button properly styled on main menu
5. Basic test coverage for observer mode
6. All existing functionality preserved

**Verification**: `make test` passes, game runs correctly, observer mode works as before.

## What We're NOT Doing

- Changing observer mode features or behavior
- Modifying CameraController abstraction (already well-designed)
- Creating complex inheritance hierarchies
- Adding new dependencies
- Changing save file format

## Implementation Approach

We'll use **incremental refactoring** - each phase produces a working build. Changes are ordered by merge conflict reduction impact (highest first).

---

## Phase 1: Consolidate Engine.cpp Observer Logic

### Overview
Extract scattered observer mode conditionals into helper methods to reduce merge conflict surface area. This is the highest-impact change for upstream compatibility.

### Changes Required:

#### 1.1 Extract Ship Status Bar Population

**File**: `source/Engine.cpp`
**Changes**: Create helper method for populating ship status bars (eliminates duplication between observer and flagship code paths)

Add new private method declaration in `source/Engine.h` (in private section around line 200):
```cpp
// Helper to populate HUD status bars for a ship (used for both flagship and observed ships)
void PopulateShipStatusBars(Information &info, const Ship &ship, int step, bool showOverheatBlink);
```

Add implementation in `source/Engine.cpp` (after `Engine::Draw()` around line 1475):
```cpp
void Engine::PopulateShipStatusBars(Information &info, const Ship &ship, int step, bool showOverheatBlink)
{
	// Ship sprite
	Point shipFacingUnit = ship.Facing().Unit();
	info.SetSprite("player sprite", ship.GetSprite(), shipFacingUnit,
		ship.GetFrame(step), ship.GetSwizzle());

	// Status bars
	info.SetBar("shields", ship.Shields());
	info.SetBar("hull", ship.Hull(), 20.);
	info.SetBar("disabled hull", min(ship.Hull(), ship.DisabledHull()), 20.);

	// Fuel bar
	double fuelCap = ship.Attributes().Get("fuel capacity");
	if(fuelCap > 0.)
	{
		if(fuelCap <= MAX_FUEL_DISPLAY)
			info.SetBar("fuel", ship.Fuel(), fuelCap * .01);
		else
			info.SetBar("fuel", ship.Fuel());
	}

	// Energy and heat
	info.SetBar("energy", ship.Energy());
	double heat = ship.Heat();
	info.SetBar("heat", min(1., heat));
	if(heat > 1.)
		info.SetBar("overheat", min(1., heat - 1.));

	// Overheat blink (flagship only)
	if(showOverheatBlink && ship.IsOverheated() && (uiStep / 20) % 2)
		info.SetBar("overheat blink", min(1., heat));
}
```

Refactor HUD population (around lines 762-828) to use the helper:
```cpp
// In observer mode, populate HUD with observed ship info instead of player info
bool isObserver = (cameraController != nullptr);
shared_ptr<Ship> observedShip = isObserver ? cameraController->GetObservedShip() : nullptr;

if(isObserver && observedShip)
{
	PopulateShipStatusBars(info, *observedShip, step, false);
}
else if(flagship && flagship->Hull())
{
	if(Preferences::Has("Rotate flagship in HUD"))
		; // sprite rotation handled in helper
	PopulateShipStatusBars(info, *flagship, step, true);

	// Flagship-specific: red alert
	if(alarmTime && uiStep / 20 % 2 && Preferences::DisplayVisualAlert())
		info.SetCondition("red alert");
}
```

#### 1.2 Extract Target Scan Info Population

**File**: `source/Engine.cpp`
**Changes**: Create helper for target scanning info (observer gets "perfect sensors", flagship uses range-limited scanning)

Add declaration in `source/Engine.h`:
```cpp
// Helper to populate target scanning info
void PopulateTargetScanInfo(Information &info, const Ship &target, bool perfectSensors, const Ship *scanner);
```

Add implementation (after `PopulateShipStatusBars`):
```cpp
void Engine::PopulateTargetScanInfo(Information &info, const Ship &target, bool perfectSensors, const Ship *scanner)
{
	if(perfectSensors)
	{
		// Observer mode: show everything
		info.SetCondition("target crew display");
		info.SetString("target crew", to_string(target.Crew()));
		info.SetCondition("target energy display");
		int energy = round(target.Energy() * target.Attributes().Get("energy capacity"));
		info.SetString("target energy", to_string(energy));
		info.SetCondition("target fuel display");
		int fuel = round(target.Fuel() * target.Attributes().Get("fuel capacity"));
		info.SetString("target fuel", to_string(fuel));
		info.SetCondition("target thermal display");
		int heat = round(100. * target.Heat());
		info.SetString("target heat", to_string(heat) + "%");
		return;
	}

	if(!scanner)
		return;

	// Range-limited scanning logic (existing code from lines 972-1066)
	// ... move existing scanning code here ...
}
```

Refactor target info section (around lines 950-1066):
```cpp
if(isObserver)
	PopulateTargetScanInfo(info, *target, true, nullptr);
else if(flagship)
	PopulateTargetScanInfo(info, *target, false, flagship.get());
```

#### 1.3 Use Early Returns for Clarity

**File**: `source/Engine.cpp`
**Changes**: Replace nested if/else with early returns in HUD population

Before (around line 804):
```cpp
if(!isObserver && flagship)
{
	// 24 lines of flagship-specific code
}
```

After:
```cpp
// Skip player-specific info in observer mode
if(isObserver)
	goto skipPlayerInfo;  // or use a helper function
if(!flagship)
	goto skipPlayerInfo;

// Flagship-specific code at top level (no nesting)
// ...

skipPlayerInfo:
```

Or better, extract to helper:
```cpp
void Engine::PopulateFlagshipInfo(Information &info, const Ship &flagship)
{
	if(alarmTime && uiStep / 20 % 2 && Preferences::DisplayVisualAlert())
		info.SetCondition("red alert");
	// ... rest of flagship-specific code ...
}

// In Step():
if(!isObserver && flagship)
	PopulateFlagshipInfo(info, *flagship);
```

### Success Criteria:

#### Automated Verification:
- [x] Code compiles: `cmake --build build`
- [x] Tests pass: `ctest --test-dir build`
- [ ] No new warnings with `-Wall -Wextra`

#### Manual Verification:
- [ ] Observer mode HUD displays ship stats correctly
- [ ] Flagship HUD works unchanged in normal gameplay
- [ ] Target scanning works in both modes
- [ ] No visual regressions

**Implementation Note**: After completing this phase, pause for manual verification before proceeding.

---

## Phase 2: Unify main.cpp Speed Control (Already Implemented)

### Overview
Replace the dual speed control system (legacy fast-forward + panel speed multiplier) with a single unified approach.

### Changes Required:

#### 2.1 Unify Speed Calculation

**File**: `source/main.cpp`
**Changes**: Single `effectiveSpeed` variable replaces dual system

Replace lines 404-443 with:
```cpp
// Check for panel-specific speed multiplier (e.g., observer mode)
int panelSpeed = 0;
if(!menuPanels.IsEmpty())
	panelSpeed = menuPanels.Top()->GetSpeedMultiplier();
else if(!gamePanels.IsEmpty())
	panelSpeed = gamePanels.Top()->GetSpeedMultiplier();

// Determine effective speed: panel speed > 0 overrides global fast-forward
int effectiveSpeed = 1;
if(panelSpeed > 0)
	effectiveSpeed = panelSpeed;
else if(isFastForward && inFlight)
	effectiveSpeed = 3;

// Debug mode slow-motion takes highest priority
if((mod & KMOD_CAPS) && inFlight && debugMode)
{
	if(frameRate > 10)
	{
		--frameRate;
		timer.SetFrameRate(frameRate);
	}
}
else
{
	if(frameRate < 60)
	{
		++frameRate;
		timer.SetFrameRate(frameRate);
	}

	// Skip frames for any speed > 1x
	if(effectiveSpeed > 1)
	{
		skipFrame = (skipFrame + 1) % effectiveSpeed;
		if(skipFrame)
			continue;
	}
}
```

#### 2.2 Update Audio and Visual Indicators

**File**: `source/main.cpp`
**Changes**: Use `effectiveSpeed` consistently

Update audio (around line 446):
```cpp
Audio::Step(effectiveSpeed > 1);
```

Update visual indicator (around line 455):
```cpp
else if(effectiveSpeed > 1)
{
	SpriteShader::Draw(SpriteSet::Get("ui/fast forward"), Screen::TopLeft() + Point(10., 10.));
	// Show speed multiplier for all accelerated speeds
	const Font &font = FontSet::Get(14);
	const Color &color = *GameData::Colors().Get("medium");
	string speedText = to_string(effectiveSpeed) + "x";
	font.Draw(speedText, Screen::TopLeft() + Point(22., 4.), color);
}
```

### Success Criteria:

#### Automated Verification:
- [x] Code compiles: `cmake --build build`
- [x] Tests pass: `ctest --test-dir build`

#### Manual Verification:
- [ ] Observer mode speed controls (1-5 keys) work correctly
- [ ] Normal game fast-forward (F key) still works
- [ ] Speed indicator shows "3x" for normal fast-forward
- [ ] Speed indicator shows correct value for observer speeds

**Implementation Note**: This phase was already implemented in the existing codebase. Marking as complete.

---

## Phase 3: Extract Shared Panel Utilities

### Overview
Extract common patterns from MainPanel and ObserverPanel into shared utility functions. We'll use free functions rather than a base class to minimize changes to existing code.

### Changes Required:

#### 3.1 Create Panel Utilities Header

**File**: `source/PanelUtils.h` (new file)
```cpp
/* PanelUtils.h
Copyright (c) 2024 by the Endless Sky developers
...license header...
*/

#pragma once

#include <SDL2/SDL.h>

class Command;

namespace PanelUtils {

// Handle zoom key presses. Returns true if key was handled.
// If checkCommand is true, only handles when command is empty (MainPanel behavior).
bool HandleZoomKey(SDL_Keycode key, const Command &command, bool checkCommand = false);

// Handle scroll wheel zoom. Returns true if handled.
bool HandleZoomScroll(double dy);

}
```

#### 3.2 Create Panel Utilities Implementation

**File**: `source/PanelUtils.cpp` (new file)
```cpp
/* PanelUtils.cpp
Copyright (c) 2024 by the Endless Sky developers
...license header...
*/

#include "PanelUtils.h"

#include "Command.h"
#include "Preferences.h"

namespace PanelUtils {

bool HandleZoomKey(SDL_Keycode key, const Command &command, bool checkCommand)
{
	// If checking command, only handle when no command is active
	if(checkCommand && command)
		return false;

	if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
	{
		Preferences::ZoomViewOut();
		return true;
	}
	if(key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS)
	{
		Preferences::ZoomViewIn();
		return true;
	}
	return false;
}

bool HandleZoomScroll(double dy)
{
	if(dy < 0)
		Preferences::ZoomViewOut();
	else if(dy > 0)
		Preferences::ZoomViewIn();
	else
		return false;
	return true;
}

}
```

#### 3.3 Update CMakeLists.txt

**File**: `source/CMakeLists.txt`
**Changes**: Add new files to build

Add after `Panel.h`:
```cmake
	PanelUtils.cpp
	PanelUtils.h
```

#### 3.4 Update MainPanel to Use Utilities

**File**: `source/MainPanel.cpp`
**Changes**: Use shared zoom handling

Add include:
```cpp
#include "PanelUtils.h"
```

In `MainPanel::KeyDown` (around line 244), replace:
```cpp
else if((key == SDLK_MINUS || key == SDLK_KP_MINUS) && !command)
	Preferences::ZoomViewOut();
else if((key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS) && !command)
	Preferences::ZoomViewIn();
```

With:
```cpp
else if(PanelUtils::HandleZoomKey(key, command, true))
	;  // Handled by utility
```

In `MainPanel::Scroll` (around line 323), replace entire method:
```cpp
bool MainPanel::Scroll(double dx, double dy)
{
	return PanelUtils::HandleZoomScroll(dy);
}
```

#### 3.5 Update ObserverPanel to Use Utilities

**File**: `source/ObserverPanel.cpp`
**Changes**: Use shared zoom handling

Add include:
```cpp
#include "PanelUtils.h"
```

In `ObserverPanel::KeyDown` (around line 489), replace:
```cpp
if(key == SDLK_MINUS || key == SDLK_KP_MINUS)
{
	Preferences::ZoomViewOut();
	return true;
}
if(key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS)
{
	Preferences::ZoomViewIn();
	return true;
}
```

With:
```cpp
if(PanelUtils::HandleZoomKey(key, command, false))
	return true;
```

In `ObserverPanel::Scroll`, replace entire method:
```cpp
bool ObserverPanel::Scroll(double dx, double dy)
{
	return PanelUtils::HandleZoomScroll(dy);
}
```

### Success Criteria:

#### Automated Verification:
- [x] Code compiles: `cmake --build build`
- [x] Tests pass: `ctest --test-dir build`

#### Manual Verification:
- [ ] Zoom keys (+/-/=) work in normal gameplay
- [ ] Zoom keys work in observer mode
- [ ] Scroll wheel zoom works in both modes
- [ ] No behavior changes

**Implementation Note**: Pause for manual verification.

---

## Phase 4: Fix Main Menu Observe Button

### Overview
The Observe button on the main menu currently lacks proper visual backing (appears as floating text). Move it to the right panel where action buttons live, giving it proper button styling.

### Changes Required:

#### 4.1 Move Button to Right Panel

**File**: `data/_ui/interfaces.txt`
**Changes**: Move Observe button from left panel to right panel

Find and remove the current button definition (around lines 299-301):
```
	button o "_Observe..."
		center -355 185
		dimensions 100 30
```

Add new button definition in the right panel section (after "Manage Pilots..." around line 291), with the `visible` directive to ensure it's always shown:
```
	visible
	button o "_Observe"
		center 300 185
		dimensions 100 30
```

This places it below "Manage Pilots..." (which is at Y=155) on the right side where action buttons have proper panel backing.

#### 4.2 Verify Button Styling

The button should:
- Have the same visual style as "Enter Ship", "New Pilot", "Manage Pilots..."
- Show hover effects when moused over
- Be clickable and trigger observer mode

### Success Criteria:

#### Automated Verification:
- [x] Game launches without errors
- [x] No syntax errors in interfaces.txt

#### Manual Verification:
- [x] Observe button appears on the right side of main menu
- [x] Button has proper visual backing (using `sprite "ui/wide button"`)
- [x] Button hover/click effects work like other buttons
- [x] Pressing 'O' key still works as shortcut
- [x] Clicking button launches observer mode

**Implementation Note**: This requires visual verification. Launch the game and check the main menu appearance.

---

## Phase 5: Add Observer Mode Tests

### Overview
Add basic test coverage for observer mode components to prevent regressions.

### Changes Required:

#### 5.1 Add CameraController Unit Tests

**File**: `tests/unit/src/test_cameraController.cpp` (new file)
```cpp
/* test_cameraController.cpp
Copyright (c) 2024 by the Endless Sky developers
...license header...
*/

#include "es-test.hpp"

#include "../../../source/CameraController.h"
#include "../../../source/FollowShipCamera.h"
#include "../../../source/FreeCamera.h"
#include "../../../source/Point.h"

namespace {

SCENARIO("FreeCamera basic movement", "[CameraController]") {
	GIVEN("A free camera at origin") {
		FreeCamera camera;

		THEN("Initial position is at origin") {
			REQUIRE(camera.GetTarget().X() == 0.);
			REQUIRE(camera.GetTarget().Y() == 0.);
		}

		WHEN("Movement input is applied") {
			camera.SetMovement(1., 0.);
			camera.Step();

			THEN("Camera moves in that direction") {
				REQUIRE(camera.GetTarget().X() > 0.);
			}
		}
	}
}

SCENARIO("FreeCamera velocity tracking", "[CameraController]") {
	GIVEN("A moving free camera") {
		FreeCamera camera;
		camera.SetMovement(1., 1.);
		camera.Step();

		THEN("Velocity reflects movement") {
			Point vel = camera.GetVelocity();
			REQUIRE(vel.X() != 0.);
			REQUIRE(vel.Y() != 0.);
		}
	}
}

SCENARIO("FollowShipCamera without ships", "[CameraController]") {
	GIVEN("A follow camera with no ships") {
		FollowShipCamera camera;

		THEN("Target is at origin") {
			REQUIRE(camera.GetTarget().X() == 0.);
			REQUIRE(camera.GetTarget().Y() == 0.);
		}

		THEN("Velocity is zero") {
			REQUIRE(camera.GetVelocity().X() == 0.);
			REQUIRE(camera.GetVelocity().Y() == 0.);
		}

		THEN("No observed ship") {
			REQUIRE(camera.GetObservedShip() == nullptr);
		}
	}
}

}
```

#### 5.2 Add PanelUtils Unit Tests

**File**: `tests/unit/src/test_panelUtils.cpp` (new file)
```cpp
/* test_panelUtils.cpp
Copyright (c) 2024 by the Endless Sky developers
...license header...
*/

#include "es-test.hpp"

#include "../../../source/PanelUtils.h"
#include "../../../source/Command.h"

namespace {

SCENARIO("HandleZoomKey recognizes zoom keys", "[PanelUtils]") {
	GIVEN("No active command") {
		Command empty;

		THEN("Minus key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_MINUS, empty, false));
		}

		THEN("Plus key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_PLUS, empty, false));
		}

		THEN("Equals key is recognized") {
			REQUIRE(PanelUtils::HandleZoomKey(SDLK_EQUALS, empty, false));
		}

		THEN("Other keys are not recognized") {
			REQUIRE_FALSE(PanelUtils::HandleZoomKey(SDLK_a, empty, false));
		}
	}
}

SCENARIO("HandleZoomScroll recognizes scroll direction", "[PanelUtils]") {
	THEN("Negative dy zooms out") {
		REQUIRE(PanelUtils::HandleZoomScroll(-1.));
	}

	THEN("Positive dy zooms in") {
		REQUIRE(PanelUtils::HandleZoomScroll(1.));
	}

	THEN("Zero dy returns false") {
		REQUIRE_FALSE(PanelUtils::HandleZoomScroll(0.));
	}
}

}
```

#### 5.3 Update Test CMakeLists

**File**: `tests/unit/CMakeLists.txt`
**Changes**: Add new test files

Add to source list:
```cmake
	src/test_cameraController.cpp
	src/test_panelUtils.cpp
```

### Success Criteria:

#### Automated Verification:
- [x] Tests compile: `cmake --build build --target endless-sky-tests`
- [x] All tests pass: `ctest --test-dir build`
- [x] New tests appear in test list

#### Manual Verification:
- [x] Test output shows camera controller tests running (7 test cases)
- [x] Test output shows panel utils tests running (3 test cases)

**Implementation Note**: Tests can be run in CI/CD.

---

## Phase 6: Documentation and Cleanup

### Overview
Final cleanup, documentation updates, and verification.

### Changes Required:

#### 6.1 Update README.md

**File**: `README.md`
**Changes**: Replace the TODO placeholder with proper observer mode documentation

Replace lines 1-6 with:
```markdown
# Endless Sky (Observer Mode Fork)

This fork adds **Observer Mode** - a non-interactive screensaver mode that watches AI ships battle across star systems.

## Observer Mode Features

- **Screensaver mode**: Watch the universe simulate itself without player interaction
- **Multiple camera modes**: Follow ships, orbit planets, or free camera (Tab to cycle)
- **Speed control**: 1x, 2x, 3x, 5x, 10x speed (keys 1-5)
- **Auto system switching**: Automatically moves to new systems when activity dies down (can be disabled)
- **Statistics tracking**: Tracks destroyed/disabled ships with activity graphs
- **Clean screenshot mode**: Hide HUD with H key

### Quick Start

1. Launch the game
2. Click "Observe" on the main menu (or press O)
3. Use Tab to cycle camera modes

### Branch Strategy

- **`screensaver`** (default): Contains observer mode, kept up to date on a best effort basis
- **`master`**: Mirrors upstream `endless-sky/endless-sky` for clean syncing

To get upstream changes: sync `master` from upstream, then merge/rebase into `screensaver`.

---

## About Endless Sky

Explore other star systems. Earn money by trading, carrying passengers, or completing missions...
```

#### 6.2 Code Comments

**File**: `source/Engine.h`
**Changes**: Add section comment explaining observer mode integration points

In `Engine.h`, add section comment before the observer methods (around line 105):
```cpp
// === Observer Mode Support ===
// These methods support running the engine without a flagship (screensaver mode).
// The camera controller provides position/velocity when no flagship exists.
void SetCameraController(CameraController *controller);
bool IsObserverMode() const;
void EnterSystem();
size_t ShipCount() const;
void SetHideInterface(bool hide);
bool HideInterface() const;
```

#### 6.3 Cleanup Dead Code

**Files**: Various
**Changes**: Search for and remove any dead references

- Verify `CycleSpeed()` has no remaining references (was removed from ObserverPanel.h)
- Check for any TODO comments that should be resolved
- Remove any commented-out code blocks

### Success Criteria:

#### Automated Verification:
- [x] Full build succeeds: `cmake --build build`
- [x] All tests pass: `ctest --test-dir build`
- [x] No compiler warnings
- [x] `grep -r "CycleSpeed" source/` returns no results

#### Manual Verification:
- [ ] Observer mode works end-to-end
- [ ] Normal gameplay unaffected
- [ ] README accurately describes observer mode
- [ ] README branch strategy section is clear

---

## Testing Strategy

### Unit Tests:
- CameraController implementations (FreeCamera, FollowShipCamera)
- PanelUtils zoom handling
- Command observer commands

### Integration Tests:
- Observer mode can be launched from main menu
- System switching works
- Camera modes cycle correctly

### Manual Testing Steps:
1. Launch game, click "Observe" on main menu
2. Verify HUD displays correctly
3. Test camera modes (Tab key)
4. Test speed controls (1-5 keys)
5. Test system switching (N/P keys)
6. Test auto-switch toggle (A key)
7. Test HUD hide (H key)
8. Test pause (configured pause key)
9. Test zoom (+/- keys and scroll)
10. Exit to menu (Esc)
11. Start normal game, verify no regressions

## Performance Considerations

- Helper method extraction should have no performance impact (inlined by compiler)
- PanelUtils functions are trivial and will be inlined
- No new allocations or complex data structures

## Migration Notes

- No save file format changes
- No user-facing behavior changes
- All existing keybindings preserved

## References

- Research document: `thoughts/shared/research/2025-12-05-observer-mode-upstream-compatibility.md`
- Engine.cpp observer integration: `source/Engine.cpp` (search for `cameraController`)
- MainPanel patterns: `source/MainPanel.cpp` (search for `SDLK_MINUS`, `ZoomViewOut`)
- ObserverPanel implementation: `source/ObserverPanel.cpp`, `source/ObserverPanel.h`
- Main menu interface: `data/_ui/interfaces.txt` (search for `"main menu"`)
- Test patterns: `tests/unit/src/test_*.cpp`

## Implementation Order Summary

Execute phases in order, with manual verification pauses:

1. **Phase 1**: Engine.cpp helper methods (highest merge conflict reduction)
2. **Phase 2**: main.cpp speed unification
3. **Phase 3**: PanelUtils extraction
4. **Phase 4**: Observe button fix (visual verification required)
5. **Phase 5**: Tests
6. **Phase 6**: README and cleanup

Each phase should result in a working build. Commit after each phase with a descriptive message like:
- `refactor: extract Engine helper methods for observer mode`
- `refactor: unify speed control in main.cpp`
- `refactor: extract shared panel utilities`
- `fix: move Observe button to right panel on main menu`
- `test: add observer mode unit tests`
- `docs: update README with observer mode documentation`
