#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/ConditionSet.h"

// Include DataNode, for generating mock node lists to parse.
#include "../../source/DataNode.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>

namespace { // test namespace

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionSet" , "[ConditionSet][Creation]" ) {
	GIVEN( "no arguments" ) {
		const auto set = ConditionSet{};
		THEN( "no conditions are created" ) {
			REQUIRE( set.IsEmpty() );
		}
	}
	GIVEN( "an empty DataNode" ) {
		const auto emptyNode = DataNode{};
		const auto set = ConditionSet{emptyNode};
		THEN( "no conditions are created" ) {
			REQUIRE( set.IsEmpty() );
		}
	}
}

SCENARIO( "Determining if condition requirements are met", "[ConditionSet][Usage]" ) {
	GIVEN( "an empty ConditionSet" ) {
		const auto emptySet = ConditionSet{};
		REQUIRE( emptySet.IsEmpty() );
		AND_GIVEN( "an empty list of Conditions" ) {
			const auto emptyConditionList = ConditionSet::Conditions{};
			THEN( "the ConditionSet is satisfied" ) {
				REQUIRE( emptySet.Test(emptyConditionList) );
			}
		}
		AND_GIVEN( "a non-empty list of Conditions" ) {
			const auto conditionList = ConditionSet::Conditions{
				{"event: war begins", 1},
			};
			THEN( "the ConditionSet is satisfied" ) {
				REQUIRE( emptySet.Test(conditionList) );
			}
		}
	}
}

SCENARIO( "Applying changes to conditions", "[ConditionSet][Usage]" ) {
	GIVEN( "an empty ConditionSet" ) {
		const auto emptySet = ConditionSet{};
		REQUIRE( emptySet.IsEmpty() );
		
		auto mutableList = ConditionSet::Conditions{};
		REQUIRE( mutableList.empty() );
		THEN( "no conditions are added via Apply" ) {
			emptySet.Apply(mutableList);
			REQUIRE( mutableList.empty() );
			
			mutableList.emplace("event: war begins", 1);
			REQUIRE( mutableList.size() == 1 );
			emptySet.Apply(mutableList);
			REQUIRE( mutableList.size() == 1 );
		}
	}
}
// #endregion unit tests



} // test namespace
