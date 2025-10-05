/* MapOutfitterPanel.h
Copyright (c) 2015 by Michael Zahniser

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

#include "MapSalesPanel.h"

#include "OutfitInfoDisplay.h"

#include <vector>

class Outfit;
class PlayerInfo;
class Sprite;



// A panel that displays the galaxy star map, along with a side panel showing
// all outfits that are for sale in known systems. You can click on one of them
// to see which systems it is available in.
class MapOutfitterPanel : public MapSalesPanel {
public:
	explicit MapOutfitterPanel(PlayerInfo &player);
	explicit MapOutfitterPanel(const MapPanel &panel, bool onlyHere = false);


protected:
	virtual const Sprite *SelectedSprite() const override;
	virtual const Sprite *CompareSprite() const override;
	virtual const ItemInfoDisplay &SelectedInfo() const override;
	virtual const ItemInfoDisplay &CompareInfo() const override;
	virtual const std::string &KeyLabel(int index) const override;

	virtual void Select(int index) override;
	virtual void Compare(int index) override;
	virtual double SystemValue(const System *system) const override;
	virtual int FindItem(const std::string &text) const override;

	virtual void DrawItems() override;


private:
	void Init();


private:
	// Flat list of all outfits shown.
	std::vector<const Outfit *> list;

	const Outfit *selected = nullptr;
	const Outfit *compare = nullptr;

	OutfitInfoDisplay selectedInfo;
	OutfitInfoDisplay compareInfo;
};
