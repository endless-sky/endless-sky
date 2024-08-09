/* Parallel.h
Copyright (c) 2024 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <version>

// Silence warnings about unimplemented vectorized algorithms.
// TODO: Replace with 'PSTL_USAGE_WARNINGS=0' definition in the cmake config
// when upstream fixes the bug that causes it to be ignored.
#ifdef _PSTL_PRAGMA_MESSAGE
#undef _PSTL_PRAGMA_MESSAGE
#define _PSTL_PRAGMA_MESSAGE(x)
#endif

#ifndef __cpp_lib_execution

#include <algorithm>
#include <utility>

#include <oneapi/tbb.h>

// Dummy for std::execution.
namespace parallel
{
	constexpr int seq = 0;
	constexpr int par = 1;
	constexpr int par_unseq = 2;
	constexpr int unseq = 3;
};

// Dummies for parallel stl functions.
template<class ExecutionPolicy, class RandomIt, class Func>
inline void for_each(const ExecutionPolicy e, RandomIt begin, RandomIt end, Func &&f)
{
	if(e == parallel::seq || e == parallel::unseq)
		std::for_each(begin, end, f);
	else
		oneapi::tbb::parallel_for_each(begin, end, f);
}

#else

#include <execution>
#include <functional>
#include <thread>
#include <vector>

namespace parallel = std::execution;

#endif
