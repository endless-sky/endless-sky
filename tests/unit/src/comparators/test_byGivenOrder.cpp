/* test_byGivenOrder.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/comparators/ByGivenOrder.h"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <random>

namespace { // test namespace

// #region unit tests
SCENARIO( "Test basic ByGivenOrder functionality." , "[ByGivenOrder]" ) {
	GIVEN("An order") {
		const std::vector<int> givenOrder = { 4, 2, 8, 6 };
		ByGivenOrder<int> c(givenOrder);

		const std::vector<int> unknownElements = { 1, 3, 5 };

		THEN( "Known elements are sorted by the given order" ) {
			for(unsigned int i = 0; i < givenOrder.size(); ++i)
				for(unsigned int j = 0; j < givenOrder.size(); ++j)
					CHECK( c(givenOrder[i], givenOrder[j]) == (i < j) );
		}

		THEN( "Unknown elements are sorted by their native order" ) {
			for(int elt : unknownElements)
				for(int elt2 : unknownElements)
					CHECK( c(elt, elt2) == (elt < elt2) );
		}

		THEN( "Unknown elements are sorted after known elements" ) {
			for(int elt : givenOrder)
				for(int elt2 : unknownElements)
				{
					CHECK( c(elt, elt2) );
					CHECK( !c(elt2, elt) );
				}
		}

		THEN( "Known elements are equal to themselves" ) {
			for(int elt : givenOrder)
				CHECK( !c(elt, elt) );
		}

		THEN( "Unknown elements are equal to themselves" ) {
			for(int elt : unknownElements)
				CHECK( !c(elt, elt) );
		}

		THEN( "Overall test" ) {
			std::vector<int> toSort = { 2, 4, 6, 8, 5, 1, 3 };
			const std::vector<int> expectedOrder = { 4, 2, 8, 6, 1, 3, 5 };

			std::random_device rd;
			std::mt19937 gen(rd());
			std::shuffle(toSort.begin(), toSort.end(), gen);
			std::sort(toSort.begin(), toSort.end(), c);
			CHECK( toSort == expectedOrder );
		}
	}
}
// #endregion unit tests



} // test namespace
