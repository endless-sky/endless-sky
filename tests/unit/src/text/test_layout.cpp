/* test_layout.cpp
Copyright (c) 2020 by Benjamin Hauch

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
#include "../../../../source/text/Layout.h"

// ... and any system includes needed for the test file.
#include <type_traits>

namespace { // test namespace
using T = Layout;

// #region unit tests
TEST_CASE( "Layout class", "[text][layout]" ) {
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial_v<T> );
		CHECK( std::is_standard_layout_v<T> );
		CHECK( std::is_nothrow_destructible_v<T> );
		CHECK( std::is_trivially_destructible_v<T> );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible_v<T> );
		CHECK( std::is_nothrow_default_constructible_v<T> );
		CHECK( std::is_copy_constructible_v<T> );
		CHECK( std::is_trivially_copy_constructible_v<T> );
		CHECK( std::is_nothrow_copy_constructible_v<T> );
		CHECK( std::is_move_constructible_v<T> );
		CHECK( std::is_trivially_move_constructible_v<T> );
		CHECK( std::is_nothrow_move_constructible_v<T> );
		SECTION( "Constructor Arguments" ) {
			CHECK( std::is_constructible_v<T, int> );
			CHECK( std::is_constructible_v<T, Alignment> );
			CHECK( std::is_constructible_v<T, Truncate> );
			CHECK( std::is_constructible_v<T, int, Alignment, Truncate> );
		}
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable_v<T> );
		CHECK( std::is_trivially_copyable_v<T> );
		CHECK( std::is_trivially_copy_assignable_v<T> );
		CHECK( std::is_nothrow_copy_assignable_v<T> );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable_v<T> );
		CHECK( std::is_trivially_move_assignable_v<T> );
		CHECK( std::is_nothrow_move_assignable_v<T> );
	}
}
SCENARIO( "Creating a Layout", "[text][layout]" ) {
	GIVEN( "No arguments" ) {
		Layout layout;
		THEN( "It has the right properties" ) {
			CHECK( layout.width == -1 );
			CHECK( layout.align == Alignment::LEFT );
			CHECK( layout.truncate == Truncate::NONE );
		}
	}
	GIVEN( "Individual arguments" ) {
		WHEN( "the argument is an int" ) {
			const int width = 123;
			auto l = Layout{width};
			THEN( "the width is set" ) {
				CHECK( l.width == width );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
