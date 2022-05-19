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

#include "pi.h"

#include <random>

#ifndef __linux__
#include <mutex>
#endif

using namespace std;

// Right now thread_local storage is only supported under Linux.
namespace {
#ifndef __linux__
	mutex workaroundMutex;
	mt19937_64 gen;
	uniform_int_distribution<uint32_t> uniform;
	uniform_real_distribution<double> real;
	bool normalBMCached = false;
	double cachedBMNormal = std::numeric_limits<double>::infinity();
#else
	thread_local mt19937_64 gen;
	thread_local uniform_int_distribution<uint32_t> uniform;
	thread_local uniform_real_distribution<double> real;
	thread_local bool normalBMCached = false;
	thread_local double cachedBMNormal = std::numeric_limits<double>::infinity();
#endif

static const double epsilon = std::numeric_limits<double>::epsilon();
}


// Seed the generator (e.g. to make it produce exactly the same random
// numbers it produced previously).
void Random::Seed(uint64_t seed)
{
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	gen.seed(seed);
}



uint32_t Random::Int()
{
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return uniform(gen);
}



uint32_t Random::Int(uint32_t upper_bound)
{
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	const uint32_t x = uniform(gen);
	return (static_cast<uint64_t>(x) * static_cast<uint64_t>(upper_bound)) >> 32;
}



double Random::Real()
{
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return real(gen);
}



// Return the expected number of failures before k successes, when the
// probability of success is p. The mean value will be k / (1 - p).
uint32_t Random::Polya(uint32_t k, double p)
{
	negative_binomial_distribution<uint32_t> polya(k, p);
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return polya(gen);
}



// Get a number from a binomial distribution (i.e. integer bell curve).
uint32_t Random::Binomial(uint32_t t, double p)
{
	binomial_distribution<uint32_t> binomial(t, p);
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return binomial(gen);
}



// Get a normally distributed number (mean = 0, sigma= 1) using std::normal_distribution.
double Random::StdNormal()
{
	normal_distribution<double> normal;
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return normal(gen);
}



// Get a normally distributed number (mean = 0, sigma= 1) using std::normal_distribution.
double Random::StdNormal(double mean, double sigma)
{
	normal_distribution<double> normal{mean, sigma};
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return normal(gen);
}



// Get a normally distributed number (mean = 0, sigma = 1) using the Box-Muller transform.
// Cache the unused value without transforming it so that it can be transformed when it's used.
double Random::BMNormal(double mean, double sigma)
{
	if(normalBMCached)
	{
		#ifndef __linux__
			lock_guard<mutex> lock(workaroundMutex);
		#endif
		normalBMCached = false;
		return sigma * cachedBMNormal + mean;
	}
	else
	{
		double u1, u2;
		do
		{
			u1 = Random::Real();
		}
		while (u1 <= epsilon);
		u2 = Random::Real();

		// Store z0 and return z1
		auto mag = sqrt(-2.0 * log(u1));
		#ifndef __linux__
			lock_guard<mutex> lock(workaroundMutex);
		#endif
		cachedBMNormal = mag * cos(PI_2 * u2);
		normalBMCached = true;
		return sigma * mag * sin(PI_2 * u2) + mean;
	}
}
