/* File.h
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FILE_H_
#define FILE_H_

#include <cstdio>
#include <string>



// RAII wrapper for FILE, to make sure it gets closed if an error occurs.
class File {
public:
	File() = default;
	explicit File(const std::string &path, bool write = false);
	File(const File &) = delete;
	File(File &&other);
	~File();
	
	// Do not allow copying the FILE pointer.
	File &operator=(const File &) = delete;
	// Move assignment is OK though.
	File &operator=(File &&other);
	
	operator bool() const;
	operator FILE*() const;
	
private:
	FILE *file = nullptr;
};



#endif
