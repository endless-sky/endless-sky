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

#ifndef PARALLEL_H_
#define PARALLEL_H_

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



template<class ExecutionPolicy, class RandomIt, class Compare>
inline void sort(ExecutionPolicy e, RandomIt begin, RandomIt end, Compare comp)
{
	if(e == parallel::seq || e == parallel::unseq)
		std::sort(begin, end, comp);
	else
		oneapi::tbb::parallel_sort(begin, end, comp);
}


template<class ExecutionPolicy, class RandomIt>
inline void sort(ExecutionPolicy e, RandomIt begin, RandomIt end)
{
	sort(e, begin, end, std::less<decltype(*begin)>());
}



template<class ExecutionPolicy, class RandomIt, class Compare>
inline void stable_sort(ExecutionPolicy, RandomIt begin, RandomIt end, Compare comp)
{
	std::stable_sort(begin, end, comp);
}



template<class ExecutionPolicy, class RandomIt>
inline void stable_sort(ExecutionPolicy, RandomIt begin, RandomIt end)
{
	std::stable_sort(begin, end);
}

#else

#include <execution>
#include <functional>
#include <thread>
#include <vector>

namespace parallel = std::execution;

#endif

namespace {
	// A forced multithreaded for_each implementation where the executing threads are guaranteed
	// to terminate before leaving the function. Unlike the standard implementation, this does not
	// permit threads to be reused for future for_each_mt calls. This allows using ResourceGuards
	// with thread-local lifetime for greater efficiency.
	// This function is available on all backends, and does not require a random access iterator.
	template<class It, class Func>
	void for_each_mt(It begin, It end, Func &&f)
	{
		if(begin == end)
			return;

		int threadCount = std::thread::hardware_concurrency();
		std::vector<std::thread> threads;

		int subtaskCount = std::distance(begin, end);
		int taskCount = std::min(subtaskCount, threadCount);
		int itemPerTask = subtaskCount / taskCount;

		threads.reserve(taskCount + 1);
		It current = begin;
		for(int i = 0; i <= taskCount && current != end; i++)
		{
			It first = current;
			It last = (i == taskCount ? end : std::next(first, itemPerTask));
			threads.emplace_back([=]
			{
				for(It it = first; it != last; it++)
					f(*it);
			});
			current = last;
		}

		for(auto &thread : threads)
			thread.join();
	}
}

#endif
