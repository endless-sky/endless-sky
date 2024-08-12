/* AlertLabel.h
Copyright (c) 2022 by Daniel Yoon

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

#include <memory>

class Projectile;
class Ship;


// A class that holds an overlay for a missile.
class AlertLabel {
public:
	AlertLabel(const Point &position, const Projectile &projectile, const std::shared_ptr<Ship> &flagship, double zoom);

	void Draw() const;


private:
	double rotation = 0.;
	Point position;
	double zoom = 1.;
	bool isTargetingFlagship = true;
	double radius = 15.;
	const Color *color;
};
