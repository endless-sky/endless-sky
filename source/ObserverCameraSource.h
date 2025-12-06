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
	void UpdateWorldState(const std::list<std::shared_ptr<Ship>> &ships,
		const std::vector<StellarObject> *stellarObjects) override;

	// Access the underlying controller for observer-specific operations
	CameraController *GetController() const { return controller; }

private:
	CameraController *controller;
};
