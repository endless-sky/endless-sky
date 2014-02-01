/* Planet.cpp
Michael Zahniser, 1 Jan 2014

Function definitions for the Planet class.
*/

#include "Planet.h"

#include "SpriteSet.h"

using namespace std;



// Default constructor.
Planet::Planet()
	: landscape(nullptr)
{
}



// Load a planet's description from a file.
void Planet::Load(const DataFile::Node &node)
{
	if(node.Size() < 2)
		return;
	name = node.Token(1);
	
	for(const DataFile::Node &child : node)
	{
		if(child.Token(0) == "landscape" && child.Size() >= 2)
			landscape = SpriteSet::Get(child.Token(1));
		else if(child.Token(0) == "description" && child.Size() >= 2)
		{
			description += child.Token(1);
			description += '\n';
		}
		else if(child.Token(0) == "spaceport" && child.Size() >= 2)
		{
			spaceport += child.Token(1);
			spaceport += '\n';
		}
	}
}



// Get the name of the planet.
const string &Planet::Name() const
{
	return name;
}



// Get the planet's descriptive text.
const string &Planet::Description() const
{
	return description;
}



// Get the landscape sprite.
const Sprite *Planet::Landscape() const
{
	return landscape;
}


	
// Check whether there is a spaceport (which implies there is also trading,
// jobs, banking, and hiring).
bool Planet::HasSpaceport() const
{
	return !spaceport.empty();
}



// Get the spaceport's descriptive text.
const string &Planet::SpaceportDescription() const
{
	return spaceport;
}
