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



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating an Account" , "[Account][Creation]" ) {
	GIVEN( "an account" ) {
		Account account;
		WHEN( "money is added" ) {
			REQUIRE( account.Credits() == 0. );
			account.AddCredits(100);
			THEN( "the balance is increased" ) {
				REQUIRE( account.Credits() == 100 );
			}
		}
		WHEN( "a fine is levied" ) {
			REQUIRE( account.TotalDebt() == 0 );
			account.AddFine(10000);
			THEN ( "debt is incurred" ) {
				REQUIRE( account.TotalDebt() == 10000 );
			}
		}
		WHEN( "Mortgage is added" ) {
			REQUIRE(account.Credits() == 0);
			account.AddMortgage(200);
			THEN( "The credits increased and the mortgage was added" ) {
				REQUIRE( account.Credits() == 200);
				REQUIRE( account.Mortgages().back().Type() == "Mortgage" );
				REQUIRE( account.Mortgages().back().Principal() == 200);
				REQUIRE( account.Mortgages().back().Interest() == ("0." + std::to_string(600 - account.CreditScore() / 2) + "%"));
			}
		}
	}
}
// #endregion unit tests



} // test namespace
