/* CaptureOdds.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CAPTURE_ODDS_H_
#define CAPTURE_ODDS_H_

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
	CaptureOdds(const Ship *attacker, const Ship *defender);
	
	double Odds(int attackingCrew, int defendingCrew) const;
	double AttackerCasualties(int attackingCrew, int defendingCrew) const;
	double DefenderCasualties(int attackingCrew, int defendingCrew) const;
	
	double AttackerPower(int attackingCrew) const;
	double DefenderPower(int defendingCrew) const;
	
	
private:
	void Calculate();
	int Index(int attackingCrew, int defendingCrew) const;
	
	static void Make(std::vector<double> *power, const Ship *ship, bool isDefender);
	
	
private:
	std::vector<double> powerA;
	std::vector<double> powerD;
	
	std::vector<double> capture;
	std::vector<double> casualtiesA;
	std::vector<double> casualtiesD;
};



#endif
