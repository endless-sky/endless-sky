/* GamerulesPanel.h
Copyright (c) 2025 by Amazinite

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

#include "ClickZone.h"
#include "Point.h"
#include "ScrollVar.h"
#include "Tooltip.h"

#include <memory>
#include <string>
#include <vector>

class Command;
class Gamerules;
class Interface;
class RenderBuffer;



// UI panel for editing gamerules.
class GamerulesPanel : public Panel {
public:
	GamerulesPanel(Gamerules &gamerules, bool existingPilot);
	virtual ~GamerulesPanel() override;

	// Draw this panel.
	virtual void Draw() override;

	virtual void UpdateTooltipActivation() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;
	virtual bool Drag(double dx, double dy) override;

	virtual void Resize() override;


private:
	void DrawGamerules();
	void DrawPresets();
	void RenderPresetDescription(const std::string &name);
	void RenderPresetDescription(const Gamerules &preset);

	void DrawTooltips();

	void HandleGamerulesString(const std::string &str);
	// Scroll the preset list until the selected preset is visible.
	void ScrollSelectedPreset();

	void HandleUp();
	void HandleDown();
	void HandleConfirm();

	void SelectPreset(const std::string &name);


private:
	// The gamerules being modified.
	Gamerules &gamerules;
	bool existingPilot;

	const Interface *gamerulesUi;
	const Interface *presetUi;

	int selectedIndex = 0;
	int hoverIndex = -1;
	int oldSelectedIndex = 0;
	int oldHoverIndex = 0;
	int latestIndex = 0;
	// Which page we're on. g = gamerules, p = presets.
	char page = 'g';

	Point hoverPoint;
	Tooltip tooltip;
	std::string selectedItem;
	std::string hoverItem;

	int currentGamerulesPage = 0;

	std::string selectedPreset;

	std::vector<ClickZone<std::string>> gameruleZones;
	std::vector<ClickZone<std::string>> presetZones;

	std::unique_ptr<RenderBuffer> presetListClip;
	std::unique_ptr<RenderBuffer> presetDescriptionBuffer;
	ScrollVar<double> presetListScroll;
	ScrollVar<double> presetDescriptionScroll;
};
