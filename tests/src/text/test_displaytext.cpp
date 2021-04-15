/* text/test_displaytext.cpp
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
#include "../../source/text/DisplayText.h"

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
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		// Contains a string, thus will not be trivially destructible unless std::string is.
		CHECK_FALSE( std::is_trivially_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value ==
			std::is_trivially_destructible<std::string>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		// Copying a string is not "trivial."
		CHECK_FALSE( std::is_trivially_copy_constructible<T>::value );
		CHECK( std::is_trivially_copy_constructible<T>::value ==
			std::is_trivially_copy_constructible<std::string>::value );
		// Copying a string may throw.
		CHECK_FALSE( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_nothrow_copy_constructible<T>::value ==
			std::is_nothrow_copy_constructible<std::string>::value );
		CHECK( std::is_move_constructible<T>::value );
		// Moving a string is not "trivial."
		CHECK_FALSE( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value ==
			std::is_trivially_move_constructible<std::string>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
		SECTION( "Constructor Arguments" ) {
			CHECK_FALSE( std::is_constructible<T, const char *>::value );
			CHECK( std::is_constructible<T, const char *, Layout>::value );
			CHECK_FALSE( std::is_constructible<T, std::string>::value );
			CHECK( std::is_constructible<T, std::string, Layout>::value );
		}
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable<T>::value );
		CHECK_FALSE( std::is_trivially_copyable<T>::value );
		CHECK( std::is_trivially_copyable<T>::value ==
			std::is_trivially_copyable<std::string>::value );
		CHECK_FALSE( std::is_trivially_copy_assignable<T>::value );
		CHECK( std::is_trivially_copy_assignable<T>::value ==
			std::is_trivially_copy_assignable<std::string>::value );
		CHECK_FALSE( std::is_nothrow_copy_assignable<T>::value );
		CHECK( std::is_nothrow_copy_assignable<T>::value ==
			std::is_nothrow_copy_assignable<std::string>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK_FALSE( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_trivially_move_assignable<T>::value ==
			std::is_trivially_move_assignable<std::string>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}
// #endregion unit tests



} // test namespace
