/* Setting.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Setting.h"

#include "DataNode.h"

#include <algorithm>
#include <utility>

using namespace std;



Setting Setting::Boolean(const string &name, bool defaultValue)
{
	return Setting(name, {"off", "on"}, defaultValue, true);
}



Setting Setting::List(const string &name, int defaultValue, const vector<string> &options, bool alwaysOn)
{
	Setting setting = Setting(name, options, defaultValue, true);
	if(alwaysOn)
		setting.SetIsOnFunc([](int index) -> bool { return true; });
	return setting;
}



Setting Setting::Unique(const string &name)
{
	return Setting(name, {}, 0, false);
}



Setting::Setting(const string &displayName, const vector<string> &options, int defaultIndex, bool save)
	: displayName(displayName), options(options), index(defaultIndex), save(save)
{
}



void Setting::SetDisplayFunc(std::function<std::string(int)> displayFunc)
{
	this->displayFunc = std::move(displayFunc);
}



void Setting::SetIsOnFunc(function<bool(int)> isOnFunc)
{
	this->isOnFunc = std::move(isOnFunc);
}



void Setting::SetToggleFunc(function<int(int)> toggleFunc)
{
	this->toggleFunc = std::move(toggleFunc);
}



void Setting::SetOnToggleFunc(function<void(int)> onToggleFunc)
{
	this->onToggleFunc = std::move(onToggleFunc);
}



const string &Setting::DisplayName() const
{
	return displayName;
}



string Setting::DisplayValue() const
{
	if(displayFunc)
		return displayFunc(index);
	return options.empty() ? "" : options[index];
}



void Setting::SetIndex(int index)
{
	this->index = clamp<int>(index, 0, options.size() - 1);
}



int Setting::Index() const
{
	return index;
}



bool Setting::IsOn() const
{
	return isOnFunc ? isOnFunc(index) : index;
}



int Setting::Toggle()
{
	if(toggleFunc)
		index = toggleFunc(index);
	else if(!options.empty())
		index = (index + 1) % options.size();
	if(onToggleFunc)
		onToggleFunc(index);
	return index;
}



bool Setting::Save() const
{
	return save;
}
