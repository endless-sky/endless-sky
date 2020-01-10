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

#include "DataNode.h"
#include "PlayerInfo.h"
#include "TestStep.h"
#include "UI.h"

#include <string>



// Class representing a single test.
class Test {
public:
	// Status indicators for the test that we selected (if any).
	enum TestStatus {STATUS_ACTIVE, STATUS_KNOWN_FAILURE, STATUS_MISSING_FEATURE};
	
public:
	class Context {
	friend class Test;
	
	protected:
		std::vector<TestStep>::size_type stepToRun = 0;
		int stepAction = 0;
	};
	
	
public:
	const std::string &Name() const;
	std::string StatusText() const;
	const std::vector<TestStep> &TestSteps() const;

	// PlayerInfo, the gamePanels and the MenuPanels together give the state of
	// the game. We just provide them as parameter here, because they are not
	// available when the test got created (and they can change due to loading
	// and saving of games).
	void Step(Context &context, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const;

	void Load(const DataNode &node);
	
	
private:
	std::vector<TestStep> testSteps;
	std::string name = "";
	TestStatus status = STATUS_ACTIVE;
};

#endif
