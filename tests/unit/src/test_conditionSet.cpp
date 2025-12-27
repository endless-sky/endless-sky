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
// Include DataWriter for read/write/read/write tests.
#include "../../../source/DataWriter.h"
// Include a helper for handling logger output.
#include "logger-output.h"
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
ConditionsStore store;
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
		const auto set = ConditionSet{childlessNode, &store};

		THEN( "no conditions are created" ) {
			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
		}
	}
	GIVEN( "a node with valid children" ) {
		auto nodeWithChildren = AsDataNode("and\n\tnever");
		const auto set = ConditionSet{nodeWithChildren, &store};

		THEN( "a non-empty ConditionSet is created" ) {
			REQUIRE_FALSE( set.IsEmpty() );
			REQUIRE( set.IsValid() );
		}
	}
	GIVEN( "a simple incomplete arithmetic add expression" ) {
		auto nodeWithIncompleteAdd = AsDataNode("toplevel\n\t4 +");
		const auto set = ConditionSet{nodeWithIncompleteAdd, &store};
		THEN( "the expression should be identified as invalid" ) {
			const std::string validationWarning = "Expected terminal after infix operator \"+\":\n";
			const std::string invalidNodeText = "toplevel\n";
			const std::string invalidNodeTextInWarning = "L2:   4 +";

			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				std::string formatted = IgnoreLogHeaders(warnings.Flush());
				REQUIRE( formatted == validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n' );
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
			const std::string validationWarning = "Child-nodes expected, found none:\ntoplevel\n\n";
			set.Load(AsDataNode("toplevel"), &store);
			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( IgnoreLogHeaders(warnings.Flush()) == validationWarning );
			}
		}
		THEN( "no expressions are added from invalid nodes" ) {
			const std::string validationWarning = "Has keyword requires a single condition:\n";
			const std::string invalidNodeText = "and\n\thas";
			const std::string invalidNodeTextInWarning = "and\nL2:   has";
			set.Load(AsDataNode(invalidNodeText), &store);
			REQUIRE( set.IsEmpty() );
			REQUIRE_FALSE( set.IsValid() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( IgnoreLogHeaders(warnings.Flush()) == validationWarning + invalidNodeTextInWarning + '\n' + '\n');
			}
		}
		THEN( "new expressions can be added from valid nodes" ) {
			set.Load(AsDataNode("and\n\tnever"), &store);
			REQUIRE_FALSE( set.IsEmpty() );
			REQUIRE( set.IsValid() );
			REQUIRE( IgnoreLogHeaders(warnings.Flush()) == "" );
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
			{"and\n\t\t11\n\t\t2\n\t\t5", 11},
			{"and\n\t\t14\n\t\t0\n\t\t5", 0},
			{"or\n\t\t8\n\t\t2\n\t\t5", 8},
			{"or\n\t\t9\n\t\t0\n\t\t5", 9},
			{"or\n\t\t0\n\t\t0\n\t\t0", 0},
			{"or\n\t\t0\n\t\t0\n\t\t-7", -7},

			// Tests for min and max single parent conditions.
			{"max\n\t\t1\n\t\t10", 10},
			{"min\n\t\t1\n\t\t10", 1},
			{"max\n\t\t-30\n\t\t11", 11},
			{"min\n\t\t-30\n\t\t11", -30},

			{"max\n\t\t-30\n\t\t11\n\t\t5", 11},
			{"min\n\t\t-30\n\t\t11\n\t\t5", -30},
			{"max\n\t\t20", 20},
			{"min\n\t\t20", 20},

			// Tests for min and max as functions
			{"4 + max ( 10 , 20 )", 24},
			{"4 + min ( 10 , 20 )", 14},
			{"4 + max ( 10 , -20 )", 14},
			{"4 + min ( 10 , -20 )", -16},
			{"max ( 10 , 20 )", 20},
			{"min ( 10 , 20 )", 10},
			{"max ( 10 , -20 )", 10},
			{"min ( 10 , -20 )", -20},

			// Tests for inline and and or
			{"2 and 11", 2},
			{"11 and 2 and 5", 11},
			{"14 and 0 and 5", 0},
			{"0 or 11", 11},
			{"0 or 0", 0},
			{"8 or 2 or 5", 8},
			{"9 or 0 or 5", 9},
			{"0 or 0 or 0", 0},
			{"0 or 0 or -7", -7},

			// Some tests with not.
			{"not someData", 0},
			{"not missingData", 1},

			// Black magic below; parser might need to handle this, but nobody should ever write comparisons like this.
			{"1 > 2 == 0", 1},
			{"11 == 11 == 1", 1},
		}));
		const auto numberSet = ConditionSet{AsDataNode("toplevel\n\t" + std::get<0>(expressionAndAnswer)), &storeWithData};
		std::string expressionString = std::get<0>(expressionAndAnswer);
		auto answer = std::get<1>(expressionAndAnswer);
		bool boolAnswer = answer;
		THEN( "The expression \'" + expressionString + "\' is valid and evaluates to the correct number" ) {
			REQUIRE_FALSE( numberSet.IsEmpty() );
			REQUIRE( numberSet.IsValid() );
			REQUIRE( numberSet.Evaluate() == answer );
			REQUIRE( numberSet.Test() == boolAnswer );
		}
		THEN( "Tree loading and saving \'" + expressionString + "\' results in identical and working expressions" )
		{
			DataWriter dw1;
			dw1.WriteToken("toplevel");
			dw1.Write();
			dw1.BeginChild();
			numberSet.Save(dw1);
			dw1.EndChild();

			ConditionSet numberSetCopy = ConditionSet{AsDataNode(dw1.SaveToString()), &storeWithData};
			REQUIRE_FALSE( numberSetCopy.IsEmpty() );
			REQUIRE( numberSetCopy.IsValid() );
			REQUIRE( numberSetCopy.Evaluate() == answer );
			REQUIRE( numberSetCopy.Test() == boolAnswer );

			DataWriter dwCopy;
			dwCopy.WriteToken("toplevel");
			dwCopy.Write();
			dwCopy.BeginChild();
			numberSetCopy.Save(dwCopy);
			dwCopy.EndChild();
			REQUIRE( dw1.SaveToString() == dwCopy.SaveToString() );

			ConditionSet numberSetCopy2 = ConditionSet{AsDataNode(dwCopy.SaveToString()), &storeWithData};
			REQUIRE_FALSE( numberSetCopy2.IsEmpty() );
			REQUIRE( numberSetCopy2.IsValid() );
			REQUIRE( numberSetCopy2.Evaluate() == answer );
			REQUIRE( numberSetCopy2.Test() == boolAnswer );
		}
		THEN( "Inline loading and saving \'" + expressionString + "\' results in identical and working expressions" )
		{
			DataWriter dw1;
			numberSet.SaveInline(dw1);
			dw1.Write();

			ConditionSet numberSetCopy(&storeWithData);
			int numberSetCopyToken = 0;
			numberSetCopy.ParseNode(AsDataNode(dw1.SaveToString()), numberSetCopyToken);
			REQUIRE_FALSE( numberSetCopy.IsEmpty() );
			REQUIRE( numberSetCopy.IsValid() );
			REQUIRE( numberSetCopy.Evaluate() == answer );
			REQUIRE( numberSetCopy.Test() == boolAnswer );

			DataWriter dwCopy;
			numberSetCopy.SaveInline(dwCopy);
			dwCopy.Write();
			REQUIRE( dw1.SaveToString() == dwCopy.SaveToString() );

			ConditionSet numberSetCopy2(&storeWithData);
			int numberSetCopyToken2 = 0;
			numberSetCopy2.ParseNode(AsDataNode(dwCopy.SaveToString()), numberSetCopyToken2);
			REQUIRE_FALSE( numberSetCopy2.IsEmpty() );
			REQUIRE( numberSetCopy2.IsValid() );
			REQUIRE( numberSetCopy2.Evaluate() == answer );
			REQUIRE( numberSetCopy2.Test() == boolAnswer );
		}
	}
	GIVEN( "various incorrect expression(s) as conditionSet" ) {
		OutputSink warnings(std::cerr);
		auto expressionAndMessage = GENERATE(table<std::string, std::string>({
			{"4 +", "Expected terminal after infix operator \"+\":\n"},
			{"4 + 6 +", "Expected terminal after infix operator \"+\":\n"},
			{"4 + 6 -", "Expected terminal after infix operator \"-\":\n"},
			{"4 - 6 -", "Expected terminal after infix operator \"-\":\n"},
			{"4 77", "Expected infix operator instead of \"77\":\n"},
			{"%%percentFail", "Expected terminal or open-bracket:\n"},
			{") + 4", "Expected terminal or open-bracket:\n"},
			{") 4", "Expected terminal or open-bracket:\n"},
			{"( 4 + 6", "Missing closing bracket:\n"},
			{"( 4 + 6 )\n\t\t5 + 5", "Unexpected child-nodes under toplevel:\n"},
			{"never + 5", "Tokens found after never keyword:\n"},
			{"has someData + 5", "Has keyword requires a single condition:\n"},
			{"or", "Child-nodes expected, found none:\n"}
		}));
		const auto numberSet = ConditionSet{AsDataNode("toplevel\n\t" + std::get<0>(expressionAndMessage)), &storeWithData};
		THEN( "Expression \'" + std::get<0>(expressionAndMessage) + "\' is invalid and triggers error-message" ) {
			REQUIRE_FALSE( numberSet.IsValid() );
			std::string formatted = IgnoreLogHeaders(warnings.Flush());
			REQUIRE( formatted.substr(0, std::get<1>(expressionAndMessage).size()) == std::get<1>(expressionAndMessage) );
			REQUIRE( numberSet.IsEmpty() );
		}
	}
}

// #endregion unit tests



} // test namespace
