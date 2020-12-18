/* Test.cpp
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Test.h"

#include "DataNode.h"
#include "Files.h"
#include "GameData.h"
#include "MainPanel.h"
#include "Panel.h"
#include "Planet.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "System.h"
#include "TestData.h"
#include "UI.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <map>
#include <stdexcept>

using namespace std;



namespace{
	const map<Test::Status, const string> STATUS_TO_TEXT = {
		{Test::Status::ACTIVE, "active"},
		{Test::Status::KNOWN_FAILURE, "known failure"},
		{Test::Status::MISSING_FEATURE, "missing feature"}
	};
	
	const map<Test::TestStep::Type, const string> STEPTYPE_TO_TEXT = {
		{Test::TestStep::Type::ASSIGN, "assign"},
		{Test::TestStep::Type::ASSERT, "assert"},
		{Test::TestStep::Type::BRANCH, "branch"},
		{Test::TestStep::Type::INJECT, "inject"},
		{Test::TestStep::Type::INPUT, "input"},
		{Test::TestStep::Type::INVALID, "invalid"},
		{Test::TestStep::Type::LABEL, "label"},
		{Test::TestStep::Type::NAVIGATE, "navigate"},
		{Test::TestStep::Type::WATCHDOG, "watchdog"}
	};
}


Test::TestStep::TestStep(Type stepType) : stepType(stepType)
{
};



void Test::LoadSequence(const DataNode &node)
{
	for(const DataNode &child: node)
	{
		const string &typeName = child.Token(0);
		auto stepType = TestStep::Type::INVALID;
		auto it = find_if(STEPTYPE_TO_TEXT.begin(), STEPTYPE_TO_TEXT.end(),
			[&typeName](const std::pair<Test::TestStep::Type, string> &e) {
				return e.second == typeName;
			});
		if(it != STEPTYPE_TO_TEXT.end())
			stepType = it->first;
		else
			child.PrintTrace("Unknown teststep type " + child.Token(0));
		steps.emplace_back(stepType);
	}
}



void Test::Load(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for test");
		return;
	}
	// If a test object is "loaded" twice, that is most likely an error (e.g.
	// due to a plugin containing a test with the same name as the base game
	// or another plugin). Tests should be globally unique.
	if(!name.empty())
	{
		node.PrintTrace("Duplicate definition of test");
		return;
	}
	// Validate if the testname contains valid characters.
	if(node.Token(1).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-") != std::string::npos)
	{
		node.PrintTrace("Invalid test name");
		return;
	}
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "status" && child.Size() >= 2)
		{
			const string &statusText = child.Token(1);
			auto it = find_if(STATUS_TO_TEXT.begin(), STATUS_TO_TEXT.end(),
				[&statusText](const std::pair<Test::Status, string> &e) {
					return e.second == statusText;
				});
			if(it != STATUS_TO_TEXT.end())
				status = it->first;
			else
				child.PrintTrace("Unknown test-status " + statusText);
		}
		else if(child.Token(0) == "sequence")
			// Test-steps are not in the basic framework.
			continue;
	}
}



const string &Test::Name() const
{
	return name;
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
void Test::Step(Context &context, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const
{
	// Wait with testing until the game is fully loaded.
	if(!GameData::IsLoaded())
		return;
	
	if(context.stepToRun >= steps.size())
	{
		// Done, no failures, exit the game with exitcode success.
		menuPanels.Quit();
		return;
	}
	
	const TestStep &stepToRun = steps[context.stepToRun];
	
	// Exit with error on a failing testStep.
	const string &stepTypeName = STEPTYPE_TO_TEXT.at(stepToRun.stepType);
	
	string testFailMessage = "Test step " + to_string(context.stepToRun) + " (" + stepTypeName + ") failed";
	Files::LogError(testFailMessage);

	// Throwing a runtime_error is kinda rude, but works for this version of
	// the tester. Might want to add a menuPanels.QuitError() function in
	// a later version (which can set a non-zero exitcode and exit properly).
	throw runtime_error(testFailMessage);
}



string Test::StatusText() const
{
	auto it = STATUS_TO_TEXT.find(status);
	if(it != STATUS_TO_TEXT.end())
		return it->second;
	else
		return "active";
}
