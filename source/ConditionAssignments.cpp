/* ConditionAssignments.cpp
Copyright (c) 2024 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ConditionAssignments.h"

#include "ConditionsStore.h"

using namespace std;



// Construct and Load() at the same time.
ConditionAssignments::ConditionAssignments(const DataNode &node)
{
	Load(node);
}



// Load a set of conditions from the children of this node.
void ConditionAssignments::Load(const DataNode &node)
{
	setToEvaluate.Load(node);
}



// Save a set of conditions.
void ConditionAssignments::Save(DataWriter &out) const
{
	setToEvaluate.Save(out);
}



// Check if there are any entries in this set.
bool ConditionAssignments::IsEmpty() const
{
	return setToEvaluate.IsEmpty();
}



// Modify the given set of conditions.
void ConditionAssignments::Apply(ConditionsStore &conditions) const
{
	setToEvaluate.Apply(conditions);
}



set<string> ConditionAssignments::RelevantConditions() const
{
	return setToEvaluate.RelevantConditions();
}



void ConditionAssignments::AddSetCondition(const std::string &name)
{
	Add("set", name);
}



void ConditionAssignments::Add(const DataNode &node)
{
	setToEvaluate.Add(node);
}



bool ConditionAssignments::Add(const string &firstToken, const string &secondToken)
{
	return setToEvaluate.Add(firstToken, secondToken);
}



bool ConditionAssignments::Add(const string &name, const string &op, const string &value)
{
	return setToEvaluate.Add(name, op, value);
}



bool ConditionAssignments::Add(const vector<string> &lhs, const string &op, const vector<string> &rhs)
{
	return setToEvaluate.Add(lhs, op, rhs);
}
