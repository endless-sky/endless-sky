/* TaskQueue.cpp
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

#include "TaskQueue.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <exception>

using namespace std;

namespace {

	// The main task queue used by the worker threads.
	queue<TaskQueue::Task> tasks;
	mutex asyncMutex;
	condition_variable asyncCondition;
	bool shouldQuit = false;

	// Worker threads for executing tasks.
	struct WorkerThreads {
		WorkerThreads() noexcept
		{
			threads.resize(max(4u, thread::hardware_concurrency()));
			for(thread &t : threads)
				t = thread(&TaskQueue::ThreadLoop);
		}
		~WorkerThreads()
		{
			{
				lock_guard<mutex> lock(asyncMutex);
				shouldQuit = true;
			}
			asyncCondition.notify_all();
			for(thread &t : threads)
				t.join();
		}

		vector<thread> threads;
	} threads;
}



TaskQueue::~TaskQueue()
{
	// Make sure every task that belongs to this queue is finished.
	Wait();
}



// Queue a function to execute in parallel, with another optional function that
// will get executed on the main thread after the first function finishes.
// Returns a future representing the future result of the async call. Ignores
// any main thread task that still need to be executed!
shared_future<void> TaskQueue::Run(function<void()> asyncTask, function<void()> syncTask)
{
	shared_future<void> result;
	{
		lock_guard<mutex> lock(asyncMutex);
		// Do nothing if we are destroying the queue already.
		if(shouldQuit)
			return result;

		// Queue this task for execution and create a future to track its state.
		tasks.push(Task{this, std::move(asyncTask), std::move(syncTask)});
		result = futures.emplace_back(tasks.back().futurePromise.get_future());
		tasks.back().futureIt = prev(futures.end());
	}
	asyncCondition.notify_one();
	return result;
}



// Process any tasks to be scheduled to be executed on the main thread.
void TaskQueue::ProcessSyncTasks()
{
	unique_lock<mutex> lock(syncMutex);
	for(int i = 0; !syncTasks.empty() && i < MAX_SYNC_TASKS; ++i)
	{
		// Extract the one item we should work on right now.
		auto task = std::move(syncTasks.front());
		syncTasks.pop();

		lock.unlock();
		task();
		lock.lock();
	}
}



// Waits for all of this queue's task to finish. Ignores any sync tasks to be processed.
void TaskQueue::Wait()
{
	while(!IsDone())
		this_thread::yield();
}



// Whether there are any outstanding async tasks left in this queue.
bool TaskQueue::IsDone() const
{
	lock_guard<mutex> lock(asyncMutex);
	return futures.empty();
}



// Thread entry point.
void TaskQueue::ThreadLoop() noexcept
{
	while(true)
	{
		unique_lock<mutex> lock(asyncMutex);
		while(true)
		{
			// Check whether it is time for this thread to quit.
			if(shouldQuit)
				return;
			// No more tasks to execute, just go to sleep.
			if(tasks.empty())
				break;

			// Extract the one item we should work on reading right now.
			auto task = std::move(tasks.front());
			tasks.pop();

			// Unlock the mutex so other threads can access the queue.
			lock.unlock();

			// Execute the task.
			try {
				if(task.async)
					task.async();
			}
			catch(...)
			{
				// Any exception by the task is caught and rethrown inside the main thread
				// so we can handle it appropriately.
				auto exception = current_exception();
				task.sync = [exception] { rethrow_exception(exception); };
			}

			// If there is a followup function to execute, queue it for execution
			// in the main thread.
			if(task.sync)
			{
				unique_lock<mutex> lock(task.queue->syncMutex);
				task.queue->syncTasks.push(std::move(task.sync));
			}

			// We are done and can mark the future as ready.
			task.futurePromise.set_value();

			lock.lock();

			// Now that the task has been executed, stop tracking the future internally.
			// Anybody who still cares about the future will have a copy themselves.
			task.queue->futures.erase(task.futureIt);
		}

		asyncCondition.wait(lock, [] { return shouldQuit || !tasks.empty(); });
	}
}
