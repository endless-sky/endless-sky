/* BayType.cpp
Copyright (c) 2024 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BayType.h"

#include "CategoryList.h"
#include "CategoryTypes.h"
#include "DataNode.h"
#include "GameData.h"
#include "Logger.h"

using namespace std;



BayType::BayType(const DataNode &node)
{
	Load(node);
}



void BayType::Load(const DataNode &node)
{
	name = node.Token(1);
	for(const DataNode &child : node)
		categories.insert(child.Token(0));
}



// Confirm that all categories of ship that this bay can hold
// are carriable. If not, set isValid to false.
void BayType::FinishLoading()
{
	const auto &bayCategories = GameData::GetCategory(CategoryType::BAY);
	for(const string &category : categories)
		if(!bayCategories.Contains(category))
		{
			isValid = false;
			Logger::LogError("The bay type \"" + name + "\" contains categories of ships that are not carriable. "
				"All bays of this type on ships will be removed.");
			return;
		}
}



bool BayType::Contains(const string &category) const
{
	return categories.count(category);
}



const set<string> &BayType::Categories() const
{
	return categories;
}



bool BayType::IsLoaded() const
{
	return !name.empty();
}



bool BayType::IsValid() const
{
	return isValid;
}
