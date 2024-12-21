/* test_conditionSet.cpp
Copyright (c) 2020 by Benjamin Hauch

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
#include "../../../source/ConditionSet.h"

// Include a helper for creating well-formed DataNodes (to enable creating non-empty ConditionSets).
#include "datanode-factory.h"
// Include a helper for capturing & asserting on logged output.
#include "output-capture.hpp"

// Include ConditionStore, to enable usage of them for testing ConditionSets.
#include "../../../source/ConditionsStore.h"

// ... and any system includes needed for the test file.
#include <cstdint>
#include <map>
#include <string>

namespace { // test namespace
using Conditions = std::map<std::string, int64_t>;
// #region mock data



// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionSet" , "[ConditionSet][Creation]" ) {

	OutputSink warnings(std::cerr);

	GIVEN( "no arguments" ) {
		const auto set = ConditionSet{};
		THEN( "no conditions are created" ) {
			REQUIRE( set.IsEmpty() );
			REQUIRE( set.IsValid() );
		}
	}
	GIVEN( "a node with no children" ) {
		auto childlessNode = AsDataNode("childless");
		const auto set = ConditionSet{childlessNode};

		THEN( "no conditions are created" ) {
			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
		}
	}
	GIVEN( "a node with valid children" ) {
		auto nodeWithChildren = AsDataNode("and\n\tnever");
		const auto set = ConditionSet{nodeWithChildren};

		THEN( "a non-empty ConditionSet is created" ) {
			REQUIRE_FALSE( set.IsEmpty() );
			REQUIRE( set.IsValid() );
		}
	}
	GIVEN( "a simple incomplete arithmetic add expression" ) {
		auto nodeWithIncompleteAdd = AsDataNode("toplevel\n\t4 +");
		const auto set = ConditionSet{nodeWithIncompleteAdd};
		THEN( "the expression should be identified as invalid" ) {
			const std::string validationWarning = "Error: expected terminal after infix operator \"+\":\n";
			const std::string invalidNodeText = "toplevel\n";
			const std::string invalidNodeTextInWarning = "L2:   4 +";

			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n' );
			}
		}
	}
	GIVEN( "a longer incomplete arithmetic add expression" ) {
		auto nodeWithIncompleteAdd = AsDataNode("toplevel\n\t4 + 6 +");
		const auto set = ConditionSet{nodeWithIncompleteAdd};
		THEN( "the expression should be identified as invalid" ) {
			const std::string validationWarning = "Error: expected terminal after infix operator \"+\":\n";
			const std::string invalidNodeText = "toplevel\n";
			const std::string invalidNodeTextInWarning = "L2:   4 + 6 +";

			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n' );
			}
		}
	}
	GIVEN( "a longer incomplete arithmetic subtract expression" ) {
		auto nodeWithIncompleteAdd = AsDataNode("toplevel\n\t4 - 6 -");
		const auto set = ConditionSet{nodeWithIncompleteAdd};
		THEN( "the expression should be identified as invalid" ) {
			const std::string validationWarning = "Error: expected terminal after infix operator \"-\":\n";
			const std::string invalidNodeText = "toplevel\n";
			const std::string invalidNodeTextInWarning = "L2:   4 - 6 -";

			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n' );
			}
		}
	}
	GIVEN( "an invalid expression of two numerical terminals" ) {
		auto nodeWithTwoTerminals = AsDataNode("toplevel\n\t4 77");
		const auto set = ConditionSet{nodeWithTwoTerminals};
		THEN( "the expression should be identified as invalid" ) {
			const std::string validationWarning = "Error: expected infix operator instead of \"77\":\n";
			const std::string invalidNodeText = "toplevel\n";
			const std::string invalidNodeTextInWarning = "L2:   4 77";

			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n' );
			}
		}
	}
	GIVEN( "an invalid token instead of a terminal" ) {
		auto nodeWithIncompleteTerminal = AsDataNode("toplevel\n\t%%percentFail");
		const auto set = ConditionSet{nodeWithIncompleteTerminal};
		THEN( "the expression should be identified as invalid" ) {
			const std::string validationWarning = "Error: expected terminal or open-bracket:\n";
			const std::string invalidNodeText = "toplevel\n";
			const std::string invalidNodeTextInWarning = "L2:   %%percentFail";

			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n' );
			}
		}
	}
}

SCENARIO( "Extending a ConditionSet", "[ConditionSet][Creation]" ) {

	OutputSink warnings(std::cerr);

	GIVEN( "an empty ConditionSet" ) {
		auto set = ConditionSet{};
		REQUIRE( set.IsEmpty() );
		REQUIRE( set.IsValid() );

		THEN( "no expressions are added from empty nodes" ) {
			const std::string validationWarning = "Error: child-nodes expected, found none:\ntoplevel\n\n";
			set.Load(AsDataNode("toplevel"));
			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning );
			}
		}
		THEN( "no expressions are added from invalid nodes" ) {
			const std::string validationWarning = "Error: has keyword requires a single condition:\n";
			const std::string invalidNodeText = "and\n\thas";
			const std::string invalidNodeTextInWarning = "and\nL2:   has";
			set.Load(AsDataNode(invalidNodeText));
			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( warnings.Flush() == validationWarning + invalidNodeTextInWarning + '\n' + '\n');
			}
		}
		THEN( "new expressions can be added from valid nodes" ) {
			set.Load(AsDataNode("and\n\tnever"));
			REQUIRE_FALSE( set.IsEmpty() );
			REQUIRE( set.IsValid() );
			REQUIRE( warnings.Flush() == "" );
		}
	}
}

SCENARIO( "Determining if condition requirements are met", "[ConditionSet][Usage]" ) {
	const auto storeWithData = ConditionsStore {
		{"event: war begins", 1},
		{"someData", 100},
		{"moreData", 100},
		{"otherData", 100},
	};

	GIVEN( "an empty ConditionSet" ) {
		const auto emptySet = ConditionSet{};
		REQUIRE( emptySet.IsEmpty() );
		REQUIRE( emptySet.IsValid() );

		AND_GIVEN( "an empty list of Conditions" ) {
			const auto emptyStore = ConditionsStore{};
			THEN( "the ConditionSet is satisfied" ) {
				REQUIRE( emptySet.Test(emptyStore) );
				REQUIRE( emptySet.IsValid() );
			}
		}
		AND_GIVEN( "a non-empty list of Conditions" ) {
			THEN( "the ConditionSet is satisfied" ) {
				REQUIRE( emptySet.Test(storeWithData) );
				REQUIRE( emptySet.IsValid() );
			}
		}
	}
	GIVEN( "various correct expression(s) as conditionSet" ) {
		auto expressionAndAnswer = GENERATE(table<std::string, int64_t>({

			// Tests with simple expressions.
			{"never", 0},
			{"0", 0},
			{"1", 1},
			{"2", 2},

			// Add and multiply arithmetic tests.
			{"2 + 6", 8},
			{"2 + 6 + 8 + 40", 56},
			{"2 * 6 * 8 * 40", 3840},
			{"2 * 6 + 8", 20},
			{"2 + 6 * 8", 50},
			{"2 + 6 * 8 * 4", 194},
			{"2 + 6 * 8 * 4 + 5", 199},
			{"2 + 6 * 8 * 4 - 5 + 22", 211},
			{"2 + 6 * 8 * 4 - 5 * 22", 84},
			{"2 + 6 * 8 * 4 - 5 * 22 / 11", 184},
			{"2 - 6 * 8 * 4 - 5 * 22 / 11", -200},
			{"2 - 6 * 8 * 4 + 5 * 22 / 11", -180},
			{"2 / 2 - 6 * 8 * 4 + 5 * 22 / 11", -181},
			{"2 * ( 6 + 8 )", 28},
			{"( 2 + 6 ) * 8", 64},
			{"2 * ( 6 + 8 ) * 10", 280},
			{"2 * ( 6 + 8 ) * 10 * ( 0 - 8 )", -2240},
			{"( 2 - 1 + 6 ) * 8", 56},
			{"( -6 + 6 ) * 8", 0},
			{"( 2 - 2 + 6 ) * 8", 48},
			{"( 2 - 4 + 6 ) * 8", 32},
			{"( 2 + 6 ) * 8", 64},
			{"100 - 100", 0},
			{"100 - 200", -100},
			{"100 + -200", -100},

			// Division and multiply tests.
			{"60 / 5", 12},
			{"60 / 5 / 3", 4},
			{"60 % 5", 0},
			{"60 % 0", 60},
			{"60 % 50", 10},

			// Tests for comparisons.
			{"10 > 20", 0},
			{"10 < 20", 1},
			{"10 == 20", 0},
			{"10 >= 20", 0},
			{"10 <= 20", 1},
			{"10 == 10", 1},
			{"10 >= 10", 1},
			{"10 <= 10", 1},

			// Tests with variables.
			{"someData + 5 > moreData", 1},
			{"someData + 5 < moreData", 0},
			{"someData <= moreData", 1},
			{"someData >= moreData", 1},
			{"someData == moreData", 1},
			{"someData - 1 <= moreData", 1},
			{"someData + 1 <= moreData", 0},
			{"someData", 100},
			{"moreData - 100", 0},
			{"moreData - 150", -50},
			{"otherData - 10 - 50 + -200", -160},
			{"otherData - otherData", 0},
			{"10 * otherData", 1000},

			// Some tests for brackets
			{"( ( ( ( 1000 ) ) ) )", 1000},
			{"( ( 20 - ( ( 1000 ) ) + 50 ) )", -930},
			{"( ( 20 - ( 1 ) ) ) + ( ( 1000 ) ) + 50", 1069},

			// Tests for and and or conditions, the first one is the implicit version.
			{"3\n\t2\n\t5", 3},
			{"and\n\t\t11\n\t\t2\n\\tt5", 11},
			{"and\n\t\t14\n\t\t0\n\\tt5", 0},
			{"or\n\t\t8\n\t\t2\n\\tt5", 8},
			{"or\n\t\t9\n\t\t0\n\\tt5", 9},

			// Black magic below; parser might need to handle this, but nobody should ever write comparisons like this.
			{"1 > 2 == 0", 1},
			{"11 == 11 == 1", 1},

		}));
		const auto numberSet = ConditionSet{AsDataNode("toplevel\n\t" + std::get<0>(expressionAndAnswer))};
		THEN( "The expression \'" + std::get<0>(expressionAndAnswer) + "\' is valid and evaluates to the correct number" ) {
			REQUIRE_FALSE( numberSet.IsEmpty() );
			REQUIRE( numberSet.IsValid() );
			REQUIRE( numberSet.Evaluate(storeWithData) == std::get<1>(expressionAndAnswer) );
		}
	}
}

// #endregion unit tests



} // test namespace
