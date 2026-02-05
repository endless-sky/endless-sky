/* ByUUID.h
Copyright (c) 2021 by Benjamin Hauch

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

#include <memory>



template<class T>
class ByUUID {
public:
	// Comparator for collections of shared_ptr<T>
	bool operator()(const std::shared_ptr<T> &a, const std::shared_ptr<T> &b) const noexcept(false)
	{
		return a->UUID() < b->UUID();
	}

	// Comparator for collections of T*, e.g. set<T *>
	bool operator()(const T *a, const T *b) const noexcept(false)
	{
		return a->UUID() < b->UUID();
	}
	// No comparator for collections of T, as std containers generally perform copy operations
	// and copying this class will eventually be disabled.
};
