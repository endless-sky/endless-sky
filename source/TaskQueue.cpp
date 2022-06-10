/* TaskQueue.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TaskQueue.h"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>

using namespace std;

namespace {
	struct Task {
		// The function to execute in parallel.
		packaged_task<void()> async;
		// If specified, this function is called in the main thread after
		// the function above has finished executing.
		function<void()> sync;
	};

	queue<Task> tasks;
	mutex asyncMutex;
	condition_variable asyncCondition;
	bool shouldQuit = false;

	// These image sets have been loaded from disk but have not been uploaded.
	queue<function<void()>> syncTasks;
	mutex syncMutex;

	// Worker threads for executing tasks.
	vector<thread> threads;

	// The maximum amount of sync tasks to execute in one go.
	constexpr int MAX_SYNC_TASKS = 100;

	// Thread entry point.
	void ThreadLoop() noexcept
	{
		while(true)
		{
			unique_lock<mutex> lock(asyncMutex);
			while(true)
			{
				// Check whether it is time for this thread to quit.
				if(shouldQuit)
					return;
				if(tasks.empty())
					break;

				// Extract the one item we should work on reading right now.
				auto task = std::move(tasks.front());
				tasks.pop();

				// Unlock the mutex so other threads can access the queue.
				lock.unlock();

				// Execute the task.
				try
				{
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
					{
						unique_lock<mutex> lock(syncMutex);
						syncTasks.push(std::move(task.sync));
					}
				}

				lock.lock();
			}

			asyncCondition.wait(lock);
		}
	}
}



// Initialize the threads used to execute the tasks.
void TaskQueue::Init()
{
	threads.resize(max(4u, thread::hardware_concurrency()));
	for(thread &t : threads)
		t = thread(&ThreadLoop);
}



void TaskQueue::Quit()
{
	{
		lock_guard<mutex> lock(asyncMutex);
		shouldQuit = true;
	}
	asyncCondition.notify_all();
	for(thread &t : threads)
		t.join();
}



// Queue a function to execute in parallel.
future<void> TaskQueue::Run(function<void()> f)
{
	return Run(std::move(f), function<void()>());
}



// Queue a function to execute in parallel and another function that
// will get executed on the main thread after the first function finishes.
future<void> TaskQueue::Run(function<void()> f, function<void()> g)
{
	future<void> result;
	{
		lock_guard<mutex> lock(asyncMutex);
		// Do nothing if we are destroying the queue already.
		if(shouldQuit)
			return result;

		// Queue this task for execution and create a future to track its state.
		packaged_task<void()> task(std::move(f));
		result = task.get_future();
		tasks.push(Task{std::move(task), std::move(g)});
	}
	asyncCondition.notify_one();

	return result;
}



// Process any tasks scheduled to be executed on the main thread.
void TaskQueue::ProcessTasks()
{
	unique_lock<mutex> lock(syncMutex);
	for(int i = 0; !syncTasks.empty() && i < MAX_SYNC_TASKS; ++i)
	{
		// Extract the one item we should work on right now.
		auto task = syncTasks.front();
		syncTasks.pop();

		lock.unlock();

		task();

		lock.lock();
	}
}
