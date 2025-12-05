/* FollowShipCamera.h
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

#include "CameraController.h"

#include <list>
#include <memory>
#include <string>

class Ship;

// Camera that follows a randomly selected ship.
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

	// Get the ship being followed (for HUD display).
	std::shared_ptr<Ship> GetObservedShip() const override;


private:
	// Check if a ship is a valid target (in system, not jumping, targetable)
	bool IsValidTarget(const std::shared_ptr<Ship> &ship) const;
	// Check if current target is still valid to follow
	bool HasValidTarget() const;

	std::list<std::shared_ptr<Ship>> ships;
	std::weak_ptr<Ship> target;
	Point lastPosition;
	Point lastVelocity;
	// Cooldown to prevent rapid target switching
	int switchCooldown = 0;
	static const std::string name;
};
