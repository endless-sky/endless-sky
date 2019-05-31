/* Squadron.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SQUADRON_H_
#define SQUADRON_H_

#include "DataNode.h"
#include "Ship.h"
#include <string>

// This class defines a squadron.
// Initial usage of the squadron would be to define larger NPC fleets where you
// want multiple identical ships in a single fleet.
// Another usage would be to have missions where the player is always escorted by
// a certain ship or group of ships, and when such a ship gets destroyed, then
// new instances of the ship will appear once the player jumps (starting from
// a planet where the ships hull is for sale).
//
// Later usage would be to also allow the player to define and use squadrons.
// The player can command each ship in a squadron he/she owns just like any other
// ship, but the first ship in the squadron acts as squadron-leader. So any other
// squadron ship will move to the squadron leaders location if the other ship has
// no other commands.
//
// A squadron could also consist of 0 ships if all ships in the squadron are
// destroyed. The squadron definition then still exists and the player can land
// on a planet where the squadron ships can be bought to get new ships in the
// squadron from the shipyard in the same way as that ammunition can be bought
// (buy ships to re-equip your squadrons? Question at the shipyard.).
//
// Buying squadron ships doesn't need to be an exact match with ships in the
// shipyard. If the hulls and relevant outfits are available (either in the
// outfitter or as cargo in the players cargo-hold), then the squadron ships
// can automatically be bought and built for the player.
//
// Squadrons owned by the player can only be started from a ship that the player
// owns and that is on the same planet as where the player has landed.
//
// The format to define squadrons in a data-file:
//	squadron "<name>"
//		template
//			<ship definition>
//		desired <number>
//		actual <number>
//

class Squadron {
public:
	// Load data for a type of ship:
	void Load(const DataNode &node);
private:
	std::string name;
	Ship * templateShip;
	int desired = 0;
	int actual = 0;
};

#endif
