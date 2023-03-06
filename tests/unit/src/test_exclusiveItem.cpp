/* test_exclusiveItem.cpp
Copyright (c) 2022 by Amazinite

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
#include "../../../source/ExclusiveItem.h"

// ... and any system includes needed for the test file.

namespace { // test namespace

// #region mock data
class Object {
	// Some data that the object holds.
	int value = 0;
public:
	Object() = default;
	Object(int value) : value(value) {}
	int GetValue() const { return value; }
	bool operator==(const Object &other) const { return this->value == other.value; }
};
// #endregion mock data



// #region unit tests
SCENARIO( "Creating an ExclusiveItem" , "[ExclusiveItem][Creation]" ) {
	GIVEN( "an exclusive item" ) {
		auto item = ExclusiveItem<Object>{};

		WHEN( "it is default-constructed" ) {
			THEN( "it contains default data" ) {
				CHECK_FALSE( item.IsStock() );
				CHECK( item->GetValue() == 0 );
			}
		}

		WHEN( "constructed with an rvalue reference" ) {
			item = ExclusiveItem<Object>(Object(2));
			THEN( "the object is obtainable and the item is nonstock" ) {
				CHECK_FALSE( item.IsStock() );
				CHECK( item->GetValue() == 2 );
			}
		}

		WHEN( "constructed with a pointer to an instance" ) {
			auto obj = Object(3);
			item = ExclusiveItem<Object>(&obj);
			THEN( "the object is obtainable and the item is stock" ) {
				CHECK( item.IsStock() );
				CHECK( item->GetValue() == 3 );
			}
		}
	}
	GIVEN( "two exclusive items" ) {
		auto firstItem = ExclusiveItem<Object>{};
		auto secondItem = ExclusiveItem<Object>{};

		WHEN( "the contents are equivalent" ) {
			const auto obj = Object(2);
			firstItem = ExclusiveItem<Object>(&obj);
			secondItem = ExclusiveItem<Object>(Object(2));
			THEN( "the exclusive items are equivalent" ) {
				CHECK( firstItem == secondItem );
			}
		}

		WHEN( "the contents are not equivalent" ) {
			firstItem = ExclusiveItem<Object>(Object(2));
			secondItem = ExclusiveItem<Object>(Object(3));
			THEN( "the exclusive items are not equivalent" ) {
				CHECK( firstItem != secondItem );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
