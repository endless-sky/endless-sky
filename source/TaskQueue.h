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
#include <list>
#include <mutex>
#include <queue>



// Class for queuing up tasks that are to be executed in parallel by using
// every thread available on the executing CPU.
// The queue is also responsible to execute follow-up tasks that need to
// executed after the async task, for example uploading a loaded to the GPU
// (which needs to happen on the main thread on OpenGL).
class TaskQueue {
public:
	// An internal structure representing a task to execute.
	struct Task {
		TaskQueue *queue;

		// The function to execute in parallel.
		std::function<void()> async;
		// If specified, this function is called in the main thread after
		// the function above has finished executing.
		std::function<void()> sync;

		// The pointer to this task's future stored in the queue.
		std::list<std::shared_future<void>>::const_iterator futureIt;

		std::promise<void> futurePromise;
	};

	// The maximum amount of sync tasks to execute in one go.
	static constexpr int MAX_SYNC_TASKS = 100;

public:
	// Initialize the threads used to execute the tasks.
	TaskQueue() = default;
	TaskQueue(const TaskQueue &) = delete;
	TaskQueue &operator=(const TaskQueue &) = delete;
	~TaskQueue();

	// Queue a function to execute in parallel, with another optional function that
	// will get executed on the main thread after the first function finishes.
	// Returns a future representing the future result of the async call. Ignores
	// any main thread task that still need to be executed!
	std::shared_future<void> Run(std::function<void()> asyncTask, std::function<void()> syncTask = {});

	// Process any tasks to be scheduled to be executed on the main thread.
	void ProcessSyncTasks();

	// Waits for all of this queue's task to finish. Ignores any sync tasks to be processed.
	void Wait();


private:
	// Whether there are any outstanding async tasks left in this queue.
	bool IsDone() const;


public:
	// Thread entry point.
	static void ThreadLoop() noexcept;


private:
	std::list<std::shared_future<void>> futures;

	// Tasks from ths queue that need to be executed on the main thread.
	std::queue<std::function<void()>> syncTasks;
	mutable std::mutex syncMutex;
};

#endif
