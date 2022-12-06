/* AfterburnerUsage.h
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

#ifndef AFTERBURNER_USAGE_H_
#define AFTERBURNER_USAGE_H_

#include "Outfit.h"



// Class used to manage afterburners and their cooldowns/usages on ships.
class AfterburnerUsage {
public:
	// Constructor
	AfterburnerUsage(const Outfit &outfit);

	bool CanUseAfterburner() const;
	// Refresh the afterburner, used if for specifying if we want to use it or not.
	void RefreshAfterburner(bool used = false);
	const Outfit *Afterburner() const;


private:
	const Outfit *afterburner = nullptr;
	// Store the duration and cooldown locally so we don't always look for it in the dictionary.
	double baseDuration;
	double baseCooldown;
	// The cooldown we need to wait for, and the time we've used it for already.
	double afterburnerCooldown = 0.;
	double afterburnerUsageTime = 0.;
};



#endif
