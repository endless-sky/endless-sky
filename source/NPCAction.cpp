/* NPCAction.cpp
Copyright (c) 2023 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "NPCAction.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "PlayerInfo.h"
#include "System.h"
#include "UI.h"

using namespace std;



// Construct and Load() at the same time.
NPCAction::NPCAction(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	Load(node, playerConditions, visitedSystems, visitedPlanets);
}



void NPCAction::Load(const DataNode &node, const ConditionsStore *playerConditions,
	const set<const System *> *visitedSystems, const set<const Planet *> *visitedPlanets)
{
	if(node.Size() >= 2)
		trigger = node.Token(1);

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		if(key == "triggered")
			triggered = true;
		else
			action.LoadSingle(child, playerConditions, visitedSystems, visitedPlanets);
	}
}



// Note: the Save() function can assume this is an instantiated action, not
// a template, so it only has to save a subset of the data.
void NPCAction::Save(DataWriter &out) const
{
	out.Write("on", trigger);
	out.BeginChild();
	{
		if(triggered)
			out.Write("triggered");

		action.SaveBody(out);
	}
	out.EndChild();
}



// Check this template or instantiated NPCAction to see if any used content
// is not fully defined (e.g. plugin removal, typos in names, etc.).
string NPCAction::Validate() const
{
	return action.Validate();
}



void NPCAction::Do(PlayerInfo &player, UI *ui, const Mission *caller, const shared_ptr<Ship> &target)
{
	// All actions are currently one-time-use. Actions that are used
	// are marked as triggered, and cannot be used again.
	if(triggered)
		return;
	triggered = true;
	action.Do(player, ui, caller, nullptr, target);
}



// Convert this validated template into a populated action.
NPCAction NPCAction::Instantiate(map<string, string> &subs, const System *origin,
	int jumps, int64_t payload) const
{
	NPCAction result;
	result.trigger = trigger;
	result.action = action.Instantiate(subs, origin, jumps, payload);
	return result;
}
