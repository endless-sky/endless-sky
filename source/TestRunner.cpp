/* Test.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PlayerInfo.h"
#include "Test.h"
#include "TestRunner.h"
#include "TestStep.h"
#include "UI.h"
#include <stdexcept>
#include <string>

using namespace std;



TestRunner::TestRunner(const Test *testToRun): testToRun(testToRun)
{
	testSteps = testToRun->TestSteps();
}



TestRunner::~TestRunner()
{
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
void TestRunner::Step(UI &menuPanels, UI &gamePanels, PlayerInfo &player)
{
	if (testSteps.empty())
	{
		//Done, no failures, exit the game with exitcode success.
		menuPanels.Quit();
		return;
	}
	TestStep* testStep = testSteps.front();

	int testResult = testStep->DoStep(menuPanels, gamePanels, player);
	// Only keep the teststep if we have a retry result. Remove the step
	// in all other cases.
	if (testResult != TestStep::RESULT_RETRY)
	{
		testSteps.erase(testSteps.begin());
		delete testStep;
	}

	// Exit with error if we are not succesfull and not retrying.
	// Throwing a runtime_error is kinda rude, but works for this version of 
	// the tester. Might want to add a menuPanels.QuitError() function in
	// a later version (which can set a non-zero exitcode and exit properly).
	if ((testResult != TestStep::RESULT_DONE) and (testResult != TestStep::RESULT_RETRY))
		throw runtime_error("Teststep failed");
}
