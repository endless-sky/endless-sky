/* AutoTester.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef AUTOTESTER_H_
#define AUTOTESTER_H_

#include "DataNode.h"
#include "PlayerInfo.h"
#include "TestStep.h"
#include "UI.h"

// Class representing the controller for automatic testing.
class AutoTester {
public:
	AutoTester();
	
	virtual void Load(const DataNode &node);
	
	// PlayerInfo, the gamePanels and the MenuPanels together give the state of
	// the game. We just provide them as parameter here, because they are not
	// available when the autotester got created (and they can change due to
	// loading and saving of games).
	virtual void Step(UI &menuPanels, UI &gamePanels, PlayerInfo &player);
	
private:
	std::vector<TestStep> testSteps;
};

#endif
