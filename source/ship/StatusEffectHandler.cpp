/* StatusEffectHandler.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "StatusEffectHandler.h"

#include "../Entity.h"
#include "../Outfit.h"
#include "../Weapon.h"

#include <cmath>

using namespace std;



void StatusEffectHandler::Setup(Entity *parent)
{
	this->parent = parent;
	attributes = &parent->Attributes();
	entityLevels = &parent->levels;
}



void StatusEffectHandler::Calibrate()
{
	auto CalibrateResistance = [this](const string &name, double &stat, ResourceLevels &cost) -> void {
		stat = attributes->Get(name + " resistance");
		// Save resistance costs as per unit of resistance.
		if(stat)
		{
			cost.energy = attributes->Get(name + " resistance energy") / stat;
			cost.heat = attributes->Get(name + " resistance heat") / stat;
			cost.fuel = attributes->Get(name + " resistance fuel") / stat;
		}
	};

	CalibrateResistance("corrosion", corrosionResistance, corrosionResistCost);
	CalibrateResistance("discharge", dischargeResistance, dischargeResistCost);
	CalibrateResistance("ion", ionizationResistance, ionizationResistCost);
	CalibrateResistance("scramble", scramblingResistance, scramblingResistCost);
	CalibrateResistance("burn", burnResistance, burnResistCost);
	CalibrateResistance("leak", leakResistance, leakageResistCost);
	CalibrateResistance("disruption", disruptionResistance, disruptionResistCost);
	CalibrateResistance("slowing", slowingResistance, slownessResistCost);
}



void StatusEffectHandler::Clear() const
{
	entityLevels->discharge = 0.;
	entityLevels->corrosion = 0.;
	entityLevels->scrambling = 0.;
	entityLevels->ionization = 0.;
	entityLevels->leakage = 0.;
	entityLevels->burning = 0.;
	entityLevels->disruption = 0.;
	entityLevels->slowness = 0.;
}



void StatusEffectHandler::Do(bool disabled) const
{
	entityLevels->hull -= entityLevels->corrosion;
	entityLevels->shields -= entityLevels->discharge;
	entityLevels->energy -= entityLevels->ionization;
	entityLevels->heat += entityLevels->burning;
	entityLevels->fuel -= entityLevels->leakage;

	// TODO: Mothership gives status resistance to carried ships?
	auto DoResistance = [this, &disabled](double &stat, double resistance, const ResourceLevels &cost)
	{
		if(!stat)
			return;

		if(disabled || resistance <= 0.)
		{
			stat = max(0., .99 * stat);
			return;
		}

		// Calculate how much resistance can be used assuming no
		// resource cost.
		resistance = .99 * stat - max(0., .99 * stat - resistance);

		// Limit the resistance by the available resources.
		if(cost.energy > 0.)
			resistance = min(resistance, entityLevels->energy / cost.energy);
		if(cost.heat < 0.)
			resistance = min(resistance, entityLevels->heat / -cost.heat);
		if(cost.fuel > 0.)
			resistance = min(resistance, entityLevels->fuel / cost.fuel);

		if(resistance > 0.)
		{
			stat = max(0., .99 * stat - resistance);
			entityLevels->energy -= resistance * cost.energy;
			entityLevels->heat += resistance * cost.heat;
			entityLevels->fuel -= resistance * cost.fuel;
		}
		else
			stat = max(0., .99 * stat);
	};

	DoResistance(entityLevels->corrosion, corrosionResistance, corrosionResistCost);
	DoResistance(entityLevels->discharge, dischargeResistance, dischargeResistCost);
	DoResistance(entityLevels->ionization, ionizationResistance, ionizationResistCost);
	DoResistance(entityLevels->scrambling, scramblingResistance, scramblingResistCost);
	DoResistance(entityLevels->burning, burnResistance, burnResistCost);
	DoResistance(entityLevels->leakage, leakResistance, leakageResistCost);
	DoResistance(entityLevels->disruption, disruptionResistance, disruptionResistCost);
	DoResistance(entityLevels->slowness, slowingResistance, slownessResistCost);
}
