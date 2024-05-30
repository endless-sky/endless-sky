/* Paragraphs.cpp
Copyright (c) 2024 by an anonymous author

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ConditionSet.h"
#include "ConditionsStore.h"
#include "Paragraphs.h"

using namespace std;

void Paragraphs::Load(const DataNode &node)
{
	for(const DataNode &child : node)
		if(child.Size() == 2 && child.Token(0) == "to" && child.Token(1) == "display")
		{
			paragraphs.emplace_back(node.Token(1) + "\n", make_shared<ConditionSet>(child));
			return;
		}
	paragraphs.emplace_back(node.Token(1) + "\n", shared_ptr<ConditionSet>());
}



void Paragraphs::Clear()
{
	paragraphs.clear();
}



bool Paragraphs::IsEmpty() const
{
	return paragraphs.empty();
}



bool Paragraphs::IsEmptyFor(const ConditionsStore &vars) const
{
	for(auto &varsText : paragraphs)
		if(!varsText.second.empty() && (varsText.first.IsEmpty() || varsText.first.Test(vars)))
			return false;
	return true;
}



string Paragraphs::ToString(const ConditionsStore &vars) const
{
	string result;
	for(auto &varsText : paragraphs)
		if(!varsText.second.empty() && (varsText.first.IsEmpty() || varsText.first.Test(vars)))
			result += varsText.second;
	return result;
}



Paragraphs Paragraphs::operator + (const Paragraphs &other) const
{
	Paragraphs result(*this);
	result.paragraphs.insert(result.paragraphs.end(), other.paragraphs.begin(), other.paragraphs.end());
	return result;
}
