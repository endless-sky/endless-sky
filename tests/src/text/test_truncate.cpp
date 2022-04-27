/* text/test_truncate.cpp
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
#include "../../../source/text/truncate.hpp"

// ... and any system includes needed for the test file.
#include <type_traits>

namespace { // test namespace
using T = Truncate;

// #region unit tests
TEST_CASE( "Truncate enum", "[enums][truncate]" ) {
	SECTION( "Class Traits" ) {
		CHECK( std::is_enum<T>::value );
		CHECK( std::is_trivial<T>::value );
		CHECK( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK( std::is_trivially_default_constructible<T>::value );
		CHECK( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		CHECK( std::is_trivially_copy_constructible<T>::value );
		CHECK( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable<T>::value );
		CHECK( std::is_trivially_copyable<T>::value );
		CHECK( std::is_trivially_copy_assignable<T>::value );
		CHECK( std::is_nothrow_copy_assignable<T>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}
// #endregion unit tests



} // test namespace
