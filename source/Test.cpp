/* Test.cpp
Copyright (c) 2019-2020 by Peter van der Meer

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
		{Test::Status::BROKEN, "broken"},
		{Test::Status::KNOWN_FAILURE, "known failure"},
		{Test::Status::MISSING_FEATURE, "missing feature"},
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
		{Test::TestStep::Type::WATCHDOG, "watchdog"},
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
		auto it = find_if(STEPTYPE_TO_TEXT.begin(), STEPTYPE_TO_TEXT.end(),
			[&typeName](const std::pair<TestStep::Type, const string> &e) {
				return e.second == typeName;
			});
		if(it != STEPTYPE_TO_TEXT.end())
			steps.emplace_back(it->first);
		else
		{
			child.PrintTrace("Unknown teststep type " + child.Token(0));
			// Set test-status to broken and break the sequence loading
			// loop since there is no point in loading more steps of a
			// broken test.
			status = Status::BROKEN;
			break;
		}
	}
}



void Test::Load(const DataNode &node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("Skipping unnamed test:");
		return;
	}
	// If a test object is "loaded" twice, that is most likely an error (e.g.
	// due to a plugin containing a test with the same name as the base game
	// or another plugin). Tests should be globally unique.
	if(!name.empty())
	{
		node.PrintTrace("Skipping duplicate test definition:");
		return;
	}
	// Validate if the testname contains valid characters.
	if(node.Token(1).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-") != std::string::npos)
	{
		node.PrintTrace("Skipping test whose name contains unsupported character(s):");
		return;
	}
	name = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "status" && child.Size() >= 2)
		{
			const string &statusText = child.Token(1);
			auto it = find_if(STATUS_TO_TEXT.begin(), STATUS_TO_TEXT.end(),
				[&statusText](const std::pair<Status, const string> &e) {
					return e.second == statusText;
				});
			if(it != STATUS_TO_TEXT.end())
			{
				// If the test already has a broken status (due to anything
				// else in loading having failed badly), then don't update
				// the status from broken.
				if(status != Status::BROKEN)
					status = it->first;
			}
			else
			{
				child.PrintTrace("Unknown test-status " + statusText);
				status = Status::BROKEN;
			}
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



// The UI panel stacks determine both what the player sees and the state of the game.
// When the menuPanels are not empty, we are in a menu for something, e.g. preferences,
// creating a new pilot, or loading or saving a game. Any such menu panels always take
// precedence over game panels.
// When the gamePanels stack contains more than one item, we are either on a planet or
// busy with something, e.g. reading a dialog, hailing a ship/planet, or boarding.
// Otherwise, the flagship is in space and controllable.
void Test::Step(Context &context, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const
{
	// Tests always wait until the game is fully loaded.
	if(!GameData::IsLoaded())
		return;
		
	if(status == Status::BROKEN)
		Fail("Test has a broken status.");
	
	if(context.stepToRun >= steps.size())
	{
		// Done, no failures, exit the game with exitcode success.
		menuPanels.Quit();
		return;
	}
	
	const TestStep &stepToRun = steps[context.stepToRun];
	
	// Exit with error on a failing testStep.
	const string &stepTypeName = STEPTYPE_TO_TEXT.at(stepToRun.stepType);
	
	Fail("Test step " + to_string(context.stepToRun) + " (" + stepTypeName + ") failed");
}



const string &Test::StatusText() const
{
	auto it = STATUS_TO_TEXT.find(status);
	if(it != STATUS_TO_TEXT.end())
		return it->second;
	else
		return STATUS_TO_TEXT.at(Status::BROKEN);
}



// Fail the test using the given message as reason.
void Test::Fail(const string &testFailMessage) const
{
	Files::LogError(testFailMessage);

	// Throwing a runtime_error is kinda rude, but works for this version of
	// the tester. Might want to add a menuPanels.QuitError() function in
	// a later version (which can set a non-zero exitcode and exit properly).
	throw runtime_error(testFailMessage);	
}
