/* TestContext.cpp
Copyright (c) 2021 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TestContext.h"

class Test;

using namespace std;



// Constructor to be used when running an actual test.
TestContext::TestContext(const Test *toRun) : testToRun(1, toRun)
{
}



const Test *TestContext::CurrentTest() const noexcept
{
	return testToRun.empty() ? nullptr : testToRun.back();
}
