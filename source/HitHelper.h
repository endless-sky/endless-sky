/* HitHelper.h
Copyright (c) 2024 by tibetiroka

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

#include "GameData.h"
#include "Gamerules.h"
#include "Ship.h"
#include "Weapon.h"



class HitHelper
{
public:
	// Check whether the given ship can be validly hit by non-targeted projectiles.
	static bool IsValid(const Ship *ship, const Weapon *weapon)
	{
		// If a ship isn't disabled, it's always a valid target.
		if(!ship->IsDisabled())
			return true;
		// Certain weapons can't hit disabled ships unless targeting them.
		if(weapon && !weapon->CanCollideDisabled())
			return false;
		// Gamerules can also provide blanket protection to some or all disabled ships.
		switch(GameData::GetGamerules().GetCrossfireImmunity())
		{
			case Gamerules::CrossfireImmunity::NONE:
				return true;
			case Gamerules::CrossfireImmunity::ALL_FIGHTERS:
				return !ship->CanBeCarried();
			case Gamerules::CrossfireImmunity::PLAYER_FIGHTERS:
				return !(ship->IsYours() && ship->CanBeCarried());
			case Gamerules::CrossfireImmunity::ALL_SHIPS:
				return false;
		}
		return false;
	}
};
