/* text/test_format.cpp
Copyright (c) 2021 by Terin

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/text/Format.h"

// ... and any system includes needed for the test file.
#include <string>

namespace { // test namespace

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO("A unit of playing time is to be made human-readable", "[Format][PlayTime]") {
	GIVEN( "A time of 0" ) {
		THEN( "0s is returned" ) {
			CHECK( Format::PlayTime(0) == "0s");
		}
	}

	GIVEN( "A time of a half second" ) {
		THEN( "0s is returned" ) {
			CHECK( Format::PlayTime(.5) == "0s");
		}
	}

	GIVEN( "A time under a minute" ) {
		THEN( "A time in only seconds is returned" ) {
			CHECK( Format::PlayTime(47) == "47s");
		}
	}

	GIVEN( "A time over a minute but under an hour " ) {
		THEN( "A time in only minutes and seconds is returned" ) {
			CHECK( Format::PlayTime(567) == "9m 27s");
		}
	}

	GIVEN( "A time over an hour but under a day" ) {
		THEN( "A time in only hours, minutes, and seconds is returned" ) {
			CHECK( Format::PlayTime(8492) == "2h 21m 32s");
		}
	}

	GIVEN( "A time over a day but under a year" ) {
		THEN( "A time in only days, hours, minutes, and seconds is returned" ) {
			CHECK( Format::PlayTime(5669274) == "65d 14h 47m 54s");
		}
	}

	GIVEN( "A time over a year" ) {
		THEN( "A time using all units is returned" ) {
			CHECK( Format::PlayTime(98957582) == "3y 50d 8h 13m 2s");
		}
	}
	GIVEN( "A negative time" ) {
		THEN( "0s is returned " ) {
			CHECK( Format::PlayTime(-300) == "0s");
		}
	}
}

// #endregion unit tests

// #region benchmarks
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
TEST_CASE( "Benchmark Format::PlayTime", "[!benchmark][format]" ) {
	BENCHMARK( "Format::PlayTime() with a value under an hour" ) {
		return Format::PlayTime(1943);
	};
	BENCHMARK( "Format::PlayTime() with a high, but realistic playtime (40-400h)" ) {
		return Format::PlayTime(1224864);
	};
	BENCHMARK( "Format::PlayTime() with an uncapped value" ) {
		return Format::PlayTime(std::numeric_limits<int>::max());
	};
}
#endif

} // test namespace
