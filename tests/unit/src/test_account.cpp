/* test_account.cpp
Copyright (c) 2022 by petervdmeer

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
#include "../../../source/Account.h"

// ... and any system includes needed for the test file.
#include <map>
#include <string>

using namespace std;

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
// run this test first so we don't have to retest the assumption later
TEST_CASE( "Add credits to an account", "[Account][AddCredits]" ) {
	Account account;
	REQUIRE(account.Credits() == 0);

	account.AddCredits(1000);
	REQUIRE(account.Credits() == 1000);
}

SCENARIO( "Create an Account" , "[Account][Creation]" ) {
	GIVEN( "an account" ) {
		Account account;
		WHEN( "a fine is levied" ) {
			REQUIRE( account.TotalDebt() == 0 );
			account.AddFine(10000);
			THEN ( "debt is incurred" ) {
				REQUIRE( account.TotalDebt() == 10000 );
			}
		}
	}
}

SCENARIO( "Step forward" , "[Account][Step]" ) {
	GIVEN( "An account with 1000 credits" ) {
		Account account;
		account.AddCredits(1000);
		THEN( "Account has 1000 credits" ) {
			REQUIRE( account.Credits() == 1000 );
		}

		WHEN( "Step() is called with no crew" ) {
			int64_t assets = 0;      // This is net worth of all ships
			int64_t salaries = 0;    // total owed in a single day's salaries
			int64_t maintenance = 0; // sum of maintenance and generated income

			string message = account.Step(assets, salaries, maintenance);
			INFO(message);
			string out = "";
			REQUIRE( message.compare(out) == 0 );
		}
	}
}

SCENARIO( "Pay crew salaries", "[Account][PayCrewSalaries]" ) {
	GIVEN( "An account" ) {
		Account account;
		WHEN( "no salaries are paid" ) {
			Bill salariesPaid = account.PayCrewSalaries(0);
			THEN( "The salaries were paid in full and no credits were paid" ) {
				REQUIRE(salariesPaid.creditsPaid == 0);
				REQUIRE(salariesPaid.paidInFull == true);
			}
		}

		WHEN( "500 in salaries are paid but the account has no credits" ) {
			REQUIRE(account.Credits() == 0);
			Bill salariesPaid = account.PayCrewSalaries(500);
			THEN( "The salaries were NOT paid in full and no credits were paid" ) {
				REQUIRE(salariesPaid.creditsPaid == 0);
				REQUIRE(salariesPaid.paidInFull == false);
			}
		}

		WHEN( "500 in salaries are paid and the account has 1000 credits" ) {
			account.AddCredits(1000);
			Bill salariesPaid = account.PayCrewSalaries(500);
			THEN( "The salaries were were paid in full and 500 credits were paid" ) {
				REQUIRE(salariesPaid.creditsPaid == 500);
				REQUIRE(salariesPaid.paidInFull == true);
			}
		}
	}
}
// #endregion unit tests



} // test namespace
