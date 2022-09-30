/* test_bitset.cpp
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
#include "../../../source/Bitset.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a Bitset instance", "[bitset]") {
	GIVEN( "an instance" ) {
		Bitset bitset;
		THEN( "it has the correct default properties" ) {
			CHECK( bitset.Size() == 0 );
			CHECK( bitset.Capacity() == 0 );
			CHECK( bitset.None() );
			CHECK( !bitset.Any() );
		}
	}
}

SCENARIO( "A Bitset instance is being copied", "[bitset]" ) {
	Bitset bitset;
	GIVEN( "a specific bitset" ) {
		bitset.Resize(5);
		bitset.Set(1);
		bitset.Set(3);

		WHEN( "the copy is made" ) {
			auto copy = bitset;
			THEN( "the copy has the correct properties" ) {
				CHECK( copy.Size() == bitset.Size() );
				CHECK( copy.Intersects(bitset) );
				CHECK( copy.Test(0) == bitset.Test(0) );
				CHECK( copy.Test(1) == bitset.Test(1) );
				CHECK( copy.Test(2) == bitset.Test(2) );
				CHECK( copy.Test(3) == bitset.Test(3) );
				CHECK( copy.Test(4) == bitset.Test(4) );
				CHECK( copy.Any() == bitset.Any() );
				CHECK( copy.None() == bitset.None() );
			}
			THEN( "the two bitsets are independent" ) {
				bitset.Set(0);
				CHECK( bitset.Test(0) );
				CHECK_FALSE( copy.Test(0) );

				copy.Set(4);
				CHECK_FALSE( bitset.Test(4) );
				CHECK( copy.Test(4) );
			}
		}
	}
}

SCENARIO( "A Bitset instance is being used", "[bitset]") {
	GIVEN( "an empty bitset" ) {
		Bitset bitset;
		THEN( "resizing it works" ) {
			bitset.Resize(10);
			CHECK( bitset.Size() >= 10 );
			CHECK( bitset.Capacity() >= 10 );
		}
	}
	GIVEN( "a bitset of a specific size" ) {
		Bitset bitset;
		bitset.Resize(10);

		REQUIRE( bitset.Size() >= 10 );
		THEN( "setting and testing bits works" ) {
			CHECK( bitset.None() );

			bitset.Set(4);
			CHECK_FALSE( bitset.Test(3) );
			CHECK( bitset.Test(4) );

			CHECK_FALSE( bitset.Test(5) );
			bitset.Set(5);
			CHECK( bitset.Test(5) );

			CHECK( bitset.Any() );
		}
		THEN( "clearing it works" ) {
			bitset.Clear();
			CHECK( bitset.Size() == 0 );
			CHECK( bitset.None() );
			CHECK_FALSE( bitset.Any() );
		}
	}
	GIVEN( "two non-empty bitsets" ) {
		Bitset one;
		one.Resize(4);
		one.Set(0);
		one.Set(1);

		Bitset two;
		two.Resize(3);
		two.Set(2);

		THEN( "bit intersect works" ) {
			CHECK_FALSE( one.Intersects(two) );
			CHECK_FALSE( two.Intersects(one) );

			two.Set(1);
			CHECK( one.Intersects(two) );
			CHECK( two.Intersects(one) );
		}
	}
}

TEST_CASE( "Large bitsets", "[bitset]") {
	int size = GENERATE(5, 10, 20, 35, 75, 100, 150, 350, 800, 1400, 2000, 3000, 4500, 6000);

	Bitset bitset;
	bitset.Resize(size);

	REQUIRE( static_cast<int>(bitset.Size()) >= size );
	CHECK( static_cast<int>(bitset.Capacity()) >= size );

	CHECK( bitset.None() );
	CHECK_FALSE( bitset.Any() );

	// Test values at various indices.
	auto increment = GENERATE(1, 3, 7, 13);
	for(int i = 0; i < size; i += increment)
		bitset.Set(i);
	for(int i = 0; i < size; ++i)
		if(i % increment)
			CHECK_FALSE( bitset.Test(i) );
		else
			CHECK( bitset.Test(i) );

	CHECK_FALSE( bitset.None() );
	CHECK( bitset.Any() );
}

// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests



} // test namespace
