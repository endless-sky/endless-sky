/* TestContext.h
Copyright (c) 2021 by Peter van der Meer

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

#include <set>
#include <vector>

class Test;

// State-information used during testing with the AC/Integration test framework.
class TestContext {
friend class Test;
public:
	TestContext() = default;
	explicit TestContext(const Test *toRun);
	const Test *CurrentTest() const noexcept;


private:
	// Class to describe a running test and running test-step within the test.
	class ActiveTestStep {
	public:
		const Test *test;
		unsigned int step;


	public:
		// Support operators for usage in containers like map and set.
		bool operator==(const ActiveTestStep &rhs) const;
		bool operator<(const ActiveTestStep &rhs) const;
	};

private:
	// Reference to the currently running test and test-step within the test.
	std::vector<ActiveTestStep> callstack;

	std::set<ActiveTestStep> branchesSinceGameStep;
};
