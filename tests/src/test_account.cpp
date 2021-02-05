#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/Account.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating an Account" , "[Account][Creation]" ) {
	GIVEN( "initial values" ) {
		Account account = Account();
		THEN( "storing and retrieving data should work" ) {
			REQUIRE( account.Credits() == 0. );
			REQUIRE( account.GetCondition("credits") == 0 );
			account.AddCredits(100);
			REQUIRE( account.GetCondition("credits") == 100 );

			REQUIRE( account.GetCondition("unpaid fines") == 0 );
			account.AddFine(10000);
			REQUIRE( account.GetCondition("unpaid fines") == 10000 );
		}
		THEN( "auto conditions should be present, and others not" )
		{
			REQUIRE( account.HasCondition("net worth") );
			REQUIRE( account.HasCondition("credits") );
			REQUIRE( account.HasCondition("unpaid mortgages") );
			REQUIRE( account.HasCondition("unpaid fines") );
			REQUIRE( account.HasCondition("unpaid salaries") );
			REQUIRE( account.HasCondition("unpaid maintenance") );
			REQUIRE( account.HasCondition("credit score") );

			REQUIRE_FALSE( account.HasCondition("ships") );
		}
	}
}
// #endregion unit tests



} // test namespace
