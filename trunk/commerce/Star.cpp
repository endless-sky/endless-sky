/* Star.cpp
Michael Zahniser, 15 Oct 2013

Function definitions for the Star class for commerce.
*/

#include "Star.h"

#include <cassert>

using namespace std;



Star::Star()
	: x(0.), y(0.)
{
}



Star::Star(const DataFile::Node &node)
{
	assert(node.Size() >= 2 && node.Token(0) == "system");
	name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "pos")
		{
			assert(child.Size() >= 3);
			x = child.Value(1);
			y = child.Value(2);
		}
		else if(child.Token(0) == "link")
		{
			assert(child.Size() >= 2);
			links.push_back(child.Token(1));
		}
		else if(child.Token(0) == "trade")
		{
			assert(child.Size() >= 3);
			trade[child.Token(1)] = child.Value(2);
		}
		else
			unrecognized.push_back(child);
	}
}



const string &Star::Name() const
{
	return name;
}



double Star::X() const
{
	return x;
}



double Star::Y() const
{
	return y;
}



const vector<string> &Star::Links() const
{
	return links;
}



double Star::Trade(const string &commodity) const
{
	auto it = trade.find(commodity);
	return (it == trade.end()) ? 0. : it->second;
}



void Star::SetTrade(const string &commodity, double value)
{
	trade[commodity] = value;
}



void Star::Write(ostream &out) const
{
	out << "system \"" << name << "\"\n";
	out << "\tpos " << x << " " << y << "\n";
	for(const string &link : links)
		out << "\tlink \"" << link << "\"\n";
	for(const auto &it : trade)
		out << "\ttrade \"" << it.first << "\" " << it.second << "\n";
	
	// Now, write everything we did not recognize.
	for(const DataFile::Node &node : unrecognized)
		node.Write(out);
	
	// Add an extra line break just for readability.
	out << "\n";
}
