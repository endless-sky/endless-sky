/* Test.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TESTRUNNER_H_
#define TESTRUNNER_H_

#include "PlayerInfo.h"
#include "Test.h"
#include "TestStep.h"
#include "UI.h"
#include <string>

// Class representing the controller for automatic testing.
class TestRunner {
public:
	TestRunner(const Test *testToRun);
	virtual ~TestRunner();
	
	// PlayerInfo, the gamePanels and the MenuPanels together give the state of
	// the game. We just provide them as parameter here, because they are not
	// available when the test got created (and they can change due to loading
	// and saving of games).
	virtual void Step(UI &menuPanels, UI &gamePanels, PlayerInfo &player);
	
private:
	const Test* testToRun;
	std::vector<TestStep *> testSteps;
	std::vector<TestStep*>::size_type stepToRun = 0;
	int stepAction = 0;
};

#endif
