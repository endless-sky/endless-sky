/* DataWriter.cpp
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

#include "DataWriter.h"

#include "DataNode.h"
#include "Files.h"

using namespace std;



// This string constant is just used for remembering what string needs to be
// written before the next token - either the full indentation or this, a space:
const string DataWriter::space = " ";



// Constructor, specifying the file to save.
DataWriter::DataWriter(const filesystem::path &path)
	: DataWriter()
{
	this->path = path;
}



// Constructor for a DataWriter that will not save its contents automatically
DataWriter::DataWriter()
	: before(&indent)
{
	out.precision(8);
}



// Destructor, which saves the file all in one block.
DataWriter::~DataWriter()
{
	if(!path.empty())
		SaveToPath(path);
}



// Save the contents to a file.
void DataWriter::SaveToPath(const filesystem::path &filepath)
{
	Files::Write(filepath, out.str());
}



// Get the contents as a string.
string DataWriter::SaveToString() const
{
	return out.str();
}



// Write a DataNode with all its children.
void DataWriter::Write(const DataNode &node)
{
	// Write all this node's tokens.
	for(int i = 0; i < node.Size(); ++i)
		WriteToken(node.Token(i).c_str());
	Write();

	// If this node has any children, call this function recursively on them.
	if(node.HasChildren())
	{
		BeginChild();
		{
			for(const DataNode &child : node)
				Write(child);
		}
		EndChild();
	}
}



// Begin a new line of the file.
void DataWriter::Write()
{
	out << '\n';
	before = &indent;
}



// Increase the indentation level.
void DataWriter::BeginChild()
{
	indent += '\t';
}



// Decrease the indentation level.
void DataWriter::EndChild()
{
	indent.erase(indent.length() - 1);
}



// Write a comment line, at the current indentation level.
void DataWriter::WriteComment(const string &str)
{
	out << *before << "# " << str;
	Write();
}



// Write a token, given as a character string.
void DataWriter::WriteToken(const char *a)
{
	WriteToken(string(a));
}



// Write a token, given as a string object.
void DataWriter::WriteToken(const string &a)
{
	out << *before;
	out << Quote(a);

	// The next token written will not be the first one on this line, so it only
	// needs to have a single space before it.
	before = &space;
}



string DataWriter::Quote(const string &a)
{
	// Figure out what kind of quotation marks need to be used for this string.
	bool hasSpace = any_of(a.begin(), a.end(), [](unsigned char c) { return isspace(c); });
	bool hasQuote = any_of(a.begin(), a.end(), [](char c) { return (c == '"'); });
	bool hasBacktick = any_of(a.begin(), a.end(), [](char c) { return (c == '`'); });
	// If the token is an empty string, it needs to be wrapped in quotes as if it had a space.
	hasSpace |= a.empty();

	if(hasQuote)
		return '`' + a + '`';
	else if(hasSpace || hasBacktick)
		return '"' + a + '"';
	else
		return a;
}
