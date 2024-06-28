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
#include <sstream>
#include <string>

using namespace std;

namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
// run this test first so we don't have to retest the assumption later

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

// Test the SetCredits and AddCredits functions second to ensure that when we
// set the balance for future tests, it is proven to work properly
SCENARIO( "Operations on credits", "[Account][credits]" ) {
	GIVEN( "An account" ) {
		Account account;
		THEN( "The account starts with 0 credits" ) {
			REQUIRE(account.Credits() == 0);
		}

		WHEN( "SetCredits is called with 5" ) {
			account.SetCredits(5);
			THEN( "The number of credits will be 5" ) {
				REQUIRE(account.Credits() == 5);
			}
		}

		WHEN( "AddCredits is called with 5" ) {
			account.AddCredits(5);
			THEN( "The number of credits will be 5" ) {
				REQUIRE(account.Credits() == 5);
			}
			AND_WHEN( "AddCredits is called with -5" ) {
				account.AddCredits(-5);
				THEN( "The number of credits will be 0" ) {
					REQUIRE(account.Credits() == 0);
				}
			}
		}
	}
}

// Tests to ensure proper adding of account balance to the history, as well
// as proving that the history is maintaing the proper size and removing
// the oldest entries as expected.
SCENARIO( "Operations on history", "[Account][history]" ) {
	GIVEN( "An account" ) {
		Account account;
		THEN( "The account starts with an empty history" ) {
			REQUIRE(account.History().empty());
				REQUIRE(account.NetWorth() == 0);
		}

		WHEN( "AddHistory is called with 1000" ) {
			account.AddHistory(1000);
			THEN( "History wil have a size of 1 and 1000 in the first index." ) {
				REQUIRE(account.History().size() == 1);
				REQUIRE(account.History().at(0) == 1000);
				REQUIRE(account.NetWorth() == 1000);
			}
			AND_WHEN( "AddHistory is called 100 more times" ) {
				for(int64_t i = 0; i < 100; i++)
				{
					account.AddHistory(i);
				}
				THEN( "Index 0 will contain 0 and the size will be 100" ) {
					REQUIRE(account.History().at(0) == 0);
					REQUIRE(account.History().size() == 100);
				}
			}
		}
	}
}

// Tests to ensure that mortgages and fines are being correctly manipulated
// by the helper functions created to handle them.
SCENARIO( "Working with mortgages on an account", "[Account][mortgages]" ) {
	GIVEN( "An account" ) {
		Account account;
		WHEN( "A mortgage is added to the account" ) {
			account.AddMortgage(480000);
			THEN( "The user has 1 mortgage and 480,000 credits" ) {
				REQUIRE(account.Credits() == 480000);
				REQUIRE(account.CreditScore() == 400);
				REQUIRE(account.Mortgages().size() == 1);
			}
			AND_WHEN( "A fine is added to the account" ) {
				account.AddFine(20000);
				THEN( "The user has 2 mortgages" ) {
					REQUIRE(account.Credits() == 480000);
					REQUIRE(account.CreditScore() == 400);
					REQUIRE(account.Mortgages().size() == 2);
				}
			}
		}
		WHEN( "The mortgage is paid off at the bank" ) {
			account.PayExtra(0, 480000);
			THEN( "The mortgage is removed from the account" ) {
				REQUIRE(account.Mortgages().size() == 0);
			}
		}
	}
}

// Test to ensure salary payment behaviour is working as expected
SCENARIO( "Paying crew salaries during Step", "[Account][Step]" ) {
	GIVEN( "An account with no credits" ) {
		Account account;
		WHEN( "Crew salaries are paid during Step" ) {
			string message = account.Step(0, 100, 0);
			THEN( "The crew salaries cannot be paid, is stored as overdue, and the credit score drops" ) {
				REQUIRE(message == "You could not pay all your crew salaries. ");
				REQUIRE(account.OverdueCrewSalaries() == 100);
				REQUIRE(account.CreditScore() == 395);
			}
			AND_WHEN( "The player has enough credits to pay off all crew salaries" ) {
				account.SetCredits(200);
				string message = account.Step(0, 100, 0);
				string expectedMessage = "You paid 200 credits in crew salaries.";
				THEN( "All overdue crew salaries are paid off and the credit score rises" ) {
					REQUIRE(account.Credits() == 0);
					REQUIRE(message == expectedMessage);
					REQUIRE(account.OverdueCrewSalaries() == 0);
					REQUIRE(account.CreditScore() == 396);
				}
			}
		}
	}
}

