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

#include "Test.h"

#include "Command.h"
#include "ConditionSet.h"
#include "DataNode.h"
#include "PlanetPanel.h"
#include "PlayerInfo.h"
#include "UI.h"

#include <string>



// Class representing a single test.
class Test {
public:
	// Status indicators for the test that we selected (if any).
	enum TestStatus {STATUS_ACTIVE, STATUS_KNOWN_FAILURE, STATUS_MISSING_FEATURE};
	
public:
	// Class representing a single step in a test
	class TestStep {
	public:
		// TODO: rename to LOAD_GAME_INLINE (and allow other loaders later?)
		// Switch to game loading from inline in the test (from a child datanode)
		enum StepType {INVALID, LOAD_GAME, ASSERT, WAITFOR, LAUNCH, LAND, INJECT, COMMAND};

		// Result-Done:  Teststep succesfull. Remove step and proceed with next.
		// Result-Fail:  Teststep failed. Fail test. Exit program with non-zero exitcode
		// Result-Retry: Teststep incomplete (waiting for a condition). Retry teststep in next update.
		// Result-NextAction: Action in teststep succesfull. Retry, but with action counter one higher.
		enum TestResult {RESULT_DONE, RESULT_FAIL, RESULT_RETRY, RESULT_NEXTACTION};

		TestStep(const DataNode &node);

		void Load(const DataNode &node);
		int DoStep(int stepAction, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const;
		
	private:
		// The type of this step
		StepType stepType = INVALID;
		// Checked condition, for teststeps of types ASSERT and WAITFOR
		ConditionSet checkedCondition;
		// Savegame pilot and name to load or save to. For teststep of type LOAD_GAME (and SAVE_GAME)
		std::string filePathOrName = "";
		// Command to send if this test-step sends a command
		Command command {};
	};
	
	
	class Context {
	friend class Test;
	
	protected:
		std::vector<TestStep>::size_type stepToRun = 0;
		int stepAction = 0;
	};
	
	
public:
	const std::string &Name() const;
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
