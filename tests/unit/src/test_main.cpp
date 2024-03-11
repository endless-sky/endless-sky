/* test_main.cpp
Copyright (c) 2020 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#define CATCH_CONFIG_RUNNER
#include "es-test.hpp"

#include "../../../source/Random.h"

#include <ctime>

int main(int argc, const char *const argv[])
{
	// Seed the random number generator.
	Random::Seed(time(nullptr));

	// Run the tests.
	return Catch::Session().run(argc, argv);
}
// Add nothing else to this file (unless you like long recompilation times)!