// Test to ensure maintenance payment behaviour is working as expected
SCENARIO( "Paying maintenance during Step", "[Account][Step]" ) {
	GIVEN( "An account with no credits" ) {
		Account account;
		WHEN( "Maintenance is paid during Step" ) {
			string message = account.Step(0, 0, 100);
			THEN( "The maintenance cannot be paid, is stored as overdue, and the credit score drops" ) {
				REQUIRE(message == "You could not pay all your maintenance costs. ");
				REQUIRE(account.OverdueMaintenance() == 100);
				REQUIRE(account.CreditScore() == 395);
			}
			AND_WHEN( "The player has enough credits to pay off all maintenance" ) {
				account.SetCredits(200);
				string message = account.Step(0, 0, 100);
				string expectedMessage = "You paid 200 credits in maintenance.";
				THEN( "All overdue maintenance is paid off and the credit score rises" ) {
					REQUIRE(account.Credits() == 0);
					REQUIRE(message == expectedMessage);
					REQUIRE(account.OverdueMaintenance() == 0);
					REQUIRE(account.CreditScore() == 396);
				}
			}
		}
	}
}

// Test to ensure mortgage payment behaviour is working as expected
SCENARIO( "Paying Mortgages during Step", "[Account][Step]" ) {
	GIVEN( "An account with a mortgage" ) {
		Account account;
		int64_t principal = 20000;
		account.AddMortgage(principal);
		int64_t expectedPayment = account.Mortgages().at(0).Payment();
		ostringstream expectedMessage;
		expectedMessage << "You paid " << expectedPayment << " credits in mortgages.";
		WHEN( "A payment is made by an account that has enough credits" ) {
			string message = account.Step(0, 0, 0);
			THEN("The mortgage payment is made successfully and the credit score increases") {
				REQUIRE(account.Credits() == (principal - expectedPayment));
				REQUIRE(message == expectedMessage.str());
				REQUIRE(account.CreditScore() == 401);
			}
		}
		WHEN( "A payment is made by an account that does NOT enough credits" ) {
			account.SetCredits(5);
			string message = account.Step(0, 0, 0);
			THEN("The mortgage payment is not made, the principal increases, and the credit score decreases") {
				REQUIRE(account.Credits() == 5);
				REQUIRE(message == "You missed a mortgage payment. ");
				REQUIRE(account.Mortgages().at(0).Principal() > principal);
				REQUIRE(account.CreditScore() == 395);
			}
		}
		WHEN( "A final payment is made" ) {
			account.SetCredits(principal * 2);
			account.PayExtra(0, principal - 1);
			string message = account.Step(0, 0, 0);
			THEN("The mortgage payment is made, the credit score increases, and the mortgage is removed") {
				REQUIRE(message == "You paid 1 credit in mortgages.");
				REQUIRE(account.Mortgages().size() == 0);
				REQUIRE(account.CreditScore() == 401);
			}
		}
	}
}

// Test to ensure fine payment behaviour is working as expected
SCENARIO( "Paying Fines during Step", "[Account][Step]" ) {
	GIVEN( "An account with a fine" ) {
		Account account;
		int64_t principal = 1000;
		account.AddCredits(principal);
		account.AddFine(principal);
		int64_t expectedPayment = account.Mortgages().at(0).Payment();
		ostringstream expectedMessage;
		expectedMessage << "You paid " << expectedPayment << " credits in fines.";
		WHEN( "A payment is made by an account that has enough credits" ) {
			string message = account.Step(0, 0, 0);
			THEN("The fine payment is made successfully and the credit score increases") {
				REQUIRE(account.Credits() == (principal - expectedPayment));
				REQUIRE(message == expectedMessage.str());
				REQUIRE(account.CreditScore() == 401);
			}
		}
		WHEN( "A payment is made by an account that does NOT enough credits" ) {
			account.SetCredits(5);
			string message = account.Step(0, 0, 0);
			THEN("The fine payment is not made, the principal increases, and the credit score decreases") {
				REQUIRE(account.Credits() == 5);
				REQUIRE(message == "You missed a mortgage payment. ");
				REQUIRE(account.Mortgages().at(0).Principal() > principal);
				REQUIRE(account.CreditScore() == 395);
			}
		}
		WHEN( "A final payment is made" ) {
			account.SetCredits(principal * 2);
			account.PayExtra(0, principal - 1);
			string message = account.Step(0, 0, 0);
			THEN("The fine payment is made, the credit score increases, and the mortgage is removed") {
				REQUIRE(message == "You paid 1 credit in fines.");
				REQUIRE(account.Mortgages().size() == 0);
				REQUIRE(account.CreditScore() == 401);
			}
		}
	}
}

// Tests to ensure that overdueCrewSalaries is being correctly manipulated
// by the helper functions created to handle them.
SCENARIO( "Operations on overdueCrewSalaries", "[Account][overdueCrewSalaries]" ) {
	GIVEN( "An account" ) {
		Account account;
		WHEN( "overdueCrewSalaries is 0" ) {
			THEN( "OverdueCrewSalaries will return 0" ) {
				REQUIRE(account.OverdueCrewSalaries() == 0);
			}
			AND_WHEN( "SetOverdueCrewSalaries is used to set a value" ) {
				account.SetOverdueCrewSalaries(1000);
				THEN( "overdueCrewSalaries is set to that value" ) {
					REQUIRE(account.OverdueCrewSalaries() == 1000);
				}
				AND_WHEN( "PayOverdueCrewSalaries is used to pay off that value" ) {
					account.SetCredits(1000);
					account.PayOverdueCrewSalaries(1000);
					THEN( "overdueCrewSalaries is 0" ) {
						REQUIRE(account.OverdueCrewSalaries() == 0);
					}
				}
			}
		}
	}
}

