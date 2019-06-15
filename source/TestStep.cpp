/* TestStep.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConditionSet.h"
#include "DataNode.h"
#include "PlayerInfo.h"
#include "TestStep.h"
#include "UI.h"
#include <string>


TestStep::TestStep()
{
	// Initialize with some sensible default values.
	stepType = 0;
	frameWait = 20;
	saveGameName = "";
}



TestStep::~TestStep()
{
}



int TestStep::StepType()
{
	return stepType;
}



const std::string TestStep::SaveGameName()
{
	return saveGameName;
}



void TestStep::Load(const DataNode &node)
{
	if (node.Token(0) == "load" and node.Size() > 1)
	{
		stepType = LOAD_GAME;
		saveGameName = node.Token(1);
	}
	else if (node.Token(0) == "assert")
	{
		stepType = ASSERT;
		checkedCondition.Load(node);
	}
	else if (node.Size() > 1 and node.Token(0) == "wait" and node.Token(1) == "for")
	{
		stepType = WAITFOR;
		checkedCondition.Load(node);
	}
	else if (node.Token(0) == "land")
	{
		stepType = LAND;
	}
	else if (node.Token(0) == "launch")
	{
		stepType = LAUNCH;
	}
	else
		node.PrintTrace("Skipping unrecognized test-step: " + node.Token(0));
}

int TestStep::DoStep(UI &menuPanels, UI &gamePanels, PlayerInfo &player)
{
	if (frameWait > 0){
		frameWait--;
		return RESULT_RETRY;
	}

	switch (stepType)
	{
		case TestStep::ASSERT:
		case TestStep::WAITFOR:
			//TODO: implement the ASSERT and WAITFOR condition checkers.
			return RESULT_FAIL;
			break;
		case TestStep::LAUNCH:
			//TODO: implement LAUNCH
			return RESULT_FAIL;
			break;
		case TestStep::LAND:
			//TODO: implement LAND
			return RESULT_FAIL;
			break;
		case TestStep::LOAD_GAME:
			//TODO: implement LOAD_GAME
			return RESULT_FAIL;
			break;
		default:
			// ERROR, unknown test-step-type
			// TODO: report error and exit with non-zero return-code
			return RESULT_FAIL;
			break;
	}
}


