#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/text/layout.hpp"

// ... and any system includes needed for the test file.
#include <type_traits>

namespace { // test namespace
using T = Layout;

// #region unit tests
TEST_CASE( "Layout class", "[text][layout]" ) {
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		CHECK( std::is_trivially_copy_constructible<T>::value );
		CHECK( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
		SECTION( "Constructor Arguments" ) {
			CHECK( std::is_constructible<T, int>::value );
			CHECK( std::is_constructible<T, Alignment>::value );
			CHECK( std::is_constructible<T, Truncate>::value );
			CHECK( std::is_constructible<T, int, Alignment, Truncate>::value );
		}
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
