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
#include <numeric>
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
		{Test::TestStep::Type::APPLY, "apply"},
		{Test::TestStep::Type::ASSERT, "assert"},
		{Test::TestStep::Type::BRANCH, "branch"},
		{Test::TestStep::Type::INJECT, "inject"},
		{Test::TestStep::Type::INPUT, "input"},
		{Test::TestStep::Type::LABEL, "label"},
		{Test::TestStep::Type::NAVIGATE, "navigate"},
		{Test::TestStep::Type::WATCHDOG, "watchdog"},
	};
	
	template<class K, class... Args>
	string ExpectedOptions(const map<K, const string, Args...> &m)
	{
		if(m.empty())
			return "no options supported";
		
		string beginning = "expected \"" + m.begin()->second;
		auto lastValidIt = prev(m.end());
		// Handle maps with just 1 element.
		if(lastValidIt == m.begin())
			return beginning + "\"";
		
		return accumulate(next(m.begin()), lastValidIt, beginning,
			[](string a, const pair<K, const string> &b) -> string
			{
				return std::move(a) + "\", \"" + b.second;
			})
			+ "\", or \"" + lastValidIt->second + '"';
	}
}



Test::TestStep::TestStep(Type stepType) : stepType(stepType)
{
};



void Test::LoadSequence(const DataNode &node)
{
	if(!steps.empty())
	{
		status = Status::BROKEN;
		node.PrintTrace("Error: duplicate sequence keyword");
		return;
	}
	
	for(const DataNode &child: node)
	{
		const string &typeName = child.Token(0);
		auto it = find_if(STEPTYPE_TO_TEXT.begin(), STEPTYPE_TO_TEXT.end(),
			[&typeName](const std::pair<TestStep::Type, const string> &e) {
				return e.second == typeName;
			});
		if(it == STEPTYPE_TO_TEXT.end())
		{
			status = Status::BROKEN;
			child.PrintTrace("Unsupported step type (" + ExpectedOptions(STEPTYPE_TO_TEXT) + "):");
			// Don't bother loading more steps once broken.
			return;
		}
		
		steps.emplace_back(it->first);
		TestStep &step = steps.back();
		switch(step.stepType)
		{
			case TestStep::Type::APPLY:
			case TestStep::Type::ASSERT:
				step.conditions.Load(child);
				break;
			case TestStep::Type::BRANCH:
				if(child.Size() < 2)
				{
					status = Status::BROKEN;
					child.PrintTrace("Error: Invalid use of \"branch\" without target label:");
					return;
				}
				step.jumpOnTrueTarget = child.Token(1);
				if(child.Size() > 2)
					step.jumpOnFalseTarget = child.Token(2);
				step.conditions.Load(child);
				break;
			case TestStep::Type::INJECT:
				if(child.Size() < 2)
				{
					status = Status::BROKEN;
					child.PrintTrace("Error: Invalid use of \"inject\" without data identifier:");
					return;
				}
				else
					step.nameOrLabel = child.Token(1);
			case TestStep::Type::INPUT:
				child.PrintTrace("Error: Not yet implemented step type input");
				status = Status::BROKEN;
				return;
			case TestStep::Type::LABEL:
				if(child.Size() < 2)
					child.PrintTrace("Ignoring empty label");
				else
				{
					step.nameOrLabel = child.Token(1);
					if(jumpTable.find(step.nameOrLabel) != jumpTable.end())
					{
						child.PrintTrace("Error: duplicate label");
						status = Status::BROKEN;
						return;
					}
					else
						jumpTable[step.nameOrLabel] = steps.size() - 1;
				}
				break;
			case TestStep::Type::NAVIGATE:
				if(child.Token(0) == "travel" && child.Size() >= 2)
					step.travelPlan.push_back(GameData::Systems().Get(child.Token(1)));
				else if(child.Token(0) == "travel destination" && child.Size() >= 2)
					step.travelDestination = GameData::Planets().Get(child.Token(1));
				else
				{
					child.PrintTrace("Error: Invalid or incomplete keywords for navigation");
					status = Status::BROKEN;
				}
				break;
			case TestStep::Type::WATCHDOG:
				step.watchdog = child.Size() >= 2 ? child.Value(1) : 0;
				break;
			default:
				child.PrintTrace("Error: unknown step type in test");
				status = Status::BROKEN;
				return;
		}
	}
	
	// Check if all jump-labels are present after loading the sequence.
	for(const TestStep &step : steps)
	{
		if(!step.jumpOnTrueTarget.empty() && jumpTable.find(step.jumpOnTrueTarget) == jumpTable.end())
		{
			node.PrintTrace("Error: missing label " + step.jumpOnTrueTarget);
			status = Status::BROKEN;
			return;
		}
		if(!step.jumpOnFalseTarget.empty() && jumpTable.find(step.jumpOnFalseTarget) == jumpTable.end())
		{
			node.PrintTrace("Error: missing label " + step.jumpOnFalseTarget);
			status = Status::BROKEN;
			return;
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
				status = Status::BROKEN;
				child.PrintTrace("Unsupported status (" + ExpectedOptions(STATUS_TO_TEXT) + "):");
			}
		}
		else if(child.Token(0) == "sequence")
			LoadSequence(child);
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
		Fail(context, player, "Test has a broken status.");
	
	if(context.stepToRun >= steps.size())
	{
		// Done, no failures, exit the game with exitcode success.
		menuPanels.Quit();
		return;
	}
	
	// All processing was done just before this step started.
	context.branchesSinceGameStep.clear();
	
	bool continueGameLoop = false;
	do
	{
		// Fail if we encounter a watchdog timeout
		if(context.watchdog == 1)
			Fail(context, player, "watchdog timeout");
		else if(context.watchdog > 1)
			--(context.watchdog);
		
		const TestStep &stepToRun = steps[context.stepToRun];
		switch(stepToRun.stepType)
		{
			case TestStep::Type::APPLY:
				stepToRun.conditions.Apply(player.Conditions());
				++(context.stepToRun);
				break;
			case TestStep::Type::ASSERT:
				if(!stepToRun.conditions.Test(player.Conditions()))
					Fail(context, player, "asserted false");
				++(context.stepToRun);
				break;
			case TestStep::Type::BRANCH:
				// If we encounter a branch entry twice, then resume the gameloop before the second encounter.
				// Encountering branch entries twice typically only happen in "wait loops" and we should give
				// the game cycles to proceed if we are in a wait loop for something that happens over time.
				if(context.branchesSinceGameStep.count(context.stepToRun))
				{
					continueGameLoop = true;
					break;
				}
				if(stepToRun.conditions.Test(player.Conditions()))
					context.stepToRun = jumpTable.find(stepToRun.jumpOnTrueTarget)->second;
				else if(!stepToRun.jumpOnFalseTarget.empty())
					context.stepToRun = jumpTable.find(stepToRun.jumpOnFalseTarget)->second;
				else
					++(context.stepToRun);
				context.branchesSinceGameStep.insert(context.stepToRun);
				break;
			case TestStep::Type::INJECT:
				{
					// Lookup the data and inject it in the game or into the environment.
					const TestData* testData = (GameData::TestDataSets()).Get(stepToRun.nameOrLabel);
					if(!testData->Inject())
						Fail(context, player, "injecting data failed");
				}
				++(context.stepToRun);
				break;
			case TestStep::Type::INPUT:
				// Give the relevant inputs here.
				Fail(context, player, "Input not implemented");
				// Make sure that we run a gameloop to process the input.
				continueGameLoop = true;
				++(context.stepToRun);
				break;
			case TestStep::Type::LABEL:
				++(context.stepToRun);
				break;
			case TestStep::Type::NAVIGATE:
				player.TravelPlan().clear();
				player.TravelPlan() = stepToRun.travelPlan;
				player.SetTravelDestination(stepToRun.travelDestination);
				break;
			case TestStep::Type::WATCHDOG:
				context.watchdog = stepToRun.watchdog;
				++(context.stepToRun);
				break;
			default:
				Fail(context, player, "Unknown step type");
				break;
		}
	}
	while(context.stepToRun < steps.size() && !continueGameLoop);
}



