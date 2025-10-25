/* test_weightedList.cpp
Copyright (c) 2021 by Amazinite

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
#include "../../../source/WeightedList.h"

// ... and any system includes needed for the test file.
#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace { // test namespace

// #region mock data
constexpr int64_t CONSTANT = 10;

class Object {
	// Some data that the object holds.
	int value;
public:
	explicit Object(int value) : value(value) {}
	int GetValue() const { return value; }
	bool operator==(const Object &other) const { return this->value == other.value; }
	int64_t GetConstant() const { return CONSTANT; }
};
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a WeightedList" , "[WeightedList][Creation]" ) {
	GIVEN( "a weighted list" ) {
		auto list = WeightedList<Object>{};

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

			const auto obj = Object(1);
			list.emplace_back(objWeight, obj);

			THEN( "the list size increases by 1" ) {
				CHECK_FALSE( list.empty() );
				CHECK( list.size() == 1 + beforeSize );
			}
			THEN( "the list weight increases by the added weight" ) {
				CHECK( list.TotalWeight() == beforeWeight + objWeight );
			}

			AND_WHEN( "a second object is added" ) {
				const auto extraWeight = 3;
				list.emplace_back(extraWeight, 2);
				THEN( "the list increases in size and weight" ) {
					CHECK_FALSE( list.empty() );
					CHECK( list.size() == 2 + beforeSize );
					CHECK( list.TotalWeight() == beforeWeight + objWeight + extraWeight );
				}
				THEN( "the object at the back of the list is the most recently inserted" ) {
					CHECK( list.back().GetValue() == 2 );
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
					}
				}

				AND_WHEN( "a range is erased from begin to end" ) {
					list.erase(list.begin(), list.end());
					THEN( "the list is empty" ) {
						CHECK( list.empty() );
						CHECK( list.TotalWeight() == 0 );
					}
				}

				AND_WHEN( "a range is erased from the middle" ) {
					// Add more objects to the list so that a range can be deleted.
					list.emplace_back(1, 3);
					list.emplace_back(5, 4);
					list.emplace_back(3, 5);
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
					}
				}

				AND_WHEN( "the erase friend function is used" ) {
					std::size_t count = erase(list, obj);
					REQUIRE( count > 0 );

					THEN( "the total weight is correctly maintained" ) {
						CHECK( list.TotalWeight() == extraWeight );
					}
				}

				AND_WHEN( "the erase-if friend function is used" ) {
					std::size_t count = erase_if(list, [](const Object &o) { return o.GetValue() == 1; });
					REQUIRE( count > 0 );

					THEN( "the total weight is correctly maintained" ) {
						CHECK( list.TotalWeight() == extraWeight );
					}
				}
			}

			AND_WHEN( "the list is cleared" ) {
				list.clear();
				THEN( "the list is now empty" ) {
					CHECK( list.empty() );
					CHECK( list.size() == 0 );
				}
				THEN( "the list no longer has any weight" ) {
					CHECK( list.TotalWeight() == 0 );
				}
			}
		}
	}
	GIVEN( "a weighted list with content" ) {
		auto list = WeightedList<Object>{};
		list.emplace_back(4, 10);
		list.emplace_back(1, 20);
		REQUIRE( list.size() == 2 );

		WHEN( "an average is computed over the same value" ) {
			auto average = list.Average(std::mem_fn(&Object::GetConstant));
			THEN( "the result is independent of choice weights" ) {
				CHECK( average == CONSTANT );
			}
		}

		WHEN( "an average is computed over different values" ) {
			REQUIRE( CONSTANT != 12 );
			auto average = list.Average(std::mem_fn(&Object::GetValue));
			THEN( "the result is dependent on the choices' weights" ) {
				CHECK( average == 12 );
			}
		}
	}
}

SCENARIO( "Erasing from a WeightedList using a predicate", "[WeightedList][Usage]" ) {
	GIVEN( "a weighted list" ) {
		auto list = WeightedList<Object>{};
		auto invocations = 0U;
		auto pred = [&invocations](const Object &o) noexcept -> bool {
			++invocations;
			return o.GetValue() % 2;
		};

		WHEN( "the list contains one valid object" ) {
			list.emplace_back(2, 2);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the list is unchanged" ) {
					CHECK( erased == 0 );
					CHECK( list.size() == 1 );
					// pred should only be invoked once per element.
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 2 );
					// pred should be false for all remaining elements.
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "the list contains one invalid object" ) {
			list.emplace_back(1, 1);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the list is empty" ) {
					CHECK( erased == 1 );
					CHECK( list.size() == 0 );
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 0 );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "all objects are valid" ) {
			list.emplace_back(2, 2);
			list.emplace_back(4, 4);
			list.emplace_back(6, 6);
			list.emplace_back(8, 8);
			list.emplace_back(10, 10);
			list.emplace_back(12, 12);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the correct number and weight was erased" ) {
					CHECK( erased == 0 );
					CHECK( list.size() == 6 );
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 42 );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "all objects are invalid" ) {
			list.emplace_back(1, 1);
			list.emplace_back(3, 3);
			list.emplace_back(5, 5);
			list.emplace_back(7, 7);
			list.emplace_back(9, 9);
			list.emplace_back(11, 11);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the correct number and weight was erased" ) {
					CHECK( erased == 6 );
					CHECK( list.size() == 0 );
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 0 );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "the half-way point is valid" ) {
			list.emplace_back(1, 1);
			list.emplace_back(2, 2);
			list.emplace_back(3, 3);
			list.emplace_back(4, 4);
			list.emplace_back(5, 5);
			list.emplace_back(6, 6);
			list.emplace_back(7, 7);
			list.emplace_back(8, 8);
			list.emplace_back(9, 9);
			list.emplace_back(10, 10);
			list.emplace_back(11, 11);
			list.emplace_back(12, 12);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the correct number and weight was erased" ) {
					CHECK( erased == 6 );
					CHECK( list.size() == 6 );
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 42 );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "the half-way point is invalid" ) {
			list.emplace_back(1, 1);
			list.emplace_back(2, 2);
			list.emplace_back(3, 3);
			list.emplace_back(4, 4);
			list.emplace_back(5, 5);
			list.emplace_back(6, 6);
			list.emplace_back(7, 7);
			list.emplace_back(8, 8);
			list.emplace_back(9, 9);
			list.emplace_back(10, 10);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the correct number and weight was erased" ) {
					CHECK( erased == 5 );
					CHECK( list.size() == 5 );
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 30 );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "there are no valid objects after the half-way point once it's reached" ) {
			list.emplace_back(1, 1);
			list.emplace_back(2, 2);
			list.emplace_back(3, 3);
			list.emplace_back(5, 5);
			list.emplace_back(7, 7);
			list.emplace_back(4, 4);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "the correct number and weight was erased" ) {
					CHECK( erased == 4 );
					CHECK( list.size() == 2 );
					CHECK( invocations == list.size() + erased );
					CHECK( list.TotalWeight() == 6 );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}

		WHEN( "random input is generated" ) {
			int chunkSize = GENERATE(range(1, 12), range(20, 1000, 31));
			std::vector<int> values = GENERATE_COPY(chunk(chunkSize, range(0, chunkSize)));
			list.reserve(values.size());
			for(auto &&v : values)
				list.emplace_back(v ? v : 1, v);
			AND_WHEN( "all odds are erased" ) {
				std::size_t erased = erase_if(list, pred);
				THEN( "no odds remain" ) {
					CHECK( invocations == list.size() + erased );
					CHECK( std::none_of(list.begin(), list.end(), pred) );
				}
			}
		}
	}
}

SCENARIO( "Obtaining a random value", "[WeightedList][Usage]" ) {
	GIVEN( "a list with no content" ) {
		const auto list = WeightedList<Object>{};
		WHEN( "a random selection is performed" ) {
			REQUIRE( list.empty() );
			THEN( "an informative runtime exception is thrown" ) {
				CHECK_THROWS_AS( list.Get(), std::runtime_error );
#ifndef __APPLE__
#if CATCH_VERSION_MAJOR >= 3
				CHECK_THROWS_WITH( list.Get(), Catch::Matchers::ContainsSubstring("empty weighted list") );
#endif
#endif
			}
		}
	}

	GIVEN( "a list with one item" ) {
		auto list = WeightedList<Object>{};
		const auto item = Object(0);
		list.emplace_back(1, item);
		WHEN( "a random selection is performed") {
			REQUIRE( list.size() == 1 );
			THEN( "the result is always the item" ) {
				CHECK( list.Get() == item );
			}
		}
	}
	GIVEN( "a list with multiple items" ) {
		auto list = WeightedList<Object>{};
		const int weights[] = {1, 10, 100};
		const auto first = Object(0);
		const auto second = Object(1);
		const auto third = Object(2);
		list.emplace_back(weights[0], first);
		list.emplace_back(weights[1], second);
		list.emplace_back(weights[2], third);

		WHEN( "a random selection is performed" ) {
			auto getSampleSummary = [&list](std::size_t size) {
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
				if(computeChiStat(totalPicks) > 9.210)
				{
					INFO("alpha = 0.05");
					CHECK( computeChiStat(totalPicks) <= 5.991 );
				}
				else
				{
					SUCCEED( "null hypothesis is not rejected" );
				}
			}
		}
	}
}

SCENARIO( "Test WeightedList error conditions.", "[WeightedList]" ) {
	GIVEN( "a new weighted list" ) {
		auto list = WeightedList<Object>{};
		REQUIRE( list.empty() );

		WHEN( "attempting to insert a negative weighted object" ) {
			THEN( "an invalid argument exception is thrown" ) {
				try {
					list.emplace_back(-1, 1);
					FAIL( "should have thrown" );
				}
				catch(const std::invalid_argument &)
				{
					SUCCEED( "threw when item weight was negative" );
					AND_THEN( "the invalid object was not inserted into the list" ) {
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
