/* CustomLink.cpp
Copyright (c) 2014 by Michael Zahniser
Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.
Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CustomLink.h"

#include "DataNode.h"
#include "GameData.h"
#include "Outfit.h"
#include "System.h"

#include <string>
#include <iostream>

using namespace std;

class DataNode;

// For sorting in a set<>
bool CustomLink::operator<(const CustomLink& link) const 
{
	if (link.system == system)
		return linkType < link.linkType;
	else
		return system < link.system;
}



void CustomLink::Load(const string &system, const string &linkType)
{
	this->system = GameData::Systems().Get(system);
	this->linkType = GameData::CustomLinkTypes().Get(linkType);
}



// Shorthand for CustomLinkType::CanTravel
bool CustomLink::CanTravel(const Ship &ship) const
{
    return linkType->CanTravel(ship);
}



const CustomLinkType* CustomLink::LinkType() const
{
	return linkType;
}



const System* CustomLink::GetSystem() const
{
	return system;
}



bool CustomLink::CanTravel(const Outfit &outfit) const
{
    return linkType->CanTravel(outfit);
}


const Color &CustomLinkType::GetColorFor(const Ship &ship, bool isClose) const
{
	bool canTravel = CanTravel(ship);

	std::cout << isClose << canTravel << std::endl;
	std::cout << closeColor.Get()[3] << std::endl;

	if (canTravel && isClose)
		return closeColor;
	if (canTravel && !isClose)
		return farColor;
	if (!canTravel && isClose)
		return unusableCloseColor;
	
	return unusableFarColor;
}


// Load this link
void CustomLinkType::Load(const DataNode &node) 
{
    const string color_names[] = {"color", "far color", "unusable color", "unusable far color"};
    Color* colorPointers[] = {&closeColor, &farColor, &unusableCloseColor, &unusableFarColor};
    bool wasDefined[] = {false, false, false, false};
    
    closeColor = Color(0.0, 0.0);
    for(const DataNode &child : node)
    {
        if(child.Size() < 2)
            child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
        else if(child.Token(0) == "requires") 
            requirement = child.Token(1);
        else
        {
	        bool found_color = false;
	        for (size_t i = 0; i < (sizeof(color_names) / sizeof(string)); ++i)
	        {
	            bool is_far = i % 2;
	            bool is_unusable = i > 1;
	            if(child.Token(0) == color_names[i])
	            {
	                if(child.Size() == 4)
	                {
	                    if(is_unusable) {
	                        child.PrintTrace("Warning: Custom link color when unusable \"" + color_names[i] + "\" did not specify an alpha value, so 0.0 (transparent) was assumed.");
	                        *colorPointers[i] = Color(child.Value(1), child.Value(2), child.Value(3), 0);
	                    }
	                    else
	                        *colorPointers[i] = Color(child.Value(1), child.Value(2), child.Value(3), 0.5 ? is_far : 1);
	                }
	                if(child.Size() == 5)
	                    *colorPointers[i] = Color(child.Value(1), child.Value(2), child.Value(3), child.Value(4));
	                wasDefined[i] = true;
	                found_color = true;
	                break;
	            }
	        }
	        if(!found_color)
	            child.PrintTrace("Skipped unrecognized key: " + child.Token(0) + ".");
        }
    }

    // Adjust everything's colors with the alpha value
    for (size_t i = 0; i < (sizeof(colorPointers) / sizeof(Color*)); ++i) 
        *colorPointers[i] = colorPointers[i]->Transparent(colorPointers[i]->Get()[3]);



    if(!wasDefined[0]) // Close color 
        node.PrintTrace("Warning: The attribute \"color\" was not specified for this custom link, gray was assumed.");
    if(!wasDefined[1]) // Far color
        farColor = closeColor.Transparent(0.5); 
    if(!wasDefined[3]) // Unusable far color
        unusableFarColor = unusableCloseColor.Transparent(0.5);
}



// Checks if a certain ship can travel through this link type
bool CustomLinkType::CanTravel(const Ship &ship) const
{
	std::cout << requirement << " " << ship.Attributes().Get(requirement) << std::endl;
    return ship.Attributes().Get(requirement);
}



// Same as above, but overloaded with outfits
bool CustomLinkType::CanTravel(const Outfit &outfit) const
{
    return outfit.Get(requirement);
}
