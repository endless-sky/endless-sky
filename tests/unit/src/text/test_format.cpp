/* test_format.cpp
Copyright (c) 2021 by Terin

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
#include "../../../../source/text/Format.h"

// ... utility classes
#include "../../../../source/DataNode.h"

// ... and any system includes needed for the test file.
#include <string>

namespace { // test namespace

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.
	std::map<std::string, int64_t> conditions = {
		{ "zero", 0 }, // "0"
		{ "negative", -5 }, // "-5"
		{ "positive", 61 }, // "61"
		{ "mass test", 4361000 }, // "4,361,000 tons"
		{ "scaled test", 3361000000 }, // "3.361B"
		{ "raw test", 1810244 }, // "1810224"
		{ "big test", 30103010301 }, // "30,103,010,301"
		{ "credits test", -2361000 }, // "-2.361M credits"
		{ "playtime test", 5000000 }, // "50d 11h 23m 20s"
		{ "balanced[][[]][]", 4361000 }, // 4361000, 4,361,000, 4.361M, 4.361M credits, 4,361,000 tons, 50d 11h 23m 20s
		{ "balanced at [[@]]", 33104 }, // "33,104"
		{ "@", 38 } // "38"
	};
	Format::ConditionGetter getter = [](const std::string &string, size_t start, size_t size)
	{
		auto found = conditions.find(string.substr(start, size));
		return found == conditions.end() ? 0 : found->second;
	};

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

	GIVEN( "The string 1 523 004" ) {
		THEN( "parses to 1523004" ) {
			CHECK( Format::Parse("1 523 004") == Approx(1523004.) );
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
		CHECK( Format::Number(0.072) == "0.07" );
	}
	SECTION( "Decimals between 10 and 100" ) {
		CHECK( Format::Number(44.1234) == "44.12" );
		CHECK( Format::Number(94.5) == "94.5" );
		CHECK( Format::Number(10.1) == "10.1" );
		CHECK( Format::Number(10.01) == "10.01" );
		CHECK( Format::Number(10.02) == "10.02" );
		CHECK( Format::Number(10.03) == "10.03" );
		CHECK( Format::Number(10.04) == "10.04" );
		CHECK( Format::Number(10.05) == "10.05" );
		CHECK( Format::Number(10.06) == "10.06" );
		CHECK( Format::Number(10.07) == "10.07" );
		CHECK( Format::Number(10.08) == "10.08" );
		CHECK( Format::Number(10.09) == "10.09" );
		CHECK( Format::Number(10.10) == "10.1" );
		CHECK( Format::Number(10.11) == "10.11" );
		CHECK( Format::Number(10.12) == "10.12" );
		CHECK( Format::Number(10.13) == "10.13" );
		CHECK( Format::Number(10.14) == "10.14" );
		CHECK( Format::Number(10.15) == "10.15" );
		CHECK( Format::Number(10.16) == "10.16" );
		CHECK( Format::Number(10.17) == "10.17" );
		CHECK( Format::Number(10.18) == "10.18" );
		CHECK( Format::Number(10.19) == "10.19" );
		CHECK( Format::Number(10.20) == "10.2" );
		CHECK( Format::Number(10.21) == "10.21" );
		CHECK( Format::Number(10.22) == "10.22" );
		CHECK( Format::Number(10.23) == "10.23" );
		CHECK( Format::Number(10.24) == "10.24" );
		CHECK( Format::Number(10.25) == "10.25" );
		CHECK( Format::Number(10.26) == "10.26" );
		CHECK( Format::Number(10.27) == "10.27" );
		CHECK( Format::Number(10.28) == "10.28" );
		CHECK( Format::Number(10.29) == "10.29" );
		CHECK( Format::Number(10.30) == "10.3" );
		CHECK( Format::Number(10.31) == "10.31" );
		CHECK( Format::Number(10.32) == "10.32" );
		CHECK( Format::Number(10.33) == "10.33" );
		CHECK( Format::Number(10.34) == "10.34" );
		CHECK( Format::Number(10.35) == "10.35" );
		CHECK( Format::Number(10.36) == "10.36" );
		CHECK( Format::Number(10.37) == "10.37" );
		CHECK( Format::Number(10.38) == "10.38" );
		CHECK( Format::Number(10.39) == "10.39" );
		CHECK( Format::Number(10.40) == "10.4" );
		CHECK( Format::Number(10.41) == "10.41" );
		CHECK( Format::Number(10.42) == "10.42" );
		CHECK( Format::Number(10.43) == "10.43" );
		CHECK( Format::Number(10.44) == "10.44" );
		CHECK( Format::Number(10.45) == "10.45" );
		CHECK( Format::Number(10.46) == "10.46" );
		CHECK( Format::Number(10.47) == "10.47" );
		CHECK( Format::Number(10.48) == "10.48" );
		CHECK( Format::Number(10.49) == "10.49" );
		CHECK( Format::Number(10.50) == "10.5" );
		CHECK( Format::Number(10.599) == "10.59" );
		CHECK( Format::Number(10.60) == "10.6" );
		CHECK( Format::Number(10.699) == "10.69" );
		CHECK( Format::Number(10.70) == "10.7" );
		CHECK( Format::Number(10.799) == "10.79" );
		CHECK( Format::Number(10.80) == "10.8" );
		CHECK( Format::Number(10.899) == "10.89" );
		CHECK( Format::Number(10.90) == "10.9" );
		CHECK( Format::Number(10.999) == "10.99" );
		CHECK( Format::Number(-12.41) == "-12.41" );
	}
	SECTION( "Calculations on numbers parsed by DataNode::Value" ) {
		CHECK( Format::Number(60. * DataNode::Value("22.1") / DataNode::Value("3.4")) == "390");
	}
	SECTION( "Decimals between 100 and 1000" ) {
		CHECK( Format::Number(256.) == "256" );
		CHECK( Format::Number(466.1948) == "466.19" );
		CHECK( Format::Number(107.093) == "107.09" );
		CHECK( Format::Number(100.1) == "100.1" );
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
	SECTION( "#6235: Decimals with 0 in tenths place" ) {
		CHECK( Format::Number(100.06) == "100.06" );
		CHECK( Format::Number(1000.03) == "1,000" );
		CHECK( Format::Number(107.09) == "107.09" );
		CHECK( Format::Number(0.0123) == "0.01" );
	}
}

TEST_CASE( "Format::Credits", "[Format][Credits]") {
	SECTION( "1 credit" ) {
		CHECK( Format::Credits(1) == "1" );
	}
	SECTION( "0 credits" ) {
		CHECK( Format::Credits(0) == "0" );
	}
	SECTION( "Positive credits" ) {
		CHECK( Format::Credits(2) == "2" );
		CHECK( Format::Credits(1000) == "1,000" );
		CHECK( Format::Credits(2200) == "2,200" );
		CHECK( Format::Credits(2200) == "2,200" );
		CHECK( Format::Credits(1000000) == "1,000,000" );
		CHECK( Format::Credits(4361000) == "4.361M" );
		CHECK( Format::Credits(1000000000) == "1,000.000M" );
		CHECK( Format::Credits(4361000000) == "4.361B" );
		CHECK( Format::Credits(1000000000000) == "1,000.000B" );
		CHECK( Format::Credits(4361000000000) == "4.361T" );
		CHECK( Format::Credits(1000000000000000ll) == "1,000.000T");
		CHECK( Format::Credits(1000000000000001ll) == "1e+15");
		CHECK( Format::Credits(4361000000000000ll) == "4.36e+15");
	}
	SECTION( "Negative credits" ) {
		CHECK( Format::Credits(-2) == "-2" );
		CHECK( Format::Credits(-1000) == "-1,000" );
		CHECK( Format::Credits(-2200) == "-2,200" );
		CHECK( Format::Credits(-2200) == "-2,200" );
		CHECK( Format::Credits(-1000000) == "-1,000,000" );
		CHECK( Format::Credits(-4361000) == "-4.361M" );
		CHECK( Format::Credits(-1000000000) == "-1,000.000M" );
		CHECK( Format::Credits(-4361000000) == "-4.361B" );
		CHECK( Format::Credits(-1000000000000) == "-1,000.000B" );
		CHECK( Format::Credits(-4361000000000) == "-4.361T" );
		CHECK( Format::Credits(-1000000000000000ll) == "-1,000.000T");
		CHECK( Format::Credits(-1000000000000001ll) == "-1e+15");
		CHECK( Format::Credits(-4361000000000000ll) == "-4.36e+15");
	}
}

TEST_CASE( "Format::CreditString", "[Format][CreditString]") {
	SECTION( "1 credit" ) {
		CHECK( Format::CreditString(1) == "1 credit" );
	}
	SECTION( "0 credits" ) {
		CHECK( Format::CreditString(0) == "0 credits" );
	}
	SECTION( "Positive credits" ) {
		CHECK( Format::CreditString(2) == "2 credits" );
		CHECK( Format::CreditString(1000) == "1,000 credits" );
		CHECK( Format::CreditString(4361000) == "4.361M credits" );
	}
	SECTION( "Negative credits" ) {
		CHECK( Format::CreditString(-1) == "-1 credits" );
		CHECK( Format::CreditString(-2) == "-2 credits" );
		CHECK( Format::CreditString(-1000) == "-1,000 credits" );
		CHECK( Format::CreditString(-4361000) == "-4.361M credits" );
	}
}

TEST_CASE( "Format::MassString", "[Format][MassString]") {
	SECTION( "1 ton" ) {
		CHECK( Format::MassString(1) == "1 ton" );
		CHECK( Format::MassString(1.) == "1 ton" );
	}
	SECTION( "0 tons" ) {
		CHECK( Format::MassString(0) == "0 tons" );
	}
	SECTION( "Positive mass" ) {
		CHECK( Format::MassString(2) == "2 tons" );
		CHECK( Format::MassString(1000) == "1,000 tons" );
		CHECK( Format::MassString(4361000) == "4,361,000 tons" );
	}
	SECTION( "Negative mass" ) {
		CHECK( Format::MassString(-1) == "-1 tons" );
		CHECK( Format::MassString(-2) == "-2 tons" );
		CHECK( Format::MassString(-1000) == "-1,000 tons" );
		CHECK( Format::MassString(-4361000) == "-4,361,000 tons" );
	}
	SECTION( "Fractional mass" ) {
		CHECK( Format::MassString(2.5) == "2.5 tons" );
		CHECK( Format::MassString(0.1) == "0.1 tons" );
	}
}

TEST_CASE( "Format::CargoString", "[Format][CargoString]") {
	SECTION( "1 ton" ) {
		CHECK( Format::CargoString(1, "cargo") == "1 ton of cargo" );
		CHECK( Format::CargoString(1., "cargo") == "1 ton of cargo" );
	}
	SECTION( "0 tons" ) {
		CHECK( Format::CargoString(0, "cargo") == "0 tons of cargo" );
	}
	SECTION( "Positive mass" ) {
		CHECK( Format::CargoString(2, "cargo") == "2 tons of cargo" );
		CHECK( Format::CargoString(1000, "cargo") == "1,000 tons of cargo" );
		CHECK( Format::CargoString(4361000, "cargo") == "4,361,000 tons of cargo" );
	}
	SECTION( "Negative mass" ) {
		CHECK( Format::CargoString(-1, "cargo") == "-1 tons of cargo" );
		CHECK( Format::CargoString(-2, "cargo") == "-2 tons of cargo" );
		CHECK( Format::CargoString(-1000, "cargo") == "-1,000 tons of cargo" );
		CHECK( Format::CargoString(-4361000, "cargo") == "-4,361,000 tons of cargo" );
	}
	SECTION( "Fractional mass" ) {
		CHECK( Format::CargoString(2.5, "cargo") == "2.5 tons of cargo" );
		CHECK( Format::CargoString(0.1, "cargo") == "0.1 tons of cargo" );
	}
}

TEST_CASE( "Format::ExpandConditions", "[Format][ExpandConditions]") {
	SECTION( "no format@ specified" ) {
		CHECK( Format::ExpandConditions("__&[zero]__&[negative]__", getter) == "__0__-5__" );
		CHECK( Format::ExpandConditions("__&[zero]__&[negative]", getter) == "__0__-5" );
		CHECK( Format::ExpandConditions("__&[zero]&[negative]__", getter) == "__0-5__" );
		CHECK( Format::ExpandConditions("&[zero]__&[negative]__", getter) == "0__-5__" );
	}
	SECTION( "unbalanced []" ) {
		CHECK( Format::ExpandConditions("&[positive]__&[", getter) == "61__&[" );
		CHECK( Format::ExpandConditions("&[positive]__&[@", getter) == "61__&[@" );
		CHECK( Format::ExpandConditions("&[positive]__&[-@", getter) == "61__&[-@" );
		CHECK( Format::ExpandConditions("&[positive]__&[[[-][]]@", getter) == "61__&[[[-][]]@" );
		CHECK( Format::ExpandConditions("&[positive]__&[[]@[", getter) == "61__&[[]@[" );
	}
	SECTION( "specify the format@" ) {
		CHECK( Format::ExpandConditions("__&[number@negative]__", getter) == "__-5__" );
		CHECK( Format::ExpandConditions("__&[number@big test]__", getter) == "__30,103,010,301__" );
		CHECK( Format::ExpandConditions("__&[raw@raw test]__", getter) == "__1810244__" );
		CHECK( Format::ExpandConditions("__&[tons@mass test]__", getter) == "__4,361,000 tons__" );
		CHECK( Format::ExpandConditions("__&[scaled@scaled test]__", getter) == "__3.361B__" );
		CHECK( Format::ExpandConditions("__&[credits@credits test]__", getter) == "__-2.361M credits__" );
		CHECK( Format::ExpandConditions("__&[playtime@playtime test]__", getter) == "__57d 20h 53m 20s__" );
	}
	SECTION( "balanced []" ) {
		CHECK( Format::ExpandConditions("__&[balanced[][[]][]]__", getter) == "__4,361,000__" );
		CHECK( Format::ExpandConditions("__&[raw@balanced[][[]][]]__", getter) == "__4361000__" );
		CHECK( Format::ExpandConditions("__&[number@balanced[][[]][]]__", getter) == "__4,361,000__" );
		CHECK( Format::ExpandConditions("__&[scaled@balanced[][[]][]]__", getter) == "__4.361M__" );
		CHECK( Format::ExpandConditions("__&[credits@balanced[][[]][]]__", getter) == "__4.361M credits__" );
		CHECK( Format::ExpandConditions("__&[tons@balanced[][[]][]]__", getter) == "__4,361,000 tons__" );
		CHECK( Format::ExpandConditions("__&[playtime@balanced[][[]][]]__", getter) == "__50d 11h 23m 20s__" );
	}
	SECTION( "corner cases" ) {
		CHECK( Format::ExpandConditions("&[]", getter) == "0" );
		CHECK( Format::ExpandConditions("[tons@positive]", getter) == "[tons@positive]" );
		CHECK( Format::ExpandConditions("&tons@positive", getter) == "&tons@positive" );
		CHECK( Format::ExpandConditions("&]tons@positive[", getter) == "&]tons@positive[" );
		CHECK( Format::ExpandConditions("&[@]", getter) == "0" );
		CHECK( Format::ExpandConditions("&[@@]", getter) == "38" );
		CHECK( Format::ExpandConditions("&[[@invalid@]@positive]", getter) == "61" );
		CHECK( Format::ExpandConditions("__&[balanced at [[@]]]__", getter) == "__33,104__" );
		CHECK( Format::ExpandConditions("", getter) == "" );
		CHECK( Format::ExpandConditions("I AM A PRETTY CHICKEN", getter) == "I AM A PRETTY CHICKEN");
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
