/* Scheduler.h
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

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <functional>
#include <thread>
#include <vector>



// Utility class for batch execution.
class Scheduler
{
public:
	// Creates a list of batched tasks for the [begin, end) range
	// which combined execute f(*it) over every element in the range.
	template<class It, class Func>
	static std::vector<std::function<void()>> Schedule(It begin, It end, Func &&f, int idealTaskCount);
};



template<class It, class Func>
std::vector<std::function<void()>> Scheduler::Schedule(It begin, It end, Func &&f, int idealTaskCount)
{
	if(begin == end)
		return {};

	std::vector<std::function<void()>> tasks;

	int subtaskCount = std::distance(begin, end);
	int taskCount = std::min(subtaskCount, idealTaskCount);
	int itemPerTask = subtaskCount / taskCount;

	tasks.reserve(taskCount + 1);
	It current = begin;
	for(int i = 0; i <= taskCount && current != end; i++)
	{
		It first = current;
		It last = (i == taskCount ? end : std::next(first, itemPerTask));
		current = last;
		tasks.emplace_back([=]
		{
			for(It it = first; it != last; it++)
				f(*it);
		});
	}
	return tasks;
}



#endif