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

#ifndef PLANET_LABEL_H_
#define PLANET_LABEL_H_

#include "Color.h"
#include "Point.h"

#include <string>

class StellarObject;
class System;



class PlanetLabel {
public:
	PlanetLabel(const Point &position, const StellarObject &object, const System *system, double zoom);

	void Draw() const;


private:
	Point position;
	double radius = 0.;
	std::string name;
	std::string government;
	Color color;
	int hostility = 0;
	int direction = 0;
};



#endif
