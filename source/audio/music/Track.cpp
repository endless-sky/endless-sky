/* Track.cpp
Copyright (c) 2025 by tibetiroka

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Track.h"

#include "../../DataNode.h"
#include "../../Logger.h"
#include "../Music.h"
#include "../supplier/SilenceSupplier.h"

using namespace std;

namespace {
	function<unique_ptr<AudioSupplier>(bool)> ParseLine(const string &data, double duration = -1.)
	{
		if(data == "silence")
		{
			if(duration > 0.)
				return [duration](bool loop){return std::make_unique<SilenceSupplier>(loop ? 999999. : duration);};
			else
			{
				Logger::LogError("\"silence\" source requires a positive duration");
				return [](bool){return unique_ptr<AudioSupplier>{};};
			}
		}
		else
			return [data](bool loop) {return Music::CreateSupplier(data, loop);};
	}
}



Track::Track(const std::string &name, double duration) : name(name)
{
	background.AddSource(ParseLine(name, duration));
	this->name = name;
}



void Track::Load(const DataNode &data)
{
	background.Clear();
	foreground.clear();

	if(data.Size() > 1)
		name = data.Token(1);
	else
		Logger::LogError("Tracks must have a name");
	for(const DataNode &child : data)
	{
		if(child.Token(0) == "background")
		{
			if(child.Size() < 2)
				Logger::LogError("\"background\" node must have a value");
			else
			{
				background.Clear();
				if(child.Size() > 2)
					background.AddSource(ParseLine(child.Token(1), child.Value(2)));
				else
					background.AddSource(ParseLine(child.Token(1)));
			}
		}
		else if(child.Token(0) == "foreground")
		{
			if(!child.HasChildren())
				Logger::LogError("\"foreground\" node must have children");
			else
			{
				foreground.emplace_back();
				for(const DataNode &source : child)
				{
					if(source.Size() == 1)
						foreground.back().AddSource(ParseLine(source.Token(0)));
					else
						foreground.back().AddSource(ParseLine(source.Token(0), source.Value(1)));
				}
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



const std::string &Track::Name() const
{
	return name;
}



bool Track::Valid() const
{
	return !name.empty();
}



vector<reference_wrapper<const Layer>> Track::Layers() const
{
	vector<reference_wrapper<const Layer>> layers;
	layers.reserve(foreground.size() + 1);
	layers.emplace_back(background);
	for(const Layer &layer : foreground)
		layers.emplace_back(layer);
	return layers;
}
