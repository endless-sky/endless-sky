/* test_angle.cpp
Copyright (c) 2021 by quyykk

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
TEST_CASE( "Angle::Angle", "[angle]") {
	Angle defaultAngle;
	CHECK_THAT( defaultAngle.Degrees(), Catch::Matchers::WithinAbs(0., 0.0001) );
	Point defaultUnit = defaultAngle.Unit();
	CHECK_THAT( defaultUnit.X(), Catch::Matchers::WithinAbs(0., 0.0001) );
	CHECK_THAT( defaultUnit.Y(), Catch::Matchers::WithinAbs(-1., 0.0001) );
	CHECK_THAT( Angle(defaultUnit).Degrees(), Catch::Matchers::WithinAbs(defaultAngle.Degrees(), 0.0001) );

	Angle halfAngle = 180.;
	CHECK_THAT( halfAngle.Degrees(), Catch::Matchers::WithinAbs(-180., 0.0001) );
	Point halfUnit = halfAngle.Unit();
	CHECK_THAT( halfUnit.X(), Catch::Matchers::WithinAbs(0., 0.01) || Catch::Matchers::WithinRel(0.0001) );
	CHECK_THAT( halfUnit.Y(), Catch::Matchers::WithinAbs(1., 0.0001) );
	CHECK_THAT( Angle(halfUnit).Degrees(), Catch::Matchers::WithinAbs(halfAngle.Degrees(), 0.0001) );

	Angle fullAngle = 360.;
	CHECK_THAT( fullAngle.Degrees(), Catch::Matchers::WithinAbs(0., 0.0001) );
	Point fullUnit = fullAngle.Unit();
	CHECK_THAT( fullUnit.X(), Catch::Matchers::WithinAbs(0., 0.0001) );
	CHECK_THAT( fullUnit.Y(), Catch::Matchers::WithinAbs(-1., 0.0001) );
	CHECK_THAT( Angle(fullUnit).Degrees(), Catch::Matchers::WithinAbs(fullAngle.Degrees(), 0.0001) );
}

TEST_CASE( "Angle::Rotate", "[angle][rotate]" ) {
	Angle angle = 180.;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(-180., 0.0001) );

	auto rotate1 = angle.Rotate(Point(0., 1.));
	CHECK_THAT( rotate1.X(), Catch::Matchers::WithinAbs(0., 0.01) || Catch::Matchers::WithinRel(0.0001) );
	CHECK_THAT( rotate1.Y(), Catch::Matchers::WithinAbs(-1., 0.0001) );

	auto rotate2 = angle.Rotate(Point(1., -1.));
	CHECK_THAT( rotate2.X(), Catch::Matchers::WithinAbs(-1., 0.0001) );
	CHECK_THAT( rotate2.Y(), Catch::Matchers::WithinAbs(1., 0.0001) );
}

TEST_CASE( "Angle arithmetic", "[angle][arithmetic]") {
	Angle angle = 60.;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(60., 0.05) || Catch::Matchers::WithinRel(0.0001) );

	angle += 45.;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(105., 0.05) || Catch::Matchers::WithinRel(0.0001) );

	angle = angle + 100.;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(-155., 0.05) || Catch::Matchers::WithinRel(0.0001) );

	angle -= 50.;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(155., 0.05) || Catch::Matchers::WithinRel(0.0001) );

	angle = angle - 25.;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(130., 0.05) || Catch::Matchers::WithinRel(0.0001) );

	angle = -angle;
	REQUIRE_THAT( angle.Degrees(), Catch::Matchers::WithinAbs(-130., 0.05) || Catch::Matchers::WithinRel(0.0001) );
}

TEST_CASE( "Angle::Random", "[angle][random]") {
	auto value = GENERATE(10, 100, 1000, 10000, 100000, 1000000, 3600000);

	for(int i = 0; i < 10; ++i)
	{
		auto random = Angle::Random(value);
		CHECK_THAT( random.Degrees(), Catch::Matchers::WithinAbs(0, 180) );

		auto unit = random.Unit();
		CHECK_THAT( unit.X(), Catch::Matchers::WithinAbs(0, 1) );
		CHECK_THAT( unit.Y(), Catch::Matchers::WithinAbs(0, 1) );
	}
}

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

// #region benchmarks
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
TEST_CASE( "Benchmark Angle::Random", "[!benchmark][angle][random]" ) {
	BENCHMARK( "Angle::Random()" ) {
		return Angle::Random();
	};
	BENCHMARK( "Angle::Random(60.)" ) {
		return Angle::Random(60.);
	};
	BENCHMARK( "Angle::Random(180.)" ) {
		return Angle::Random(180.);
	};
	BENCHMARK( "Angle::Random(360.)" ) {
		return Angle::Random(360.);
	};
}
#endif
// #endregion benchmarks



} // test namespace
