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



// Constructor, specifying the file to save.
DataWriter::DataWriter(const string &path)
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



// Gets the current contents of the DataWriter.
string DataWriter::GetText()
{
	return out.str();
}



// Save the contents to a file.
void DataWriter::SaveToPath(const string &filepath)
{
	Files::Write(filepath, out.str());
}



// Writes the contents of the string without any escaping, quoting or
// any other kind of modification.
DataWriter &DataWriter::WriteRaw(const string &c)
{
	out << c;
	return *this;
}



// Writes the string that separates two tokens. If no tokens are present on the current line,
// it writes the indentation string instead.
DataWriter &DataWriter::WriteSeparator()
{
	if(before)
	{
		out << *before;
		before = nullptr;
	}
	return *this;
}



// Write a DataNode with all its children.
DataWriter &DataWriter::Write(const DataNode &node)
{
	WriteTokens(node);
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
	return *this;
}



// Begin a new line of the file.
DataWriter &DataWriter::Write()
{
	out << '\n';
	before = &indent;
	return *this;
}



// Increase the indentation level.
DataWriter &DataWriter::BeginChild()
{
	indent += indentString;
	return *this;
}



// Decrease the indentation level.
DataWriter &DataWriter::EndChild()
{
	indent.erase(indent.length() - indentString.length());
	return *this;
}



// Write a comment line, at the current indentation level.
DataWriter &DataWriter::WriteComment(const string &str)
{
	WriteSeparator();
	out << "# " << str;
	Write();
	return *this;
}



// Write a token, given as a character string.
DataWriter &DataWriter::WriteToken(const char *a)
{
	WriteToken(string(a));
	return *this;
}



// Write a token, given as a string object.
DataWriter &DataWriter::WriteToken(const string &a)
{
	// Figure out what kind of quotation marks need to be used for this string.
	bool hasSpace = any_of(a.begin(), a.end(), [](char c) { return isspace(c); });
	bool hasQuote = any_of(a.begin(), a.end(), [](char c) { return (c == '"'); });
	// Write the token, enclosed in quotes if necessary.
	WriteSeparator();
	if(hasQuote)
		out << '`' << a << '`';
	else if(hasSpace)
		out << '"' << a << '"';
	else
		out << a;

	// The next token written will not be the first one on this line, so it only
	// needs to have a single space before it.
	before = &separator;
	return *this;
}



// Write the tokens of this DataNode without writing its children.
DataWriter &DataWriter::WriteTokens(const DataNode &node)
{
	for(int i = 0; i < node.Size(); ++i)
		WriteToken(node.Token(i).c_str());
	return *this;
}


// Changes the separator used between tokens. The default is a single space.
DataWriter &DataWriter::SetSeparator(const string &sep)
{
	separator = sep;
	return *this;
}



// Changes the indentation used before child tokens. The default is a single tabulator.
DataWriter &DataWriter::SetIndentation(const std::string &indent)
{
	indentString = indent;
	return *this;
}
