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

#ifdef __APPLE__

#include "pstl/algorithm"
#include "pstl/execution"

namespace parallel = pstl::execution;
using namespace pstl;

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
