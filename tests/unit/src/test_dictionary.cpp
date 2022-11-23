/* test_dictionary.cpp
Copyright (c) 2022 by quyykk

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
#include "../../../source/Dictionary.h"

// ... and any system includes needed for the test file.
#include <string>
#include <vector>

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a Dictionary instance", "[dictionary]") {
	GIVEN( "an instance" ) {
		Dictionary dict;
		THEN( "it has the correct default properties" ) {
			CHECK( dict.empty() );
			CHECK( dict.begin() == dict.end() );
		}
	}
}

SCENARIO( "A Dictionary instance is being used", "[dictionary]") {
	GIVEN( "an empty dictionary" ) {
		Dictionary dict;
		THEN( "add new elements works" ) {
			dict["foo"] = 10.;
			dict["bar"] = 42.;
			CHECK( dict.Get("foo") == 10. );
			CHECK( dict["bar"] == 42. );
			CHECK( std::distance(dict.begin(), dict.end()) == 2 );
		}
	}
}

// #region benchmarks
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
TEST_CASE( "Benchmark Dictionary::Get", "[!benchmark][dictionary]" ) {
	constexpr int SIZE = 100;
	constexpr int AVERAGE_ATTRIBUTE_LENGTH = 20;

	Dictionary dict;
	std::vector<std::string> strings;
	for(int i = 0; i < SIZE; ++i)
	{
		auto str = std::to_string(i);
		const int size = str.size();
		for(int j = 0; j < AVERAGE_ATTRIBUTE_LENGTH / size; ++j)
			str += str;

		dict[str] = i;
		strings.emplace_back(std::move(str));
	}

	BENCHMARK( "Dictionary::Get()", i ) {
		return dict.Get(strings[i % SIZE]);
	};
}
#endif
// #endregion benchmarks



} // test namespace
