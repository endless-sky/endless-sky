#include "catch.hpp"

// Include only the tested class's header.
#include "../../source/Point.h"

namespace { // test namespace
// #region mock data
// #endregion mock data


// #region unit tests
SCENARIO( "A position or other geometric vector must be expressed", "[Point]" ) {
	GIVEN( "No initial values" ) {
		Point a;
		WHEN( "the point is created" ) {
			THEN( "it represents (0, 0)" ) {
				REQUIRE( a.X() == 0. );
				REQUIRE( a.Y() == 0. );
			}
		}
		WHEN( "Set is called" ) {
			a.Set(1, 3);
			THEN( "X and Y are updated" ) {
				REQUIRE( a.X() == 1. );
				REQUIRE( a.Y() == 3. );
			}
		}
	}
	
	GIVEN( "any Point" ) {
		auto a = Point();
		WHEN( "the point represents (0, 0)" ) {
			a.X() = 0.;
			a.Y() = 0.;
			THEN( "it can be converted to boolean FALSE" ) {
				REQUIRE( static_cast<bool>(a) == false );
			}
		}
		WHEN( "the point has non-zero X" ) {
			a.X() = 0.00001;
			THEN( "it can be converted to boolean TRUE" ) {
				REQUIRE( a.Y() == 0. );
				REQUIRE( static_cast<bool>(a) == true );
			}
		}
		WHEN( "the point has non-zero Y") {
			a.Y() = 0.00001;
			THEN( "it can be converted to boolean TRUE" ) {
				REQUIRE( a.X() == 0. );
				REQUIRE( static_cast<bool>(a) == true );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
