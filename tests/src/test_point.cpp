/* test_point.cpp
Copyright (c) 2020 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/Point.h"

// ... and any system includes needed for the test file.

namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "A position or other geometric vector must be expressed", "[Point]" ) {
	GIVEN( "No initial values" ) {
		Point a;
		WHEN( "the point is created" ) {
			THEN( "it represents (0, 0)" ) {
				CHECK( a.X() == 0. );
				CHECK( a.Y() == 0. );
			}
		}
		WHEN( "Set is called" ) {
			a.Set(1, 3);
			THEN( "X and Y are updated" ) {
				CHECK( a.X() == 1. );
				CHECK( a.Y() == 3. );
			}
		}
	}
	
	GIVEN( "any Point" ) {
		auto a = Point();
		WHEN( "the point represents (0, 0)" ) {
			a.X() = 0.;
			a.Y() = 0.;
			THEN( "it can be converted to boolean FALSE" ) {
				CHECK( static_cast<bool>(a) == false );
			}
		}
		WHEN( "the point has non-zero X" ) {
			a.X() = 0.00001;
			REQUIRE( a.Y() == 0. );
			THEN( "it can be converted to boolean TRUE" ) {
				CHECK( static_cast<bool>(a) == true );
			}
		}
		WHEN( "the point has non-zero Y") {
			a.Y() = 0.00001;
			REQUIRE( a.X() == 0. );
			THEN( "it can be converted to boolean TRUE" ) {
				CHECK( static_cast<bool>(a) == true );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
