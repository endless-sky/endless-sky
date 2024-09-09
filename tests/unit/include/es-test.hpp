/* es-test.hpp
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

// Enable the BENCHMARK macro sets, unless told not to
#ifndef NO_BENCHMARKING
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#endif

#ifdef _WIN32
// We require SEH for Windows builds, so we test with SEH support too.
#define CATCH_CONFIG_WINDOWS_SEH
// Check for memory leaks from test code.
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif

#include <catch2/catch_all.hpp>
