/* Government.cpp
Michael Zahniser, 31 Jan 2014

Function definitions for the Government class.
*/

#include "Government.h"

using namespace std;



// Default constructor.
Government::Government()
	: name("Unknown Government"), color(0)
{
}



// Load a government's definition from a file.
void Government::Load(const DataFile::Node &node, const Set<Government> &others)
{
	if(node.Size() >= 2)
		name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "color" && child.Size() >= 2)
			color = child.Value(1);
		else if(child.Token(0) == "ally" && child.Size() >= 2)
			allies.insert(others.Get(child.Token(1)));
		else if(child.Token(0) == "enemy" && child.Size() >= 2)
			enemies.insert(others.Get(child.Token(1)));
	}
}



// Get the name of this government.
const string &Government::GetName() const
{
	return name;
}



// Get the color swizzle to use for ships of this government.
int Government::GetColor() const
{
	return color;
}



// Check whether ships of this government will come to the aid of ships of
// the given government that are under attack.
bool Government::IsAlly(const Government *other) const
{
	return (allies.find(other) != allies.end());
}



// Check whether ships of this government will preemptively attack ships of
// the given government.
bool Government::IsEnemy(const Government *other) const
{
	return (enemies.find(other) != enemies.end()
		|| provoked.find(other) != provoked.end());
}



// Mark that this government is, for the moment, fighting the given
// government, which is not necessarily one of its normal enemies, because
// a ship of that government attacked it or one of its allies.
void Government::Provoke(const Government *other)
{
	provoked.insert(other);
}



// Reset the record of who has provoked whom. Typically this will happen
// whenever you move to a new system.
void Government::ResetProvocation()
{
	provoked.clear();
}
