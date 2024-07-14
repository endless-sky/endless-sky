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

#include "Scheduler.h"

#include <version>
#include <execution>



namespace execution = std::execution;

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
	for(const auto &item : Scheduler::Schedule(begin, end, f, 8 * std::thread::hardware_concurrency()))
		Run(item);
	Wait();
}



// Dummies for parallel stl functions.
template<class ExecutionPolicy, class RandomIt, class Func>
inline void for_each(ExecutionPolicy e, RandomIt begin, RandomIt end, Func &&f)
{
	if(e == execution::seq)
		std::for_each(begin, end, f);
	else
		Parallel::RunBulk(begin, end, f);
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

#endif

namespace {
	// A forced multithreaded for_each implementation where the executing threads are guaranteed
	// to terminate before leaving the function. Unlike the standard implementation, this does not
	// permit threads to be reused for future for_each_mt calls. This allows using ResourceGuards
	// with thread-local lifetime for greater efficiency.
	// This function is available on all backends, and does not require a random access iterator.
	template<class It, class Func>
	inline void for_each_mt(It begin, It end, Func &&f)
	{
		const auto tasks = Scheduler::Schedule(begin, end, f, std::thread::hardware_concurrency());
		std::vector<std::thread> threads;
		threads.reserve(tasks.size());

		for(const auto &task : tasks)
			threads.emplace_back(task);

		for(auto &thread : threads)
			thread.join();
	}
}

#endif
