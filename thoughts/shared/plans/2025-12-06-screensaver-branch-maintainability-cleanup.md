# Screensaver Branch Maintainability Cleanup

## Overview

Clean up the screensaver branch to minimize diff surface against upstream master, reducing future merge conflict risk. This involves deleting dead code, reverting unnecessary refactorings, and restoring original code structure where observer mode changes aren't needed.

## Current State Analysis

The screensaver branch adds observer mode functionality but includes several changes that unnecessarily increase merge conflict risk:
- Dead code (FlagshipCameraSource) that's compiled but never used
- Personal config file (keys.txt) accidentally committed
- Code restructuring (helper method extractions) that changes existing file structure
- Unnecessary code reordering (radar drawing)
- Trivial variable renames

### Key Discoveries:
- `FlagshipCameraSource` is never instantiated - MainPanel doesn't use CameraSource abstraction
- `keys.txt` is a user-specific keybindings file, not project config
- Radar drawing was reordered for no functional reason
- `PopulateShipStatusBars` helper only called twice in same block - no real benefit
- `UpdateCameraPosition` helper called 3 times - simpler to keep inline
- PanelUtils extracts zoom code from MainPanel unnecessarily

## Desired End State

After cleanup:
1. **~280 fewer lines** in the diff against master
2. **4 fewer files** (FlagshipCameraSource x2, keys.txt, possibly PanelUtils x2)
3. **Engine.cpp structure** matches master more closely (radar order, variable names, inline code)
4. **MainPanel.cpp** unchanged from master except essential observer additions
5. **All observer mode functionality** still works correctly

### Verification:
```bash
# Compare diff size before and after
git diff master...HEAD --stat | tail -1  # Note line count before
# After cleanup:
git diff master...HEAD --stat | tail -1  # Should show ~280 fewer lines

# Verify observer mode still works
cmake --build build/macos-arm --config Debug
./build/macos-arm/Debug/endless-sky  # Test 'O' key, camera modes, speed control
```

## What We're NOT Doing

- NOT removing any observer mode functionality
- NOT changing ObserverPanel, CameraController, or other new files
- NOT modifying the observer mode save/load system
- NOT touching Command.h/cpp observer command additions
- NOT removing Panel.h/cpp virtual method additions (GetSpeedMultiplier, IsPaused)

## Implementation Approach

Work through changes in order of risk (lowest first). Each phase should be independently verifiable. The implementing agent MUST use `git diff master...HEAD` comparisons to verify each change reduces the diff.

---

## Phase 1: Delete Dead Code and Config Files

### Overview
Remove files that shouldn't exist: unused code and accidentally committed config.

### Changes Required:

#### 1.1 Delete FlagshipCameraSource Files

**Files to delete**:
- `source/FlagshipCameraSource.cpp`
- `source/FlagshipCameraSource.h`

```bash
rm source/FlagshipCameraSource.cpp source/FlagshipCameraSource.h
```

#### 1.2 Update source/CMakeLists.txt

**File**: `source/CMakeLists.txt`
**Changes**: Remove FlagshipCameraSource entries from the source list

The implementing agent should:
1. Run `git show master:source/CMakeLists.txt` to see original
2. Run `git diff master...HEAD -- source/CMakeLists.txt` to see current changes
3. Edit to remove only the FlagshipCameraSource lines while keeping other observer mode files

#### 1.3 Delete keys.txt

**File**: `keys.txt`

```bash
rm keys.txt
```

#### 1.4 Add keys.txt to .gitignore (if not already present)

**File**: `.gitignore`
**Changes**: Add `keys.txt` if not present

### Success Criteria:

#### Automated Verification:
- [x] Files deleted: `! test -f source/FlagshipCameraSource.cpp && ! test -f source/FlagshipCameraSource.h && ! test -f keys.txt`
- [x] Build succeeds: `cmake --build build/macos-arm --config Debug 2>&1 | tail -5`
- [x] Diff reduced: `git diff master...HEAD --stat | grep -E "files? changed"` shows fewer files

#### Manual Verification:
- [ ] Game launches and observer mode works (press 'O' from main menu)

**Implementation Note**: After completing this phase, verify the build succeeds before proceeding.

---

## Phase 2: Revert Engine.cpp Trivial Changes

### Overview
Revert changes that don't affect functionality but increase diff size.

### Changes Required:

#### 2.1 Revert Variable Rename (object → obj)

**File**: `source/Engine.cpp`
**Location**: Around line 527-529 in current file

