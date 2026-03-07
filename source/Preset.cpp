/* Preset.cpp
Copyright (c) 2026 by Noelle Devonshire

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Preset.h"

#include <utility>

#include "DataFile.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameData.h"
#include "Outfit.h"

using namespace std;



Preset::Preset(string name, string shipModel, map<const Outfit*, int>& outfits)
	: name(std::move(name)),
	shipModel(std::move(shipModel)),
	outfits(std::move(outfits))
{
}



Preset *Preset::Load(const filesystem::path &path)
{
	const DataFile file(path);
	map<const Outfit*, int> outfits;
	string shipModel;
	bool isEmpty = false;
	for(const DataNode &node : file)
	{
		if(const string &key = node.Token(0); key == "ship model")
		{
			shipModel = node.Token(1);
		}
		else if(key == "outfits")
		{
			for(const DataNode &child : node)
			{
				if(child.Token(0) == "Empty")
				{
					isEmpty = true;
					break;
				}
				// Make sure the outfit actually exists in the data.
				if(const Outfit *outfit = GameData::Outfits().Get(child.Token(0)))
				{
					// TODO: test outfit... legality? obtainability? whether player can interact?
					int amount;
					try {
						if(child.Size() >= 2)
							amount = stoi(child.Token(1));
						else
							// If no amount specified, assume 1.
							amount = 1;
					}
					catch(...)
					{
						// Non-number and other erroneous values can probably just default to 1.
						amount = 1;
					}
					// Discard non-positive values. Should this default to 1?
					if(amount > 0)
						outfits[outfit] = amount;
				}
			}
		}
	}

	// If the named ship model exists and there's a nonzero number of outfits,
	// or the preset is marked as empty, we have a valid preset.
	if(GameData::Ships().Get(shipModel) && (!outfits.empty() || isEmpty))
		return new Preset(path.stem().string(), shipModel, outfits);
	return nullptr;
}



bool Preset::Exists(const std::string &name)
{
	const filesystem::path path = GetFilepath(name);
	return Files::Exists(path);
}



string Preset::ShipModel() const
{
	return shipModel;
}



map<const Outfit*, int> Preset::Outfits()
{
	return outfits;
}



string Preset::Name() const
{
	return name;
}



bool Preset::Save() const
{
	const filesystem::path path = GetFilepath(name);
	auto *out = new DataWriter(path);

	out->Write("ship model", shipModel);
	out->Write("outfits");
	out->BeginChild();
	{
		// Allow explicit creation of empty outfit files.
		if(outfits.empty())
			out->Write("empty");
		else {
			using OutfitElement = pair<const Outfit* const, int>;
			WriteSorted(outfits,
				[](const OutfitElement *lhs, const OutfitElement *rhs)
					{ return lhs->first->TrueName() < rhs->first->TrueName(); },
				[&out](const OutfitElement &it)
				{
					if(it.second == 1)
						out->Write(it.first->TrueName());
					else
						out->Write(it.first->TrueName(), it.second);
				});
		}
	}
	out->EndChild();

	delete out;

	return Files::Exists(path);
}



bool Preset::Delete() const
{
	const filesystem::path path = GetFilepath(name);
	Files::Delete(path);
	return !Files::Exists(path);
}



filesystem::path Preset::GetFilepath(const string& fileName)
{
	return Files::Presets() / (fileName + ".txt");
}
