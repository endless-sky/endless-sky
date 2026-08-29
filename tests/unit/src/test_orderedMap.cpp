/* test_orderedMap.cpp
Copyright (c) 2026 by Amazinite

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

#include "../../../source/OrderedMap.h"

#include <string>
#include <utility>

namespace { // test namespace

// #region mock data
constexpr int64_t CONSTANT = 10;

class Object {
	// Some data that the object holds.
	int value;
public:
	Object() : value(CONSTANT) {}
	explicit Object(int value) : value(value) {}
	int GetValue() const { return value; }
	bool operator==(const Object &other) const { return this->value == other.value; }
	int64_t GetConstant() const { return CONSTANT; }
};
// #endregion mock data



// #region unit tests
SCENARIO( "Creating an OrderedMap" , "[OrderedMap][Creation]" ) {
	GIVEN( "an ordered map" ) {
		auto map = OrderedMap<std::string, Object>{};

		WHEN( "it has no content" ) {
			THEN( "it has the correct attributes" ) {
				CHECK( map.empty() );
				CHECK( map.size() == 0 );
				CHECK( std::begin(map) == std::end(map) );
			}
		}

		WHEN( "an object is added with operator[]" ) {
			auto beforeSize = map.size();

			auto obj = Object(1);
			map["first"] = obj;

			THEN( "the map size increases by 1" ) {
				CHECK_FALSE( map.empty() );
				CHECK( map.size() == 1 + beforeSize );
			}
			THEN( "the map contains the added key" ) {
				CHECK( map.contains("first") );
			}
			THEN( "the map does not contain a different key" ) {
				CHECK_FALSE( map.contains("second") );
			}
			THEN( "using operator[] on the same key overwrites the previous value" )
				auto size = map.size();
				auto replaceObj = Object(2);
				map["first"] = replaceObj;
				CHECK( size == map.size() );

				auto it = map.find("first");
				CHECK( it != map.end() );
				CHECK( it->first == "first" );
				CHECK( it->second == replaceObj );
			}
			THEN( "using emplace_back on the same key does not overwrite the previous value" ) {
				auto size = map.size();
				auto emplaceResult = map.emplace_back("first", 2);
				CHECK( size == map.size() );

				auto it = map.find("first");
				CHECK( it != map.end() );
				CHECK( it->first == "first" );
				CHECK_FALSE( it->second == Object(2) );
				CHECK( it->second == obj );
				CHECK( it->second == emplaceResult );
			}
			THEN( "using insert on the same key does not overwrite the previous value" ) {
				auto size = map.size();
				auto insertResult = map.insert(std::make_pair("first", Object(2)));
				auto it = map.find("first");
				CHECK( size == map.size() );

				CHECK( it != map.end() );
				CHECK( it->first == "first" );
				CHECK_FALSE( it->second == Object(2) );
				CHECK( it->second == obj );
				CHECK( insertResult.first == it );
				CHECK( insertResult.second == false );
			}

			AND_WHEN( "a second object is added with emplace_back" ) {
				auto emplaceResult = map.emplace_back("second", 2);
				THEN( "emplace returns the constructed object" ) {
					CHECK( emplaceResult == Object(2) );
				}
				THEN( "the map increases in size" ) {
					CHECK_FALSE( map.empty() );
					CHECK( map.size() == 2 + beforeSize );
				}
				THEN( "the object at the front of the map is the first object that was inserted" ) {
					auto front = map.front();
					CHECK( front.first == "first" );
					CHECK( front.second.GetValue() == 1 );

					auto at = map.at("first");
					CHECK( at.GetValue() == 1 );

					auto index = map.index(0);
					CHECK( index.first == "first" );
					CHECK( index.second.GetValue() == 1 );

					auto it = map.begin();
					CHECK( it->first == "first" );
					CHECK( it->second.GetValue() == 1 );
				}
				THEN( "the object at the back of the map is the most recently inserted" ) {
					auto back = map.back();
					CHECK( back.first == "second" );
					CHECK( back.second.GetValue() == 2 );

					auto at = map.at("second");
					CHECK( at.GetValue() == 2 );

					auto index = map.index(1);
					CHECK( index.first == "second" );
					CHECK( index.second.GetValue() == 2 );

					auto it = std::prev(map.end());
					CHECK( it->first == "second" );
					CHECK( it->second.GetValue() == 2 );
				}

				AND_WHEN( "a third object is added with insert" ) {
					auto insertResult = map.insert(std::make_pair("third", Object(3)));
					THEN( "insert returns a pair of iterator and boolean reporting the insert results" ) {
						CHECK( insertResult.first == map.find("third") );
						CHECK( insertResult.second == true );
					}
					THEN( "the map is in insertion order" ) {
						std::vector<std::pair<std::string, Object>> comparisonVector = {
							{"first", Object(1)},
							{"second", Object(2)},
							{"third", Object(3)},
						};
						int compareIndex = 0;
						for(auto it = map.begin(); it != map.end(); ++it, ++compareIndex)
						{
							auto &compare = comparisonVector[compareIndex];
							CHECK( it->first == compare.first );
							CHECK( it->second == compare.second );
						}
					}
					THEN( "objects can be found in the map by key" ) {
						auto it = map.find("first");
						CHECK( it->first == "first" );
						CHECK( it->second == Object(1) );

						it = map.find("second");
						CHECK( it->first == "second" );
						CHECK( it->second == Object(2) );

						it = map.find("third");
						CHECK( it->first == "third" );
						CHECK( it->second == Object(3) );

						it = map.find("fourth");
						CHECK( it == map.end() );
					}
				}

				auto beforeErase = map.size();
				AND_WHEN( "an element is erased from the map by iterator" ) {
					auto it = map.find("second");
					it = map.erase(it);
					THEN( "the map size is updated and an iterator is returned to the next object" ) {
						CHECK( map.size() == beforeErase - 1);
						CHECK( it == map.end() );
					}
				}

				AND_WHEN( "a range of elements are erased from the map by iterator" )
				{
					auto it = map.erase(map.begin(), map.end());
					THEN( "the map size is updated and an iterator is returned to the next object" ) {
						CHECK( map.empty() );
						CHECK( map.size() == 0 );
						CHECK( it == map.end() );
					}
				}

				AND_WHEN( "an element is erased from the map by key" ) {
					std::size_t val = map.erase("first");
					THEN( "the map size is updated when removing a value that is in the map" ) {
						CHECK( val == 1 );
						CHECK( map.size() == beforeErase - 1 );
					}

					val = map.erase("fourth");
					THEN( "the map size is not updated when removing a value that isn't in the map" ) {
						CHECK( val == 0 );
						CHECK( map.size() == beforeErase - 1 );
					}
				}
			}

			AND_WHEN( "the map is cleared" ) {
				map.clear();
				THEN( "the map is now empty" ) {
					CHECK( map.empty() );
					CHECK( map.size() == 0 );
				}
			}
		}
	}
}

// #endregion unit tests



} // test namespace
