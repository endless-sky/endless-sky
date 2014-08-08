/* DataWriter.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DataWriter.h"

#include "DataNode.h"

using namespace std;



const std::string DataWriter::space = " ";



DataWriter::DataWriter(const string &path)
	: before(&indent)
{
	out.open(path);
	out.precision(8);
}



void DataWriter::Write(const DataNode &node)
{
	for(int i = 0; i < node.Size(); ++i)
		WriteToken(node.Token(i).c_str());
	Write();
	
	if(node.begin() != node.end())
	{
		BeginChild();
		for(const DataNode &child : node)
			Write(child);
		EndChild();
	}
}



void DataWriter::Write()
{
	out << '\n';
	before = &indent;
}



void DataWriter::BeginChild()
{
	indent += '\t';
}



void DataWriter::EndChild()
{
	indent.erase(indent.length() - 1);
}



void DataWriter::WriteComment(const string &str)
{
	out << indent << "# " << str << '\n';
}



void DataWriter::WriteToken(const char *a)
{
	bool hasSpace = false;
	bool hasQuote = false;
	for(const char *it = a; *it; ++it)
	{
		hasSpace |= (*it <= ' ');
		hasQuote |= (*it == '"');
	}
	
	out << *before;
	if(hasSpace && hasQuote)
		out << '`' << a << '`';
	else if(hasSpace)
		out << '"' << a << '"';
	else
		out << a;
	before = &space;
}
