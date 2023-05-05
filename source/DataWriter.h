/* DataWriter.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DATA_WRITER_H_
#define DATA_WRITER_H_

#include <algorithm>
#include <functional>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

class DataNode;



// This class writes data in a hierarchical format, where an indented line is
// considered the "child" of the first line above it that is less indented. By
// using this class, you can have a function add data to the file without having
// to tell that function what indentation level it is at. This class also
// automatically adds quotation marks around strings if they contain whitespace.
class DataWriter {
public:
	// Constructor, specifying the file to write
	// and the specified string (usually tab or space) for indentation.
	explicit DataWriter(const std::string &path, const char *indentString = "\t");
	// Constructor for a DataWriter that will not save its contents automatically
	// and uses the specified string (usually tab or space) for indentation.
	explicit DataWriter(const char *indentString = "\t");
	DataWriter(const DataWriter &) = delete;
	DataWriter(DataWriter &&) = delete;
	DataWriter &operator=(const DataWriter &) = delete;
	DataWriter operator=(DataWriter &&) = delete;
	// The file is not actually saved until the destructor is called. This makes
	// it possible to write the whole file in a single chunk.
	~DataWriter();

	// Save the contents to a file.
	void SaveToPath(const std::string &path);
	// Save the contents to a stream.
	void SaveToStream(std::ostream &stream);
	// Pass the contents to a function.
	void SaveToFunction(const std::function<void(std::string)> func);

	// Writes the contents of the string without any escaping, quoting or
	// any other kind of modification.
	DataWriter &WriteRaw(const std::string &c);
	template <class C>
	DataWriter &WriteRaw(const C &c);
	// Writes the string that separates two tokens. If no tokens are present on the current line,
	// it writes the indentation string instead.
	DataWriter &WriteSeparator();

	// The Write() function can take any number of arguments. Each argument is
	// converted to a token. Arguments may be strings or numeric values.
	template <class A, class ...B>
	DataWriter &Write(const A &a, B... others);
	// Write the entire structure represented by a DataNode, including any
	// children that it has.
	DataWriter &Write(const DataNode &node);
	// End the current line. This can be used to add line breaks or to terminate
	// a line you have been writing token by token with WriteToken().
	DataWriter &Write();

	// Begin a new line that is a "child" of the previous line.
	DataWriter &BeginChild();
	// Finish writing a block of child nodes and decrease the indentation.
	DataWriter &EndChild();

	// Write a comment. It will be at the current indentation level, and will
	// have "# " inserted before it.
	DataWriter &WriteComment(const std::string &str);

	// Write a token, without writing a whole line. Use this very carefully.
	DataWriter &WriteToken(const char *a);
	DataWriter &WriteToken(const std::string &a);
	// Write a token of any arithmetic type.
	template <class A>
	DataWriter &WriteToken(const A &a);
	// Writes a series of tokens without terminating the line.
	template <class A, class ...B>
	DataWriter &WriteToken(const A &a, B... others);
	// Write the tokens of this DataNode without writing its children.
	DataWriter &WriteTokens(const DataNode &node);

	// Changes the separator used between tokens. The default is a single space.
	DataWriter &SetSeparator(const std::string &sep);


private:
	// Save path (in UTF-8). Empty string for in-memory DataWriter.
	std::string path;
	// Current indentation level.
	std::string indent;
	// The string used for indentation.
	const std::string indentString;
	// Before writing each token, we will write either the indentation string
	// above, or this string.
	std::string separator = " ";
	// Remember which string should be written before the next token. This is
	// "indent" for the first token in a line and "separator" for subsequent tokens.
	// Null pointer signals that this string was already written for the next token.
	const std::string *before;
	// Compose the output in memory before writing it to file.
	std::ostringstream out;
};



// The Write() function can take any number of arguments, each of which becomes
// a token. They must be either strings or numeric types.
template <class A, class ...B>
DataWriter &DataWriter::Write(const A &a, B... others)
{
	WriteToken(a);
	Write(others...);
	return *this;
}



// Write any numeric type as a single token.
template <class A>
DataWriter &DataWriter::WriteToken(const A &a)
{
	static_assert(std::is_arithmetic<A>::value,
		"DataWriter cannot output anything but strings and arithmetic types.");

	WriteSeparator();
	out << a;
	before = &separator;
	return *this;
}



// Writes a series of tokens without terminating the line.
template <class A, class ...B>
DataWriter &DataWriter::WriteToken(const A &a, B... others)
{
	WriteToken(a);
	const std::size_t n = sizeof...(B);
	if(n)
		WriteToken(others...);
	return *this;
}



// Encapsulate the logic for writing the contents of a collection in a sorted manner. The caller
// should provide a sorting method; it will be called with pointers to the type of the container.
// The provided write method will be called for each element of the container.
template <class T, template<class, class...> class C, class... Args, typename A, typename B>
void WriteSorted(const C<T, Args...> &container, A sortFn, B writeFn)
{
	std::vector<const T *> sorted;
	sorted.reserve(container.size());
	for(const auto &it : container)
		sorted.emplace_back(&it);
	std::sort(sorted.begin(), sorted.end(), sortFn);

	for(const auto &sit : sorted)
		writeFn(*sit);
}
template <class K, class V, class... Args, typename A, typename B>
void WriteSorted(const std::map<const K *, V, Args...> &container, A sortFn, B writeFn)
{
	std::vector<const std::pair<const K *const, V> *> sorted;
	sorted.reserve(container.size());
	for(const auto &it : container)
		sorted.emplace_back(&it);
	std::sort(sorted.begin(), sorted.end(), sortFn);

	for(const auto &sit : sorted)
		writeFn(*sit);
}



#endif
