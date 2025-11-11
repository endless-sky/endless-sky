/* Personality.h
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

#pragma once

#include "Point.h"

#include <bitset>

class DataNode;
class DataWriter;



// Class defining an AI "personality": what actions it takes, and how skilled
// and aggressive it is in combat. This also includes some more specialized
// behaviors, like plundering ships or launching surveillance drones, that are
// used to make some fleets noticeably different from others.
class Personality {
public:
	Personality() noexcept;

	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	bool IsDefined() const;

	// Who a ship decides to attack:
	bool IsPacifist() const;
	bool IsForbearing() const;
	bool IsTimid() const;
	bool IsHunting() const;
	bool IsNemesis() const;
	bool IsDaring() const;

	// How they fight:
	bool IsFrugal() const;
	bool Disables() const;
	bool Plunders() const;
	bool IsVindictive() const;
	bool IsUnconstrained() const;
	bool IsUnrestricted() const;
	bool IsRestricted() const;
	bool IsCoward() const;
	bool IsAppeasing() const;
	bool IsOpportunistic() const;
	bool IsMerciful() const;
	bool IsRamming() const;
	bool IsGetaway() const;

	// Mission NPC states:
	bool IsStaying() const;
	bool IsEntering() const;
	bool IsWaiting() const;
	bool IsLaunching() const;
	bool IsFleeing() const;
	bool IsDerelict() const;
	bool IsUninterested() const;

	// Non-combat goals:
	bool IsSurveillance() const;
	bool IsMining() const;
	bool Harvests() const;
	bool IsSwarming() const;
	bool IsLingering() const;
	bool IsSecretive() const;

	// Special flags:
	bool IsEscort() const;
	bool IsTarget() const;
	bool IsMarked() const;
	bool IsTracked() const;
	bool IsMute() const;
	bool IsDecloaked() const;
	bool IsQuiet() const;

	// Current inaccuracy in this ship's targeting:
	const Point &Confusion() const;
	void UpdateConfusion(bool isFiring);

	// Personality to use for ships defending a planet from domination:
	static Personality Defender();
	static Personality DefenderFighter();


private:
	void Parse(const DataNode &node, int index, bool remove);


private:
	// Make sure this matches the number of items in PersonalityTrait,
	// or the build will fail.
	static const int PERSONALITY_COUNT = 39;

	bool isDefined = false;

	std::bitset<PERSONALITY_COUNT> flags;
	double confusionMultiplier;
	double aimMultiplier;
	Point confusion;
	Point confusionVelocity;
};
