/* datanode-factory.cpp
Copyright (c) 2022 by an anonymous author

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "condition-tools.h"



ConditionMaker::ConditionMaker()
{
}



ConditionMaker::ConditionMaker(const std::vector<std::pair<std::string,int64_t>> &from)
{
	for(auto &f : from)
		store.Set(f.first, f.second);
}



ConditionMaker::~ConditionMaker()
{
}



const ConditionsStore &ConditionMaker::Store()
{
	return store;
}



Condition<ConditionsStore::ValueType> ConditionMaker::AsCondition(const std::string &key)
{
	return Condition<ConditionsStore::ValueType>(
		static_cast<ConditionsStore::ValueType>(store.Get(key)),key);
}



ConditionsStore::ValueType ConditionMaker::Get(const std::string &key)
{
	return store.Get(key);
}



ConditionsStore::ValueType ConditionMaker::Set(const std::string &key, const ConditionsStore::ValueType &value)
{
	return store.Set(key,value);
}
