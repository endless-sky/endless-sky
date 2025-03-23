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
#include "DataNode.h"
#include "DataWriter.h"

#include <algorithm>
#include <numeric>

using namespace std;



namespace {
	const auto ASSIGN_OP_TO_TEXT = map<ConditionAssignments::AssignOp, const string> {
		{ConditionAssignments::AssignOp::ASSIGN, "="},
		{ConditionAssignments::AssignOp::ADD, "+="},
		{ConditionAssignments::AssignOp::SUB, "-="},
		{ConditionAssignments::AssignOp::MUL, "*="},
		{ConditionAssignments::AssignOp::DIV, "/="},
		{ConditionAssignments::AssignOp::LT, "<?="},
		{ConditionAssignments::AssignOp::GT, ">?="},
	};
}



// Construct and Load() at the same time.
ConditionAssignments::ConditionAssignments(const DataNode &node)
{
	Load(node);
}



// Load a set of conditions from the children of this node.
void ConditionAssignments::Load(const DataNode &node)
{
	if(!node.HasChildren())
		node.PrintTrace("Error: Loading empty set of assignments");

	// Loop through all children, and parse each line into an Assignment.
	for(const DataNode &child : node)
	{
		Add(child);
	}
}



// Save a set of conditions.
void ConditionAssignments::Save(DataWriter &out) const
{
	for(const Assignment &assignment : assignments)
	{
		AssignOp aso = assignment.assignOperator;

		auto it = ASSIGN_OP_TO_TEXT.find(aso);
		if(it != ASSIGN_OP_TO_TEXT.end())
		{
			out.WriteToken(assignment.conditionToAssignTo);
			out.WriteToken(it->second);
			assignment.expressionToEvaluate.SaveSubset(out);
			out.Write();
		}
	}
}



// Check if there are any entries in this set.
bool ConditionAssignments::IsEmpty() const
{
	return assignments.empty();
}



// Modify the given set of conditions.
void ConditionAssignments::Apply(ConditionsStore &conditions) const
{
	for(const Assignment &assignment : assignments)
	{
		auto &ce = conditions[assignment.conditionToAssignTo];
		int64_t newValue = assignment.expressionToEvaluate.Evaluate(conditions);
		switch(assignment.assignOperator)
		{
			case AssignOp::ASSIGN:
				ce = newValue;
				break;
			case AssignOp::ADD:
				ce += newValue;
				break;
			case AssignOp::SUB:
				ce -= newValue;
				break;
			case AssignOp::MUL:
				ce = static_cast<int64_t>(ce) * newValue;
				break;
			case AssignOp::DIV:
				ce = newValue ? static_cast<int64_t>(ce) / newValue : numeric_limits<int64_t>::max();
				break;
			case AssignOp::LT:
				ce = min(static_cast<int64_t>(ce), newValue);
				break;
			case AssignOp::GT:
				ce = max(static_cast<int64_t>(ce), newValue);
				break;
		}
	}
}



set<string> ConditionAssignments::RelevantConditions() const
{
	set<string> result;
	for(const Assignment &assignment : assignments)
	{
		result.insert(assignment.conditionToAssignTo);
		for(const string &cs : assignment.expressionToEvaluate.RelevantConditions())
			result.insert(cs);
	}
	return result;
}



void ConditionAssignments::AddSetCondition(const std::string &name)
{
	assignments.emplace_back(name, AssignOp::ASSIGN, ConditionSet(1));
}



void ConditionAssignments::Add(const DataNode &node)
{
	if(node.Token(0) == "set" || node.Token(0) == "clear")
	{
		if(node.Size() != 2 || !DataNode::IsConditionName(node.Token(1)))
		{
			node.PrintTrace("Parse error; " + node.Token(0) + " keyword requires a single valid condition:");
			return;
		}
		assignments.emplace_back(node.Token(1), AssignOp::ASSIGN, ConditionSet(node.Token(0) == "set" ? 1 : 0));
	}
	else if(node.Size() == 2 && (node.Token(1) == "++" || node.Token(1) == "--"))
	{
		if(!DataNode::IsConditionName(node.Token(0)))
		{
			node.PrintTrace("Parse error; " + node.Token(1) + " operator requires a single valid condition:");
			return;
		}
		assignments.emplace_back(node.Token(0), node.Token(1) == "++" ? AssignOp::ADD : AssignOp::SUB,
			ConditionSet(1));
	}
	else if(node.Size() >= 3)
	{
		// Parse the assignment operator.
		AssignOp ao = AssignOp::ASSIGN;
		const string assignOpString = node.Token(1);
		auto it = find_if(ASSIGN_OP_TO_TEXT.begin(), ASSIGN_OP_TO_TEXT.end(),
			[&assignOpString](const std::pair<AssignOp, const string> &e) {
				return e.second == assignOpString;
			});
		if(it != ASSIGN_OP_TO_TEXT.end())
			ao = it->first;
		else
		{
			node.PrintTrace("Parse error; Unsupported assignment operator (" + assignOpString + "):");
			return;
		}

		// Parse the expression.
		ConditionSet expr;
		int tokenNr = 2;
		if(!expr.ParseNode(node, tokenNr))
			return;

		// Perform optimization of the parsed expression.
		expr.Optimize(node);

		// Add the assignment when all parsing succeeded.
		assignments.emplace_back(node.Token(0), ao, expr);
	}
	else
	{
		node.PrintTrace("Error: Incomplete assignment");
		return;
	}
}



ConditionAssignments::Assignment::Assignment(string conditionToAssignTo, AssignOp assignOperator,
	ConditionSet expressionToEvaluate) : conditionToAssignTo(conditionToAssignTo), assignOperator(assignOperator),
	expressionToEvaluate(expressionToEvaluate)
{
}
