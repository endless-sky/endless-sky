/* test_weightedList.cpp
Copyright (c) 2021 by Jonathan Steck

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/WeightedList.h"

// ... and any system includes needed for the test file.
#include <stdexcept>

namespace { // test namespace

// #region mock data
// WeightedList contains objects with a Weight() function that returns an integer.
class WeightedObject {
public:
	WeightedObject(int value, int weight) : value(value), weight(weight) {}
	// Some data that the object holds.
	int value;
	// This object's weight.
	int Weight() const { return weight; };
	int weight;
};
// #endregion mock data



// #region unit tests
SCENARIO( "Test basic WeightedSet functionality." , "[WeightedList]" ) {
	auto list = WeightedList<WeightedObject>{};
	GIVEN( "A new weighted list." ) {
		THEN( "The list is empty." ) {
			REQUIRE( list.empty() );
			REQUIRE( list.size() == 0 );
		}
		THEN( "The list has no weight." ) {
			REQUIRE( list.TotalWeight() == 0 );
		}
		
		WHEN( "One object is added to the list." ) {
			list.emplace_back(1, 2);
			THEN( "The list is no longer empty." ) {
				REQUIRE_FALSE( list.empty() );
				REQUIRE( list.size() == 1 );
			}
			THEN( "The list has a total weight of 2." ) {
				REQUIRE( list.TotalWeight() == 2 );
			}
			THEN( "The list only returns the one object inserted into it." ) {
				CHECK( list.Get().value == 1 );
				CHECK( list.Get().Weight() == 2 );
			}
			THEN( "The list is unchanged after calling Get." ) {
				CHECK_FALSE( list.empty() );
				CHECK( list.size() == 1 );
				CHECK( list.TotalWeight() == 2 );
			}
			
			AND_WHEN( "A second object is added to the list." ) {
				list.emplace_back(2, 3);
				THEN( "The list has increased in size and weight." ) {
					REQUIRE_FALSE( list.empty() );
					REQUIRE( list.size() == 2 );
					REQUIRE( list.TotalWeight() == 5 );
				}
				THEN( "The object at the back of the list is the most recently inserted." ) {
					CHECK( list.back().value == 2 );
					CHECK( list.back().Weight() == 3 );
				}
			}
			
			AND_WHEN( "The list is cleared" ) {
				list.clear();
				THEN( "The list is now empty." ) {
					REQUIRE( list.empty() );
					REQUIRE( list.size() == 0 );
				}
				THEN( "The list no longer has any weight." ) {
					REQUIRE( list.TotalWeight() == 0 );
				}
			}
		}
	}
}

SCENARIO( "Test WeightedList error conditions.", "[WeightedList]" ) {
	auto list = WeightedList<WeightedObject>{};
	GIVEN( "A new weighed list." ) {
		REQUIRE( list.empty() );
		WHEN( "Attempting to get from an empty list." ) {
			THEN( "A runtime error exception is thrown." ) {
				try{
					list.Get();
					REQUIRE( false );
				} catch(const std::runtime_error &e) {
					REQUIRE( true );
				}
			}
		}
		
		WHEN( "Attempting to insert a negative weighted object." ) {
			THEN( "An invalid argument exception is thrown." ) {
				try{
					list.emplace_back(1, -1);
					REQUIRE( false );
				} catch(const std::invalid_argument &e) {
					REQUIRE( true );
					AND_THEN( "The invalid object was not inserted into the list." ) {
						REQUIRE( list.empty() );
						REQUIRE( list.size() == 0 );
						REQUIRE( list.TotalWeight() == 0 );
					}
				}
			}
		}
	}
}
// #endregion unit tests



} // test namespace
