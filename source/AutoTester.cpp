/* AutoTester.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "AutoTester.h"
#include "DataNode.h"
#include "PlayerInfo.h"
#include "TestStep.h"
#include "UI.h"

AutoTester::AutoTester()
{
}

void AutoTester::Load(const DataNode &node)
{
	//TODO: implement
}


// The panel-stacks determine both what the player sees and the state of the
// game.
// If the menuPanels stack is not empty, then we are in a menu for something
// like preferences, creating a new pilot or loading or saving a game.
// The menuPanels stack takes precedence over the gamePanels stack.
// If the gamePanels stack contains more than one panel, then we are either
// on a planet (if the PlanetPanel is in the stack) or we are busy with
// something like a mission-dialog, hailing or boarding.
// If the gamePanels stack contains only a single panel, then we are flying
// around in our flagship.
void AutoTester::Step(UI &menuPanels, UI &gamePanels, PlayerInfo &player)
{
	if (testSteps.empty())
	{
		//Done, no failures, exit the game with exitcode success.
		menuPanels.Quit();
		return;
	}
	TestStep testStep = testSteps.front();
	switch (testStep.StepType())
	{
		case TestStep::ASSERT:
		case TestStep::WAITFOR:
			//TODO: implement the ASSERT and WAITFOR condition checkers.
			break;
		case TestStep::LAUNCH:
			//TODO: implement LAUNCH
			testSteps.erase(testSteps.begin());
			break;
		case TestStep::LAND:
			//TODO: implement LAND
			testSteps.erase(testSteps.begin());
			break;
		case TestStep::LOAD_GAME:
			//TODO: implement LOAD_GAME
			testSteps.erase(testSteps.begin());
			break;
		default:
			// ERROR, unknown test-step-type
			// TODO: report error and exit with non-zero return-code
			menuPanels.Quit();
			break;
	}
}
