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
		Account account;
		THEN( "storing and retrieving data should work" ) {
			REQUIRE( account.Credits() == 0. );
			account.AddCredits(100);
			REQUIRE( account.Credits() == 100 );

			REQUIRE( account.TotalDebt() == 0 );
			account.AddFine(10000);
			REQUIRE( account.TotalDebt() == 10000 );
		}
	}
}
// #endregion unit tests



} // test namespace
