/* Date.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DATE_H_
#define DATE_H_

#include <ctime>
#include <string>



// Class representing a date (day, month, and year).
class Date {
public:
	Date();
	Date(int day, int month, int year);
	
	const std::string &ToString() const;
	
	void operator++();
	void operator++(int);
	
	// Get the number of days that have elapsed since the "epoch".
	double DaysSinceEpoch() const;
	
	// Get the date as numbers.
	int Day() const;
	int Month() const;
	int Year() const;
	
	
private:
	time_t today;
	mutable std::string str;
};



#endif
