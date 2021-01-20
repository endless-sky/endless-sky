#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/text/alignment.hpp"

// ... and any system includes needed for the test file.
#include <type_traits>

namespace { // test namespace
using T = Alignment;

// #region unit tests
TEST_CASE( "Alignment enum", "[enums][alignment]" ) {
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