The implementing agent should:
1. Run `git show master:source/Engine.cpp | grep -n "for(const StellarObject &object" -A 2 | grep 518` to find master's version
2. Run `grep -n "for(const StellarObject &obj" source/Engine.cpp` to find current location
3. Change `obj` back to `object` in the for loop (3 occurrences in that block)

**Master has**:
```cpp
for(const StellarObject &object : player.GetSystem()->Objects())
    if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsAccessible(flagship.get()))
        labels.emplace_back(labels, *player.GetSystem(), object);
```

#### 2.2 Revert Radar Drawing Order

**File**: `source/Engine.cpp`
**Location**: Draw() function

The implementing agent should:
1. Run `git show master:source/Engine.cpp | grep -n "hud->Draw" -A 30 | head -40` to see master's order
2. In current file, move the radar drawing block back to its original position (right after `hud->Draw(info)`, before target indicator)

**Master order** (lines ~1309-1330):
```cpp
hud->Draw(info);
if(hud->HasPoint("radar"))   // RADAR FIRST
{
    radar[currentDrawBuffer].Draw(...);
}
if(hud->HasPoint("target") && targetVector.Length() > 20.)  // THEN target
{
    ...
}
// Draw the faction markers.
if(targetSwizzle && hud->HasPoint("faction markers"))
{
    ...
}
```

### Success Criteria:

#### Automated Verification:
- [x] Build succeeds: `cmake --build build/macos-arm --config Debug`
- [x] Variable name matches master: `git diff master...HEAD -- source/Engine.cpp | grep "StellarObject &obj"` returns nothing
- [x] Radar order matches master: `git show master:source/Engine.cpp | grep -n "hud->Draw" -A 5` matches current structure

#### Manual Verification:
- [ ] Game runs, HUD displays correctly in normal gameplay
- [ ] Observer mode HUD displays correctly

**Implementation Note**: After completing this phase, run the game briefly to verify HUD still works.

---

## Phase 3: Revert MainPanel.cpp PanelUtils Usage

### Overview
Restore MainPanel.cpp to use inline zoom code matching master, instead of PanelUtils.

### Changes Required:

#### 3.1 Revert MainPanel.cpp KeyDown Zoom Handling

**File**: `source/MainPanel.cpp`

The implementing agent should:
1. Run `git show master:source/MainPanel.cpp | grep -n "SDLK_MINUS" -A 3` to see master's version
2. Run `git diff master...HEAD -- source/MainPanel.cpp` to see current changes
3. Revert the zoom key handling to master's inline version

**Master has** (lines ~244-247):
```cpp
else if((key == SDLK_MINUS || key == SDLK_KP_MINUS) && !command)
    Preferences::ZoomViewOut();
else if((key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS) && !command)
    Preferences::ZoomViewIn();
```

**Current has**:
```cpp
else if(PanelUtils::HandleZoomKey(key, command, true))
    ;  // Handled by utility
```

#### 3.2 Revert MainPanel.cpp Scroll Handling

**File**: `source/MainPanel.cpp`

**Master has** (Scroll function, lines ~324-331):
```cpp
bool MainPanel::Scroll(double dx, double dy)
{
    if(dy < 0)
        Preferences::ZoomViewOut();
    else if(dy > 0)
        Preferences::ZoomViewIn();
    else
        return false;

    return true;
}
```

**Current has**:
```cpp
bool MainPanel::Scroll(double dx, double dy)
{
    return PanelUtils::HandleZoomScroll(dy);
}
```

#### 3.3 Remove PanelUtils.h Include from MainPanel.cpp

**File**: `source/MainPanel.cpp`
**Changes**: Remove `#include "PanelUtils.h"` line

### Success Criteria:

#### Automated Verification:
- [x] Build succeeds: `cmake --build build/macos-arm --config Debug`
- [x] MainPanel.cpp matches master's zoom code: `git diff master...HEAD -- source/MainPanel.cpp | grep -c "PanelUtils"` returns 0
- [x] No PanelUtils include: `grep "PanelUtils" source/MainPanel.cpp` returns nothing

#### Manual Verification:
- [ ] Zoom in/out works with +/- keys in normal gameplay
- [ ] Scroll wheel zoom works in normal gameplay

**Implementation Note**: PanelUtils files are kept for now since ObserverPanel still uses them.

---

## Phase 4: Inline Engine.cpp Helper Methods (Optional - Higher Risk)

### Overview
Remove helper methods that restructure code unnecessarily. This phase is more complex and optional.

**IMPORTANT**: Only proceed with this phase if Phases 1-3 are verified working.

### Changes Required:

#### 4.1 Inline PopulateShipStatusBars

**File**: `source/Engine.cpp`

