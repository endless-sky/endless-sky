/* Storyline.cpp
Copyright (c) 2024 by Timothy Collett

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Storyline.h"

#include "DataNode.h"
#include "GameData.h"
#include "ExclusiveItem.h"
#include "Mission.h"

using namespace std;



Storyline::Storyline(const DataNode &node)
{
	Load(node);
}



void Storyline::Load(const DataNode &node)
{
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "color")
		{
			if(child.Size() >= 4)
				color = ExclusiveItem<Color>(Color(child.Value(1),
						child.Value(2), child.Value(3)));
			else if(child.Size() >= 2)
				color = ExclusiveItem<Color>(GameData::Colors().Get(child.Token(1)));
		}
		else if(child.Token(0) == "main")
			main = true;
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



// Get the storyline's name.
string Storyline::Name() const {
	return name;
}



// Get the storyline's associated color.
const Color &Storyline::GetColor() const {
	return *color;
}



// Get whether the storyline is part of the main plot.
bool Storyline::IsMain() const {
	return main;
}



void Storyline::AddMission(Mission *mission) {
	missions.push_back(mission);
}
