/* test_displaytext.cpp
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
#include "../../../source/text/DisplayText.h"

// ... and any system includes needed for the test file.
#include <string>
#include <type_traits>

namespace { // test namespace
using T = DisplayText;

// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "DisplayText class", "[text][DisplayText]" ) {
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial_v<T> );
		CHECK( std::is_nothrow_destructible_v<T> );
		// Contains a string, thus will not be standard layout unless std::string is.
		CHECK( std::is_standard_layout_v<T> ==
			std::is_standard_layout_v<std::string> );
		// Contains a string, thus will not be trivially destructible unless std::string is.
		CHECK_FALSE( std::is_trivially_destructible_v<T> );
		CHECK( std::is_trivially_destructible_v<T> ==
			std::is_trivially_destructible_v<std::string> );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible_v<T> );
		// In gcc-5 (Steam Scout Runtime), std::string is not default nothrow constructible
		CHECK( std::is_nothrow_default_constructible_v<T> ==
			std::is_nothrow_default_constructible_v<std::string> );
		CHECK( std::is_copy_constructible_v<T> );
		// Copying a string is not "trivial."
		CHECK_FALSE( std::is_trivially_copy_constructible_v<T> );
		CHECK( std::is_trivially_copy_constructible_v<T> ==
			std::is_trivially_copy_constructible_v<std::string> );
		// Copying a string may throw.
		CHECK_FALSE( std::is_nothrow_copy_constructible_v<T> );
		CHECK( std::is_nothrow_copy_constructible_v<T> ==
			std::is_nothrow_copy_constructible_v<std::string> );
		CHECK( std::is_move_constructible_v<T> );
		// Moving a string is not "trivial."
		CHECK_FALSE( std::is_trivially_move_constructible_v<T> );
		CHECK( std::is_trivially_move_constructible_v<T> ==
			std::is_trivially_move_constructible_v<std::string> );
		CHECK( std::is_nothrow_move_constructible_v<T> );
		SECTION( "Constructor Arguments" ) {
			CHECK_FALSE( std::is_constructible_v<T, const char *> );
			CHECK( std::is_constructible_v<T, const char *, Layout> );
			CHECK_FALSE( std::is_constructible_v<T, std::string> );
			CHECK( std::is_constructible_v<T, std::string, Layout> );
		}
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable_v<T> );
		CHECK_FALSE( std::is_trivially_copyable_v<T> );
		CHECK( std::is_trivially_copyable_v<T> ==
			std::is_trivially_copyable_v<std::string> );
		CHECK_FALSE( std::is_trivially_copy_assignable_v<T> );
		CHECK( std::is_trivially_copy_assignable_v<T> ==
			std::is_trivially_copy_assignable_v<std::string> );
		CHECK_FALSE( std::is_nothrow_copy_assignable_v<T> );
		CHECK( std::is_nothrow_copy_assignable_v<T> ==
			std::is_nothrow_copy_assignable_v<std::string> );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable_v<T> );
		CHECK_FALSE( std::is_trivially_move_assignable_v<T> );
		CHECK( std::is_trivially_move_assignable_v<T> ==
			std::is_trivially_move_assignable_v<std::string> );
		// In gcc-5 (Steam Scout Runtime), move-assigning a string may throw
		CHECK( std::is_nothrow_move_assignable_v<T> ==
			std::is_nothrow_move_assignable_v<std::string> );
	}
}
// #endregion unit tests



} // test namespace
