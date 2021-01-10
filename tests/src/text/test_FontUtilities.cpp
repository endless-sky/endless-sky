#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/text/FontUtilities.h"

// ... and any system includes needed for the test file.

namespace { // test namespace
using namespace FontUtilities;

// #region mock data
// #endregion mock data



// #region unit tests
TEST_CASE( "FontUtilities namespace", "[text][FontUtilities]" ) {
	SECTION( "Function Escape" ) {
		CHECK( Escape("&abcd") == "&amp;abcd" );
		CHECK( Escape("a&bcd") == "a&amp;bcd" );
		CHECK( Escape("abc&d") == "abc&amp;d" );
		CHECK( Escape("abcd&") == "abcd&amp;" );
		
		CHECK( Escape("ab<cd") == "ab&lt;cd" );
		CHECK( Escape("ab>cd") == "ab&gt;cd" );
		
		CHECK( Escape("&<a>") == "&amp;&lt;a&gt;" );
		CHECK( Escape("a&<b>c") == "a&amp;&lt;b&gt;c" );
		
		CHECK( Escape("a&amp&ltb&gtc") == "a&amp;amp&amp;ltb&amp;gtc" );
	}
	SECTION( "Function Unescape" ) {
		CHECK( Unescape("&amp;abcd") == "&abcd" );
		CHECK( Unescape("a&amp;bcd") == "a&bcd" );
		CHECK( Unescape("abc&amp;d") == "abc&d" );
		CHECK( Unescape("abcd&amp;") == "abcd&" );
		
		CHECK( Unescape("ab&lt;cd") == "ab<cd" );
		CHECK( Unescape("ab&gt;cd") == "ab>cd" );
		
		CHECK( Unescape("&amp;&lt;a&gt;") == "&<a>" );
		CHECK( Unescape("a&amp;&lt;b&gt;c") == "a&<b>c" );
		
		CHECK( Unescape("a&amp&ltb&gtc") == "a&amp&ltb&gtc" );
	}
}
// #endregion unit tests



} // test namespace
