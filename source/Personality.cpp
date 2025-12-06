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
#include "pi.h"
#include "Random.h"

#include <cmath>
#include <map>
#include <vector>

using namespace std;

namespace {
	// Make sure the length of PersonalityTrait matches PERSONALITY_COUNT
	// or the build will fail.
	enum PersonalityTrait {
		PACIFIST,
		FORBEARING,
		TIMID,
		DISABLES,
		PLUNDERS,
		HUNTING,
		STAYING,
		ENTERING,
		NEMESIS,
		SURVEILLANCE,
		UNINTERESTED,
		WAITING,
		DERELICT,
		FLEEING,
		ESCORT,
		FRUGAL,
		COWARD,
		VINDICTIVE,
		SWARMING,
		UNCONSTRAINED,
		MINING,
		HARVESTS,
		APPEASING,
		MUTE,
		OPPORTUNISTIC,
		MERCIFUL,
		TARGET,
		MARKED,
		TRACKED,
		LAUNCHING,
		LINGERING,
		DARING,
		SECRETIVE,
		RAMMING,
		UNRESTRICTED,
		RESTRICTED,
		DECLOAKED,
		QUIET,
		GETAWAY,

		// This must be last so it can be used for bounds checking.
		LAST_ITEM_IN_PERSONALITY_TRAIT_ENUM
	};

	const map<string, PersonalityTrait> TOKEN = {
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
		{"tracked", TRACKED},
		{"launching", LAUNCHING},
		{"lingering", LINGERING},
		{"daring", DARING},
		{"secretive", SECRETIVE},
		{"ramming", RAMMING},
		{"unrestricted", UNRESTRICTED},
		{"restricted", RESTRICTED},
		{"decloaked", DECLOAKED},
		{"quiet", QUIET},
		{"getaway", GETAWAY}
	};

	// Tokens that combine two or more flags.
	const map<string, vector<PersonalityTrait>> COMPOSITE_TOKEN = {
		{"heroic", {DARING, HUNTING}}
	};
}



// Default settings for player's ships.
Personality::Personality() noexcept
	: flags(1LL << DISABLES), confusionMultiplier(2.), period(240.),
	focusMultiplier(.2), gainFocusTime(120.), loseFocusTime(1800.), tick(Random::Int(period)), focusPercentage(0.)
{
	static_assert(LAST_ITEM_IN_PERSONALITY_TRAIT_ENUM == PERSONALITY_COUNT,
		"PERSONALITY_COUNT must match the length of PersonalityTraits");
}



void Personality::Load(const DataNode &node)
{
	bool add = (node.Token(0) == "add");
	bool remove = (node.Token(0) == "remove");
	if(!(add || remove))
		flags.reset();
	for(int i = 1 + (add || remove); i < node.Size(); ++i)
		Parse(node, i, remove);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "confusion")
		{
			if(add || remove)
			{
				child.PrintTrace("Cannot \"" + node.Token(0) + "\" a confusion node:");
				continue;
			}
			// Accept the old method of defining confusion for backwards compatibility.
			if(child.Size() >= 2)
				confusionMultiplier = child.Value(1);
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				if(grand.Size() < 2)
					grand.PrintTrace("Skipping attribute with no value specified:");
				else if(grandKey == "max confusion")
					confusionMultiplier = max(0., grand.Value(1));
				else if(grandKey == "period")
					period = max(1., grand.Value(1));
				else if(grandKey == "focus multiplier")
					focusMultiplier = max(0., grand.Value(1));
				else if(grandKey == "gain focus time")
					gainFocusTime = max(1., grand.Value(1));
				else if(grandKey == "lose focus time")
					loseFocusTime = max(1., grand.Value(1));
				else
					grand.PrintTrace("Skipping unknown confusion attribute:");
			}
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
			if(flags.test(it.second))
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
	return flags.test(PACIFIST);
}



bool Personality::IsForbearing() const
{
	return flags.test(FORBEARING);
}



bool Personality::IsTimid() const
{
	return flags.test(TIMID);
}



bool Personality::IsHunting() const
{
	return flags.test(HUNTING);
}



bool Personality::IsNemesis() const
{
	return flags.test(NEMESIS);
}



bool Personality::IsDaring() const
{
	return flags.test(DARING);
}



bool Personality::IsFrugal() const
{
	return flags.test(FRUGAL);
}



bool Personality::Disables() const
{
	return flags.test(DISABLES);
}



bool Personality::Plunders() const
{
	return flags.test(PLUNDERS);
}



bool Personality::IsVindictive() const
{
	return flags.test(VINDICTIVE);
}



