/* Test.h
Copyright (c) 2021 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ENDLESS_SKY_AC_TESTCONTEXT_H_
#define ENDLESS_SKY_AC_TESTCONTEXT_H_

#include <set>
#include <vector>

class Test;

// State-information used during testing with the AC/Integration test framework.
class TestContext {
friend class Test;
public:
	TestContext() = default;
	TestContext(const Test *toRun);
	const Test *CurrentTest() const noexcept;


private:
	// Pointer to the test we are running.
	std::vector<const Test *> testToRun;

	// Teststep to run.
	std::vector<unsigned int> stepToRun = { 0 };
	unsigned int watchdog = 0;
	std::set<std::vector<unsigned int>> branchesSinceGameStep;
};

#endif
