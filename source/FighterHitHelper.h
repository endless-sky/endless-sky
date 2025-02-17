/* FighterHitHelper.h
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

#include "Preferences.h"
#include "Ship.h"



class FighterHitHelper
{
public:
	// Checks whether the given ship is a valid target for non-targeted projectiles.
	static inline bool IsValidTarget(const Ship *ship)
	{
		if(!ship->CanBeCarried() || !ship->IsDisabled())
			return true;
		switch(Preferences::FightersHitWhenDisabled())
		{
			case Preferences::FighterDodgePolicy::ALL: return false;
			case Preferences::FighterDodgePolicy::NONE: return true;
			case Preferences::FighterDodgePolicy::ONLY_PLAYER: return !ship->IsYours();
		}
		return false;
	}
};
