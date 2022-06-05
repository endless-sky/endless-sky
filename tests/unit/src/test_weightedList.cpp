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
#include "../../../source/WeightedList.h"

// ... and any system includes needed for the test file.
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace { // test namespace

// #region mock data
constexpr int64_t CONSTANT = 10;

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

	// Some methods with a result that can be averaged.
	int64_t GetConstant() const { return CONSTANT; }
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
	GIVEN( "A weighted list with content" ) {
		auto list = WeightedList<WeightedObject>{};
		list.emplace_back(10, 4);
		list.emplace_back(20, 1);
		REQUIRE( list.size() == 2 );

		WHEN( "an average is computed over the same value" ) {
			auto average = list.Average(std::mem_fn(&WeightedObject::GetConstant));
			THEN( "the result is independent of choice weights" ) {
				CHECK( average == CONSTANT );
			}
		}

		WHEN( "an average is computed over different values" ) {
			REQUIRE( CONSTANT != 12 );
			auto average = list.Average(std::mem_fn(&WeightedObject::GetValue));
			THEN( "the result is dependent on the choices' weights" ) {
				CHECK( average == 12 );
			}
		}
	}
}

SCENARIO( "Obtaining a random value", "[WeightedList][Usage]") {
	GIVEN( "a list with no content" ) {
		const auto list = WeightedList<WeightedObject>{};
		WHEN( "a random selection is performed" ) {
			REQUIRE( list.empty() );
			THEN( "an informative runtime exception is thrown." ) {
				CHECK_THROWS_AS( list.Get(), std::runtime_error );
				CHECK_THROWS_WITH( list.Get(), Catch::Matchers::Contains("empty weighted list") );
			}
		}
	}

	GIVEN( "a list with one item" ) {
		auto list = WeightedList<WeightedObject>{};
		const auto item = WeightedObject(0, 1);
		list.emplace_back(item);
		WHEN( "a random selection is performed") {
			REQUIRE( list.size() == 1 );
			THEN( "the result is always the item" ) {
				CHECK( list.Get() == item );
			}
		}
	}
	GIVEN( "a list with multiple items" ) {
		auto list = WeightedList<WeightedObject>{};
		const int weights[] = {1, 10, 100};
		const auto first = WeightedObject(0, weights[0]);
		const auto second = WeightedObject(1, weights[1]);
		const auto third = WeightedObject(2, weights[2]);
		list.emplace_back(first);
		list.emplace_back(second);
		list.emplace_back(third);

		WHEN( "a random selection is performed" ) {
			auto getSampleSummary = [&list](std::size_t size){
				auto l = std::map<int, unsigned>{};
				for(unsigned i = 0; i < size; ++i)
					++l[list.Get().GetValue()];
				return l;
			};
			// To compare distributions, we use the chi-squared test.
			auto computeChiStat = [&](std::size_t count) -> double {
				const auto summary = getSampleSummary(count);
				auto toExpected = [=](int weight) -> unsigned {
					return static_cast<double>(weight) / list.TotalWeight() * count;
				};
				auto toTermValue = [](unsigned actual, unsigned expected) -> double {
					double difference = static_cast<double>(std::max(actual, expected) - std::min(actual, expected));
					return pow(difference, 2) / expected;
				};
				return toTermValue(summary.at(first.GetValue()), toExpected(weights[0]))
					+ toTermValue(summary.at(second.GetValue()), toExpected(weights[1]))
					+ toTermValue(summary.at(third.GetValue()), toExpected(weights[2]))
				;
			};
			const unsigned totalPicks = 1 << 16;
			THEN( "each item is selectable in accordance with its weight" ) {
				// To provide reasonable assurance that we have implemented things properly, without also
				// causing a large number of spurious failures, we perform a strong goodness-of-fit test
				// first, and then, if it did not pass, compare a second sample against a lower threshold.
				// Compare the observed distribution to the expected distribution using the chi-squared
				// statistic with a two degrees of freedom and significance levels of 0.01 and 0.05.
				// (Critical values available via NIST: https://www.itl.nist.gov/div898/handbook/eda/section3/eda3674.htm)
				if (computeChiStat(totalPicks) > 9.210)
				{
					INFO("alpha = 0.05");
					CHECK( computeChiStat(totalPicks) <= 5.991 );
				} else {
					SUCCEED( "null hypothesis is not rejected" );
				}
			}
		}
	}
}

SCENARIO( "Test WeightedList error conditions.", "[WeightedList]" ) {
	GIVEN( "A new weighted list." ) {
		auto list = WeightedList<WeightedObject>{};
		REQUIRE( list.empty() );

		WHEN( "Attempting to insert a negative weighted object." ) {
			THEN( "An invalid argument exception is thrown." ) {
				try{
					list.emplace_back(1, -1);
					FAIL( "should have thrown" );
				} catch(const std::invalid_argument &e) {
					SUCCEED( "threw when item weight was negative" );
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
