/* test_conditionAssignments.cpp
Copyright (c) 2024 by Peter van der Meer

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
#include "../../../source/ConditionAssignments.h"

// Include a helper for creating well-formed DataNodes (to enable creating non-empty ConditionSets).
#include "datanode-factory.h"
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
SCENARIO( "Creating ConditionAssignments" , "[ConditionAssignments][Creation]" ) {
	GIVEN( "no arguments" ) {
		const auto set = ConditionAssignments{};
		THEN( "no conditions are created" ) {
			REQUIRE( set.IsEmpty() );
		}
	}
}

SCENARIO( "Extending ConditionAssignments", "[ConditionAssignments][Creation]" ) {
	ConditionsStore store;
	OutputSink warnings(std::cerr);

	GIVEN( "empty ConditionAssignments" ) {
		auto set = ConditionAssignments{};
		REQUIRE( set.IsEmpty() );

		THEN( "no assignments are added from empty nodes" ) {
			const std::string validationWarning = "Loading empty set of assignments\ntoplevel\n\n";
			set.Load(AsDataNode("toplevel"), &store);
			REQUIRE( set.IsEmpty() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( IgnoreLogHeaders(warnings.Flush()) == validationWarning );
			}
		}
		THEN( "no assignments are added from invalid nodes" ) {
			const std::string validationWarning = "Incomplete assignment.\n";
			const std::string invalidNodeText = "apply\n\thas";
			const std::string invalidNodeTextInWarning = "apply\nL2:   has";
			set.Load(AsDataNode(invalidNodeText), &store);
			REQUIRE( set.IsEmpty() );
			AND_THEN( "a log message is printed to assist the user" ) {
				REQUIRE( IgnoreLogHeaders(warnings.Flush()) == "" + validationWarning + invalidNodeTextInWarning + '\n' + '\n');
			}
		}
		THEN( "new assignments can be added from valid nodes" ) {
			set.Load(AsDataNode("apply\n\tsomeCondition = 5"), &store);
			REQUIRE_FALSE( set.IsEmpty() );
			REQUIRE( IgnoreLogHeaders(warnings.Flush()) == "" );
		}
	}
}



SCENARIO( "Applying changes to conditions", "[ConditionAssignments][Usage]" ) {
	auto store = ConditionsStore{};
	REQUIRE( store.PrimariesSize() == 0 );

	GIVEN( "empty ConditionAssignments" ) {
		const auto emptySet = ConditionAssignments{};
		REQUIRE( emptySet.IsEmpty() );

		THEN( "no conditions are added via Apply" ) {
			emptySet.Apply();
			REQUIRE( store.PrimariesSize() == 0 );

			store.Set("event: war begins", 1);
			REQUIRE( store.PrimariesSize() == 1 );
			emptySet.Apply();
			REQUIRE( store.PrimariesSize() == 1 );
		}
	}
	GIVEN( "ConditionAssignments with an assignable expression" ) {
		const auto applySet = ConditionAssignments{AsDataNode("and\n\tyear = 3013"), &store};
		REQUIRE_FALSE( applySet.IsEmpty() );

		THEN( "the condition list is updated via Apply" ) {
			applySet.Apply();
			REQUIRE_FALSE( store.PrimariesSize() == 0 );
			REQUIRE( store.Get("year") );
			CHECK( store["year"] == 3013 );
		}
	}
	GIVEN( "valid ConditionAssignments" ) {
		auto storeWithData = ConditionsStore {
			{"event: war begins", 1},
			{"someData", 100},
			{"moreData", 100},
			{"otherData", 100},
		};

		auto expressionAndOutcome = GENERATE(table<std::string, std::string, int64_t>({
			{"year = 3013", "year", 3013},
			{"myVariable = -223", "myVariable", -223},
			{"someData >?= -223", "someData", 100},
			{"someData <?= -223", "someData", -223},
			{"someData += 223", "someData", 323},
			{"someData -= 223", "someData", -123},
			{"someData /= 50", "someData", 2},
		}));

		const auto applySet = ConditionAssignments{AsDataNode("toplevel\n\t" + std::get<0>(expressionAndOutcome)),
			&storeWithData};
		THEN( "The expression \'" + std::get<0>(expressionAndOutcome) + "\' assigns the expected number" ) {
			applySet.Apply();
			REQUIRE( storeWithData[std::get<1>(expressionAndOutcome)] == std::get<2>(expressionAndOutcome) );
		}
	}
}

// #endregion unit tests



} // test namespace
