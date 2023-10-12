/* TaskQueue.h
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <vector>



// Class for queuing up tasks that are to be executed in parallel by using
// every thread available on the executing CPU.
class TaskQueue {
public:
	// An internal structure representing an async task to execute.
	struct Task {
		TaskQueue *queue;

		// The function to execute in parallel.
		std::function<void()> async;
		// If specified, this function is called in the main thread after
		// the function above has finished executing.
		std::function<void()> sync;

		std::promise<void> futurePromise;
	};


public:
	// Initialize the threads used to execute the tasks.
	TaskQueue() noexcept = default;
	TaskQueue(const TaskQueue &) = delete;
	TaskQueue &operator=(const TaskQueue &) = delete;
	~TaskQueue();

	// Queue a function to execute in parallel, with an another optional function that
	// will get executed on the main thread after the first function finishes.
	void Run(std::function<void()> f, std::function<void()> g = {});

	// Process any tasks to be scheduled to be executed on the main thread.
	void ProcessTasks();

	// Whether there are any outstanding tasks left in this queue.
	bool IsDone() const;

	// Waits for all of this queue's task to finish.
	void Wait();


public:
	// Thread entry point.
	static void ThreadLoop() noexcept;


private:
	std::vector<std::shared_future<void>> futures;

	// Tasks from ths queue that need to be executed on the main thread.
	std::queue<std::function<void()>> syncTasks;
	std::mutex syncMutex;
};

#endif
