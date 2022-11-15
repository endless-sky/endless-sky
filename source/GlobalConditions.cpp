/* GlobalSettings.cpp
Copyright (c) 2014 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GlobalConditions.h"

#include <map>

using namespace std;

namespace {
	map<string, bool> globalConditions;
};



void GlobalConditions::Load()
{
	DataFile prefs(Files::Config() + "globalConditions.txt");
	for(const DataNode &node : prefs)
		globalConditions[node.Token(0)] = (node.Size() == 1 || node.Value(1));
}



void GlobalConditions::Save()
{
	DataWriter out(Files::Config() + "globalConditions.txt");
	for(const auto &it : globalConditions)
		out.Write(it.first, it.second);
}



bool GlobalConditions::HasSetting(const std::string settingName)
{
	return globalConditions[settingName];
}



void GlobalConditions::SetSetting(const std::string settingName, bool active)
{
	globalConditions[settingName] = active;
}
