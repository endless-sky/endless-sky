/* test_random.cpp
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
#include "../../source/Random.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "Random::Int", "[random]") {
	REQUIRE( Random::Int(1) == 0 );
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
TEST_CASE( "Benchmark Random::Int", "[!benchmark][random]" ) {
	BENCHMARK( "Random::Int()" ) {
		return Random::Int();
	};
	BENCHMARK( "Random::Int(60)" ) {
		return Random::Int(60);
	};
	BENCHMARK( "Random::Int(600)" ) {
		return Random::Int(600);
	};
	BENCHMARK( "Random::Int(10000)" ) {
		return Random::Int(10000);
	};
}
TEST_CASE( "Benchmark Random::Real", "[!benchmark][random]" ) {
	BENCHMARK( "Random::Real" ) {
		return Random::Real();
	};
}
#endif
// #endregion benchmarks



} // test namespace
