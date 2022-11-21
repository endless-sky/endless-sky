/* OutfitFilter.cpp
Copyright (c) 2022 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "OutfitFilter.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Ship.h"



OutfitFilter::OutfitFilter(const DataNode &node)
{
	Load(node);
}



void OutfitFilter::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "not" && child.Size() >= 3)
		{
			if(child.Token(1) == "attribute")
				for(unsigned int it = 2; it < child.Tokens().size(); ++it)
					notOutfitAttributes.emplace_back(child.Token(it));
			else if(child.Token(1) == "tag")
				for(unsigned int it = 2; it < child.Tokens().size(); ++it)
					notOutfitTags.emplace_back(child.Token(it));
		}
		else if(child.Token(1) == "attribute" && child.Size() >= 2)
			for(unsigned int it = 1; it < child.Tokens().size(); ++it)
				outfitAttributes.emplace_back(child.Token(it));
		else if(child.Token(1) == "tag" && child.Size() >= 2)
			for(unsigned int it = 1; it < child.Tokens().size(); ++it)
				outfitTags.emplace_back(child.Token(it));
	}

	isValid = true;
}



void OutfitFilter::Save(DataWriter &out)
{
	out.BeginChild();
	{
		if(!outfitTags.empty())
		{
			out.WriteToken("tag");
			for(const auto &tag : outfitTags)
				out.WriteToken(tag);
			out.Write();
		}
		if(!notOutfitTags.empty())
		{
			out.WriteToken("not");
			out.WriteToken("tag");
			for(const auto &tag : outfitTags)
				out.WriteToken(tag);
			out.Write();
		}
		if(!outfitAttributes.empty())
		{
			out.WriteToken("attribute");
			for(const auto &attrib : outfitAttributes)
				out.WriteToken(attrib);
			out.Write();
		}
		if(!notOutfitTags.empty())
		{
			out.WriteToken("not");
			out.WriteToken("attribute");
			for(const auto &attrib : outfitAttributes)
				out.WriteToken(attrib);
			out.Write();
		}
	}
	out.EndChild();
}



bool OutfitFilter::IsValid() const
{
	return isValid;
}



std::vector<const Outfit *> OutfitFilter::MatchingOutfits(const Ship *ship) const
{
	std::vector<const Outfit *> matchingOutfits;
	for(const auto &outfit : ship->Outfits())
		if(Matches(outfit.first))
			matchingOutfits.emplace_back(outfit.first);

	return matchingOutfits;
}



bool OutfitFilter::Matches(const Outfit *outfit) const
{
	for(const auto &tag : outfitTags)
		if(!outfit->HasTag(tag))
			return false;

	for(const auto &tag : notOutfitTags)
		if(outfit->HasTag(tag))
			return false;

	for(const auto &attrib : outfitAttributes)
		if(!outfit->Get(attrib))
			return false;

	for(const auto &attrib : notOutfitAttributes)
		if(outfit->Get(attrib))
			return false;

	return true;
}
