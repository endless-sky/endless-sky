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

#include "Command.h"
#include "ConditionSet.h"

#include <set>
#include <string>
#include <vector>

class DataNode;
class Planet;
class PlayerInfo;
class UI;
class System;



// Class representing a single test.
class Test {
public:
	// Status indicators for the test that we selected (if any).
	enum TestStatus {STATUS_ACTIVE, STATUS_KNOWN_FAILURE, STATUS_MISSING_FEATURE};
	
public:
	// Class representing a single step in a test
	class TestStep {
	public:
		// The different types of teststeps.
		enum StepType {
			// Step that assigns a value to a condition.
			ASSIGN,
			// Step that verifies if a certain condition is true
			ASSERT,
			// Step that breaks execution of a REPEAT loop (or stops the execution
			// of the test itself if at toplevel) when a certain condition is
			// true.
			BREAK_IF,
			// Step that adds game-data, either in the config-directories or in the game directly.
			INJECT,
			// Step that performs input (key, mouse, command).
			INPUT,
			// Invalid test-step type, should not be used in tests. Used to detect issues in test-framework.
			INVALID,
			// Step that performs land of the players flagship.
			LAND,
			// Step that launches the players flagship.
			LAUNCH,
			// Step to perform loading of a savegame
			LOAD_GAME,
			// Instructs the game to set navigation / travel plan to a target system
			NAVIGATE,
			// Step that contains a set of test-steps below it. Repeats the
			// steps inside it a number of times or until a break is given within
			// the step.
			REPEAT,
			// Step that waits for a certain condition to become true
			WAITFOR,
			// Sets the watchdog timer. No value or zero disables the watchdog. Non-zero gives
			// a watchdog in number of frames/steps.
			WATCHDOG,
		};

		// Result returned from a TestStep.
		enum TestResult {
			// Teststep succesfull. Proceed with next teststep.
			RESULT_DONE,
			// Teststep failed. Fail test. Exit program with non-zero exitcode
			RESULT_FAIL,
			// Teststep incomplete (waiting for a condition). Retry teststep in next frame-step.
			RESULT_RETRY,
			// Teststep incomplete (but some action was done). Retry, but with action counter one higher.
			RESULT_NEXTACTION,
			// Teststep indicates to break of an outer loop or break off a test (succesfully)
			// Teststep should use RESULT_FAIL for breaking off a test with a failure.
			RESULT_BREAK,
			// Teststep indicated to start a loop under the current step (in case of REPEAT).
			RESULT_LOOP
		};

		TestStep(const DataNode &node);

		void Load(const DataNode &node);
		TestResult Step(int stepAction, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const;
		
		// Get TestSteps embedded in the current teststep (in case of loop/REPEAT type).
		const std::vector<TestStep> SubSteps() const;
		
	private:
		// The type of this step
		StepType stepType = INVALID;
		// Checked condition, for teststeps of types ASSERT, WAITFOR and BREAK_IF.
		ConditionSet checkedCondition;
		// Savegame pilot and name to load or save to, for teststeps of type LOAD_GAME (and SAVE_GAME).
		std::string stepInputString = "";
		// Command to send if this test-step sends a command.
		Command command {};
		// Set of teststeps under the current teststep, used for REPEAT type.
		std::vector<Test::TestStep> testSteps;
		// Variables for travelpan/NAVIGATE steps.
		std::vector<const System *> travelPlan;
		const Planet *travelDestination = nullptr;
		
		// Inputkeys pressed by this test-step.
		std::set<std::string> inputKeys;
		// Mouse inputs given by this test-step.
		bool mouseInput = false;
		bool mouseXpercent = false;
		bool mouseYpercent = false;
		bool mouseLeftClick = false;
		bool mouseRightClick = false;
		double mousePosX;
		double mousePosY;
	};
	
	class Context {
	friend class Test;
	
	protected:
		// Vector with the step to run. This array typically has only one element,
		// but when a loop (REPEAT) is active, then it will have another element
		// where the highest element gives the step within the loop and the lowest
		// element gives the test-step on toplevel that has the loop.
		std::vector<unsigned int> stepToRun = { 0 };
		int stepAction = 0;
	};
	
	
public:
	const std::string &Name() const;
	std::string DebugMessage(Context &context) const;
	std::string StatusText() const;

	// PlayerInfo, the gamePanels and the MenuPanels together give the state of
	// the game. We just provide them as parameter here, because they are not
	// available when the test got created (and they can change due to loading
	// and saving of games).
	void Step(Context &context, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const;

	void Load(const DataNode &node);
	
	
private:
	std::vector<Test::TestStep> testSteps;
	std::string name = "";
	TestStatus status = STATUS_ACTIVE;
};

#endif