// Tests to ensure that overdueMaintenance is being correctly manipulated
// by the helper functions created to handle them.
SCENARIO( "Operations on overdueMaintenance", "[Account][overdueMaintenance]" ) {
	GIVEN( "An account" ) {
		Account account;
		WHEN( "overdueMaintenance is 0" ) {
			THEN( "overdueMaintenance will return 0" ) {
				REQUIRE(account.OverdueMaintenance() == 0);
			}
			AND_WHEN( "SetOverdueMaintenance is used to set a value" ) {
				account.SetOverdueMaintenance(1000);
				THEN( "overdueMaintenance is set to that value" ) {
					REQUIRE(account.OverdueMaintenance() == 1000);
				}
				AND_WHEN( "PayOverdueMaintenance is used to pay off that value" ) {
					account.SetCredits(1000);
					account.PayOverdueMaintenance(1000);
					THEN( "overdueMaintenance is 0" ) {
						REQUIRE(account.OverdueMaintenance() == 0);
					}
				}
			}
		}
	}
}

// Tests to ensure that salaries (e.g, Free Worlds pay, pirate tribute, etc.) is
// being correctly manipulated by the helper functions created to handle them.
SCENARIO( "Operations on player salaries", "[Account][salariesIncome]" ) {
	GIVEN( "An account" ) {
		Account account;
		THEN( "The account starts with no player salaries" ) {
			REQUIRE(account.SalariesIncome().empty());
		}

		WHEN( "SetSalariesIncome is called" ) {
			account.SetSalariesIncome("test", 1000);
			THEN( "The data is stored properly" ) {
				REQUIRE(account.SalariesIncome().size() == 1);
				REQUIRE(account.SalariesIncome().find("test")->second == 1000);
			}
			AND_WHEN( "SalariesIncomeTotal is called after adding another couple salaries" ) {
				account.SetSalariesIncome("test2", 2000);
				account.SetSalariesIncome("test3", 3000);
				THEN("The correct total is returned") {
					REQUIRE(account.SalariesIncomeTotal() == 6000);
				}
				AND_WHEN("SetSalariesIncome is called to zero out the first salary") {
					account.SetSalariesIncome("test", 0);
					THEN( "Only the last two salaries remain" ) {
						REQUIRE(account.SalariesIncome().size() == 2);
						REQUIRE(account.SalariesIncome().find("test") ==
							account.SalariesIncome().end());
					}
				}
			}
		}
	}
}

// Overarching tests of the Step function given various account states.
// Ensures that all functions added and new logic matches the output of
// Step()'s pre-refactor output.
SCENARIO( "Step forward" , "[Account][Step]" ) {
	GIVEN( "An account" ) {
		Account account;

		WHEN( "Step() is called with no payments needed" ) {
			string message = account.Step(0, 0, 0);
			THEN( "There will be no message returns" ) {
				REQUIRE(message == "");
				REQUIRE(account.History().size() == 1);
			}
		}
		WHEN( "Step is called with salaries, maintenance, and a fine that cannot be paid" ) {
			account.AddFine(1000);
			string message = account.Step(0, 100, 100);
			ostringstream expectedMessage;
			expectedMessage << "You could not pay all your crew salaries. ";
			expectedMessage << "You could not pay all your maintenance costs. ";
			expectedMessage << "You missed a mortgage payment. ";
			THEN( "The message will contain data" ) {
				REQUIRE(message == expectedMessage.str());
			}
		}
		WHEN( "Step is called with salaries and maintenance that can be paid" ) {
			account.AddCredits(1000);
			string message = account.Step(0, 100, 100);
			THEN( "The message will contain data" ) {
				REQUIRE(message == "You paid 100 credits in crew salaries and 100 credits in maintenance.");
			}
		}
		WHEN( "Step is called with salaries, maintenance, a mortgage, and a fine that can be paid" ) {
			account.AddMortgage(1000);
			account.AddFine(100);
			string message = account.Step(0, 100, 100);
			ostringstream expectedMessage;
			expectedMessage << "You paid 100 credits in crew salaries, ";
			expectedMessage << "2 credits in fines, ";
			expectedMessage << "100 credits in maintenance, ";
			expectedMessage << "and 5 credits in mortgages.";
			THEN( "The message will contain data" ) {
				REQUIRE(message == expectedMessage.str());
			}
		}
		WHEN( "Step is called with amounts owed for each type, but only enough credits for non-mortgages" ) {
			account.AddMortgage(1000);
			account.AddFine(100);
			account.SetCredits(200);
			string message = account.Step(0, 100, 100);
			ostringstream expectedMessage;
			expectedMessage << "You missed a mortgage payment. ";
			expectedMessage << "You paid 100 credits in crew salaries and 100 credits in maintenance.";
			THEN( "The message will contain data" ) {
				REQUIRE(message == expectedMessage.str());
			}
		}
	}
}

// #endregion unit tests



} // test namespace
