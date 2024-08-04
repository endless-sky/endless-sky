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
#include "Plugins.h"
#include "Point.h"
#include "ScrollVar.h"
#include "TaskQueue.h"
#include "text/WrappedText.h"

#include <future>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

class Command;
struct Plugin;
class Sprite;
class RenderBuffer;



// UI panel for editing preferences, especially the key mappings.
class PreferencesPanel : public Panel {
public:
	PreferencesPanel();
	virtual ~PreferencesPanel();

	// Draw this panel.
	virtual void Draw() override;
	virtual void Step() override;


protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;
	virtual bool Drag(double dx, double dy) override;

	virtual void EndEditing() override;


private:
	void DrawControls();
	void DrawSettings();
	void DrawPlugins();
	void DrawPluginInstalls();
	void RenderPluginDescription(const std::string &pluginName);
	void RenderPluginDescription(const Sprite *sprite, const std::string &description);

	void DrawTooltips();

	void Exit();

	void HandleSettingsString(const std::string &str, Point cursorPosition);

	void HandleUp();
	void HandleDown();
	void HandleConfirm();

	void ProcessPluginIndex();
	// Scroll the plugin list until the selected plugin is visible.
	void ScrollSelectedPlugin();


private:
	int editing;
	int selected;
	int hover;
	int oldSelected;
	int oldHover;
	int latest;
	// Which page of the preferences we're on.
	char page = 'c';

	Point hoverPoint;
	int hoverCount = 0;
	std::string selectedItem;
	std::string hoverItem;
	std::string tooltip;
	WrappedText hoverText;

	int currentControlsPage = 0;
	int currentSettingsPage = 0;

	std::string selectedPlugin;


	// Pointers to keep track of what plugin is selected/clicked/hovered over.
	Plugins::InstallData *latestPlugin = nullptr;

	Plugins::InstallData *selectedPluginInstall = nullptr;
	Plugins::InstallData *oldSelectedPluginInstall = nullptr;
	Plugins::InstallData *clickedPluginInstall = nullptr;
	Plugins::InstallData *oldClickedPluginInstall = nullptr;
	Plugins::InstallData *hoverPluginInstall = nullptr;

	size_t pluginInstallPages = 1;
	size_t currentPluginInstallPage = 0;
	// If the plugin index was already downloaded.
	bool downloadedInfo = false;
	// Vector to store the feedback of the async tasks from installing/updating/deleting.
	std::vector<std::future<void>> installFeedbacks;
	// A list of plugins that can be installed.
	std::vector<Plugins::InstallData> pluginInstallData;
	// Queue to load icons for installable plugins and a list of those.
	TaskQueue queue;
	Set<Sprite> icons;

	std::vector<ClickZone<Command>> zones;
	std::vector<ClickZone<std::string>> prefZones;
	std::vector<ClickZone<std::string>> pluginZones;
	std::vector<ClickZone<Plugins::InstallData *>> pluginInstallZones;

	std::unique_ptr<RenderBuffer> pluginListClip;
	std::unique_ptr<RenderBuffer> pluginDescriptionBuffer;
	ScrollVar<double> pluginListScroll;
	ScrollVar<double> pluginDescriptionScroll;
	int pluginListHeight = 0;
};
