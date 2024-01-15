/* test_scrollVar.cpp
Copyright (c) 2023 by thewierdnut

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
#include "../../../source/ScrollVar.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ScrollVar", "[ScrollVar][Creation]" ) {
	GIVEN( "a new ScrollVar" ) {
		ScrollVar<double> sv;
		WHEN( "no changes are made to it" ) {
			THEN( "it isn't scrollable" ) {
				CHECK_FALSE(sv.Scrollable());
				CHECK(sv == 0.0);
				CHECK(sv.AnimatedValue() == 0.0);
				CHECK(sv.MaxValue() == 0.0);
			}
		}
		WHEN( "its max scroll value is set to 10" ) {
			sv.SetMaxValue(10.0);
			THEN( "the SrollVar is now scrollable and is at the min scroll" ) {
				CHECK(sv.Scrollable());
				CHECK(sv.IsScrollAtMin());
				CHECK_FALSE(sv.IsScrollAtMax());
				CHECK(sv == 0.0);
				CHECK(sv.AnimatedValue() == 0.0);
				CHECK(sv.MaxValue() == 10.0);
			}
			AND_WHEN( "you scroll 5 units" ) {
				sv.Scroll(5.0);
				THEN( "the ScrollVar is not at the min or max scroll" ) {
					CHECK(sv.Scrollable());
					CHECK_FALSE(sv.IsScrollAtMin());
					CHECK_FALSE(sv.IsScrollAtMax());
					CHECK(sv == 5.0);
					CHECK(sv.AnimatedValue() == 0.0);
					CHECK(sv.MaxValue() == 10.0);
				}
				AND_WHEN( "you step once" ) {
					sv.Step();
					THEN( "the ScrollVar animation advances one unit" ) {
						CHECK(sv.AnimatedValue() == 1.0);
					}
				}
				AND_WHEN( "you step five times" ) {
					for(int i = 0; i < 5; ++i)
						sv.Step();
					THEN( "the ScrollVar animation advances five units" ) {
						CHECK(sv.AnimatedValue() == 5.0);
					}
				}
				AND_WHEN( "you step six times" ) {
					for(int i = 0; i < 6; ++i)
						sv.Step();
					THEN( "the ScrollVar animation advances only five units" ) {
						CHECK(sv.AnimatedValue() == 5.0);
					}
				}
			}
			AND_WHEN( "you scroll 10 units" ) {
				sv.Scroll(10.0);
				THEN( "the ScrollVar is at the max scroll" ) {
					CHECK(sv.Scrollable());
					CHECK_FALSE(sv.IsScrollAtMin());
					CHECK(sv.IsScrollAtMax());
					CHECK(sv == 10.0);
					CHECK(sv.AnimatedValue() == 0.0);
					CHECK(sv.MaxValue() == 10.0);
				}
			}
			AND_WHEN( "you scroll 15 units" ) {
				sv.Scroll(15.0);
				THEN( "the ScrollVar does not exceed the max value" ) {
					CHECK(sv.MaxValue() == 10.0);
					CHECK(sv == 10.0);
				}
			}
			AND_WHEN( "you scroll 10 units and then change the max to 5 units" ) {
				sv.Scroll(10.0);
				sv.SetMaxValue(5.0);
				THEN( "the ScrollVar value is clamped to the new max") {
					CHECK(sv.MaxValue() == 5.0);
					CHECK(sv == 5.0);
				}
			}
			AND_WHEN( "you set the display size to 5" )
			{
				sv.SetDisplaySize(5.0);
				THEN( "the ScrollVar is scrollable up to 5 units" ) {
					CHECK(sv.Scrollable());
					CHECK(sv.IsScrollAtMin());
					CHECK(!sv.IsScrollAtMax());

					sv.Scroll(10.0);
					CHECK(!sv.IsScrollAtMin());
					CHECK(sv.IsScrollAtMax());
					CHECK(sv == 5.0);
				}
			}
			AND_WHEN( "you set the display size to 10" )
			{
				sv.SetDisplaySize(10.0);
				THEN( "the ScrollVar is no longer scrollable and is at the min scroll" ) {
					CHECK_FALSE(sv.Scrollable());
					CHECK(sv == 0.0);
					CHECK(sv.AnimatedValue() == 0.0);
				}
			}
		}
	}
}

// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests

// #region benchmarks

// #endregion benchmarks



} // test namespace
