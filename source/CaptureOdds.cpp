/* CaptureOdds.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "CaptureOdds.h"

#include "Outfit.h"
#include "Ship.h"

#include <algorithm>

using namespace std;



CaptureOdds::CaptureOdds(const Ship *attacker, const Ship *defender)
{
	Make(&powerA, attacker, false);
	Make(&powerD, defender, true);
	Calculate();
}



double CaptureOdds::Odds(int attackingCrew, int defendingCrew) const
{
	if(!defendingCrew)
		return 1.;
	
	int index = Index(attackingCrew, defendingCrew);
	if(attackingCrew < 2 || index < 0)
		return 0.;
	
	return capture[index];
}



double CaptureOdds::AttackerCasualties(int attackingCrew, int defendingCrew) const
{
	int index = Index(attackingCrew, defendingCrew);
	if(attackingCrew < 2 || !defendingCrew || index < 0)
		return 0.;
	
	return casualtiesA[index];
}



double CaptureOdds::DefenderCasualties(int attackingCrew, int defendingCrew) const
{
	int index = Index(attackingCrew, defendingCrew);
	if(attackingCrew < 2 || !defendingCrew || index < 0)
		return 0.;
	
	return casualtiesD[index];
}



double CaptureOdds::AttackerPower(int attackingCrew) const
{
	if(static_cast<unsigned>(attackingCrew - 1) >= powerA.size())
		return 0.;
	
	return powerA[attackingCrew - 1];
}



double CaptureOdds::DefenderPower(int defendingCrew) const
{
	if(static_cast<unsigned>(defendingCrew - 1) >= powerD.size())
		return 0.;
	
	return powerD[defendingCrew - 1];
}



void CaptureOdds::Calculate()
{
	if(powerD.empty() || powerA.empty())
		return;
	
	// The first row represents the case where the attacker has only one crew left.
	// In that case, the defending ship can never be successfully captured.
	capture.resize(powerD.size(), 0.);
	casualtiesA.resize(powerD.size(), 0.);
	casualtiesD.resize(powerD.size(), 0.);
	unsigned up = 0;
	for(unsigned a = 2; a <= powerA.size(); ++a)
	{
		double ap = powerA[a - 1];
		// Special case: odds for defender having only one person,
		// because 0 people is outside the end of the table.
		double odds = ap / (ap + powerD[0]);
		capture.push_back(odds + (1. - odds) * capture[up]);
		casualtiesA.push_back((1. - odds) * (casualtiesA[up] + 1.));
		casualtiesD.push_back(odds + (1. - odds) * casualtiesD[up]);
		++up;
		
		for(unsigned d = 2; d <= powerD.size(); ++d)
		{
			odds = ap / (ap + powerD[d - 1]);
			capture.push_back(odds * capture.back() + (1. - odds) * capture[up]);
			casualtiesA.push_back(odds * casualtiesA.back() + (1. - odds) * (casualtiesA[up] + 1.));
			casualtiesD.push_back(odds * (casualtiesD.back() + 1.) + (1. - odds) * casualtiesD[up]);
			++up;
		}
	}
}



int CaptureOdds::Index(int attackingCrew, int defendingCrew) const
{
	if(static_cast<unsigned>(attackingCrew - 1) > powerA.size())
		return -1;
	if(static_cast<unsigned>(defendingCrew - 1) > powerD.size())
		return -1;
	
	return (attackingCrew - 1) * powerD.size() + (defendingCrew - 1);
}



void CaptureOdds::Make(vector<double> *power, const Ship *ship, bool isDefender)
{
	power->clear();
	int crew = ship->Crew();
	if(!crew)
		return;
	
	const string weaponAttribute = (isDefender ? "capture defense" : "capture attack");
	const string automatedAttribute = (isDefender ? "automated capture defense" : "automated capture attack");
	const double crewPower = (isDefender ? 2. : 1.);
	
	// First calculate the automated attack or defense power, which serves as a base-value.
	double automatedPower = ship->BaseAttributes().Get(automatedAttribute);
	for(const auto &it : ship->Outfits())
	{
		double value = it.GetOutfit()->Get(automatedAttribute);
		if(value > 0. && it.GetQuantity() > 0) 
			automatedPower += it.GetQuantity() * value;
	}

	// Each crew member can wield one weapon. They use the most powerful ones
	// that can be wielded by the remaining crew.
	for(const auto &it : ship->Outfits())
	{
		double value = it.GetOutfit()->Get(weaponAttribute);
		if(value > 0. && it.GetQuantity() > 0)
			power->insert(power->end(), it.GetQuantity(), value);
	}
	// Use the best weapons first.
	sort(power->begin(), power->end(), greater<double>());
	
	// Resize the vector to have exactly one entry per crew member.
	power->resize(ship->Crew(), 0.);
	
	// The final power vector contains the total remaining power
	// when i crew members are still fighting at each index i. 
	power->front() += crewPower + automatedPower;
	for(unsigned i = 1; i < power->size(); ++i)
		(*power)[i] += (*power)[i - 1] + crewPower;
}
