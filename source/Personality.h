/* Personality.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PERSONALITY_H_
#define PERSONALITY_H_

#include "Angle.h"
#include "Point.h"

#include <string>

class DataNode;
class DataWriter;



// Class defining an AI "personality": what actions it takes, and how skilled
// and aggressive it is in combat. This also includes some more specialized
// behaviors, like plundering ships or launching surveillance drones, that are
// used to make some fleets noticeably different from others.
class Personality {
public:
	Personality();
	
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;
	
	// Who a ship decides to attack:
	bool IsPacifist() const;
	bool IsForbearing() const;
	bool IsTimid() const;
	bool IsHeroic() const;
	bool IsNemesis() const;
	
	// How they fight:
	bool IsFrugal() const;
	bool Disables() const;
	bool Plunders() const;
	bool IsVindictive() const;
	bool IsUnconstrained() const;
	bool IsCoward() const;
	bool IsAppeasing() const;
	bool IsOpportunistic() const;
	
	// Mission NPC states:
	bool IsStaying() const;
	bool IsEntering() const;
	bool IsWaiting() const;
	bool IsFleeing() const;
	bool IsDerelict() const;
	bool IsUninterested() const;
	
	// Non-combat goals:
	bool IsSurveillance() const;
	bool IsMining() const;
	bool Harvests() const;
	bool IsSwarming() const;
	
	// Special flags:
	bool IsEscort() const;
	bool IsTarget() const;
	bool IsMarked() const;
	bool IsMute() const;
	
	// Current inaccuracy in this ship's targeting:
	const Point &Confusion() const;
	void UpdateConfusion(bool isFiring);
	
	// Personality to use for ships defending a planet from domination:
	static Personality Defender();
	
	
private:
	void Parse(const std::string &token);
	
	
private:
	int flags;
	double confusionMultiplier;
	double aimMultiplier;
	Point confusion;
	Point confusionVelocity;
};



#endif