bool Personality::IsUnconstrained() const
{
	return flags.test(UNCONSTRAINED);
}



bool Personality::IsUnrestricted() const
{
	return flags.test(UNRESTRICTED);
}



bool Personality::IsRestricted() const
{
	return flags.test(RESTRICTED);
}



bool Personality::IsCoward() const
{
	return flags.test(COWARD);
}



bool Personality::IsAppeasing() const
{
	return flags.test(APPEASING);
}



bool Personality::IsOpportunistic() const
{
	return flags.test(OPPORTUNISTIC);
}



bool Personality::IsMerciful() const
{
	return flags.test(MERCIFUL);
}



bool Personality::IsRamming() const
{
	return flags.test(RAMMING);
}



bool Personality::IsGetaway() const
{
	return flags.test(GETAWAY);
}



bool Personality::IsStaying() const
{
	return flags.test(STAYING);
}



bool Personality::IsEntering() const
{
	return flags.test(ENTERING);
}



bool Personality::IsWaiting() const
{
	return flags.test(WAITING);
}



bool Personality::IsLaunching() const
{
	return flags.test(LAUNCHING);
}



bool Personality::IsFleeing() const
{
	return flags.test(FLEEING);
}



bool Personality::IsDerelict() const
{
	return flags.test(DERELICT);
}



bool Personality::IsUninterested() const
{
	return flags.test(UNINTERESTED);
}



bool Personality::IsSurveillance() const
{
	return flags.test(SURVEILLANCE);
}



bool Personality::IsMining() const
{
	return flags.test(MINING);
}



bool Personality::Harvests() const
{
	return flags.test(HARVESTS);
}



bool Personality::IsSwarming() const
{
	return flags.test(SWARMING);
}



bool Personality::IsLingering() const
{
	return flags.test(LINGERING);
}



bool Personality::IsSecretive() const
{
	return flags.test(SECRETIVE);
}



bool Personality::IsEscort() const
{
	return flags.test(ESCORT);
}



bool Personality::IsTarget() const
{
	return flags.test(TARGET);
}



bool Personality::IsMarked() const
{
	return flags.test(MARKED);
}



bool Personality::IsTracked() const
{
	return flags.test(TRACKED);
}



bool Personality::IsMute() const
{
	return flags.test(MUTE);
}



bool Personality::IsDecloaked() const
{
	return flags.test(DECLOAKED);
}



bool Personality::IsQuiet() const
{
	return flags.test(QUIET);
}



const double &Personality::Confusion() const
{
	return confusion;
}



void Personality::UpdateConfusion(bool isFocusing)
{
	if(!confusionMultiplier)
		return;
	tick++;

	// If you're focusing, aiming accuracy should slowly improve.
	// Gain and lose focus times are stored as the number of ticks to reach and lose the maximum aiming bonus,
	// so use their inverse to determine the amount of accuracy to gain or lose each tick.
	if(isFocusing)
		focusPercentage += 1. / gainFocusTime;
	else
		focusPercentage -= 1. / loseFocusTime;
	focusPercentage = min(1., max(0., focusPercentage));

	confusion = confusionMultiplier * (1. - (1. - focusMultiplier) * focusPercentage) * cos(tick * PI * 2 / period);
}



Personality Personality::Defender()
{
	Personality defender;
	defender.flags = bitset<PERSONALITY_COUNT>((1LL << STAYING) | (1LL << MARKED) | (1LL << HUNTING) | (1LL << DARING)
			| (1LL << UNCONSTRAINED) | (1LL << TARGET));
	return defender;
}



// Remove target and marked since the defender defeat check doesn't actually care
// about carried ships.
Personality Personality::DefenderFighter()
{
	Personality defender;
	defender.flags = bitset<PERSONALITY_COUNT>((1LL << STAYING) | (1LL << HUNTING) | (1LL << DARING)
			| (1LL << UNCONSTRAINED));
	return defender;
}



void Personality::Parse(const DataNode &node, int index, bool remove)
{
	const string &token = node.Token(index);

	auto it = TOKEN.find(token);
	if(it == TOKEN.end())
	{
		auto cit = COMPOSITE_TOKEN.find(token);
		if(cit == COMPOSITE_TOKEN.end())
			node.PrintTrace("Skipping unrecognized personality \"" + token + "\":");
		else
		{
			if(remove)
				for(auto personality : cit->second)
					flags &= ~(1LL << personality);
			else
				for(auto personality : cit->second)
					flags |= 1LL << personality;
		}
	}
	else
	{
		if(remove)
			flags &= ~(1LL << it->second);
		else
			flags |= 1LL << it->second;
	}
}
