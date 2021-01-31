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
			REQUIRE( store.Locals().empty() );
		}
	}
	GIVEN( "an initializer list" ) {
		const auto store = ConditionsStore{{"hello world", 100}, {"goodbye world", 404}};
		
		THEN( "given conditions are in the Store" ) {
			REQUIRE( store.GetCondition("hello world") == 100 );
			REQUIRE( store.GetCondition("goodbye world") == 404 );
			REQUIRE( store.Locals().size() == 2 );
		}
		THEN( "not given conditions return the default value" ) {
			REQUIRE( store.GetCondition("ungreeted world") == 0 );
			REQUIRE( store.Locals().size() == 2 );
		}
	}
	GIVEN( "an initializer map" ) {
		const std::map<std::string, int64_t> initmap {{"hello world", 100}, {"goodbye world", 404}};
		const auto store = ConditionsStore(initmap);
		
		THEN( "given conditions are in the Store" ) {
			REQUIRE( store.GetCondition("hello world") == 100 );
			REQUIRE( store.GetCondition("goodbye world") == 404 );
		}
		THEN( "not given conditions return the default value" ) {
			REQUIRE( store.GetCondition("ungreeted world") == 0 );
		}
	}
}
// #endregion unit tests



} // test namespace
