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
#include <string>



// Collection of functions for generating random numbers with a variety of
// different distributions. (This is done partly because on some systems the
// random number generation is not thread-safe.)
class Random {
public:
	// Seed the generator (e.g. to make it produce exactly the same random
	// numbers it produced previously).
	static void Seed(uint64_t seed);
	
	static uint32_t Int();
	static uint32_t Int(uint32_t modulus);
	
	static double Real();
	
	// Functions for getting random numbers that are "just for effect", meaning
	// that calling them doesn't change the RNG used for more important game
	// state. Examples are random pitch changes in sound effects and the
	// generation of the background star field at program startup.
	static uint32_t EffectInt();
	static uint32_t EffectInt(uint32_t modulus);

	static double EffectReal();
	
	// Slower functions for getting random numbers from a given distribution.
	// Do not use these functions in time-critical code.
	
	// Return the expected number of failures before k successes, when the 
	// probability of success is p. The mean value will be k / (1 - p).
	static uint32_t Polya(uint32_t k, double p = .5);
	// Get a number from a binomial distribution (i.e. integer bell curve).
	static uint32_t Binomial(uint32_t t, double p = .5);
	// Get a normally distributed number (mean = 0, sigma= 1).
	static double Normal();
	
	// Functions for loading and saving the state of the random number generator.
	// Get the RNG state.
	static std::string GetState();
	// Set the RNG state.
	static void SetState(const std::string &state);
};



#endif
