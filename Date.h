/* Date.h
Michael Zahniser, 28 Oct 2013

Class representing a date (day, month, and year).
*/

#ifndef DATE_H_INCLUDED
#define DATE_H_INCLUDED

#include <ctime>
#include <string>



class Date {
public:
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
