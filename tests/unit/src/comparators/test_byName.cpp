/* test_byName.cpp
Copyright (c) 2022 by Michael Zahniser

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
#include "../../../source/comparators/ByName.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
class NamedObject {
public:
	NamedObject(int name) : name(name) {}
	int Name() const { return name; };
	int name;
};
// #endregion mock data



// #region unit tests
SCENARIO( "Test basic ByName functionality." , "[ByName]" ) {
	NamedObject values[3] = { 10, 20, 30 };
	ByName<NamedObject> c;

	CHECK( c(&values[0], &values[0]) == false );
	CHECK( c(&values[0], &values[1]) == true );
	CHECK( c(&values[0], &values[2]) == true );

	CHECK( c(&values[1], &values[0]) == false );
	CHECK( c(&values[1], &values[1]) == false );
	CHECK( c(&values[1], &values[2]) == true );

	CHECK( c(&values[2], &values[0]) == false );
	CHECK( c(&values[2], &values[1]) == false );
	CHECK( c(&values[2], &values[2]) == false );

}
// #endregion unit tests



} // test namespace
