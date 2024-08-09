/* PartiallyGuardedList.h
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

#pragma once

#include <list>
#include <mutex>



// Vectors that guard against most concurrent modifications. Use with caution.
// For most purposes, this can be treated and passed around like any stl container,
// but any function modifying it inside a concurrent context MUST receive it as a PartiallyGuarded object
// to ensure proper handling.
// Iterators, clear(), swap() and some other functions are left unguarded, as they are generally not used
// for concurrent modifications.
template<class T, class Allocator = std::allocator<T>>
class PartiallyGuardedList : public std::list<T, Allocator> {
public:
	template<class... Args>
	inline T &emplace_back(Args &&... args);

	// Prevent accidental unsafe operations.
	// These can be implemented later, if necessary.
	void push_back(const T &value) = delete;
	void push_back(T &&value) = delete;
	void push_front(const T &value) = delete;
	void push_front(T &&value) = delete;
	void insert(typename std::list<T, Allocator>::const_iterator pos, const T& value) = delete;
	template<class InputIt>
	void insert(typename std::list<T, Allocator>::const_iterator pos, InputIt first, InputIt last) = delete;
	template<class ...Args>
	void emplace(typename std::list<T, Allocator>::const_iterator pos, Args &&...args) = delete;
	void pop_back() = delete;
	void pop_front() = delete;
	template<class ...Args>
	void emplace_front(Args &&...args) = delete;


private:
	std::mutex write_mutex;
};



template<class T, class Allocator>
template<class... Args>
inline T &PartiallyGuardedList<T, Allocator>::emplace_back(Args &&...args)
{
	const std::lock_guard<std::mutex> lock(write_mutex);
	return std::list<T, Allocator>::emplace_back(args...);
}
