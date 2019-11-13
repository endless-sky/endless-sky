/* CrewEconomics.h
Copyright (c) 2019 by Luke Arndt

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CREW_ECONOMICS_H_
#define CREW_ECONOMICS_H_

#include "Ship.h"

using namespace std;

class CrewEconomics
{
public:
  // Calculate one day's salaries for the Player's fleet
  static int64_t CalculateSalaries(const Ship *flagship, const vector<shared_ptr<Ship>> ships);

  // Maybe these could be game settings?
  static const int64_t CREW_PER_OFFICER = 5;
  static const int64_t CREDITS_PER_COMMANDER = 1000;
  static const int64_t CREDITS_PER_OFFICER = 250;
  static const int64_t CREDITS_PER_REGULAR = 100;
};

#endif
