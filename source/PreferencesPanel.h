/* PreferencesPanel.h
Copyright (c) 2014 by Michael Zahniser

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
#include "Command.h"
#include "Point.h"
#include "ScrollVar.h"
#include "Tooltip.h"

#include <memory>
#include <string>
#include <vector>

class PlayerInfo;
class RenderBuffer;
struct Plugin;



// UI panel for editing preferences, especially the key mappings.
class PreferencesPanel : public Panel {
public:
	explicit PreferencesPanel(PlayerInfo &player);
	virtual ~PreferencesPanel();

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

	virtual void EndEditing() override;


private:
	void DrawControls();
	void DrawSettings();
	void DrawPlugins();
	void RenderPluginDescription(const std::string &pluginName);
	void RenderPluginDescription(const Plugin &plugin);

	void DrawTooltips();

	void Exit();

	void HandleSettingsString(const std::string &str, Point cursorPosition);

	void HandleUp();
	void HandleDown();
	void HandleConfirm();

	// Scroll the plugin list until the selected plugin is visible.
	void ScrollSelectedPlugin();


private:
	PlayerInfo &player;
	// Determine if the player's mission deadlines need to be recached when
	// this panel is closed due to the deadline blink preference changing.
	bool recacheDeadlines = false;

	int editing = -1;
	int selected = 0;
	int hover = -1;
	int oldSelected;
	int oldHover;
	int latest;
	// Which page of the preferences we're on.
	char page = 'c';

	Point hoverPoint;
	Tooltip tooltip;
	std::string selectedItem;
	std::string hoverItem;

	int currentControlsPage = 0;
	int currentSettingsPage = 0;

	std::string selectedPlugin;

	std::vector<ClickZone<Command>> zones;
	std::vector<ClickZone<std::string>> prefZones;
	std::vector<ClickZone<std::string>> pluginZones;

	std::unique_ptr<RenderBuffer> pluginListClip;
	std::unique_ptr<RenderBuffer> pluginDescriptionBuffer;
	ScrollVar<double> pluginListScroll;
	ScrollVar<double> pluginDescriptionScroll;
};
