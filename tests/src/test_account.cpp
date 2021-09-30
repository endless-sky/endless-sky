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
	GIVEN( "an account" ) {
		Account account;
		WHEN( "money is added" ){
			REQUIRE( account.Credits() == 0. );
			account.AddCredits(100);
			THEN( "the balance is increased" ){
				REQUIRE( account.Credits() == 100 );
			}
		}
		WHEN( "a fine is levied" ){
			REQUIRE( account.TotalDebt() == 0 );
			account.AddFine(10000);
			THEN ( "debt is incurred" ) {
				REQUIRE( account.TotalDebt() == 10000 );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
