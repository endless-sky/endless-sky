/* LockProvider.cpp
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

#include "LockProvider.h"

#include <thread>

using namespace std;



LockProvider::LockGuard::LockGuard(int index, std::mutex &mutex) : index(index), mutex(mutex)
{
}



LockProvider::LockGuard::~LockGuard()
{
	mutex.unlock();
}



// Gets the index of this mutex in the parent LockProvider.
int LockProvider::LockGuard::Index() const
{
	return index;
}



// Creates a provider with the specified number of mutexes.
LockProvider::LockProvider(int size) : locks(size)
{
}



// Creates a provider with the default number of mutexes.
LockProvider::LockProvider() : LockProvider(thread::hardware_concurrency())
{
}



// Lock a mutex, and return a guard for that lock.
const LockProvider::LockGuard LockProvider::Lock()
{
	// Find a free lock, and call MoveShip with the locked lists.
	size_t id = std::hash<thread::id>{}(this_thread::get_id());
	const int index = id % Size();
	if(locks[index].try_lock())
		return LockGuard(index, locks[index]);
	else
	{
		for(unsigned newIndex = 0; newIndex < Size(); newIndex++)
			if(locks[newIndex].try_lock())
				return LockGuard(newIndex, locks[newIndex]);
	}
	// If there is no free lock, fall back to the original guess.
	locks[index].lock();
	return LockGuard(index, locks[index]);
}



// The number of mutexes stored.
int LockProvider::Size() const
{
	return locks.size();
}
