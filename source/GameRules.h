/* GameRules.h
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_RULES_H_
#define GAME_RULES_H_

#include "Dictionary.h"

#include <string>

class DataNode;


// GameRules contains a list of constants that define game behavior, for example,
// how fast depreciation occurs or the length of the depreciation grace period,
// or the lifetime of flotsams.
class GameRules {
public:
	// Load a gamerules node. Only save to default if this is during the initial
	// loading of the game. All later changes to gamerules do not get saved to
	// the default.
	void Load(const DataNode &node, bool saveToDefault = false);
	
	// Reset to the initial gamerules defined in the game data.
	void Reset();
	
	// Get a gamerule constant.
	double Get(const std::string &key) const;
	
	
private:
	Dictionary rules;
	Dictionary defaultRules;
};



#endif
