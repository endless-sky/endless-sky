/* Confusion.cpp
Copyright (c) 2026 by Anarchist2

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Confusion.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "pi.h"
#include "Random.h"

#include <cmath>

using namespace std;



Confusion::Confusion(const DataNode &node)
{
	Load(node);
}



void Confusion::Load(const DataNode &node)
{
	if(node.Size() >= 2)
	{
		if(node.IsNumber(1))
		{
			confusionMultiplier = max(0., node.Value(1));
			return;
		}
		name = node.Token(1);
	}

	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(child.Size() < 2)
			child.PrintTrace("Expected key to have a value:");
		else if(key == "max confusion")
			confusionMultiplier = max(0., child.Value(1));
		else if(key == "period")
			period = max(1., child.Value(1));
		else if(key == "focus multiplier")
			focusMultiplier = max(0., child.Value(1));
		else if(key == "gain focus time")
			gainFocusTime = max(1., child.Value(1));
		else if(key == "lose focus time")
			loseFocusTime = max(1., child.Value(1));
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Confusion::Save(DataWriter &out) const
{
	out.Write("confusion");
	out.BeginChild();
	{
		out.Write("max confusion", confusionMultiplier);
		out.Write("period", period);
		out.Write("focus multiplier", focusMultiplier);
		out.Write("gain focus time", gainFocusTime);
		out.Write("lose focus time", loseFocusTime);
	}
	out.EndChild();
}



const string &Confusion::Name() const
{
	return name;
}



void Confusion::RandomizePeriod()
{
	tick = Random::Int(period);
}



double Confusion::CurrentConfusion() const
{
	return confusion;
}



void Confusion::UpdateConfusion(bool isFocusing)
{
	if(!confusionMultiplier)
		return;
	tick++;

	// If you're focusing, aiming accuracy should slowly improve.
	// Gain and lose focus times are stored as the number of ticks to reach and lose the maximum aiming bonus,
	// so use their inverse to determine the amount of accuracy to gain or lose each tick.
	double focus = 1.;
	if(focusMultiplier != 1.)
	{
		if(isFocusing)
			focusPercentage += 1. / gainFocusTime;
		else
			focusPercentage -= 1. / loseFocusTime;
		focusPercentage = min(1., max(0., focusPercentage));
		focus = 1. - (1. - focusMultiplier) * focusPercentage;
	}

	confusion = confusionMultiplier * focus * cos(tick * PI * 2 / period);
}
