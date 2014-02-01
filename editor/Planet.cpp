/* Planet.cpp
Michael Zahniser, 30 Dec 2013

Function definitions for the Planet class.
*/

#include "Planet.h"

#include <cassert>

using namespace std;



void Planet::Load(const DataFile::Node &node)
{
	assert(node.Size() >= 2 && node.Token(0) == "planet");
	*this = Planet();
	name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "landscape" && child.Size() >= 2)
			landscape = child.Token(1);
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			// Coalesce any number of "description" tags into one entry, with
			// each one being a separate paragraph.
			description += child.Token(1);
			description += '\n';
		}
		else
			unrecognized.push_back(child);
	}
}



void Planet::Write(ostream &out) const
{
	out << "planet \"" << name << "\"\n";
	if(!landscape.empty())
		out << "\tlandscape \"" << landscape << "\"\n";
	
	if(!description.empty())
	{
		size_t start = 0;
		// The description should always end in '\n'.
		while(start < description.length())
		{
			size_t end = description.find('\n', start);
			if(end == string::npos)
				end = description.length();
			out << "\tdescription \"" << description.substr(start, end - start) << "\"\n";
			start = end + 1;
		}
	}
	
	// Now, write everything we did not recognize.
	for(const DataFile::Node &node : unrecognized)
		node.Write(out);
	
	// Add an extra line break just for readability.
	out << "\n";
}
