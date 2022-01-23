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
	GIVEN("An order") {
		const std::vector<int> givenOrder = { 4, 2, 8, 6 };
		ByGivenOrder<int> c(givenOrder);

		THEN( "Known elements are sorted by the given order" ) {
			std::vector<int> toSort = { 2, 4, 6 };
			const std::vector<int> expectedOrder = { 4, 2, 6 };
			std::sort(toSort.begin(), toSort.end(), c);
			CHECK( toSort == expectedOrder );
		}

		THEN( "Unknown elements are sorted by their native order" ) {
			std::vector<int> toSort = { 5, 1, 3 };
			const std::vector<int> expectedOrder = { 1, 3, 5 };
			std::sort(toSort.begin(), toSort.end(), c);
			CHECK( toSort == expectedOrder );
		}

		THEN( "Unknown elements are sorted after known elements" ) {
			std::vector<int> toSort = { 8, 1 };
			const std::vector<int> expectedOrder = { 8, 1 };
			std::sort(toSort.begin(), toSort.end(), c);
			CHECK( toSort == expectedOrder );
		}

		THEN( "Known elements are equal to themselves" ) {
			CHECK( !c(4, 4) );
		}

		THEN( "Unknown elements are equal to themselves" ) {
			CHECK( !c(5, 5) );
		}

		THEN( "Overall test" ) {
			std::vector<int> toSort = { 2, 4, 6, 8, 5, 1, 3 };
			const std::vector<int> expectedOrder = { 4, 2, 8, 6, 1, 3, 5 };
			std::sort(toSort.begin(), toSort.end(), c);
			CHECK( toSort == expectedOrder );
		}
	}
}
// #endregion unit tests



} // test namespace
