---
date: 2025-12-04T00:00:00-08:00
researcher: abdul
git_commit: 9d4f6f96c9d2e659c68580e96e5669bdea75c635
branch: master
repository: endless-sky
topic: "Galactic Observer Mode - Architecture Research and Implementation Plan"
tags: [research, codebase, observer-mode, camera, simulation, ui, architecture]
status: complete
last_updated: 2025-12-04
last_updated_by: abdul
last_updated_note: "Added scope decisions and refined implementation plan"
---

# Research: Galactic Observer Mode - Architecture and Implementation Plan

**Date**: 2025-12-04
**Researcher**: abdul
**Git Commit**: 9d4f6f96c9d2e659c68580e96e5669bdea75c635
**Branch**: master
**Repository**: endless-sky

## Research Question

Research the codebase architecture to understand what would be needed to implement a "Galactic Observer" mode - a spectator/screensaver mode where the player watches the universe simulate itself without direct interaction.

## Scope Decisions

### In Scope
- Camera decoupling from flagship
- New camera modes (FollowShip, OrbitPlanet, FreeRoam, WatchBattle)
- Observer HUD (replacing standard HUD)
- Ghost player using intro mission logic
- Menu entry point (can modify/break existing menu)
- Auto-saving for interesting events
- Save file compatibility with original game
- Existing fast-forward (rendering skip) for time control

### Out of Scope
- Multi-system background simulation
- Trade flow visualization/heatmaps
- 10x+ time scaling (requires delta-time physics rewrite)
- Player "drop-in" to take control

### Extra Features (Post-MVP)
- Utilize existing AI commands (mine, harvest, patrol) for enhanced activity
- Auto-switch to different systems periodically

## Summary

The Galactic Observer is **feasible with moderate effort**. Key approach:

1. **Ghost Player**: Reuse intro mission logic where player exists without a flagship - simulation continues running
2. **Camera Decoupling**: Replace ~8 flagship references with configurable camera target
3. **New ObserverPanel**: Clean separation from MainPanel, launched from menu
4. **Observer HUD**: Data-driven via Interface class, replaces standard HUD elements

**Note**: The game only simulates the current system. Ships in other systems are frozen. This is acceptable for MVP - we observe one system at a time.

## Detailed Findings

### 1. Camera System Architecture

**Location**: `source/Camera.h`, `source/Camera.cpp`, `source/Engine.cpp`

The camera is a dedicated class with smooth following behavior:

```cpp
// Camera.h:36-41
class Camera {
    Point center;      // Camera position in world coordinates
    Point velocity;    // For motion blur calculation
    Point accel;       // Smooth acceleration tracking
    Point oldTarget;   // Previous target for velocity calculation
};
```

**Key Methods**:
- `SnapTo(target)` - Instant positioning (used for system entry, landing)
- `MoveTo(target, hyperspaceInfluence)` - Smooth interpolation toward target

**Coupling Points in Engine.cpp**:
| Location | Context | Code Pattern |
|----------|---------|--------------|
| Line 270 | Landing on planet | `camera.SnapTo(object->Position())` |
| Line 399 | Taking off | `camera.SnapTo(flagship->Center())` |
| Line 507-511 | Main step loop | `camera.MoveTo(flagship->Center(), ...)` |
| Line 532 | No flagship | Camera stops (no movement) |
| Line 1568 | After hyperspace | `camera.SnapTo(flagship, true)` |
| Line 1626-1632 | Render calculation | `newCamera.MoveTo(flagship->Center(), ...)` |

**Implementation Approach**:
- Camera class is well-abstracted - can track any `Point` target
- Create `CameraController` interface with implementations for each mode
- Engine gets `SetCameraTarget()` or similar method
- Default behavior unchanged for normal gameplay

### 2. Simulation System

**Location**: `source/Engine.cpp`, `source/AI.cpp`

**Important**: Only the player's current system is actively simulated. Ships in other systems are frozen.

