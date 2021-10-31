/* Id.h
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ID_H_
#define ID_H_



// This class represents a unique id for an object. It is used to be able
// to "tag" objects that are batched drawn together.
class ID {
public:
	ID() noexcept : id(NewID()) {}
	ID(ID &&other) noexcept : id(other.id) { other.id = 0; }
	ID &operator=(ID &&other) noexcept { id = other.id; other.id = 0; return *this; }

	operator unsigned() const noexcept { return id; }


private:
	static unsigned NewID();


private:
	// The id number of this id.
	unsigned id;
};



#endif
