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

#include <list>
#include <memory>
#include <vector>

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

	// Update with current world state (ships, stellar objects). Default does nothing.
	virtual void UpdateWorldState(const std::list<std::shared_ptr<Ship>> &ships,
		const std::vector<StellarObject> *stellarObjects) {}
};
