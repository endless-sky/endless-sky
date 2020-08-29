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

#include <stdexcept>

using namespace std;

namespace {
	bool SendFlightCommand(const Command &command, UI &gamePanels)
	{
		// We need to send the command to the top gamepanel. And it needs to be active.
		if(gamePanels.IsEmpty() || gamePanels.Root() != gamePanels.Top())
			return false;
		
		// Both get as well as the cast can result in a nullpointer. In both cases we
		// return false.
		MainPanel* mainPanel = dynamic_cast<MainPanel*> (gamePanels.Root().get());
		if(!mainPanel)
			return false;
		
		mainPanel->GiveCommand(command);
		return true;
	}
	
	bool PlayerIsFlyingAround(UI &menuPanels, UI &gamePanels, PlayerInfo &player)
	{
		// Code borrowed from main.cpp
		bool inFlight = (menuPanels.IsEmpty() && gamePanels.Root() == gamePanels.Top());
		return inFlight;
	}

	bool PlayerMenuIsActive(UI &menuPanels)
	{
		return !menuPanels.IsEmpty();
	}

	PlanetPanel* GetPlanetPanelIfAvailable(UI &gamePanels)
	{
		if(gamePanels.IsEmpty()){
			return nullptr;
		}
		Panel *topPanel = gamePanels.Top().get();
		// If we can cast the topPanel from the gamePanels to planetpanel,
		// then we have landed and are on a planet.
		return dynamic_cast<PlanetPanel*>(topPanel);
	}

	bool PlayerOnPlanetMainScreen(UI &menuPanels, UI &gamePanels, PlayerInfo &player)
	{
		if(!menuPanels.IsEmpty())
			return false;
		if(gamePanels.Root() == gamePanels.Top())
			return false;

		PlanetPanel *topPlanetPanel = GetPlanetPanelIfAvailable(gamePanels);
		return topPlanetPanel != nullptr;
	}
	
	// Send an SDL_event to on of the UIs.
	bool EventToUI(UI &menuOrGamePanels, const SDL_Event &event)
	{
		return menuOrGamePanels.Handle(event);
	}
	
	// Send an keyboard input to one of the UIs.
	bool KeyInputToUI(UI &menuOrGamePanels, const char* keyName, bool shift=false, bool ctrl=false, bool alt=false)
	{
		// Construct the event to send (from keyboard code and modifiers)
		SDL_Event event;
		event.type = SDL_KEYDOWN;
		event.key.state = SDL_PRESSED;
		event.key.repeat = 0;
		event.key.keysym.sym = SDL_GetKeyFromName(keyName);
		event.key.keysym.mod = KMOD_NONE;
		if(shift)
			event.key.keysym.mod |= KMOD_SHIFT;
		if(ctrl)
			event.key.keysym.mod |= KMOD_CTRL;
		if(alt)
			event.key.keysym.mod |= KMOD_ALT;
		
		// Sending directly as event to the UI. We might want to switch to
		// SDL_PushEvent in the future to use the regular SDL event-handling loops.
		return EventToUI(menuOrGamePanels, event);
	}

	// Retrieve a set of teststeps based on the stepToRun from context. This is usually directly from the test-sequence, but
	// can be from an inner loop when a REPEAT-loop or other loop is used.
	const vector<Test::TestStep> GetTestSteps(const vector<unsigned int> stepToRun, const vector<Test::TestStep> topLevel)
	{
		if(stepToRun.empty())
			return vector<Test::TestStep>();
		if(stepToRun.size()==1)
			return topLevel;
		// Construct temporary array for accessing one level lower.
		vector<unsigned int> tmpSteps(stepToRun.begin() + 1, stepToRun.end());
		// Perform recursive call to get the vector from one level lower.
		return GetTestSteps(tmpSteps, topLevel[stepToRun[0]].SubSteps());
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
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "status" && child.Size() >= 2)
		{
			if(child.Token(1) == "Active")
				status = STATUS_ACTIVE;
			else if(child.Token(1) == "Known Failure")
				status = STATUS_KNOWN_FAILURE;
			else if(child.Token(1) == "Missing Feature")
				status = STATUS_MISSING_FEATURE;
		}
		else if(child.Token(0) == "sequence")
			for(const DataNode &seqChild : child)
				testSteps.emplace_back(TestStep(seqChild));
	}
}



