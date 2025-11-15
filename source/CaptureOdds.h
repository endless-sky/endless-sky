/* CaptureOdds.h
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

#include <vector>

class Ship;



// This class stores the odds that one ship will be able to capture another, and
// can report the odds for any number of crew up to what each ship starts out
// with (because the odds change each time a crew member is lost). If either
// ship has hand-to-hand weapons available, the crew members will make use of
// them starting with whatever weapon is most useful to them. In each round of
// combat, one ship will lose one crew member. Which ship loses depends on the
// ratio of the strengths of the two crews (plus weapons), and whether each crew
// is attacking or defending; defending crew get a +1 power bonus.
class CaptureOdds {
public:
	// Store the hand-to-hand combat power of the attacker and defender.
	// Since calculating the capture odds can be expensive, the odds
	// are only calculated when needed by calling the Calculate function.
	CaptureOdds(const Ship &attacker, const Ship &defender);

	// Generate the lookup table.
	void Calculate();

	// Get the odds of the attacker winning if the two ships have the given
	// number of crew members remaining.
	double Odds(int attackingCrew, int defendingCrew) const;
	// Get the expected number of casualties in the remainder of the battle if
	// the two ships have the given number of crew remaining.
	double AttackerCasualties(int attackingCrew, int defendingCrew) const;
	double DefenderCasualties(int attackingCrew, int defendingCrew) const;

	// Get the total power (inherent crew power plus bonuses from hand to hand
	// weapons) for each ship when they have the given number of crew remaining.
	double AttackerPower(int attackingCrew) const;
	double DefenderPower(int defendingCrew) const;


private:
	// Map crew numbers into an index in the lookup table.
	int Index(int attackingCrew, int defendingCrew) const;

	// Calculate attack or defense power for each number of crew members up to
	// the given ship's full complement.
	static std::vector<double> Power(const Ship &ship, bool isDefender);


private:
	// Attacker and defender power lookup tables.
	std::vector<double> powerA;
	std::vector<double> powerD;

	// Capture odds lookup table.
	std::vector<double> capture;
	// Expected casualties lookup table.
	std::vector<double> casualtiesA;
	std::vector<double> casualtiesD;
};
