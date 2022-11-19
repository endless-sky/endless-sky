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



// Class used to manage afterburners and their cooldowns/usages.
class AfterburnerUsage {
public:
    AfterburnerUsage(const Outfit &outfit);
    bool CanUseAfterburner() const;
    // Refresh the afterburner, specifying if it will be used or not, and return if it can be used.
    void RefreshAfterburner(bool used = false);
    const Outfit *Afterburner() const;


private:
    const Outfit &afterburner;
    double baseDuration;
    double baseCooldown;
    double afterburnerCooldown = 0.;
    double afterburnerUsageTime = 0.;
};



#endif 