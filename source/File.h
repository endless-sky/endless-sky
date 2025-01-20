/* File.h
Copyright (c) 2015 by Michael Zahniser

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

#include <cstdio>
#include <filesystem>



// RAII wrapper for SDL_RWops, to make sure it gets closed if an error occurs.
class File {
public:
	File() noexcept = default;
	explicit File(const std::filesystem::path &path, bool write = false);
	File(const File &) = delete;
	File(File &&other) noexcept;
	~File() noexcept;

	// Do not allow copying the FILE pointer.
	File &operator=(const File &) = delete;
	// Move assignment is OK though.
	File &operator=(File &&) noexcept;

	explicit operator bool() const;
	operator struct SDL_RWops*() const;
	struct SDL_RWops* operator->();

private:
	struct SDL_RWops *file = nullptr;
};
