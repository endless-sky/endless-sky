/* Personality.h
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
	static const int PACIFIST = (1 << 0);
	static const int FORBEARING = (1 << 1);
	static const int TIMID = (1 << 2);
	static const int DISABLES = (1 << 3);
	static const int PLUNDERS = (1 << 4);
	static const int HEROIC = (1 << 5);
	static const int STAYING = (1 << 6);
	static const int ENTERING = (1 << 7);
	static const int NEMESIS = (1 << 8);
	static const int SURVEILLANCE = (1 << 9);
	static const int UNINTERESTED = (1 << 10);
	static const int WAITING = (1 << 11);
	static const int DERELICT = (1 << 12);
	static const int FLEEING = (1 << 13);
	static const int ESCORT = (1 << 14);
	static const int FRUGAL = (1 << 15);
	static const int COWARD = (1 << 16);
	static const int VINDICTIVE = (1 << 17);
	
	static const map<string, int> TOKEN = {
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
		{"vindictive", VINDICTIVE}
	};
	
	double DEFAULT_CONFUSION = 10.;
}



// Default settings for player's ships.
Personality::Personality()
	: flags(DISABLES), confusionMultiplier(DEFAULT_CONFUSION), confusionAngle(Angle::Random())
{
}



void Personality::Load(const DataNode &node)
{
	flags = 0;
	for(int i = 1; i < node.Size(); ++i)
		Parse(node.Token(i));
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "confusion" && child.Size() >= 2)
			confusionMultiplier = child.Value(1);
		else
		{
			for(int i = 0; i < child.Size(); ++i)
				Parse(child.Token(i));
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



bool Personality::Disables() const
{
	return flags & DISABLES;
}



bool Personality::Plunders() const
{
	return flags & PLUNDERS;
}



bool Personality::IsHeroic() const
{
	return flags & HEROIC;
}



bool Personality::IsStaying() const
{
	return flags & STAYING;
}



bool Personality::IsEntering() const
{
	return flags & ENTERING;
}



bool Personality::IsNemesis() const
{
	return flags & NEMESIS;
}



bool Personality::IsSurveillance() const
{
	return flags & SURVEILLANCE;
}



bool Personality::IsUninterested() const
{
	return flags & UNINTERESTED;
}



bool Personality::IsWaiting() const
{
	return flags & WAITING;
}



bool Personality::IsDerelict() const
{
	return flags & DERELICT;
}



bool Personality::IsFleeing() const
{
	return flags & FLEEING;
}



bool Personality::IsEscort() const
{
	return flags & ESCORT;
}



bool Personality::IsFrugal() const
{
	return flags & FRUGAL;
}



bool Personality::IsCoward() const
{
	return flags & COWARD;
}



bool Personality::IsVindictive() const
{
	return flags & VINDICTIVE;
}



const Point &Personality::Confusion() const
{
	return confusion;
}



void Personality::UpdateConfusion()
{
	confusionAngle += Angle::Random(10) - Angle::Random(10);
	confusion += (.1 * confusionMultiplier) * confusionAngle.Unit();
	confusion *= .9;
}



Personality Personality::Defender()
{
	Personality defender;
	defender.flags = STAYING | NEMESIS | HEROIC;
	return defender;
}



void Personality::Parse(const string &token)
{
	auto it = TOKEN.find(token);
	if(it != TOKEN.end())
		flags |= it->second;
}
