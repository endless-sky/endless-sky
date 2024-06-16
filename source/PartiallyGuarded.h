/* PartiallyGuarded.h
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

#ifndef PARTIALLY_GUARDED_H_
#define PARTIALLY_GUARDED_H_

#include <mutex>
#include <list>

// A list and vector that guards against concurrent emplace_back calls, and nothing else. Use with caution.
// For most purposes, this can be treated and passed around like any std::list or std::vector,
// but any function modifying it inside a concurrent context MUST receive it as a PartiallyGuardedList/Vector
// to ensure proper handling.
template<class T, class Allocator = std::allocator<T>>
class PartiallyGuardedList : public std::list<T, Allocator> {
public:
	template<class... Args>
	inline T &emplace_back(Args &&... args)
	{
		const std::lock_guard<std::mutex> lock(write_mutex);
		return std::list<T, Allocator>::emplace_back(args...);
	}

private:
	std::mutex write_mutex;
};

template<class T, class Allocator = std::allocator<T>>
class PartiallyGuardedVector : public std::vector<T, Allocator> {
public:
	template<class... Args>
	inline T &emplace_back(Args &&... args)
	{
		const std::lock_guard<std::mutex> lock(write_mutex);
		return std::vector<T, Allocator>::emplace_back(args...);
	}

private:
	std::mutex write_mutex;
};

#endif