```cpp
// Engine.cpp:1659
if(ship->GetSystem() == playerSystem && ship->HasSprite())
    // Ship is processed - others are SKIPPED
```

**System Pre-population** - When entering a system:
```cpp
// Engine.cpp:1517-1522
for(int i = 0; i < 5; ++i)  // 5 iterations = ~5 seconds worth
    for(const auto &fleet : system->Fleets())
        fleet.Get()->Place(*system, newShips);
```

**For Observer Mode**:
- Accept single-system observation (MVP)
- Leverage existing AI behaviors - ships already patrol, trade, fight
- Extra feature: Can use existing fleet AI commands (mine, harvest) to generate more activity

### 3. Time Control

**Location**: `source/main.cpp`, `source/Engine.cpp`

**Current Architecture**: Fixed 60 FPS tick rate

**Existing Fast-Forward**:
```cpp
// main.cpp:421-426
if(isFastForward && inFlight) {
    skipFrame = (skipFrame + 1) % 3;
    if(skipFrame)
        continue;  // Skip RENDERING, simulation still runs at 60 ticks/sec
}
```

**For Observer Mode**:
- Use existing fast-forward mechanism (skip 2/3 frames)
- This gives perceived ~2-3x speed without physics changes
- True time scaling is out of scope (requires delta-time rewrite)

### 4. UI Overlay System

**Location**: `source/Interface.h`, `source/Information.h`, `data/_ui/interfaces.txt`

**Architecture**: Data-driven UI via Interface class

```cpp
// Interface.h - Element types
ImageElement        // Sprites, images
BasicTextElement    // Labels, strings
WrappedTextElement  // Multi-line text
BarElement          // Progress bars, rings
PointerElement      // Directional indicators
FillElement         // Solid color fills
```

**Current HUD** (`data/_ui/interfaces.txt:1015-1290`):
- Player status box (top-right): shields, hull, fuel, energy, heat
- Radar display (top-left)
- Target display
- Messages box (bottom)
- Escorts box (left)
- Ammo box (right)

**Observer HUD Approach**:
- Create new `observer` interface in interfaces.txt
- Replace player-specific elements with observer info:
  - Current camera mode
  - Followed ship info (if following)
  - System statistics (ship counts by faction)
  - Battle activity indicator
- Keep: Radar, Messages (for event notifications)

### 5. Ghost Player Approach

**Location**: `source/PlayerInfo.h`, `source/PlayerInfo.cpp`

**Key Insight**: The intro missions already handle a player without a flagship.

**PlayerInfo state for observer**:
- `system` - Set to observed system (required for simulation)
- `planet` - Can be nullptr (in space)
- `ships` - Empty vector (no flagship)
- `firstName/lastName` - Set (required for IsLoaded())

