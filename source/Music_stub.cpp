/* Music.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Music.h"

void Music::Init(const std::vector<std::string> &sources) {}

Music::Music() {}
Music::~Music() {}

void Music::SetSource(const std::string &name) {}
const std::vector<int16_t> &Music::NextChunk() { return next; }

void Music::Decode() {}

const std::string &Music::GetSource() const { return currentSource; }
