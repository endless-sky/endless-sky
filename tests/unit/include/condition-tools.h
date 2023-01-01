/* condition-tools.h
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

#ifndef ES_TEST_HELPER_CONDITION_TOOLS_H_
#define ES_TEST_HELPER_CONDITION_TOOLS_H_

#include "../../../source/Condition.h"
#include "../../../source/ConditionsStore.h"

#include <memory>
#include <string>
#include <vector>

class ConditionMaker {
	struct Provider
	{
		Provider(const std::string &name, const ConditionsStore::ValueType &value);
		bool has(const std::string &name);
		bool set(const std::string &name, const ConditionsStore::ValueType &value);
		ConditionsStore::ValueType get(const std::string &name);

		std::string name;
		ConditionsStore::ValueType value;
	};
public:
	ConditionMaker();
	ConditionMaker(const std::vector<std::pair<std::string,int64_t>> &from);

	void AddProviderNamed(const std::string &providerName, const std::string &givenName,
		const ConditionsStore::ValueType &initialValue);
	std::shared_ptr<ConditionsStore> Store();
	Condition<ConditionsStore::ValueType> AsCondition(const std::string &key);
	ConditionsStore::ValueType Get(const std::string &key);
	ConditionsStore::ValueType Set(const std::string &key, const ConditionsStore::ValueType &value);
private:
	std::shared_ptr<ConditionsStore> store;
};

#endif
