/* Personality.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Personality.h"

#include "Angle.h"
#include "DataNode.h"
#include "DataWriter.h"

#include <map>
#include <cstdint>

using namespace std;

namespace {
	const int64_t ONE = 1;
	const int64_t PACIFIST = (ONE << 0);
	const int64_t FORBEARING = (ONE << 1);
	const int64_t TIMID = (ONE << 2);
	const int64_t DISABLES = (ONE << 3);
	const int64_t PLUNDERS = (ONE << 4);
	const int64_t HUNTING = (ONE << 5);
	const int64_t STAYING = (ONE << 6);
	const int64_t ENTERING = (ONE << 7);
	const int64_t NEMESIS = (ONE << 8);
	const int64_t SURVEILLANCE = (ONE << 9);
	const int64_t UNINTERESTED = (ONE << 10);
	const int64_t WAITING = (ONE << 11);
	const int64_t DERELICT = (ONE << 12);
	const int64_t FLEEING = (ONE << 13);
	const int64_t ESCORT = (ONE << 14);
	const int64_t FRUGAL = (ONE << 15);
	const int64_t COWARD = (ONE << 16);
	const int64_t VINDICTIVE = (ONE << 17);
	const int64_t SWARMING = (ONE << 18);
	const int64_t UNCONSTRAINED = (ONE << 19);
	const int64_t MINING = (ONE << 20);
	const int64_t HARVESTS = (ONE << 21);
	const int64_t APPEASING = (ONE << 22);
	const int64_t MUTE = (ONE << 23);
	const int64_t OPPORTUNISTIC = (ONE << 24);
	const int64_t MERCIFUL = (ONE << 25);
	const int64_t TARGET = (ONE << 26);
	const int64_t MARKED = (ONE << 27);
	const int64_t LAUNCHING = (ONE << 28);
	const int64_t LINGERING = (ONE << 29);
	const int64_t DARING = (ONE << 30);
	const int64_t SECRETIVE = (ONE << 31);
	const int64_t RAMMING = (ONE << 32);

	const map<string, int64_t> TOKEN = {
		{"pacifist", PACIFIST},
		{"forbearing", FORBEARING},
		{"timid", TIMID},
		{"disables", DISABLES},
		{"plunders", PLUNDERS},
		{"hunting", HUNTING},
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
		{"merciful", MERCIFUL},
		{"target", TARGET},
		{"marked", MARKED},
		{"launching", LAUNCHING},
		{"lingering", LINGERING},
		{"daring", DARING},
		{"secretive", SECRETIVE},
		{"ramming", RAMMING}
	};

	// Tokens that combine two or more flags.
	const map<string, int64_t> COMPOSITE_TOKEN = {
		{"heroic", DARING | HUNTING}
	};

	const double DEFAULT_CONFUSION = 10.;
}



// Default settings for player's ships.
Personality::Personality() noexcept
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
				child.PrintTrace("Error: Cannot \"" + node.Token(0) + "\" a confusion value:");
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
	isDefined = true;
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



bool Personality::IsDefined() const
{
	return isDefined;
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



bool Personality::IsHunting() const
{
	return flags & HUNTING;
}



bool Personality::IsNemesis() const
{
	return flags & NEMESIS;
}



bool Personality::IsDaring() const
{
	return flags & DARING;
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



bool Personality::IsMerciful() const
{
	return flags & MERCIFUL;
}



bool Personality::IsRamming() const
{
	return flags & RAMMING;
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



bool Personality::IsLingering() const
{
	return flags & LINGERING;
}



bool Personality::IsSecretive() const
{
	return flags & SECRETIVE;
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
	defender.flags = STAYING | MARKED | HUNTING | DARING | UNCONSTRAINED | TARGET;
	return defender;
}



// Remove target and marked since the defender defeat check doesn't actually care
// about carried ships.
Personality Personality::DefenderFighter()
{
	Personality defender;
	defender.flags = STAYING | HUNTING | DARING | UNCONSTRAINED;
	return defender;
}



void Personality::Parse(const DataNode &node, int index, bool remove)
{
	const string &token = node.Token(index);

	auto it = TOKEN.find(token);
	if(it == TOKEN.end())
	{
		it = COMPOSITE_TOKEN.find(token);
		if(it == COMPOSITE_TOKEN.end())
		{
			node.PrintTrace("Warning: Skipping unrecognized personality \"" + token + "\":");
			return;
		}
	}

	if(remove)
		flags &= ~it->second;
	else
		flags |= it->second;
}
