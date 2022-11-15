/* GlobalConditions.h
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

#ifndef GLOBALCONDITIONS_H_
#define GLOBALCONDITIONS_H_

#include <string>

class DataNode;



class GlobalConditions {
public:
	static void Load();
	static void Save();

	static bool HasSetting(const std::string settingName);
	static void SetSetting(const std::string settingName, bool active = true);
};



#endif /* GlobalSettings_h */
