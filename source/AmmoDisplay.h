/* AmmoDisplay.h
Copyright (c) 2022 by warp-core

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

#include "ClickZone.h"

#include <map>
#include <vector>

class Outfit;
class PlayerInfo;
class Point;
class Rectangle;
class Ship;



// A class for handling the secondary weapon icons displayed in the HUD.
class AmmoDisplay {
public:
	explicit AmmoDisplay(PlayerInfo &player);
	void Reset();
	void Update(const Ship &flagship);
	void Draw(const Rectangle &ammoBox, const Point &iconDimensions) const;
	bool Click(const Point &clickPoint, bool control);
	bool Click(const Rectangle &clickBox);

private:
	std::map<const Outfit *, int> ammo;
	mutable std::vector<ClickZone<const Outfit *>> ammoIconZones;
	PlayerInfo &player;
};
