/* AfterburnerUsage.cpp
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AfterburnerUsage.h"



AfterburnerUsage::AfterburnerUsage(const Outfit &outfit) : 
	afterburner(outfit), baseDuration(outfit.Attributes().Get("afterburner duration")), baseCooldown(outfit.Attributes().Get("base cooldown")) {}



bool AfterburnerUsage::CanUseAfterburner() const
{
	return !baseCooldown || (!afterburnerCooldown && afterburnerUsageTime < baseDuration);
}



void AfterburnerUsage::RefreshAfterburner(bool used)
{
	if(!baseCooldown)
		return;
	if(!used)
	{
		if(afterburnerUsageTime)
			afterburnerUsageTime--;
		else if(afterburnerCooldown)
			afterburnerCooldown--;
	}
	else if(used)
	{
		if(afterburnerUsageTime < baseDuration)
		{
			if(!afterburnerCooldown--)
				afterburnerUsageTime++;
		}
		else
		{
			afterburnerUsageTime = 0.;
			afterburnerCooldown = baseCooldown;
		}
	}
}



const Outfit *AfterburnerUsage::Afterburner() const
{
	return &afterburner;
}