const string &Test::Name() const
{
	return name;
}



string Test::DebugMessage(Context &context) const
{
	// Generate a message that tells which part of the test is running
	string testStatus = "";
	for(unsigned int i = 0; i < context.stepToRun.size(); ++i)
		testStatus += "stack: " + to_string(i) + ", step: " + to_string(context.stepToRun[i]) + "; ";
	return testStatus;
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
	
	if(context.stepToRun.empty() || context.stepToRun[0] >= testSteps.size())
	{
		// Done, no failures, exit the game with exitcode success.
		menuPanels.Quit();
		return;
	}
	
	// Get the container of the inner loop and the currently running teststep within the loop.
	// Needed to use a helper function to ensure const qualifiers remained working.
	const vector<TestStep> &testStepsContainer = GetTestSteps(context.stepToRun, testSteps);
	const TestStep &testStep = testStepsContainer[context.stepToRun[context.stepToRun.size() - 1]];

	Test::TestStep::TestResult testResult = testStep.Step(context.stepAction, menuPanels, gamePanels, player);
	switch(testResult)
	{
		case TestStep::RESULT_DONE:
			// Test-step is done. Start with the first action of the next
			// step next time this function gets called.
			++context.stepToRun[context.stepToRun.size()-1];
			context.stepAction = 0;
			break;
		case TestStep::RESULT_NEXTACTION:
			++context.stepAction;
			break;
		case TestStep::RESULT_RETRY:
			break;
		case TestStep::RESULT_BREAK:
			// First pop-off the top-level item
			if(context.stepToRun.size() > 0)
				context.stepToRun.pop_back();
			// Then advance the command above it
			if(context.stepToRun.size() > 0)
				++context.stepToRun[context.stepToRun.size()-1];
			context.stepAction = 0;
			break;
		case TestStep::RESULT_LOOP:
			context.stepToRun.push_back(0);
			break;
		case TestStep::RESULT_FAIL:
		default:
			// Exit with error on a failing testStep.
			// Throwing a runtime_error is kinda rude, but works for this version of
			// the tester. Might want to add a menuPanels.QuitError() function in
			// a later version (which can set a non-zero exitcode and exit properly).
			throw runtime_error("Teststep " + DebugMessage(context) + " failed");
	}
	
	// Special case: if we are in a loop, then stepping beyond the last entry should return us to the first.
	if(context.stepToRun.size() > 1 && context.stepToRun[context.stepToRun.size()-1] >= GetTestSteps(context.stepToRun, testSteps).size())
		context.stepToRun[context.stepToRun.size()-1] = 0;
}



