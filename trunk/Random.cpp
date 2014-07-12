/* Random.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Random.h"

#include <random>

using namespace std;

namespace {
	thread_local mt19937_64 gen;
	thread_local uniform_int_distribution<uint32_t> uniform;
	thread_local bool isInit = false;
	
	inline void Init()
	{
		if(!isInit)
		{
			random_device rd;
			gen = mt19937_64(rd());
			isInit = true;
		}
	}
}



uint32_t Random::Int()
{
	Init();
	return uniform(gen);
}



uint32_t Random::Int(uint32_t modulus)
{
	Init();
	return uniform(gen) % modulus;
}

// Return the expected number of failures before k successes, when the 
// probability of success is p. The mean value will be k / (1 - p).
uint32_t Random::Polya(uint32_t k, double p)
{
	Init();
	negative_binomial_distribution<uint32_t> polya(k, p);
	return polya(gen);
}

