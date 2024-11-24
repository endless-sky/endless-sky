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
				REQUIRE( warnings.Flush() ==  validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n');
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
				REQUIRE( warnings.Flush() ==  validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n');
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
				REQUIRE( warnings.Flush() ==  validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n');
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
				REQUIRE( warnings.Flush() ==  validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n');
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
				REQUIRE( warnings.Flush() ==  validationWarning + invalidNodeText + invalidNodeTextInWarning + '\n' + '\n');
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
	GIVEN( "a set containing 'never'" ) {
		// This test might need to be removed once keywords cannot be used as conditions.
		const auto neverSet = ConditionSet{AsDataNode("and\n\tnever")};
		REQUIRE_FALSE( neverSet.IsEmpty() );
		REQUIRE( neverSet.IsValid() );				

		AND_GIVEN( "a condition list containing the literal 'never'" ) {
			const auto storeWithNever = ConditionsStore {
				{"never", 1},
			};
			THEN( "the ConditionSet is not satisfied" ) {
				REQUIRE_FALSE( neverSet.Test(storeWithNever) );
				REQUIRE( neverSet.IsValid() );
			}
		}
	}

	GIVEN( "a single number" ) {
		const auto numberSet = ConditionSet{AsDataNode("toplevel\n\t2")};
		REQUIRE_FALSE( numberSet.IsEmpty() );
		REQUIRE( numberSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			THEN( "the ConditionSet is not satisfied" ) {
				REQUIRE( numberSet.Test(storeWithData) );
				REQUIRE( numberSet.IsValid() );
				REQUIRE( numberSet.Evaluate(storeWithData) == 2 );
			}
		}
	}
	GIVEN( "a simple arithmetic add expression" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t2 + 6")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 8 );
		}
	}
	GIVEN( "a somewhat longer arithmetic add expression" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t2 + 6 + 8 + 40")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 56 );
		}
	}
	GIVEN( "a somewhat longer arithmetic multiply expression" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t2 * 6 * 8 * 40")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 3840 );
		}
	}
	GIVEN( "an arithmetic set with multiplication and add" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t2 * 6 + 8")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 20 );
		}
	}
	GIVEN( "an arithmetic set with add and multiplication" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t2 + 6 * 8")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 50 );
		}
	}
	GIVEN( "an arithmetic set with multiplication and bracketed add" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t2 * ( 6 + 8 )")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 28 );
		}
	}
	GIVEN( "an arithmetic set with bracketed add and multiplication" ) {
		const auto arithmeticSet = ConditionSet{AsDataNode("toplevel\n\t( 2 + 6 ) * 8")};
		REQUIRE_FALSE( arithmeticSet.IsEmpty() );
		REQUIRE( arithmeticSet.IsValid() );
		THEN( "The condition evaluates to the correct number" ) {
			REQUIRE( arithmeticSet.Test(storeWithData) );
			REQUIRE( arithmeticSet.IsValid() );
			REQUIRE( arithmeticSet.Evaluate(storeWithData) == 64 );
		}
	}}

// #endregion unit tests



} // test namespace
