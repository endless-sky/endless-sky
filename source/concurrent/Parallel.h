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



// Only use this header if the underlying system doesn't support the standard algorithms,
// or if the ES_PARALLEL_USE_TASK_QUEUE flag is defined.
#if !defined(__cpp_lib_parallel_algorithm) || defined(ES_PARALLEL_USE_TASK_QUEUE)

// Mark that we are using a TaskQueue for parallel algorithms, instead of sdl functions.
#ifndef ES_PARALLEL_USE_TASK_QUEUE
#define ES_PARALLEL_USE_TASK_QUEUE
#endif


#include "../TaskQueue.h"

#include <algorithm>
#include <utility>



#if !defined(__cpp_lib_execution) || defined(ES_PARALLEL_USE_TASK_QUEUE)

// Dummy for std::execution.
namespace parallel
{
	constexpr int seq = 0;
	constexpr int par = 1;
	constexpr int par_unseq = 2;
};

#endif



// Executes tasks in parallel using a TaskQueue.
class Parallel
{
public:
	// Executes the given function over every element in the range.
	template<class RandomIt, class Func>
	static void RunBulk(const RandomIt begin, const RandomIt end, Func &&f);
	// Executes the given function asynchronously.
	static inline void Run(std::function<void()> f) {queue.Run(std::move(f));}
	// Waits for all tasks to finish.
	static inline void Wait() {queue.Wait();}


private:
	static TaskQueue queue;
};



// Executes the given function over every element in the range.
template<class RandomIt, class Func>
void Parallel::RunBulk(const RandomIt begin, const RandomIt end, Func &&f)
{
	if(begin == end)
		return;

	int subtaskCount = std::distance(begin, end);
	int taskCount = std::min(subtaskCount, static_cast<int>(8 * std::thread::hardware_concurrency()));
	int itemPerTask = subtaskCount / taskCount;

	RandomIt current = begin;
	for(int i = 0; i <= taskCount && current != end; i++)
	{
		RandomIt first = current;
		RandomIt last = (i == taskCount ? end : first + itemPerTask);
		current = last;
		Run([=]
		{
			for(RandomIt it = first; it != last; it++)
				f(*it);
		});
	}
}



// Dummies for parallel stl functions.
template<class ExecutionPolicy, class RandomIt, class Func>
inline void for_each(ExecutionPolicy e, RandomIt begin, RandomIt end, Func &&f)
{
	if(e == parallel::seq)
		std::for_each(begin, end, f);
	else
	{
		Parallel::RunBulk(begin, end, f);
		Parallel::Wait();
	}
}



template<class ExecutionPolicy, class RandomIt, class Compare>
inline void sort(ExecutionPolicy, RandomIt first, RandomIt last, Compare comp)
{
	std::sort(first, last, comp);
}



template<class ExecutionPolicy, class RandomIt>
inline void sort(ExecutionPolicy, RandomIt first, RandomIt last)
{
	std::sort(first, last);
}



template<class ExecutionPolicy, class RandomIt, class Compare>
inline void stable_sort(ExecutionPolicy, RandomIt first, RandomIt last, Compare comp)
{
	std::stable_sort(first, last, comp);
}



template<class ExecutionPolicy, class RandomIt>
inline void stable_sort(ExecutionPolicy, RandomIt first, RandomIt last)
{
	std::stable_sort(first, last);
}

#else

#include <execution>

namespace parallel = std::execution;

#endif

#endif
