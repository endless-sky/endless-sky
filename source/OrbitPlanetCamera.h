/* OrbitPlanetCamera.h
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
#include "Angle.h"
#include "Point.h"

#include <string>
#include <vector>

class StellarObject;

// Camera that orbits around a stellar object.
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
	void CycleTarget() override;


private:
	// Store a pointer to the original vector to avoid dangling pointers.
	// This is safe because the System's objects vector is stable.
	const std::vector<StellarObject> *stellarObjects = nullptr;
	// Indices of objects that have sprites (are visible)
	std::vector<size_t> visibleIndices;
	size_t currentIndex = 0;
	Angle orbitAngle;
	double orbitDistance = 400.;
	Point currentPosition;
	Point velocity;
	static const std::string name;
};
