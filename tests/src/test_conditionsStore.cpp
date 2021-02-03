#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/ConditionsStore.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating a ConditionsStore" , "[ConditionsStore][Creation]" ) {
	GIVEN( "no arguments" ) {
		const auto store = ConditionsStore();
		THEN( "no conditions are stored" ) {
			REQUIRE( store.GetPrimaryConditions().empty() );
		}
	}
	GIVEN( "an initializer list" ) {
		const auto store = ConditionsStore{{"hello world", 100}, {"goodbye world", 404}};
		
		THEN( "given conditions are in the Store" ) {
			REQUIRE( store["hello world"] == 100 );
			REQUIRE( store["goodbye world"] == 404 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
		THEN( "not given conditions return the default value" ) {
			REQUIRE( 0 == store["ungreeted world"] );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store["ungreeted world"] == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
	}
	GIVEN( "an initializer map" ) {
		const std::map<std::string, int64_t> initmap {{"hello world", 100}, {"goodbye world", 404}};
		const auto store = ConditionsStore(initmap);
		
		THEN( "given conditions are in the Store" ) {
			REQUIRE( store["hello world"] == 100 );
			REQUIRE( store["goodbye world"] == 404 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
		THEN( "not given conditions return the default value" ) {
			REQUIRE( store["ungreeted world"] == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
	}
}

SCENARIO( "Setting and erasing conditions", "[ConditionSet][ConditionSetting]" ) {
	GIVEN( "an empty starting store" ) {
		auto store = ConditionsStore();
		THEN( "stored values can be retrieved" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.SetCondition("myFirstVar", 10) == true );
			REQUIRE( store["myFirstVar"] == 10 );
			REQUIRE( store.HasCondition("myFirstVar") == true );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
		}
		THEN( "defaults are returned for unstored values and are not stored" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store["mySecondVar"] == 0 );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.HasCondition("mySecondVar") == false );
		}
		THEN( "erased conditions are indeed removed" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store.SetCondition("myFirstVar", 10) == true );
			REQUIRE( store.HasCondition("myFirstVar") == true );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store["myFirstVar"] == 10 );
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.EraseCondition("myFirstVar") == true );
			REQUIRE( store.HasCondition("myFirstVar") == false );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
			REQUIRE( store["myFirstVar"] == 0 );
			REQUIRE( store.HasCondition("myFirstVar") == false );
			REQUIRE( store.GetPrimaryConditions().size() == 0 );
		}
	}
}

SCENARIO( "Adding and removing on condition values", "[ConditionSet][ConditionArithmetic]" ) {
	GIVEN( "an starting store with 1 condition" ) {
		auto store = ConditionsStore{{"myFirstVar", 10}};
		THEN( "adding to an existing variable gives the new value" ) {
			REQUIRE( store["myFirstVar"] == 10 );
			REQUIRE( store.AddCondition("myFirstVar", 10) == true );
			REQUIRE( store["myFirstVar"] == 20 );
			REQUIRE( store.AddCondition("myFirstVar", -15) == true );
			REQUIRE( store["myFirstVar"] == 5 );
			REQUIRE( store.AddCondition("myFirstVar", -15) == true );
			REQUIRE( store["myFirstVar"] == -10 );
		}
		THEN( "adding to an non-existing variable sets the new value" ) {
			REQUIRE( store.GetPrimaryConditions().size() == 1 );
			REQUIRE( store.AddCondition("mySecondVar", -30) == true );
			REQUIRE( store["mySecondVar"] == -30 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
			REQUIRE( store.HasCondition("mySecondVar") == true );
			REQUIRE( store.AddCondition("mySecondVar", 60) == true );
			REQUIRE( store["mySecondVar"] == 30 );
			REQUIRE( store.GetPrimaryConditions().size() == 2 );
		}
	}
}
// #endregion unit tests



} // test namespace
