/* WithMutex.h
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

#include <mutex>



// Helper class to add a mutex to Body without declaring the full copy constructor.
// These operations are not thread safe. Do not copy the object in a concurrent context,
// or with a locked mutex.
// The use of the mutex must be enforced externally.
class WithMutex
{
public:
	WithMutex() = default;
	WithMutex(const WithMutex &other);
	WithMutex(const WithMutex &&other) noexcept;
	WithMutex &operator=(const WithMutex &other);
	WithMutex &operator=(const WithMutex &&other) noexcept;

	// Gets the mutex for this object.
	std::mutex &GetMutex();
	[[nodiscard]] std::lock_guard<std::mutex> Lock() const;
protected:
	mutable std::mutex mutex;
};
