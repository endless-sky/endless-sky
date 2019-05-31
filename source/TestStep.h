/* TestStep.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TESTSTEP_H_
#define TESTSTEP_H_

#include "ConditionSet.h"
#include "DataNode.h"
#include <string>

// Class representing a single step in a test
class TestStep {
public:
	TestStep();

	static const int LOAD_GAME = 1;
	static const int ASSERT = 2;
	static const int WAITFOR = 3;
	static const int LAUNCH = 4;
	static const int LAND = 5;

	// Result-Done:  Teststep succesfull. Remove step and proceed with next.
	// Result-Fail:  Teststep failed. Fail test. Exit program with non-zero exitcode
	// Result-Retry: Teststep incomplete (waiting for a condition). Retry teststep in next update.
	static const int RESULT_DONE = 0;
	static const int RESULT_FAIL = 1;
	static const int RESULT_RETRY = 2;

	virtual void Load(const DataNode &node);
	virtual const std::string SaveGameName();
	virtual int StepType();

	
private:
	// The type of this step
	int stepType;
	// Checked condition, for teststeps of types ASSERT and WAITFOR
	ConditionSet checkedCondition;
	// Savegame to load or save to. For teststep of type LOAD_GAME (and SAVE_GAME)
	std::string saveGameName;
};

#endif
