/* LockedOrderedSet.h
Copyright (c) 2026 by xobes

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

#include "OrderedSet.h"

#include <mutex>



template<typename T>
class LockedOrderedSet {
public:
	LockedOrderedSet(std::mutex &guard, OrderedSet<T> &data) : guard(guard), data(data) {}
	OrderedSet<T> *operator->() { return &data; }
	OrderedSet<T> &operator*() { return data; }

private:
	std::lock_guard<std::mutex> guard;
	OrderedSet<T> &data;
};
