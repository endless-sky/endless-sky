/* PresetsPanel.h
Copyright (c) 2026 by Endless Sky contributors

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

#include "Panel.h"

#include "Color.h"
#include "Interface.h"
#include "OutfitterPanel.h"
#include "PlayerInfo.h"
#include "Preset.h"
#include "ScrollBar.h"
#include "ScrollVar.h"

#include <string>



// A panel dedicated to creating and applying ship presets.
class PresetsPanel : public Panel {
public:
	explicit PresetsPanel(PlayerInfo &player, OutfitterPanel &parent, std::set<Ship*> &playerShips,
						  Sale<Outfit> &outfitter, int day);
	~PresetsPanel() override;

	void Step() override;
	void Draw() override;
	bool Click(int x, int y, MouseButton button, int clicks) override;
	bool Hover(int x, int y) override;
	bool Drag(double dx, double dy) override;
	bool Scroll(double dx, double dy) override;
	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;


private:
	// Inner helper class for organizing preset sourcing data.
	class OutfitSources {
	public:
		std::string byPreset, installed, inCargo, inStorage, fromOutfitter, stillRequired;
	};

	// Helper enum for where outfits go when removed from your ship.
	enum PresetDestination {
		STORAGE,
		CARGO,
		OUTFITTER
	};


private:
	void LoadPresets();
	void CheckPresetSelected();
	void RefreshPresetsBox();
	void RefreshPresetData();
	void SavePreset(const std::string &name);
	void DeletePreset();
	void ApplyPreset();

	// Draw sub-methods to make the Draw method more readable.
	void DrawPresetsModule();
	void DrawSelectedModule();
	void DrawRemovingModule();
	void DrawSettingsModule();
	void DrawAccountingModule() const;


private:
	// Some code wants to rotate through the PresetDestination enum, so the index is recorded here.
	const int destinationsSize = 3;

	// Cache rendered values to only be calculated once per click.
	std::map<std::string, std::map<const Outfit*, OutfitSources*>> presetListings;
	std::map<std::string, std::map<const Outfit*, std::string*>> removalListings;

	// External data and interaction.
	PlayerInfo &player;
	std::set<Ship*> *playerShips;
	Sale<Outfit> *outfitter;
	const Interface *presetPanelUi;
	int day;

	// Preset management.
	std::vector<Preset*> presets;
	std::vector<Preset*> visiblePresets;
	Preset* selectedPreset = nullptr;
	int64_t presetSales = 0;
	double cargoChange = 0;
	double storageChange = 0;
	// If the player enters a filename that exists, prompt before overwriting it.
	std::string nameToConfirm;

	// Preset selection scrollbar.
	int selected = 0;
	ScrollVar<double> presetScroll;
	ScrollBar presetScrollBar;
	bool presetDrag = false;
	bool presetHovered = false;

	// Selected preset data scrollbar.
	ScrollVar<double> dataScroll;
	ScrollBar dataScrollBar;
	bool dataDrag = false;
	bool dataHovered = false;

	// Display of the removed outfits scrollbar.
	ScrollVar<double> removeScroll;
	ScrollBar removeScrollBar;
	bool removeDrag = false;
	bool removeHovered = false;

	// Data values cached on screen open to prevent excessive GameData calls.
	const Color &active;
	const Font &bigFont;
	const Font &smallFont;
	Rectangle presetsBox;
	Rectangle selectedBox;
	Rectangle removedBox;
	Rectangle settingsBox;
	Rectangle cargoBox;
	Rectangle creditsBox;
	Rectangle tooltipBox;
	Rectangle deleteBox;
	Rectangle saveBox;
	Rectangle openBox;
	Rectangle applyBox;
	const Color &backgroundColor;
	const Color &dimColor;
	const Color &mediumColor;
	const Color &brightColor;
	const Color &errorColor;
	const Color &faintColor;

	// Settings control.
	Rectangle uniqueBox;
	Rectangle handToHandBox;
	Rectangle enforceBox;
	Rectangle moveUnequippedBox;
	Rectangle toStorageBox;
	Rectangle toCargoBox;
	Rectangle toOutfitterBox;
	PresetDestination presetDestination = OUTFITTER;
	bool includeHandToHand = false;
	bool includeUnique = false;
	bool enforceShipTypes = true;

	// Tooltip management.
	Tooltip tooltip;
	std::string hoveredTooltip;
	std::map<Rectangle*, std::string> tooltipKeys;

};
