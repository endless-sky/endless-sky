/* test_angleIsInRange.cpp
Copyright (c) 2024 by 1010todd

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
#include "../../../source/Angle.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "Angle::IsInRange", "[angle][isInRange]" ) {
	Angle base = 0.;
	Angle limit = 359.;
	REQUIRE( Angle(0.).IsInRange(base, limit) );
	REQUIRE( Angle(359.).IsInRange(base, limit) );
	REQUIRE( Angle(180.).IsInRange(base, limit) );

	base = -20.;
	limit = 20.;
	REQUIRE( Angle(0.).IsInRange(base, limit) );
	REQUIRE( Angle(20.).IsInRange(base, limit) );
	REQUIRE( Angle(-20.).IsInRange(base, limit) );
	REQUIRE_FALSE( Angle(21.).IsInRange(base, limit) );
	REQUIRE_FALSE( Angle(-21.).IsInRange(base, limit) );
	REQUIRE_FALSE( Angle(180.).IsInRange(base, limit) );
}
// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests



} // test namespace
