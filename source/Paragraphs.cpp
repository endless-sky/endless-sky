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

#include "Paragraphs.h"

#include "ConditionSet.h"
#include "ConditionsStore.h"
#include "DataNode.h"

using namespace std;



void Paragraphs::Load(const DataNode &node)
{
	for(const DataNode &child : node)
		if(child.Size() == 2 && child.Token(0) == "to" && child.Token(1) == "display")
		{
			text.emplace_back(child, node.Token(1) + "\n");
			return;
		}
	text.emplace_back(ConditionSet(), node.Token(1) + "\n");
}



void Paragraphs::Clear()
{
	text.clear();
}



bool Paragraphs::IsEmpty() const
{
	return text.empty();
}



bool Paragraphs::IsEmptyFor(const ConditionsStore &vars) const
{
	for(const auto &varsText : text)
		if(!varsText.second.empty() && (varsText.first.IsEmpty() || varsText.first.Test(vars)))
			return false;
	return true;
}



string Paragraphs::ToString(const ConditionsStore &vars) const
{
	string result;
	for(const auto &varsText : text)
		if(!varsText.second.empty() && (varsText.first.IsEmpty() || varsText.first.Test(vars)))
			result += varsText.second;
	return result;
}



Paragraphs::ConstIterator Paragraphs::begin() const
{
	return text.begin();
}



Paragraphs::ConstIterator Paragraphs::end() const
{
	return text.end();
}
