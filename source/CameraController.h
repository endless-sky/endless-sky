/* CameraController.h
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
#include <string>
#include <vector>

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

	// Get the ship being observed (if any). Used for HUD display.
	virtual std::shared_ptr<Ship> GetObservedShip() const;

	// Cycle to the next target (for modes that support it).
	virtual void CycleTarget();

	// Set camera movement direction (for free camera mode).
	virtual void SetMovement(double dx, double dy);
};
