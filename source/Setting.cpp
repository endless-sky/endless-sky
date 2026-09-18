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
#include "UI.h"

#include <algorithm>
#include <utility>

using namespace std;



Setting Setting::Boolean(const string &name, bool defaultValue)
{
	return Setting(name, {"off", "on"}, defaultValue, true, false);
}



Setting Setting::List(const string &name, int defaultValue, const vector<string> &options, bool alwaysOn)
{
	return Setting(name, options, defaultValue, true, alwaysOn);
}



Setting Setting::Unique(const string &name)
{
	return Setting(name, {}, 0, false, true);
}



Setting::Setting(const string &displayName, const vector<string> &options, int defaultIndex, bool save,
		bool alwaysOn)
	: displayName(displayName), options(options), index(defaultIndex), alwaysOn(alwaysOn), save(save)
{
}



void Setting::SetDisplayFunc(std::function<pair<string, bool>(int)> displayFunc)
{
	this->displayFunc = std::move(displayFunc);
}



void Setting::SetToggleFunc(function<int(int)> toggleFunc)
{
	this->toggleFunc = std::move(toggleFunc);
}



void Setting::SetOnToggleFunc(function<void(UI *)> onToggleFunc)
{
	this->onToggleFunc = std::move(onToggleFunc);
}



void Setting::SetScrollFunc(std::function<void(double)> scrollFunc)
{
	this->scrollFunc = std::move(scrollFunc);
}



const string &Setting::DisplayName() const
{
	return displayName;
}



pair<string, bool> Setting::DisplayValue() const
{
	if(displayFunc)
		return displayFunc(index);
	return {options.empty() ? "" : options[index], alwaysOn ? true : index};
}



void Setting::SetIndex(int index)
{
	this->index = clamp<int>(index, 0, options.size() - 1);
}



int Setting::Index() const
{
	return index;
}



int Setting::Toggle(UI *ui)
{
	if(toggleFunc)
		index = toggleFunc(index);
	else if(!options.empty())
		index = (index + 1) % options.size();
	if(onToggleFunc)
		onToggleFunc(ui);
	return index;
}



void Setting::Scroll(double dy)
{
	if(scrollFunc)
		scrollFunc(dy);
}



bool Setting::Save() const
{
	return save;
}
