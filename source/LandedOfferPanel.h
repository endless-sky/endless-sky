/* LandedOfferPanel.h
Copyright (c) 2023 by an anonymous author

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LANDED_OFFER_PANEL_H_
#define LANDED_OFFER_PANEL_H_

#include "Mission.h"
#include "Panel.h"

#include <memory>



class LandedOfferPanel : public Panel {
public:
	LandedOfferPanel(PlayerInfo &player, Mission::Location location, std::shared_ptr<Panel> otherPanel = nullptr);
	bool TimeToLeaveOrDie() const;
	virtual void Step() override;


protected:
	PlayerInfo &player;
	Mission::Location location;
	std::shared_ptr<Panel> otherPanel;
};

#endif
