/* SecondaryWeaponIconDisplay.h
Copyright (c) 2022 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SECONDARY_WEAPON_ICON_DISPLAY_H_
#define SECONDARY_WEAPON_ICON_DISPLAY_H_

#include "ClickZone.h"

#include <vector>
#include <utility>

class Outfit;
class PlayerInfo;
class Point;
class Rectangle;
class Ship;



class SecondaryWeaponIconDisplay {
public:
	SecondaryWeaponIconDisplay(PlayerInfo &player);
	void Update(const Ship &flagship);
	void Draw(const Rectangle &ammoBox, const Point &iconDimensions) const;
	bool Click(const Point &clickPoint, bool control);

private:
	std::vector<std::pair<const Outfit *, int>> ammo;
	mutable std::vector<ClickZone<const Outfit *>> ammoIconZones;
	PlayerInfo &player;
};



#endif
