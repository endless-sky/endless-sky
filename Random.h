/* Random.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef RANDOM_H_
#define RANDOM_H_

#include <cstdint>



class Random {
public:
	static uint32_t Int();
	static uint32_t Int(uint32_t modulus);
	
	// Return the expected number of failures before k successes, when the 
	// probability of success is p. The mean value will be k / (1 - p).
	static uint32_t Polya(uint32_t k, double p);
};



#endif
