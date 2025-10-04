/* Test.h
Copyright (c) 2019-2020 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "../Command.h"
#include "../ConditionAssignments.h"
#include "../ConditionSet.h"

#include <SDL2/SDL.h>

#include <map>
#include <set>
#include <string>
#include <vector>

class DataNode;
class Planet;
class PlayerInfo;
class System;
class TestContext;
class UI;



// Class representing a single test.
class Test {
public:
	// Status indicators for the test that we selected (if any).
	enum class Status {ACTIVE, PARTIAL, BROKEN, KNOWN_FAILURE, MISSING_FEATURE};

	// A tag type to denote a failing test that is not an error, such as a
	// "known failure" test failing.
	struct known_failure_tag {};


public:
	// Class representing a single step in a test
	class TestStep {
	public:
		// The different types of teststeps.
		enum class Type {
			// Step that assigns a value to a condition. Does not cause the game to step.
			APPLY,
			// Step that verifies if a certain condition is true. Does not cause the game to step.
			ASSERT,
			// Branch with a label to jump to when the condition in child is true.
			// When a second label is given, then the second is to jump to on false.
			// Does not cause the game to step, except when no step was done since last BRANCH or GOTO.
			BRANCH,
			// Step that calls another test to handle some generic common actions.
			CALL,
			// Step that prints a debug-message to the output.
			DEBUG,
			// Step that adds game-data, either in the config-directories or in the game directly.
			INJECT,
			// Step that performs input (key, mouse, command). Does cause the game to step (to process the inputs).
			INPUT,
			// Label to jump to (similar as is done in conversations). Does not cause the game to step.
			LABEL,
			// Instructs the game to set navigation / travel plan to a target system
			NAVIGATE,
		};



	public:
		explicit TestStep(Type stepType);
		void LoadInput(const DataNode &node);


	public:
		Type stepType = Type::ASSERT;
		std::string nameOrLabel;
		// Variables for travelpan/navigate steps.
		std::vector<const System *> travelPlan;
		const Planet *travelDestination = nullptr;
		// For applying condition changes.
		ConditionAssignments assignConditions;
		// For branching based on conditions or checking asserts (similar to Conversations).
		ConditionSet checkConditions;
		// Labels to jump to in case of branches. We could optimize during
		// load to look up the step numbers (and provide integer step numbers
		// here), but we can also use the textual information during error/
		// debug printing, so keeping the strings for now.
		std::string jumpOnTrueTarget;
		std::string jumpOnFalseTarget;

		// Input variables.
		Command command;
		std::set<std::string> inputKeys;
		Uint16 modKeys = 0;

		// Mouse/Pointer input variables.
		int XValue = 0;
		int YValue = 0;
		bool clickLeft = false;
		bool clickMiddle = false;
		bool clickRight = false;
	};


public:
	const std::string &Name() const;
	Status GetStatus() const;
	const std::string &StatusText() const;
	std::set<std::string> RelevantConditions() const;

	// Check the game status and perform the next test action.
	void Step(TestContext &context, PlayerInfo &player, Command &commandToGive) const;

	void Load(const DataNode &node, const ConditionsStore *playerConditions);


private:
	void LoadSequence(const DataNode &node, const ConditionsStore *playerConditions);

	// Fail the test using the given message as reason.
	void Fail(const TestContext &context, const PlayerInfo &player, const std::string &testFailReason) const;
	void UnexpectedSuccessResult() const;


private:
	std::string name;
	Status status = Status::ACTIVE;
	// Jump-table that specifies which labels map to which teststeps.
	std::map<std::string, unsigned int> jumpTable;
	std::vector<TestStep> steps;
};
