/* Random.cpp
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

#include "Random.h"

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
	normal_distribution<double> normal;
#else
	thread_local mt19937_64 gen;
	thread_local uniform_int_distribution<uint32_t> uniform;
	thread_local uniform_real_distribution<double> real;
	thread_local normal_distribution<double> normal;
#endif
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



// Get a normally distributed number with standard or specified mean and stddev.
double Random::Normal(double mean, double sigma)
{
#ifndef __linux__
	lock_guard<mutex> lock(workaroundMutex);
#endif
	return sigma * normal(gen) + mean;
}
