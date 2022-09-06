/* InterfaceObjects.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "InterfaceObjects.h"

#include "Command.h"
#include "DataNode.h"
#include "Information.h"

using namespace std;



bool InterfaceObjects::LoadNode(const DataNode &node)
{
	const string &key = node.Token(0);
	if(key == "color" && node.Size() >= 6)
		colors.Get(node.Token(1))->Load(
			node.Value(2), node.Value(3), node.Value(4), node.Value(5));
	else if(key == "interface" && node.Size() >= 2)
	{
		interfaces.Get(node.Token(1))->Load(node);

		// If we modified the "menu background" interface, then
		// we also update our cache of it.
		if(node.Token(1) == "menu background")
		{
			lock_guard<mutex> lock(menuBackgroundMutex);
			menuBackgroundCache.Load(node);
		}
	}
	else if((key == "tip" || key == "help") && node.Size() >= 2)
	{
		string &text = (key == "tip" ? tooltips : helpMessages)[node.Token(1)];
		text.clear();
		for(const DataNode &child : node)
		{
			if(!text.empty())
			{
				text += '\n';
				if(child.Token(0)[0] != '\t')
					text += '\t';
			}
			text += child.Token(0);
		}
	}
	else
		return false;

	return true;
}



const Set<Color> &InterfaceObjects::Colors()
{
	return colors;
}



const Set<Interface> &InterfaceObjects::Interfaces()
{
	return interfaces;
}



const string &InterfaceObjects::Tooltip(const string &label)
{
	static const string EMPTY;
	auto it = tooltips.find(label);
	// Special case: the "cost" and "sells for" labels include the percentage of
	// the full price, so they will not match exactly.
	if(it == tooltips.end() && !label.compare(0, 4, "cost"))
		it = tooltips.find("cost:");
	if(it == tooltips.end() && !label.compare(0, 9, "sells for"))
		it = tooltips.find("sells for:");
	return (it == tooltips.end() ? EMPTY : it->second);
}



string InterfaceObjects::HelpMessage(const string &name)
{
	static const string EMPTY;
	auto it = helpMessages.find(name);
	return Command::ReplaceNamesWithKeys(it == helpMessages.end() ? EMPTY : it->second);
}



const map<string, string> &InterfaceObjects::HelpTemplates()
{
	return helpMessages;
}




void InterfaceObjects::DrawMenuBackground(Panel *panel) const
{
	lock_guard<mutex> lock(menuBackgroundMutex);
	menuBackgroundCache.Draw(Information(), panel);
}
