/* Test.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/


#include "DataNode.h"
#include "PlayerInfo.h"
#include "Test.h"
#include "TestStep.h"
#include "UI.h"
#include <stdexcept>
#include <string>

using namespace std;



Test::Test()
{
	name = "";
}



void Test::Load(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for test");
		return;
	}
	if (node.Token(0) != "test")
	{
		node.PrintTrace("Non-test found in test parsing");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "status" && child.Size() >= 2)
		{
			if (child.Token(1) == "Active")
				status = STATUS_ACTIVE;
			else if (child.Token(1) == "Known Failure")
				status = STATUS_KNOWN_FAILURE;
			else if (child.Token(1) == "Missing Feature")
				status = STATUS_MISSING_FEATURE;
		}
		else if (child.Token(0) == "sequence")
		{
			for (const DataNode &seqChild : child)
			{
				testSteps.push_back(new TestStep());
				(testSteps.back())->Load(seqChild);
			}
		}
	}
}



string Test::Name() const
{
	return name;
}



string Test::StatusText() const
{
	switch (status)
	{
		case Test::STATUS_KNOWN_FAILURE:
			return "KNOWN FAILURE";
		case Test::STATUS_MISSING_FEATURE:
			return "MISSING FEATURE";
		case Test::STATUS_ACTIVE:
		default:
			return "ACTIVE";
	}
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
void Test::Step(UI &menuPanels, UI &gamePanels, PlayerInfo &player)
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