The implementing agent should:
1. Run `git show master:source/Engine.cpp | grep -n "if(flagship)" -A 30 | grep -A 25 "fuel capacity"` to see master's inline status bar code
2. Find `PopulateShipStatusBars` helper method definition and its call sites
3. For the **flagship branch only**, replace the call with master's inline code
4. Keep a simplified observer branch that just sets the bars without the helper
5. Delete the `PopulateShipStatusBars` method definition
6. Remove the declaration from Engine.h

**Master's inline code** (in `if(flagship && flagship->Hull())` block):
```cpp
double fuelCap = flagship->Attributes().Get("fuel capacity");
if(fuelCap <= MAX_FUEL_DISPLAY)
    info.SetBar("fuel", flagship->Fuel(), fuelCap * .01);
else
    info.SetBar("fuel", flagship->Fuel());
info.SetBar("energy", flagship->Energy());
double heat = flagship->Heat();
info.SetBar("heat", min(1., heat));
if(heat > 1.)
    info.SetBar("overheat", min(1., heat - 1.));
if(flagship->IsOverheated() && (uiStep / 20) % 2)
    info.SetBar("overheat blink", min(1., heat));
info.SetBar("shields", flagship->Shields());
info.SetBar("hull", flagship->Hull(), 20.);
info.SetBar("disabled hull", min(flagship->Hull(), flagship->DisabledHull()), 20.);
```

#### 4.2 Simplify UpdateCameraPosition (Optional)

**File**: `source/Engine.cpp`

This is more invasive. Consider keeping the helper if inlining becomes too complex.

The implementing agent should:
1. Check if reverting creates cleaner code
2. If the helper is needed for observer mode, keep it but ensure non-observer paths use original inline code

### Success Criteria:

#### Automated Verification:
- [x] Build succeeds: `cmake --build build/macos-arm --config Debug`
- [ ] Tests pass (if any): `ctest --preset macos-arm-test`

#### Manual Verification:
- [ ] HUD status bars display correctly for player ship
- [ ] HUD status bars display correctly in observer mode
- [ ] Camera positioning works in all modes

**Implementation Note**: PopulateShipStatusBars was inlined. UpdateCameraPosition kept as helper (inlining too invasive).
**Result**: PopulateShipStatusBars inlined successfully. UpdateCameraPosition kept as-is per plan guidance.

---

## Phase 5: Final Verification

### Overview
Verify the cleanup achieved the goal and all functionality works.

### Verification Steps:

```bash
# 1. Check diff reduction
git diff master...HEAD --stat | tail -1
# Compare to pre-cleanup baseline

# 2. Verify build
cmake --build build/macos-arm --config Debug

# 3. Run tests
ctest --preset macos-arm-test

# 4. Manual testing checklist (run the game)
```

### Success Criteria:

#### Automated Verification:
- [x] Build succeeds with no warnings related to changes
- [x] `git diff master...HEAD --stat` shows reduced file count and line changes (34 files, down from 36 at start - FlagshipCameraSource deleted)
- [x] No references to deleted files: `grep -r "FlagshipCameraSource" source/` returns nothing
- [x] Tests pass: `ctest --preset macos-arm-test` - 100% passed

#### Manual Verification:
- [ ] Normal gameplay works (start new game, fly ship, zoom, HUD displays)
- [ ] Observer mode works (press 'O', cameras work, speed control, system switching)
- [ ] Observer save/load works
- [ ] Preferences panel shows observer controls

---

## Testing Strategy

### Automated Tests:
- Build must succeed
- Existing unit tests must pass
- No compiler warnings from changed files

### Manual Testing Steps:
1. Launch game from main menu
2. Start new pilot, verify normal HUD works
3. Verify zoom (+/-) and scroll wheel work
4. Return to main menu, press 'O' for observer mode
5. Verify all camera modes (Tab to cycle)
6. Verify speed control (1-5 keys)
7. Verify system switching (N/B keys)
8. Exit observer, load regular save, verify it works

## Risk Assessment

| Phase | Risk Level | Rollback Strategy |
|-------|------------|-------------------|
| Phase 1 (Delete files) | Low | `git checkout` the files |
| Phase 2 (Trivial reverts) | Low | `git checkout` specific lines |
| Phase 3 (MainPanel revert) | Medium | `git checkout source/MainPanel.cpp` |
| Phase 4 (Inline helpers) | High | Skip if complex, or `git checkout source/Engine.cpp` |

## References

- Research document: `thoughts/shared/research/2025-12-06-screensaver-branch-cleanup-opportunities.md`
- Prior maintainability analysis: `thoughts/shared/research/2025-12-05-observer-mode-maintainability-analysis.md`
