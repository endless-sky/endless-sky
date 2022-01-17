/* test_byGivenOrder.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/comparators/ByGivenOrder.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region unit tests
SCENARIO( "Test basic ByGivenOrder functionality." , "[ByGivenOrder]" ) {
	const std::vector<int> givenOrder = { 4, 2, 8, 6 };
	std::vector<int> toSort = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	const std::vector<int> expectedOrder = { 4, 2, 8, 6, 0, 1, 3, 5, 7, 9 };

	std::sort(toSort.begin(), toSort.end(), ByGivenOrder<int>(givenOrder));
	REQUIRE( toSort == expectedOrder );

}
// #endregion unit tests



} // test namespace