**What already works without flagship**:
- Being landed on a planet (intro state)
- Economy simulation
- Mission system (though we won't use it)

**Crash Points to Handle**:
| File:Line | Code | Solution |
|-----------|------|----------|
| Engine.cpp:399 | `camera.SnapTo(flagship->Center())` | Use camera controller target |
| Engine.cpp:1617 | `ai.MovePlayer(*player.Flagship(), ...)` | Skip in observer mode |

**Soft Failures** (acceptable for observer):
- DistanceMap pathfinding returns empty - not needed
- AmmoDisplay needs flagship - replaced by observer HUD
- MiniMap needs flagship - can adapt or hide

### 6. Game Architecture

**Location**: `source/main.cpp`, `source/UI.h`, `source/Panel.h`

**Dual UI Stack System**:
```cpp
// main.cpp:265-277
UI gamePanels;   // MainPanel (in-flight) lives here
UI menuPanels;   // Menus, loading, preferences
```

**Panel Properties**:
- `isFullScreen` - blocks panels underneath
- `trapAllEvents` - prevents event passthrough
- `AllowsFastForward()` - enables fast-forward while active

**ObserverPanel Design**:
```cpp
class ObserverPanel : public Panel {
    PlayerInfo &player;      // Ghost player for simulation
    Engine engine;           // Reuse existing Engine
    CameraController camera; // New camera abstraction

    // Override Panel methods
    void Step() override;
    void Draw() override;
    bool KeyDown(key) override;  // Camera controls
};
```

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      OBSERVER MODE ARCHITECTURE                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐         ┌─────────────────────────────────┐   │
│  │  MenuPanel   │         │         ObserverPanel           │   │
│  ├──────────────┤         ├─────────────────────────────────┤   │
│  │ [New Game]   │         │                                 │   │
│  │ [Load Game]  │         │  Ghost Player (no flagship)     │   │
│  │ [OBSERVE] ◀──┼────────▶│         │                       │   │
│  │ [Quit]       │         │         ▼                       │   │
│  └──────────────┘         │  ┌─────────────────────────┐    │   │
│                           │  │   CameraController      │    │   │
│                           │  │  ┌───────┐ ┌────────┐   │    │   │
│                           │  │  │Follow │ │ Orbit  │   │    │   │
│                           │  │  │ Ship  │ │ Planet │   │    │   │
│                           │  │  └───────┘ └────────┘   │    │   │
│                           │  │  ┌───────┐ ┌────────┐   │    │   │
│                           │  │  │ Free  │ │ Watch  │   │    │   │
│                           │  │  │ Roam  │ │ Battle │   │    │   │
│                           │  │  └───────┘ └────────┘   │    │   │
│                           │  └─────────────────────────┘    │   │
│                           │         │                       │   │
│                           │         ▼                       │   │
│                           │  ┌─────────────────────────┐    │   │
│                           │  │     Engine (reused)     │    │   │
│                           │  │  • Ships[] with AI      │    │   │
│                           │  │  • Projectiles          │    │   │
│                           │  │  • No player commands   │    │   │
│                           │  └─────────────────────────┘    │   │
│                           │         │                       │   │
│                           │         ▼                       │   │
│                           │  ┌─────────────────────────┐    │   │
│                           │  │    Observer HUD         │    │   │
│                           │  │  • Camera mode display  │    │   │
│                           │  │  • Ship info (if follow)│    │   │
│                           │  │  • System statistics    │    │   │
│                           │  │  • Event notifications  │    │   │
│                           │  └─────────────────────────┘    │   │
│                           └─────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Implementation Plan

### Task 1: Ghost Player Setup
**Goal**: Create player state that allows simulation without flagship

**Files to modify**:
- `source/PlayerInfo.h/cpp` - Add observer mode factory/setup method

**Implementation**:
```cpp
// New method in PlayerInfo
static PlayerInfo CreateObserver(const System *startSystem) {
    PlayerInfo observer;
    observer.firstName = "Observer";
    observer.lastName = "Mode";
    observer.system = startSystem;
    observer.planet = nullptr;  // In space
    // No ships - empty fleet
    return observer;
}
```

### Task 2: Camera Controller Abstraction
**Goal**: Decouple camera from flagship

**New files**:
- `source/CameraController.h`
- `source/CameraController.cpp`

**Interface**:
```cpp
class CameraController {
public:
    virtual ~CameraController() = default;
    virtual Point GetTarget() const = 0;
    virtual Point GetVelocity() const = 0;
    virtual void Step() = 0;  // Update internal state
    virtual void SetShips(const std::list<std::shared_ptr<Ship>> &ships) {}
};

class FollowShipCamera : public CameraController { /* ... */ };
class OrbitPlanetCamera : public CameraController { /* ... */ };
class FreeCameraController : public CameraController { /* ... */ };
class BattleCameraController : public CameraController { /* ... */ };
```

### Task 3: Engine Camera Decoupling
**Goal**: Make Engine use CameraController instead of flagship

**Files to modify**:
- `source/Engine.h` - Add CameraController member
- `source/Engine.cpp` - Replace flagship references at 6 locations

**Key changes**:
```cpp
// Engine.h - add member
CameraController *cameraController = nullptr;

// Engine.cpp - example change at line 507-511
if(cameraController) {
    camera.MoveTo(cameraController->GetTarget(), 0.);
} else if(flagship) {
    camera.MoveTo(flagship->Center(), hyperspacePercentage);
}
```

### Task 4: ObserverPanel Class
**Goal**: New panel for observer mode

**New files**:
- `source/ObserverPanel.h`
- `source/ObserverPanel.cpp`

**Structure**:
```cpp
class ObserverPanel : public Panel {
public:
    ObserverPanel(PlayerInfo &player, const System *system);

    void Step() override;
    void Draw() override;
    bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
    bool AllowsFastForward() const override { return true; }

private:
    PlayerInfo &player;
    Engine engine;
    std::unique_ptr<CameraController> cameraController;
    int cameraMode = 0;  // 0=follow, 1=orbit, 2=free, 3=battle

    void CycleCamera();
    void SelectRandomShip();
};
```

### Task 5: Menu Integration
**Goal**: Add "Observe Galaxy" option to main menu

**Files to modify**:
- `source/MenuPanel.cpp` - Add button handler
- `data/_ui/interfaces.txt` - Add button to menu interface

### Task 6: Observer HUD
**Goal**: Custom HUD for observer mode

**Files to modify/create**:
- `data/_ui/interfaces.txt` - Add `interface "observer"` section

**Elements**:
- Camera mode indicator (top-left)
- Followed ship info panel (if applicable)
- System name and statistics
- Controls hint
- Radar (keep existing)
- Messages (keep for notifications)

### Task 7: Auto-Save Integration
**Goal**: Save periodically and on interesting events

**Files to modify**:
- `source/ObserverPanel.cpp` - Add save triggers

**Triggers**:
- Periodic timer (every N minutes)
- Battle detected (multiple ships destroyed)
- New ship types enter system

**Save compatibility**: Use standard save format - files can be loaded in original game

## Code References

### Camera System
- `source/Camera.h:36-41` - Camera class definition
- `source/Camera.cpp:46-75` - MoveTo() smooth interpolation
- `source/Engine.cpp:507-511` - Flagship tracking in Step()
- `source/Engine.cpp:1626-1632` - Flagship tracking in CalculateStep()

### Simulation
- `source/Engine.cpp:1591-1670` - CalculateStep() main simulation
- `source/Engine.cpp:1726-1882` - CalculateUnpaused() ship movement
- `source/AI.cpp:677-1232` - AI::Step() ship AI processing
- `source/Engine.cpp:1517-1522` - System pre-population on entry

### UI System
- `source/Interface.h:37-278` - Interface class for data-driven UI
- `source/Information.h:33-75` - Data binding for interfaces
- `data/_ui/interfaces.txt:1015-1290` - HUD interface definition
- `source/Messages.h:36-67` - Notification system

### Player/Intro Logic
- `source/PlayerInfo.h:443-559` - PlayerInfo class members
- `source/PlayerInfo.cpp:1066-1092` - FlagshipPtr() logic
- `data/intro missions.txt` - Intro mission handling (reference for no-ship state)

### Architecture
- `source/main.cpp:265-277` - Dual UI stack creation
- `source/Panel.h:43-175` - Panel base class
- `source/MainPanel.h:34-96` - Main gameplay panel
- `source/MenuPanel.cpp` - Menu panel implementation

## Extra Features (Post-MVP)

### System Auto-Switching
Automatically change observed system periodically:
- Timer-based switch (every 5-10 minutes)
- Or trigger on "quiet" periods (no battles for X time)
- Pre-populate new system with ships on entry

### Enhanced AI Activity
Use existing AI commands to generate more interesting behavior:
- `source/AI.cpp` - Existing fleet commands
- Could spawn mining fleets, patrol routes
- Leverage existing government fleet definitions

---

*Research completed 2025-12-04*
*Scope decisions added 2025-12-04*
