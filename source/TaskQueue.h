/* TaskQueue.h
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#include <functional>
#include <future>



// Class for queuing up tasks that are to be executed in parallel by using
// every thread available on the executing CPU.
class TaskQueue {
public:
	// Initialize the threads used to execute the tasks.
	TaskQueue();
	~TaskQueue();

	// Queue a function to execute in parallel.
	static std::future<void> Run(std::function<void()> f);
	// Queue a function to execute in parallel and another function that
	// will get executed on the main thread after the first function finishes.
	static std::future<void> Run(std::function<void()> f, std::function<void()> g);

	// Process any tasks scheduled to be executed on the main thread.
	static void ProcessTasks();
};

#endif
