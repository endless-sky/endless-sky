/* Test.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TEST_H_
#define TEST_H_

#include "DataNode.h"
#include "PlayerInfo.h"
#include "TestStep.h"
#include "UI.h"
#include <string>

// Class representing the controller for automatic testing.
class Test {
public:
	Test();
	
	// Status indicators for the test that we selected (if any).
	static const int STATUS_ACTIVE = 0;
	static const int STATUS_KNOWN_FAILURE = 1;
	static const int STATUS_MISSING_FEATURE = 2;
	
	virtual std::string Name() const;
	virtual std::string StatusText() const;

	virtual void Load(const DataNode &node);
	
	// PlayerInfo, the gamePanels and the MenuPanels together give the state of
	// the game. We just provide them as parameter here, because they are not
	// available when the testegot created (and they can change due to loading
	// and saving of games).
	virtual void Step(UI &menuPanels, UI &gamePanels, PlayerInfo &player);
	
private:
	std::vector<TestStep *> testSteps;
	std::string name = "";
	int status = STATUS_ACTIVE;
};

#endif
