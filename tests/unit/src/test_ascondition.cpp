/* test_conditionsstore_datanode.cpp
Copyright (c) 2020 by anonmyous author

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

// Include only the two classes whose interactions are being tested.
#include "../../../source/ConditionsStore.h"
#include "../../../source/DataNode.h"

// Include a helper for creating well-formed DataNodes.
#include "datanode-factory.h"
#include "output-capture.hpp"

// ... and any system includes needed for the test file.
#include <string>
#include <vector>


namespace { // test namespace
// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
TEST_CASE( "DataNode connections to ConditionsStore and Condition", "[ConditionsStoreDataNode]" ) {
	using T = DataNode;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		// The class layout apparently satisfies StandardLayoutType when building/testing for Steam, but false otherwise.
		// This may change in the future, with the expectation of false everywhere (due to the list<DataNode> field).
		// CHECK_FALSE( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		CHECK_FALSE( std::is_trivially_destructible<T>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_default_constructible<T>::value );
		// We explicitly allocate some memory when default-constructing DataNodes.
		CHECK_FALSE( std::is_nothrow_default_constructible<T>::value );
		CHECK( std::is_copy_constructible<T>::value );
		// We have work to do when copy-constructing, including allocations.
		CHECK_FALSE( std::is_trivially_copy_constructible<T>::value );
		CHECK_FALSE( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		// We have work to do when move-constructing.
		CHECK_FALSE( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
	}
	SECTION( "Copy Traits" ) {
		CHECK( std::is_copy_assignable<T>::value );
		// The class data can be spread out due to vector + list contents.
		CHECK_FALSE( std::is_trivially_copyable<T>::value );
		// We have work to do when copying.
		CHECK_FALSE( std::is_trivially_copy_assignable<T>::value );
		CHECK_FALSE( std::is_nothrow_copy_assignable<T>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		CHECK_FALSE( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}

}

SCENARIO( "Creating a Condition from a DataNode", "[DataNode]") {
	GIVEN( "Conditions within a DataNode" ) {
		ConditionsStore vars;
		double value = 131, defaultValue = 5, literalValue = 3;
		vars.Set("notmissing", value);
		DataNode node = AsDataNode("missing 3 notmissing");
		WHEN( "using AsCondition on a missing condition" ) {
			THEN( "should return a Condition with the default value and requested key" ) {
				CHECK( node.AsCondition(0, &vars, defaultValue) == defaultValue );
				CHECK( node.AsCondition(0, &vars, defaultValue).Key() == "missing" );
			}
		}
		WHEN( "using AsCondition with no ConditionsStore" ) {
			THEN( "should return a Condition with the default value and requested key" ) {
				CHECK( node.AsCondition(0, nullptr, defaultValue) == defaultValue );
				CHECK( node.AsCondition(0, nullptr, defaultValue).Key() == "missing" );
			}
		}
		WHEN( "using AsCondition with a literal and a ConditionsStore" ) {
			THEN( "should return a Condition with the specified value and an empty key" ) {
				CHECK( node.AsCondition(1, &vars, defaultValue) == literalValue);
				CHECK( node.AsCondition(1, &vars, defaultValue).Key().empty() );
			}
		}
		WHEN( "using AsCondition with a literal and no ConditionsStore" ) {
			THEN( "should return a Condition with the specified value and an empty key" ) {
				CHECK( node.AsCondition(1, nullptr, defaultValue) == literalValue);
				CHECK( node.AsCondition(1, nullptr, defaultValue).Key().empty() );
			}
		}
		WHEN( "using AsCondition with a non-missing condition in a ConditionsStore" ) {
			THEN( "should return a Condition with the value from the ConditionStore and the requested key" ) {
				CHECK( node.AsCondition(2, &vars, defaultValue) == value );
				CHECK( node.AsCondition(2, &vars, defaultValue).Key() == "notmissing" );
			}
		}
		WHEN( "using AsCondition on an index past the end of the list" ) {
			THEN( "should return a Condition with the default value and an empty key" ) {
				CHECK( node.AsCondition(12, &vars, defaultValue) == defaultValue );
				CHECK( node.AsCondition(12, &vars, defaultValue).Key().empty() );
			}
		}
		WHEN( "using AsCondition on a negative index" ) {
			THEN( "should return a Condition with the default value and an empty key" ) {
				CHECK( node.AsCondition(12, &vars, defaultValue) == defaultValue );
				CHECK( node.AsCondition(12, &vars, defaultValue).Key().empty() );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
