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

SCENARIO("A player-entered quantity can be parsed to a number", "[Format][Parse]") {
	GIVEN( "The string 123.45" ) {
		THEN( "parses to 123.45" ) {
			CHECK( Format::Parse("123.45") == Approx(123.45) );
		}
	}

	GIVEN( "The string 1,234K" ) {
		THEN( "parses to 1234000" ) {
			CHECK( Format::Parse("1,234K") == Approx(1234000.) );
		}
	}
}

TEST_CASE( "Format::Capitalize", "[Format][Capitalize]") {
	SECTION( "Single-word strings" ) {
		CHECK( Format::Capitalize("magnesium") == "Magnesium" );
		CHECK( Format::Capitalize("hydroxide") == "Hydroxide" );
	}
	SECTION( "Words separated by whitespace" ) {
		CHECK( Format::Capitalize("canned fruit") == "Canned Fruit" );
		CHECK( Format::Capitalize("canned	fruit") == "Canned	Fruit" );
		CHECK( Format::Capitalize("canned\tfruit") == "Canned\tFruit" );
		CHECK( Format::Capitalize("canned\nfruit") == "Canned\nFruit" );
	}
	SECTION( "Precapitalized strings" ) {
		CHECK( Format::Capitalize("RPGs") == "RPGs" );
		CHECK( Format::Capitalize("MAGNESIUM") == "MAGNESIUM" );
	}
	SECTION( "Words containing punctuation" ) {
		CHECK( Format::Capitalize("de-ionizers") == "De-ionizers" );
		CHECK( Format::Capitalize("anti-inflammatories") == "Anti-inflammatories" );
		CHECK( Format::Capitalize("ka'het") == "Ka'het" );
		CHECK( Format::Capitalize("A.I.") == "A.I.");
		CHECK( Format::Capitalize("trains/planes") == "Trains/planes");
	}
	SECTION( "Words with possessive qualifiers" ) {
		CHECK( Format::Capitalize("plumbers' pipes") == "Plumbers' Pipes");
		CHECK( Format::Capitalize("plumber's pipe") == "Plumber's Pipe");
	}
}

TEST_CASE( "Format::Number", "[Format][Number]") {
	SECTION( "0-valued inputs" ) {
		CHECK( Format::Number(-0) == "0" );
		CHECK( Format::Number(0) == "0" );
		CHECK( Format::Number(-.0) == "0" );
		CHECK( Format::Number(.0) == "0" );
	}
	SECTION( "Integral inputs" ) {
		CHECK( Format::Number(1) == "1" );
		CHECK( Format::Number(-1.) == "-1" );
		CHECK( Format::Number(1000.) == "1,000" );
	}
	SECTION( "Decimals between 0 and 1" ) {
		CHECK( Format::Number(0.51) == "0.51" );
		CHECK( Format::Number(0.56) == "0.56" );
		CHECK( Format::Number(0.871) == "0.87" );
	}
	SECTION( "Decimals between 10 and 100" ) {
		CHECK( Format::Number(44.1234) == "44.12" );
		CHECK( Format::Number(94.5) == "94.5" );
		CHECK( Format::Number(-12.41) == "-12.41" );
	}
	SECTION( "Decimals between 100 and 1000" ) {
		CHECK( Format::Number(256.) == "256" );
		CHECK( Format::Number(466.1948) == "466.19" );
		CHECK( Format::Number(-761.1) == "-761.1" );
	}
	SECTION( "Decimals between 1000 and 10'000" ) {
		CHECK( Format::Number(2345.123) == "2,345.1" );
		CHECK( Format::Number(4444.03) == "4,444" );
		CHECK( Format::Number(-5641.23) == "-5,641.2" );
	}
	SECTION( "Decimals greater than 10'000" ) {
		CHECK( Format::Number(12325.120) == "12,325" );
		CHECK( Format::Number(45123.05) == "45,123" );
		CHECK( Format::Number(-56413.2) == "-56,413" );
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
TEST_CASE( "Benchmark Format::Number", "[!benchmark][format]" ) {
	BENCHMARK( "Zero" ) {
		return Format::Number(0.);
	};
	BENCHMARK( "A nonzero whole number" ) {
		return Format::Number(100.);
	};
	BENCHMARK( "A decimal between 0 and 1k" ) {
		return Format::Number(-10.312345);
	};
	BENCHMARK( "A decimal between 1k and 10k" ) {
		return Format::Number(5555.5555);
	};
}
#endif

} // test namespace
