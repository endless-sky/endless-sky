/* output-capture.hpp
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

#include <iostream>
#include <sstream>
#include <string>

class OutputSink {
public:
	// Redirect an ostream, such as std::cout or std::cerr.
	explicit OutputSink(std::ostream &toCapture)
		: captured(toCapture), original(toCapture.rdbuf())
	{
		// Store anything sent to the captured stream in our buffer.
		toCapture.rdbuf(storage.rdbuf());
	}

	// Restore the original buffer.
	~OutputSink() { captured.rdbuf(original); }
	// No moves/copies.
	OutputSink(const OutputSink &) = delete;
	OutputSink(OutputSink &&) = delete;

	// Read the captured buffer.
	std::string Peek() const { return storage.str(); }

	std::string Flush()
	{
		std::string result = Peek();
		Clear();
		return result;
	}

	// Clear the captured buffer.
	void Clear()
	{
		storage.str("");
		storage.clear();
	}


private:
	// The captured stream.
	std::ostream &captured;
	// The original stream's buffer.
	std::streambuf *original;
	// Our redirection buffer
	std::stringstream storage;
};
