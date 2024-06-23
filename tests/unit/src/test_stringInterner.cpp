/* test_stringInterner.cpp
Copyright (c) 2021 by Peter van der Meer

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
#include "../../../source/StringInterner.h"

// ... and any system includes needed for the test file.

namespace { // test namespace
// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Interning String", "[StringInterner]" ) {
	GIVEN( "An empty start" ) {
		std::string ahAh = "ah ah";
		std::string blaBla = "bla bla";
		std::string daDa = "da da";
		const char *blaBlaPtr = nullptr;

		WHEN( "the string is interned" ) {
			blaBlaPtr = StringInterner::Intern(blaBla);
			THEN( "it still represents the same string" ) {
				// This is a string comparison (since blaBla is a string).
				CHECK( blaBla == blaBlaPtr );
			}
			THEN( "interning twice results in the same char pointer twice" ) {
				const char *blaBlaPtr2 = StringInterner::Intern(blaBla);
				// This is a pointer comparison (since both arguments are char-pointers).
				CHECK( blaBlaPtr2 == blaBlaPtr );
			}
			THEN( "interning other strings still results in the same char pointer" ) {
				const char *blaBlaPtr3 = StringInterner::Intern(blaBla);
				// This is a pointer comparison (since both arguments are char-pointers).
				CHECK( blaBlaPtr3 == blaBlaPtr );
				StringInterner::Intern(ahAh);
				StringInterner::Intern(daDa);
				const char *blaBlaPtr4 = StringInterner::Intern(blaBla);
				// This is a pointer comparison (since both arguments are char-pointers).
				CHECK( blaBlaPtr4 == blaBlaPtr );
				// Those are string comparisons (since blaBla is a string).
				CHECK( blaBla == blaBlaPtr4 );
			}
			THEN( "interning another string results in another pointer" )
			{
				const char *daDaPtr = StringInterner::Intern(daDa);
				// This is a pointer comparison (since both arguments are char-pointers).
				CHECK( blaBlaPtr != daDaPtr );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
