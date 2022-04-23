/* test_weightedList.cpp
Copyright (c) 2021 by Amazinite

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
class Object {
	// Some data that the object holds.
	int value;
public:
	Object(int value) : value(value) {}
	int GetValue() const { return value; }
	bool operator==(const Object &other) const { return this->value == other.value; }
};
class WeightedObject : public Object {
public:
	WeightedObject(int value, int weight) : Object(value), weight(weight) {}
	// This object's weight.
	int Weight() const { return weight; };
	int weight;
};
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a WeightedList" , "[WeightedList][Creation]" ) {
	GIVEN( "a weighted list" ) {
		auto list = WeightedList<WeightedObject>{};

		WHEN( "it has no content" ) {
			THEN( "it has the correct attributes" ) {
				CHECK( list.empty() );
				CHECK( list.size() == 0 );
				CHECK( list.TotalWeight() == 0 );
				CHECK( std::begin(list) == std::end(list) );
			}
		}

		WHEN( "an object is added" ) {
			const int objWeight = 2;
			const auto beforeSize = list.size();
			const auto beforeWeight = list.TotalWeight();

			const auto obj = WeightedObject(1, objWeight);
			list.emplace_back(obj);

			THEN( "the list size increases by 1" ) {
				CHECK_FALSE( list.empty() );
				CHECK( list.size() == 1 + beforeSize );
			}
			THEN( "the list weight increases by the added weight" ) {
				CHECK( list.TotalWeight() == beforeWeight + objWeight );
			}

			AND_WHEN( "a second object is added" ) {
				const auto extraWeight = 3;
				list.emplace_back(2, extraWeight);
				THEN( "the list increases in size and weight" ) {
					CHECK_FALSE( list.empty() );
					CHECK( list.size() == 2 + beforeSize );
					CHECK( list.TotalWeight() == beforeWeight + objWeight + extraWeight );
				}
				THEN( "the object at the back of the list is the most recently inserted" ) {
					CHECK( list.back().GetValue() == 2 );
					CHECK( list.back().Weight() == extraWeight );
				}

				AND_WHEN( "a single element is erased" ) {
					auto it = list.eraseAt(list.begin());

					THEN( "the list decreases in size and weight" ) {
						CHECK_FALSE( list.empty() );
						CHECK( list.size() == 1 + beforeSize );
						CHECK( list.TotalWeight() == beforeWeight + extraWeight );
					}
					THEN( "an iterator pointing to the next object in the list is returned" ) {
						REQUIRE( it != list.end() );
						CHECK( it->GetValue() == 2 );
						CHECK( it->Weight() == extraWeight );
					}
				}

				AND_WHEN( "A range is erased from begin to end" ) {
					list.erase(list.begin(), list.end());
					THEN( "The list is empty." ) {
						CHECK( list.empty() );
						CHECK( list.TotalWeight() == 0 );
					}
				}

				AND_WHEN( "a range is erased from the middle" ) {
					// Add more objects to the list so that a range can be deleted.
					list.emplace_back(3, 1);
					list.emplace_back(4, 5);
					list.emplace_back(5, 3);
					REQUIRE( list.size() == 5 );
					CHECK( list.TotalWeight() == 14 );

					// Delete objects with values 1, 2, and 3.
					auto it = list.erase(list.begin(), list.begin() + 3);
					THEN( "the list shrinks by the size and weight of the erased range" ) {
						CHECK( list.size() == 2 );
						CHECK( list.TotalWeight() == 8 );
					}
					THEN( "an iterator pointing to the next object in the list is returned" ) {
						REQUIRE( it != list.end() );
						CHECK( it->GetValue() == 4 );
						CHECK( it->Weight() == 5 );
					}
				}

				AND_WHEN( "the erase-remove idiom is used" ) {
					auto removeIt = std::remove_if(list.begin(), list.end(), [](const Object &o) { return o.GetValue() == 1; });
					REQUIRE( removeIt != list.begin() );
					REQUIRE( removeIt != list.end() );
					list.erase(removeIt, list.end());

					THEN( "the total weight is correctly maintained" ) {
						CHECK( list.TotalWeight() == extraWeight );
					}
				}
			}

			AND_WHEN( "The list is cleared." ) {
				list.clear();
				THEN( "The list is now empty." ) {
					CHECK( list.empty() );
					CHECK( list.size() == 0 );
				}
				THEN( "The list no longer has any weight." ) {
					CHECK( list.TotalWeight() == 0 );
				}
			}
		}
	}
}

SCENARIO( "Test WeightedList error conditions.", "[WeightedList]" ) {
	GIVEN( "A new weighted list." ) {
		auto list = WeightedList<WeightedObject>{};
		REQUIRE( list.empty() );
		WHEN( "Attempting to get from an empty list." ) {
			THEN( "A runtime error exception is thrown." ) {
				CHECK_THROWS_AS( list.Get(), std::runtime_error );
				CHECK_THROWS_WITH( list.Get(), Catch::Matchers::Contains("empty weighted list") );
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
						CHECK( list.empty() );
						CHECK( list.size() == 0 );
						CHECK( list.TotalWeight() == 0 );
					}
				}
			}
		}
	}
}
// #endregion unit tests



} // test namespace
