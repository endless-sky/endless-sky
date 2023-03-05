/* CaptureOdds.cpp
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

#include "CaptureClass.h"

#include "DataNode.h"
#include "DataWriter.h"

#include <algorithm>

using namespace std;


CaptureClass::CaptureClass(const DataNode &node)
{
	name = node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Size() < 2)
			child.PrintTrace("Skipping invalid capture class key with no value:");
		const string &key = child.Token(0);
		double value = child.Value(1);
		if(key == "success")
			successChance = max(0., min(1., value));
		else if(key == "self destruct")
			selfDestructChance = max(0., min(1., value));
		else if(key == "lock down")
			lockDownChance = max(0., min(1., value));
		else if(key == "failure break")
			breakOnSuccessChance = max(0., min(1., value));
		else if(key == "success break")
			breakOnFailureChance = max(0., min(1., value));
		else
			child.PrintTrace("Skipping unrecognized capture class key:");
	}
}



void CaptureClass::Save(DataWriter &out) const
{
	out.Write("capture class", name);
	out.BeginChild();
	{
		out.Write("success", successChance);
		out.Write("self destruct", selfDestructChance);
		out.Write("lock down", lockDownChance);
		out.Write("success break", breakOnSuccessChance);
		out.Write("failure break", breakOnFailureChance);
	}
	out.EndChild();
}



const string &CaptureClass::Name() const
{
	return name;
}



double CaptureClass::SuccessChance() const
{
	return successChance;
}



double CaptureClass::SelfDestructChance() const
{
	return selfDestructChance;
}



double CaptureClass::LockDownChance() const
{
	return lockDownChance;
}



double CaptureClass::BreakOnSuccessChance() const
{
	return breakOnSuccessChance;
}



double CaptureClass::BreakOnFailureChance() const
{
	return breakOnFailureChance;
}
