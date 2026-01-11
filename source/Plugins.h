/* Plugins.h
Copyright (c) 2022 by Sam Gleske (samrocketman on GitHub)

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

#include "OrderedSet.h"
#include "TaskQueue.h"

#include <filesystem>
#include <future>
#include <set>
#include <string>
#include <vector>



// For consistent handling of mutex locks on the sets of plugins (installed/available) between contexts.
// Usage:
// auto iPlugins = Plugins::GetPluginsLocked();
// iPlugins->Get(...) or iPlugins->Find(...)
// C can be either Set or OrderedSet
template<template<class> class C, typename T>
class LockedSet {
public:
	LockedSet(std::mutex &guard, C<T> &data) : guard(guard), data(data) {}
	C<T> *operator->() { return &data; }
	C<T> &operator*() { return data; }

private:
	std::lock_guard<std::mutex> guard;
	C<T> &data;
};


// Plugin class represents information about a single plugin.
// Concepts:
//  - Plugins are consumed by the game on start-up, this translates to `inUse = true`.
//  - Plugins which are enabled *will be* inUse on the next restart.
//  - Library of plugins which are available for installation online:
//    - Updated version available (plugin installed, update available)
//    - Plugin available for install and not installed yet at all
//    - On install: mark desired state as enabled
class Plugin
{
public:
	struct PluginDependencies {
		// Checks if there are any dependencies of any kind.
		bool IsEmpty() const;
		// Checks if there are any duplicate dependencies. E.g. the same dependency in both required and conflicted.
		bool IsValid() const;

		// The game version to match against.
		std::string gameVersion;
		// The plugins, if any, which are required by this plugin.
		std::set<std::string> required;
		// The plugins, if any, which are designed to work with this plugin but aren't required.
		std::set<std::string> optional;
		// The plugins, if any, which can't be run alongside this plugin.
		std::set<std::string> conflicted;
	};


public:
	// Attempt to load a plugin at the given path.
	Plugin() = default;
	Plugin(const std::string &pluginName, const std::string &pluginUrl, const std::string &pluginVersion,
			const std::string &pluginDescription, const std::vector<std::string> &pluginAuthors,
			const std::string &pluginHomepage, const std::string &pluginLicense)
		: name(pluginName), description(pluginDescription), version(pluginVersion), authors(pluginAuthors),
			homepage(pluginHomepage), license(pluginLicense), url(pluginUrl) {}

	// Constructs a description of the plugin from its name, tags, dependencies, etc.
	[[nodiscard]] std::string CreateDescription() const;
	[[nodiscard]] std::string GetIconName() const;
	[[nodiscard]] bool InUse() const;
	[[nodiscard]] const std::string &Name() const;
	// Checks whether this plugin is valid, i.e. whether it exists.
	[[nodiscard]] bool IsValid() const;
	[[nodiscard]] bool IsDownloading() const;
	[[nodiscard]] bool HasChanged() const;
	[[nodiscard]] bool Search(const std::string &search) const;

	void SetInUse(bool useState);
	void SetDesiredState(bool newDesiredState);
	void SetVersion(const std::string &newVersion);

protected:
	// The name that identifies this plugin.
	std::string name;
	// The path to the plugin's folder.
	std::filesystem::path path;
	// The about text, if any, of this plugin.
	std::string description;
	// The version of this plugin, important if it has been installed over ES.
	std::string version = "???";
	// The set of tags which are used to categorize the plugin.
	std::set<std::string> tags;
	// The set of people who have created the plugin.
	std::vector<std::string> authors;
	std::string homepage;
	std::string license;
	// The url used for installation, relevant when installed = false;
	std::string url;

	// Other plugins which are required for, optional for, or conflict with this plugin.
	PluginDependencies dependencies;

	// This class is also used for plugins which are not yet installed and or can be updated.
	bool installed = true;
	bool outdated = false;
	std::string installedVersion; // Not set by default; Only used in available plugin data

	// Whether this plugin is in use, i.e. if it was loaded by the game on this start-up.
	bool inUse = true;
	// TODO: if a plugin zip is corrupted and doesn't load, we should indicate as such.
	// The desired state of the plugin on the next game start, true = enabled.
	bool desiredState = true;

	// These affect whether the UI is telling the user that the game must be restarted.
	// If this plugin has been deleted.
	bool removed = false;
	// If this plugin has been updated (version on start-up no longer matches the files on disk)
	bool updated = false;

	friend class Plugins;
	friend class PreferencesPanel;
};



// Tracks enabled and disabled plugins for loading plugin data or skipping it.
// This object is updated by toggling plugins in the Preferences UI.
class Plugins {
public:
	// Attempt to load a plugin at the given path.
	static const Plugin *Load(const std::filesystem::path &path);
	static void LoadAvailablePlugins(TaskQueue &queue, const std::filesystem::path &pluginsJsonPath);
	static void LoadSettings();
	static void Save();

	static void AddLibraryUrl(const std::string & token);
	static std::map<std::string, bool> &GetPluginLibraryUrls();

	// Whether the path points to a valid plugin.
	static bool IsPlugin(const std::filesystem::path &path);
	// Returns true if any plugin enabled or disabled setting has changed since
	// launched via user preferences.
	static bool HasChanged();
	// Returns true if there is active install/update activity.
	static bool DownloadingInBackground();

	// Returns the list of plugins that have been identified in the online plugin library.
	static LockedSet<OrderedSet, Plugin> GetAvailablePluginsLocked();
	// Returns the list of plugins that have been identified by the game.
	static LockedSet<OrderedSet, Plugin> GetPluginsLocked();

	// Re-order the installed plugins list by moving the Plugin at index `index` by `offset` (positive toward back).
	static int Move(int index, int offset);

	// Toggles enabling or disabling a plugin for the next game restart.
	static void TogglePlugin(const std::string &name);

	// Install, update or delete a plugin.
	static std::future<std::string> InstallOrUpdate(const std::string &name);
	static std::string DeletePlugin(const std::string &name);

	static bool Download(const std::string &url, const std::filesystem::path &location);

private:
	// Maintain the order which plugins are loaded in.
	std::vector<std::string> pluginLoadOrder;
};