const vector<Test::TestStep> Test::TestStep::SubSteps() const
{
	return testSteps;
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



Test::TestStep::TestStep(const DataNode &node)
{
	Load(node);
}



void Test::TestStep::Load(const DataNode &node)
{
	if(node.Token(0) == "assign")
	{
		stepType = ASSIGN;
		checkedCondition.Load(node);
	}
	else if(node.Token(0) == "assert")
	{
		stepType = ASSERT;
		checkedCondition.Load(node);
	}
	else if(node.Token(0) == "break if")
	{
		stepType = BREAK_IF;
		checkedCondition.Load(node);
	}
	else if(node.Token(0) == "land")
		stepType = LAND;
	else if(node.Token(0) == "launch")
		stepType = LAUNCH;
	else if(node.Token(0) == "repeat")
	{
		stepType = REPEAT;
		for(const DataNode &seqChild : node)
			testSteps.emplace_back(TestStep(seqChild));
	}
	else if(node.Token(0) == "navigate")
	{
		stepType = NAVIGATE;
		for(const DataNode &child : node)
		{
			if(child.Token(0) == "travel" && child.Size() >= 2)
			{
				// Using get instead of find since the test might be loaded
				// before the actual system is loaded.
				const System *next = GameData::Systems().Get(child.Token(1));
				if(next)
					travelPlan.push_back(next);
			}
			// Using get instead of find since the test might be loaded before
			// the actual planet is loaded.
			else if(child.Token(0) == "travel destination" && child.Size() >= 2)
				travelDestination = GameData::Planets().Get(child.Token(1));
		}
	}
	else if(node.Token(0) == "wait for")
	{
		stepType = WAITFOR;
		checkedCondition.Load(node);
	}
	else if(node.Token(0) == "input")
	{
		stepType = INPUT;
		for(const DataNode &child : node)
		{
			int childSize = child.Size();
			if(child.Token(0) == "key" && childSize >= 2)
			{
				for(int i=1; i < childSize; ++i)
					inputKeys.insert(child.Token(i));
			}
			else if(child.Token(0) == "mouse" && childSize >= 3)
			{
				mouseInput = true;
				mousePosX = child.Value(1);
				mousePosY = child.Value(2);
				for(int i=3; i < childSize; ++i)
				{
					if(child.Token(i) == "percent")
					{
						mouseXpercent = true;
						mouseYpercent = true;
					}
					else if(child.Token(i) == "x-percent")
						mouseXpercent = true;
					else if(child.Token(i) == "y-percent")
						mouseYpercent = true;
					else if(child.Token(i) == "left")
						mouseLeftClick = true;
					else if(child.Token(i) == "right")
						mouseRightClick = true;
				}
			}
			else if(child.Token(0) == "command")
				command.Load(child);
		}
	}
	else if(node.Size() < 2)
		node.PrintTrace("Skipping unrecognized or incomplete test-step: " + node.Token(0));
	else if(node.Token(0) == "command")
	{
		stepType = INPUT;
		command.Load(node);
	}
	else if(node.Token(0) == "load")
	{
		stepType = LOAD_GAME;
		stepInputString = node.Token(1);
	}
	else if(node.Token(0) == "inject")
	{
		stepType = INJECT;
		stepInputString = node.Token(1);
	}
	else
		node.PrintTrace("Skipping unrecognized test-step: " + node.Token(0));
}



Test::TestStep::TestResult Test::TestStep::Step(int stepAction, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const
{
	// If a step gives input, then it is always handled, regardless of the step-type.
	if(command && !SendFlightCommand(command, gamePanels))
		return RESULT_FAIL;
	if(mouseInput)
	{
		// TODO: handle mouse-input
	}
	if(!inputKeys.empty())
	{
		// TODO: handle as single set of input-keys
		// TODO: handle keys also in-flight
		// TODO: combine keys with mouse-inputs
		for(const string key : inputKeys)
		{
			const char* inputChar = key.c_str();
			if(PlayerMenuIsActive(menuPanels))
			{
				if(!KeyInputToUI(menuPanels, inputChar))
					return RESULT_FAIL;
			}
			else if(!KeyInputToUI(gamePanels, inputChar))
				return RESULT_FAIL;
		}
	}
	
	switch (stepType)
	{
		case TestStep::ASSIGN:
			checkedCondition.Apply(player.Conditions());
			return RESULT_DONE;
			
		case TestStep::ASSERT:
			if(checkedCondition.Test(player.Conditions()))
				return RESULT_DONE;
			return RESULT_FAIL;
			
		case TestStep::BREAK_IF:
			if(checkedCondition.Test(player.Conditions()))
				return RESULT_BREAK;
			// We only break if the condition is true.
			return RESULT_DONE;
			
		case TestStep::INPUT:
			// The input is already handled earlier.
			return RESULT_DONE;
			
		case TestStep::INJECT:
			{
				// Lookup the data and inject it in the game or the games environment
				const TestData* testData = (GameData::TestDataSets()).Get(stepInputString);
				if(testData->Inject())
					return RESULT_DONE;
				return RESULT_FAIL;
			}
			
		case TestStep::LAND:
			// If the player/game menu is active, then we are not in a state
			// where land makes sense.
			if(PlayerMenuIsActive(menuPanels))
				return RESULT_FAIL;
			// If we are still flying around, then we are not on a planet.
			if(PlayerIsFlyingAround(menuPanels, gamePanels, player))
			{
				Ship * playerFlagShip = player.Flagship();
				if(!playerFlagShip)
					return RESULT_FAIL;
				if(stepAction == 0)
				{
					// Send the land command here
					// Player commands are handled in Engine.cpp (at the moment this code was updated)
					// TODO: Landing should also have a stellar target (just to reduce ambiguity)
					Command command;
					command.Set(Command::LAND);
					if(!SendFlightCommand(command, gamePanels))
						return RESULT_FAIL;
				}
				return RESULT_NEXTACTION;
			}
			if(PlayerOnPlanetMainScreen(menuPanels, gamePanels, player))
				return RESULT_DONE;

			// Unknown state/screen. Landing fails.
			return RESULT_FAIL;
			
		case TestStep::LAUNCH:
			// If we have no flagship, then we cannot launch
			if(!player.Flagship())
				return RESULT_FAIL;
			// Should implement some function to close this menu. But
			// fail for now if the player/game menu is active.
			if(PlayerMenuIsActive(menuPanels))
				return RESULT_FAIL;
			if(PlayerOnPlanetMainScreen(menuPanels, gamePanels, player)){
				// Launch using the conversation mechanism. Not the most
				// appropriate way to launch, but works for now.
				player.BasicCallback(Conversation::LAUNCH);
				return RESULT_RETRY;
			}
			// Wait for launch sequence to complete (zoom go to one)
			// We already checked earlier if we have a flagship
			if(player.Flagship()->Zoom() < 1.)
				return RESULT_RETRY;
			// If flying around, then launching the ship succesfully happened
			if(PlayerIsFlyingAround(menuPanels, gamePanels, player))
				return RESULT_DONE;
			// No idea where we are and what we are doing. But we are not
			// in a position to launch, so fail this teststep.
			return RESULT_FAIL;
			
		case TestStep::LOAD_GAME:
			{
				// Check if the savegame actually exists
				if(!Files::Exists(Files::Saves() + stepInputString))
					return RESULT_FAIL;
				
				// Inspired by LoadPanel.LoadCallback() to stop parallel threads.
				// TODO: Check if we can get to a single shared function with  LoadPanel.
				gamePanels.Reset();
				gamePanels.CanSave(true);
				
				// Perform the load and verify that player is loaded.
				player.Load(Files::Saves() + stepInputString);
				if(!player.IsLoaded())
					return RESULT_FAIL;
				
				// Remove all menus to start from freshly loaded state
				// TODO: We should send keystrokes / commands that perform the player actions instead of modifying game structures directly.
				int maxPops = 100;
				while(!menuPanels.IsEmpty() && maxPops>0)
				{
					menuPanels.Pop(menuPanels.Top().get());
					// Perform StepAll to execute the PushOrPop() for the
					// actual removal of the panel.
					menuPanels.StepAll();
					menuPanels.StepAll();
					maxPops--;
				}
				if(!menuPanels.IsEmpty())
					return RESULT_FAIL;
				
				// Start from new gamePanel
				gamePanels.Push(new MainPanel(player));
				
				// From LoadPanel.LoadCallback():
				// It takes one step to figure out the planet panel should be created, and
				// another step to actually place it. So, take two steps to avoid a flicker.
				gamePanels.StepAll();
				gamePanels.StepAll();
			}
			// Load succeeded, finish teststep.
			return RESULT_DONE;
			
		case TestStep::NAVIGATE:
			player.TravelPlan().clear();
			player.TravelPlan() = travelPlan;
			if(travelDestination)
				player.SetTravelDestination(travelDestination);
			else
				player.SetTravelDestination(nullptr);
			return RESULT_DONE;
			
		case TestStep::REPEAT:
			return RESULT_LOOP;
			
		case TestStep::WAITFOR:
			// If we reached the condition, then we are done.
			if(checkedCondition.Test(player.Conditions()))
				return RESULT_DONE;
			return RESULT_RETRY;
			
		default:
			// ERROR, unknown test-step-type. Just return failure.
			return RESULT_FAIL;
			break;
	}
}
