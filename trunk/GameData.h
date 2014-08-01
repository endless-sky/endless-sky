/* GameData.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_DATA_H_
#define GAME_DATA_H_

#include "Color.h"
#include "Conversation.h"
#include "Effect.h"
#include "Fleet.h"
#include "Government.h"
#include "Interface.h"
#include "Key.h"
#include "Outfit.h"
#include "Planet.h"
#include "Sale.h"
#include "Set.h"
#include "Ship.h"
#include "ShipName.h"
#include "SpriteQueue.h"
#include "StarField.h"
#include "System.h"
#include "Trade.h"

#include <map>
#include <string>

class Date;



// Class storing all the data used in the game: sprites, data files, etc.
class GameData {
public:
	static void BeginLoad(const char * const *argv);
	static void LoadShaders();
	static double Progress();
	// Begin loading a sprite that was previously deferred. Currently this is
	// done with all landscapes to speed up the program's startup.
	static void Preload(const Sprite *sprite);
	static void FinishLoading();
	
	// Revert any changes that have been made to the universe.
	static void Revert();
	static void SetDate(const Date &date);
	
	static const Set<Color> &Colors();
	static const Set<Conversation> &Conversations();
	static const Set<Effect> &Effects();
	static const Set<Fleet> &Fleets();
	static const Set<Government> &Governments();
	static const Set<Interface> &Interfaces();
	static const Set<Outfit> &Outfits();
	static const Set<Planet> &Planets();
	static const Set<Ship> &Ships();
	static const Set<ShipName> &ShipNames();
	static const Set<System> &Systems();
	
	static const std::vector<Trade::Commodity> &Commodities();
	
	static const StarField &Background();
	
	// Get the mapping of keys to commands.
	static const Key &Keys();
	static void SetKey(Key::Command command, int key);
	static const Key &DefaultKeys();
	
	static bool ShouldShowLoad();
	
	
private:
	static void LoadFile(const std::string &path);
	static void LoadImage(const std::string &path, std::map<std::string, std::string> &images);
	static std::string Name(const std::string &path);
};



#endif
