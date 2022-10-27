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
	CHECK( defaultAngle.Degrees() == Approx(0.) );
	Point defaultUnit = defaultAngle.Unit();
	CHECK( defaultUnit.X() == Approx(0.) );
	CHECK( defaultUnit.Y() == Approx(-1.) );
	CHECK( Angle(defaultUnit).Degrees() == Approx(defaultAngle.Degrees()) );

	Angle halfAngle = 180.;
	CHECK( halfAngle.Degrees() == Approx(-180.) );
	Point halfUnit = halfAngle.Unit();
	CHECK( halfUnit.X() == Approx(0.).margin(0.01) );
	CHECK( halfUnit.Y() == Approx(1.) );
	CHECK( Angle(halfUnit).Degrees() == Approx(halfAngle.Degrees()) );

	Angle fullAngle = 360.;
	CHECK( fullAngle.Degrees() == Approx(0.) );
	Point fullUnit = fullAngle.Unit();
	CHECK( fullUnit.X() == Approx(0.) );
	CHECK( fullUnit.Y() == Approx(-1.) );
	CHECK( Angle(fullUnit).Degrees() == Approx(fullAngle.Degrees()) );
}

TEST_CASE( "Angle::Rotate", "[angle][rotate]" ) {
	Angle angle = 180.;
	REQUIRE( angle.Degrees() == Approx(-180.) );

	auto rotate1 = angle.Rotate(Point(0., 1.));
	CHECK( rotate1.X() == Approx(0.).margin(0.01) );
	CHECK( rotate1.Y() == Approx(-1.) );

	auto rotate2 = angle.Rotate(Point(1., -1.));
	CHECK( rotate2.X() == Approx(-1.) );
	CHECK( rotate2.Y() == Approx(1.) );
}

TEST_CASE( "Angle arithmetic", "[angle][arithmetic]") {
	Angle angle = 60.;
	REQUIRE( angle.Degrees() == Approx(60.).margin(0.05) );

	angle += 45.;
	REQUIRE( angle.Degrees() == Approx(105.).margin(0.05) );

	angle = angle + 100.;
	REQUIRE( angle.Degrees() == Approx(-155.).margin(0.05) );

	angle -= 50.;
	REQUIRE( angle.Degrees() == Approx(155.).margin(0.05) );

	angle = angle - 25.;
	REQUIRE( angle.Degrees() == Approx(130.).margin(0.05) );

	angle = -angle;
	REQUIRE( angle.Degrees() == Approx(-130.).margin(0.05) );
}

TEST_CASE( "Angle::Random", "[angle][random]") {
	auto value = GENERATE(10, 100, 1000, 10000, 100000, 1000000, 3600000);

	for(int i = 0; i < 10; ++i)
	{
		auto random = Angle::Random(value);
		CHECK( random.Degrees() >= Approx(-180.).margin(0.05));
		CHECK( random.Degrees() <= Approx(180.).margin(0.05));

		auto unit = random.Unit();
		CHECK( unit.X() >= Approx(-1.) );
		CHECK( unit.X() <= Approx(1.) );
		CHECK( unit.Y() >= Approx(-1.) );
		CHECK( unit.Y() <= Approx(1.) );
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
