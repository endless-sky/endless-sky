/* Personality.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Personality.h"

#include "Angle.h"
#include "DataNode.h"
#include "DataWriter.h"

#include <map>

using namespace std;

namespace {
	const int PACIFIST = (1 << 0);
	const int FORBEARING = (1 << 1);
	const int TIMID = (1 << 2);
	const int DISABLES = (1 << 3);
	const int PLUNDERS = (1 << 4);
	const int HEROIC = (1 << 5);
	const int STAYING = (1 << 6);
	const int ENTERING = (1 << 7);
	const int NEMESIS = (1 << 8);
	const int SURVEILLANCE = (1 << 9);
	const int UNINTERESTED = (1 << 10);
	const int WAITING = (1 << 11);
	const int DERELICT = (1 << 12);
	const int FLEEING = (1 << 13);
	const int ESCORT = (1 << 14);
	const int FRUGAL = (1 << 15);
	const int COWARD = (1 << 16);
	const int VINDICTIVE = (1 << 17);
	const int SWARMING = (1 << 18);
	const int UNCONSTRAINED = (1 << 19);
	const int MINING = (1 << 20);
	const int HARVESTS = (1 << 21);
	const int APPEASING = (1 << 22);
	const int MUTE = (1 << 23);
	const int OPPORTUNISTIC = (1 << 24);
	const int TARGET = (1 << 25);
	const int MARKED = (1 << 26);
	const int LAUNCHING = (1 << 27);
	
	const map<string, int> TOKEN = {
		{"pacifist", PACIFIST},
		{"forbearing", FORBEARING},
		{"timid", TIMID},
		{"disables", DISABLES},
		{"plunders", PLUNDERS},
		{"heroic", HEROIC},
		{"staying", STAYING},
		{"entering", ENTERING},
		{"nemesis", NEMESIS},
		{"surveillance", SURVEILLANCE},
		{"uninterested", UNINTERESTED},
		{"waiting", WAITING},
		{"derelict", DERELICT},
		{"fleeing", FLEEING},
		{"escort", ESCORT},
		{"frugal", FRUGAL},
		{"coward", COWARD},
		{"vindictive", VINDICTIVE},
		{"swarming", SWARMING},
		{"unconstrained", UNCONSTRAINED},
		{"mining", MINING},
		{"harvests", HARVESTS},
		{"appeasing", APPEASING},
		{"mute", MUTE},
		{"opportunistic", OPPORTUNISTIC},
		{"target", TARGET},
		{"marked", MARKED},
		{"launching", LAUNCHING}
	};
	
	const double DEFAULT_CONFUSION = 10.;
}



// Default settings for player's ships.
Personality::Personality()
	: flags(DISABLES), confusionMultiplier(DEFAULT_CONFUSION), aimMultiplier(1.)
{
}



void Personality::Load(const DataNode &node)
{
	bool add = (node.Token(0) == "add");
	bool remove = (node.Token(0) == "remove");
	if(!(add || remove))
		flags = 0;
	for(int i = 1 + (add || remove); i < node.Size(); ++i)
		Parse(node, i, remove);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "confusion")
		{
			if(add || remove)
				child.PrintTrace("Cannot \"" + node.Token(0) + "\" a confusion value:");
			else if(child.Size() < 2)
				child.PrintTrace("Skipping \"confusion\" tag with no value specified:");
			else
				confusionMultiplier = child.Value(1);
		}
		else
		{
			for(int i = 0; i < child.Size(); ++i)
				Parse(child, i, remove);
		}
	}
}



void Personality::Save(DataWriter &out) const
{
	out.Write("personality");
	out.BeginChild();
	{
		out.Write("confusion", confusionMultiplier);
		for(const auto &it : TOKEN)
			if(flags & it.second)
				out.Write(it.first);
	}
	out.EndChild();
}



bool Personality::IsPacifist() const
{
	return flags & PACIFIST;
}



bool Personality::IsForbearing() const
{
	return flags & FORBEARING;
}



bool Personality::IsTimid() const
{
	return flags & TIMID;
}



bool Personality::IsHeroic() const
{
	return flags & HEROIC;
}



bool Personality::IsNemesis() const
{
	return flags & NEMESIS;
}



bool Personality::IsFrugal() const
{
	return flags & FRUGAL;
}



bool Personality::Disables() const
{
	return flags & DISABLES;
}



bool Personality::Plunders() const
{
	return flags & PLUNDERS;
}



bool Personality::IsVindictive() const
{
	return flags & VINDICTIVE;
}



bool Personality::IsUnconstrained() const
{
	return flags & UNCONSTRAINED;
}



bool Personality::IsCoward() const
{
	return flags & COWARD;
}



bool Personality::IsAppeasing() const
{
	return flags & APPEASING;
}



bool Personality::IsOpportunistic() const
{
	return flags & OPPORTUNISTIC;
}



bool Personality::IsStaying() const
{
	return flags & STAYING;
}



bool Personality::IsEntering() const
{
	return flags & ENTERING;
}



bool Personality::IsWaiting() const
{
	return flags & WAITING;
}



bool Personality::IsLaunching() const
{
	return flags & LAUNCHING;
}



bool Personality::IsFleeing() const
{
	return flags & FLEEING;
}



bool Personality::IsDerelict() const
{
	return flags & DERELICT;
}



bool Personality::IsUninterested() const
{
	return flags & UNINTERESTED;
}



bool Personality::IsSurveillance() const
{
	return flags & SURVEILLANCE;
}



bool Personality::IsMining() const
{
	return flags & MINING;
}



bool Personality::Harvests() const
{
	return flags & HARVESTS;
}



bool Personality::IsSwarming() const
{
	return flags & SWARMING;
}



bool Personality::IsEscort() const
{
	return flags & ESCORT;
}



bool Personality::IsTarget() const
{
	return flags & TARGET;
}



bool Personality::IsMarked() const
{
	return flags & MARKED;
}



bool Personality::IsMute() const
{
	return flags & MUTE;
}



const Point &Personality::Confusion() const
{
	return confusion;
}



void Personality::UpdateConfusion(bool isFiring)
{
	// If you're firing weapons, aiming accuracy should slowly improve until it
	// is 4 times more precise than it initially was.
	aimMultiplier = .99 * aimMultiplier + .01 * (isFiring ? .5 : 2.);
	
	// Try to correct for any error in the aim, but constantly introduce new
	// error and overcompensation so it oscillates around the origin. Apply
	// damping to the position and velocity to avoid extreme outliers, though.
	if(confusion.X() || confusion.Y())
		confusionVelocity -= .001 * confusion.Unit();
	confusionVelocity += .001 * Angle::Random().Unit();
	confusionVelocity *= .999;
	confusion += confusionVelocity * (confusionMultiplier * aimMultiplier);
	confusion *= .9999;
}



Personality Personality::Defender()
{
	Personality defender;
	defender.flags = STAYING | MARKED | HEROIC | UNCONSTRAINED | TARGET;
	return defender;
}



void Personality::Parse(const DataNode &node, int index, bool remove)
{
	const string &token = node.Token(index);
	
	auto it = TOKEN.find(token);
	if(it != TOKEN.end())
	{
		if(remove)
			flags &= ~it->second;
		else
			flags |= it->second;
	}
	else
		node.PrintTrace("Invalid personality setting: \"" + token + "\"");
}
