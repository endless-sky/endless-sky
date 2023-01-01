/* condition-tools.cpp
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


ConditionMaker::Provider::Provider(const std::string &name, const ConditionsStore::ValueType &value):
	name(name), value(value)
{
}


bool ConditionMaker::Provider::has(const std::string &name)
{
	return this->name == name;
}


bool ConditionMaker::Provider::set(const std::string &name, const ConditionsStore::ValueType &value)
{
	if(name != this->name)
		return false;
	this->value = value;
	return true;
}


ConditionsStore::ValueType ConditionMaker::Provider::get(const std::string &name)
{
	if(name != this->name)
		return ConditionsStore::ValueType();
	else
		return value;
}


ConditionMaker::ConditionMaker():
	store(std::make_shared<ConditionsStore>())
{
}



ConditionMaker::ConditionMaker(const std::vector<std::pair<std::string,int64_t>> &from):
	store(std::make_shared<ConditionsStore>())
{
	for(auto &f : from)
		store->Set(f.first, f.second);
}


void ConditionMaker::AddProviderNamed(const std::string &providerName, const std::string &givenName,
		const ConditionsStore::ValueType &initialValue)
{
	auto &&dayProvider = store->GetProviderNamed(providerName);
	std::shared_ptr<Provider> provider = std::make_shared<Provider>( givenName, initialValue );

	dayProvider.SetGetFunction([=](const std::string &name) { return provider->get(name); });
	dayProvider.SetHasFunction([=](const std::string &name) { return provider->has(name); });
	dayProvider.SetSetFunction([=](const std::string &name, const ConditionsStore::ValueType &newValue)
		{
			return provider->set(name, newValue);
		}
	);
}



std::shared_ptr<ConditionsStore> ConditionMaker::Store()
{
	return store;
}



Condition<ConditionsStore::ValueType> ConditionMaker::AsCondition(const std::string &key)
{
	return Condition<ConditionsStore::ValueType>(store, key);
}



ConditionsStore::ValueType ConditionMaker::Get(const std::string &key)
{
	return store->Get(key);
}



ConditionsStore::ValueType ConditionMaker::Set(const std::string &key, const ConditionsStore::ValueType &value)
{
	return store->Set(key, value);
}
