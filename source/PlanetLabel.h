/* PlanetLabel.h
Copyright (c) 2016 by Michael Zahniser

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

#include "Color.h"
#include "Point.h"
#include "Rectangle.h"

#include <string>
#include <vector>

class StellarObject;
class System;



class PlanetLabel {
public:
	PlanetLabel(const std::vector<PlanetLabel> &labels, const System &system, const StellarObject &object);

	void Update(const Point &center, double zoom, const std::vector<PlanetLabel> &labels, const System &system);

	void Draw() const;


private:
	void UpdateData(const std::vector<PlanetLabel> &labels, const System &system);
	// Overlap detection.
	void SetBoundingBox(const Point &labelDimensions, double angle);
	Rectangle GetBoundingBox(double zoom) const;
	bool HasOverlaps(const std::vector<PlanetLabel> &labels, const System &system,
		const StellarObject &object, double zoom) const;


private:
	const StellarObject *object;

	Point drawCenter;

	// Used for overlap detection during label creation.
	Rectangle box;
	Point zoomOffset;

	// Position and radius for drawing label.
	Point position;
	double radius = 0.;

	std::string name;
	std::string government;
	Point nameOffset;
	Point governmentOffset;
	Color color;
	int hostility = 0;
	double innerAngle = -1.;
};
