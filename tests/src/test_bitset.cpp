/* test_bitset.cpp
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/Bitset.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a Bitset instance", "[bitset]") {
	GIVEN( "an instance" ) {
		Bitset bitset;
		THEN( "it has the correct default properties" ) {
			CHECK( bitset.size() == 0 );
		}
	}
}

SCENARIO( "A Bitset instance is being copied", "[bitset]" ) {
	Bitset bitset;
	GIVEN( "a specific bitset" ) {
		bitset.resize(5);
		bitset.set(1);
		bitset.set(3);

		WHEN( "the copy is made" ) {
			auto copy = bitset;
			THEN( "the copy has the correct properties" ) {
				CHECK( copy.size() == bitset.size() );
				CHECK( copy.intersects(bitset) );
				CHECK( copy.test(0) == bitset.test(0) );
				CHECK( copy.test(1) == bitset.test(1) );
				CHECK( copy.test(2) == bitset.test(2) );
				CHECK( copy.test(3) == bitset.test(3) );
				CHECK( copy.test(4) == bitset.test(4) );
				CHECK( copy.any() == bitset.any() );
				CHECK( copy.none() == bitset.none() );
			}
		}
	}
}

SCENARIO( "A Bitset instance is being used", "[bitset]") {
	GIVEN( "an empty bitset" ) {
		Bitset bitset;
		THEN( "resizing it works" ) {
			bitset.resize(10);
			CHECK( bitset.size() >= 10 );
		}
	}
	GIVEN( "a bitset of a specific size" ) {
		Bitset bitset;
		bitset.resize(10);
		THEN( "setting and testing bits works" ) {
			CHECK( bitset.none() );

			bitset.set(4);
			CHECK_FALSE( bitset.test(3) );
			CHECK( bitset.test(4) );

			CHECK_FALSE( bitset.test(5) );
			bitset.set(5);
			CHECK( bitset.test(5) );

			CHECK( bitset.any() );
		}
		THEN( "clearing it works" ) {
			bitset.clear();
			CHECK( bitset.size() == 0 );
		}
	}
	GIVEN( "two non-empty bitsets" ) {
		Bitset one;
		one.resize(4);
		one.set(0);
		one.set(1);

		Bitset two;
		two.resize(3);
		two.set(1);
		two.set(2);

		THEN( "bit intersect works" ) {
			CHECK( one.intersects(two) );
			CHECK( two.intersects(one) );
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



} // test namespace
