/* LockProvider.h
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

#ifndef LOCK_PROVIDER_H_
#define LOCK_PROVIDER_H_

#include <mutex>
#include <vector>



// Stores various mutexes, and provides an efficient way of locking them.
class LockProvider {
public:
	// Holds a locked mutex until destruction.
	class LockGuard {
	public:
		LockGuard(int index, std::mutex &mutex);
		~LockGuard();

		// Don't allow copying or moving
		LockGuard(const LockGuard &other) = delete;
		LockGuard(const LockGuard &&other) = delete;
		LockGuard &operator=(const LockGuard &other) = delete;
		LockGuard &operator=(const LockGuard &&other) = delete;

		// Gets the index of this mutex in the parent LockProvider.
		int Index() const;
	private:
		int index;
		std::mutex &mutex;
	};


public:
	// Creates a provider with the specified number of mutexes.
	LockProvider(int size);
	// Creates a provider with the default number of mutexes.
	LockProvider();

	// Don't allow copying or moving
	LockProvider(const LockProvider &other) = delete;
	LockProvider(const LockProvider &&other) = delete;
	LockProvider &operator=(const LockProvider &other) = delete;
	LockProvider &operator=(const LockProvider &&other) = delete;

	// Lock a mutex, and return a guard for that lock.
	const LockGuard Lock();
	// The number of mutexes stored.
	int Size() const;
private:
	std::vector<std::mutex> locks;
};



#endif
