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
			
				AND_WHEN( "A single element is erased." ) {
					auto it = list.eraseAt(list.begin());
					
					THEN( "The list has decreased in size and weight." ) {
						CHECK_FALSE( list.empty() );
						CHECK( list.size() == 1 );
						CHECK( list.TotalWeight() == 3 );
					}
					THEN( "An iterator pointing to the next object in the list is returned." ) {
						REQUIRE( it != list.end() );
						CHECK( it->value == 2 );
						CHECK( it->Weight() == 3 );
					}
				}
				
				AND_WHEN( "A range is erased from begin to end." ) {
					list.erase(list.begin(), list.end());
					THEN( "The list is empty." ) {
						CHECK( list.empty() );
						CHECK( list.TotalWeight() == 0 );
					}
				}
				
				AND_WHEN( "A range is erased from the middle." ) {
					// Add more objects to the list so that a range can be deleted.
					list.emplace_back(3, 1);
					list.emplace_back(4, 5);
					list.emplace_back(5, 3);
					REQUIRE( list.size() == 5 );
					REQUIRE( list.TotalWeight() == 14 );
					
					// Delete objects with values 1, 2, and 3.
					auto it = list.erase(list.begin(), list.begin() + 3);
					THEN( "The list shrinks by the size and weight of the erased range." ) {
						CHECK( list.size() == 2 );
						CHECK( list.TotalWeight() == 8 );
					}
					THEN( "An iterator pointing to the next object in the list is returned." ) {
						REQUIRE( it != list.end() );
						CHECK( it->value == 4 );
						CHECK( it->Weight() == 5 );
					}
				}
			}
			
			AND_WHEN( "The list is cleared." ) {
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
