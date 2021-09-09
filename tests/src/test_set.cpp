/* test_set.cpp
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
#include "../../source/Set.h"

// ... and any system includes needed for the test file.
#include <string>

namespace { // test namespace
// #region mock data
class T {
public:
	int a = 1;
};
// #endregion mock data


// #region unit tests
SCENARIO( "a Set can be interacted with by consuming classes even when const", "[Set]" ) {
	auto key = std::string{"a value"};
	
	GIVEN( "data for the key does not exist" ) {
		const auto s = Set<T>{};
		REQUIRE( s.size() == 0 );
		REQUIRE_FALSE( s.Has(key) );
		
		WHEN( "Get(key) is called" ) {
			const auto &dataPtr = s.Get(key);
			THEN( "the Set increases in size" ) {
				CHECK( s.size() == 1 );
			}
			THEN( "a valid pointer is returned" ) {
				CHECK( dataPtr != nullptr );
			}
			THEN( "the data is default-constructed" ) {
				const auto &value = *dataPtr;
				CHECK( value.a == 1 );
			}
		}
		
		WHEN( "Find(key) is called" ) {
			const auto &dataPtr = s.Find(key);
			THEN( "the Set does not increase in size" ) {
				CHECK( s.size() == 0 );
			}
			THEN( "nullptr is returned" ) {
				CHECK( dataPtr == nullptr );
			}
		}
	}
	
	GIVEN( "data for the key exists" ) {
		const auto s = Set<T>{};
		const auto &firstPtr = s.Get(key);
		REQUIRE( s.Has(key) );
		
		WHEN( "Get(key) is called" ) {
			const auto &secondPtr = s.Get(key);
			THEN( "the Set does not increase in size" ) {
				CHECK( s.size() == 1 );
			}
			THEN( "the same, valid pointer is returned" ) {
				CHECK( firstPtr == secondPtr );
			}
		}
		
		WHEN( "Find(key) is called" ) {
			const auto &secondPtr = s.Find(key);
			THEN( "the Set does not increase in size" ) {
				CHECK( s.size() == 1 );
			}
			THEN( "the same, valid pointer is returned" ) {
				CHECK( secondPtr == firstPtr );
			}
		}
	}
}


SCENARIO( "A Set can be reverted to an earlier state", "[Set]" ) {
	auto init = [](Set<T> &container, int val) {
		container.Get("A")->a = val;
		container.Get("B")->a = val;
		container.Get("C")->a = val;
	};
	auto original = Set<T>{};
	
	GIVEN( "a Set<T> exists with data" ) {
		init(original, 0);
		
		AND_GIVEN( "another Set<T> exists with the same keys" ) {
			auto instance = original;
			init(instance, 2);
			
			WHEN( "Revert is called on the instance with the original" ) {
				instance.Revert(original);
				THEN( "the instance's data is copied from the original" ) {
					CHECK( instance.Find("A")->a == original.Find("A")->a );
					CHECK( instance.Find("A") != original.Find("A") );
					CHECK( instance.size() == original.size() );
				}
				THEN( "the original Set is unchanged" ) {
					CHECK( original.Find("B")->a == 0 );
				}
				THEN( "changes to the second do not modify the original" ) {
					instance.Get("A")->a = 4;
					CHECK( original.Find("A")->a == 0 );
				}
			}
		}
		
		AND_GIVEN( "another Set<T> exists with a subset of keys" ) {
			auto instance = original;
			instance.Get("D")->a = 3;
			
			WHEN( "Revert is called on the instance with the original" ) {
				instance.Revert(original);
				THEN( "the instance's keys are that of the original" ) {
					CHECK( instance.Has("A") );
					CHECK_FALSE( instance.Has("D") );
					CHECK( instance.size() == original.size() );
				}
				THEN( "the instance's data is copied from the original" ) {
					CHECK( instance.Find("A")->a == original.Find("A")->a );
					CHECK( instance.Find("A") != original.Find("A") );
				}
			}
		}
	}
}
// #endregion unit tests



} // test namespace