const string &Test::StatusText() const
{
	return STATUS_TO_TEXT.at(status);
}



// Fail the test using the given message as reason.
void Test::Fail(const Context &context, const PlayerInfo &player, const string &testFailReason) const
{
	string message = "Test failed";
	if(context.stepToRun < steps.size())
		message += " at step " + to_string(1 + context.stepToRun) + " (" +
			STEPTYPE_TO_TEXT.at(steps[context.stepToRun].stepType) + ")";
	
	if(!testFailReason.empty())
		message += ": " + testFailReason;
	
	// Only log the conditions that start with test; we don't want to overload the terminal or errorlog.
	// Future versions of the test-framework could also print all conditions that are used in the test.
	string conditions = "";
	const string TEST_PREFIX = "test: ";
	auto it = player.Conditions().lower_bound(TEST_PREFIX);
	for( ; it != player.Conditions().end() && !it->first.compare(0, TEST_PREFIX.length(), TEST_PREFIX); ++it)
		conditions += "Condition: \"" + it->first + "\" = " + to_string(it->second) + "\n";
	
	if(!conditions.empty())
		Files::LogError(conditions);
	else
		Files::LogError("No conditions were set at the moment of failure.");
	
	// Throwing a runtime_error is kinda rude, but works for this version of
	// the tester. Might want to add a menuPanels.QuitError() function in
	// a later version (which can set a non-zero exitcode and exit properly).
	throw runtime_error(message);
}
