/* Mortgage.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>
#include <string>

class DataNode;
class DataWriter;



// Class representing a mortgage (or a fine that can be paid in installments).
// The interest rate depends on your credit score, and on whether this is an
// ordinary mortgage versus a fine (high interest) or a crew bonus (low). This
// also handles the calculations for determining how big a new mortgage you can
// qualify for, based on your average daily income.
class Mortgage {
public:
	// Find out how much you can afford to borrow with the given annual revenue
	// and the given credit score (which should be between 200 and 800).
	static int64_t Maximum(int64_t annualRevenue, int creditScore, double currentPayments);


public:
	Mortgage() = default;
	// Create a new mortgage of the given amount. If this is a fine, set the
	// credit score to zero for a higher interest rate.
	Mortgage(std::string type, int64_t principal, int creditScore, int term = 365);
	// Create a mortgage with a specific interest rate instead of using the player's
	// credit score. Due to how the class is set up, the interest rate must currently
	// be within the range [0, 1).
	Mortgage(std::string type, int64_t principal, double interest, int term = 365);
	// Construct and Load() at the same time.
	Mortgage(const DataNode &node);

	// Load or save mortgage data.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Make a mortgage payment. The return value is the amount paid.
	int64_t MakePayment();
	void MissPayment();
	// Pay down additional principal. Unlike a "real" mortgage, this reduces
	// the minimum amount of your future payments, not the term of the mortgage.
	// This returns the actual amount paid, which may be less if the total
	// principal remaining is less than the given amount.
	int64_t PayExtra(int64_t amount);

	// The type is "Mortgage" if this is a mortgage you applied for from a bank,
	// "Fine" if this is a fine imposed on you for illegal activities, and
	// "Debt" if this is debt given to you by a mission.
	const std::string &Type() const;
	// Get the remaining mortgage principal.
	int64_t Principal() const;
	// Get the interest rate. It is formatted as a string, because all that the
	// program will ever do with this is display it.
	const std::string &Interest() const;
	// Get the remaining number of payments that must be made.
	int Term() const;
	// Check the amount of the next payment due (rounded to the nearest credit).
	int64_t Payment() const;
	// Check the amount of the next payment due.
	double PrecisePayment() const;


private:
	// Note: once a mortgage is set up, only the principal and term will change.
	std::string type;
	int64_t principal = 0;
	double interest = 0.;
	std::string interestString;
	int term = 365;
};
