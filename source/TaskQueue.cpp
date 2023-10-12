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



	// The maximum amount of sync tasks to execute in one go.
	constexpr int MAX_SYNC_TASKS = 100;

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



// Queue a function to execute in parallel, with an another optional function that
// will get executed on the main thread after the first function finishes.
void TaskQueue::Run(function<void()> f, function<void()> g)
{
	{
		lock_guard<mutex> lock(asyncMutex);
		// Do nothing if we are destroying the queue already.
		if(shouldQuit)
			return;

		// Queue this task for execution and create a future to track its state.
		tasks.push(Task{this, std::move(f), std::move(g)});
		futures.emplace_back(tasks.back().futurePromise.get_future());
	}
	asyncCondition.notify_one();
}



// Process any tasks to be scheduled to be executed on the main thread.
void TaskQueue::ProcessTasks()
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



bool TaskQueue::IsDone() const
{
	return std::all_of(futures.begin(), futures.end(), [](const shared_future<void> &future) {
			return future.wait_for(std::chrono::seconds(0)) == future_status::ready;
		}) && syncTasks.empty();
}


// Waits for all of this queue's task to finish.
void TaskQueue::Wait()
{
	// Process tasks while any task is still being executed.
	while(!IsDone())
		ProcessTasks();
	futures.clear();
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
			// No more tasks to execute, just to to sleep.
			if(tasks.empty())
				break;

			// Extract the one item we should work on reading right now.
			auto task = std::move(tasks.front());
			tasks.pop();

			// Unlock the mutex so other threads can access the queue.
			lock.unlock();

			if(task.sync)
			{
				// If the queue's sync task is full, push this task back into the queue
				// to help reduce load on the main thread.
				unique_lock<mutex> syncLock(task.queue->syncMutex);
				if(task.queue->syncTasks.size() > MAX_SYNC_TASKS)
				{
					lock.lock();
					tasks.push(std::move(task));
					continue;
				}
			}

			// Execute the task.
			try {
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
		}

		asyncCondition.wait(lock);
	}
}
