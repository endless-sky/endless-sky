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
TEST_CASE( "ScrollVar::ScrollVar()" ) {
	ScrollVar<double> sv;
	CHECK_FALSE(sv.Scrollable());
	CHECK(sv == 0.0);
	CHECK(sv.AnimatedValue() == 0.0);
	CHECK(sv.MaxValue() == 0.0);
}

TEST_CASE( "ScrollVar::SetMaxValue()" ) {
	ScrollVar<double> sv;
	sv.SetMaxValue(10.0);
	CHECK(sv.Scrollable());
	CHECK(sv.IsScrollAtMin());
	CHECK(!sv.IsScrollAtMax());
	CHECK(sv == 0.0);
	CHECK(sv.AnimatedValue() == 0.0);
	CHECK(sv.MaxValue() == 10.0);
}

TEST_CASE( "ScrollVar::Scroll()" ) {
	ScrollVar<double> sv;
	sv.SetMaxValue(10.0);
	sv.Scroll(5.0);
	CHECK(sv.Scrollable());
	CHECK(!sv.IsScrollAtMin());
	CHECK(!sv.IsScrollAtMax());
	CHECK(sv == 5.0);
	CHECK(sv.AnimatedValue() == 0.0);
	CHECK(sv.MaxValue() == 10.0);
}

TEST_CASE( "ScrollVar::Step()" ) {
	ScrollVar<double> sv;
	sv.SetMaxValue(10.0);
	sv.Scroll(5.0);
	CHECK(sv.Scrollable());
	CHECK(!sv.IsScrollAtMin());
	CHECK(!sv.IsScrollAtMax());
	CHECK(sv == 5.0);
	CHECK(sv.AnimatedValue() == 0.0);
	CHECK(sv.MaxValue() == 10.0);
	sv.Step();
	CHECK(sv.AnimatedValue() == Approx(1.0));
	sv.Step();
	CHECK(sv.AnimatedValue() == Approx(2.0));
	sv.Step();
	CHECK(sv.AnimatedValue() == Approx(3.0));
	sv.Step();
	CHECK(sv.AnimatedValue() == Approx(4.0));
	sv.Step();
	CHECK(sv.AnimatedValue() == Approx(5.0));
	sv.Step();
	CHECK(sv.AnimatedValue() == Approx(5.0));
}

TEST_CASE( "ScrollVar::Clamp()" ) {
	ScrollVar<double> sv;
	sv.SetMaxValue(10.0);
	sv.Scroll(15.0);

	CHECK(sv.MaxValue() == 10.0);
	CHECK(sv == 10.0);

	sv.SetMaxValue(5.0);
	CHECK(sv.MaxValue() == 5.0);
	CHECK(sv == 5.0);
}

TEST_CASE( "ScrollVar::SetDisplaySize()" ) {
	ScrollVar<double> sv;
	sv.SetMaxValue(10.0);
	sv.SetDisplaySize(10);
	CHECK(!sv.Scrollable());

	sv.SetDisplaySize(5.0);
	CHECK(sv.Scrollable());
	CHECK(sv.IsScrollAtMin());
	CHECK(!sv.IsScrollAtMax());

	sv.Scroll(10.0);
	CHECK(!sv.IsScrollAtMin());
	CHECK(sv.IsScrollAtMax());
	CHECK(sv == 5.0);
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
