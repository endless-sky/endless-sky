/* PluginManager.cpp
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

#include "PluginManager.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "Logger.h"
#include "Plugin.h"
#include "Set.h"

using namespace std;

namespace {
	Set<Plugin> plugins;

	void LoadSettingsFromFile(const filesystem::path &path)
	{
		DataFile prefs(path);
		for(const DataNode &node : prefs)
		{
			if(node.Token(0) != "state")
				continue;

			for(const DataNode &child : node)
				if(child.Size() == 2)
				{
					auto *plugin = plugins.Get(child.Token(0));
					plugin->enabled = child.Value(1);
					plugin->currentState = child.Value(1);
				}
		}
	}
}



// Attempt to load a plugin at the given path.
const Plugin *PluginManager::Load(const filesystem::path &path)
{
	// Get the name of the folder containing the plugin.
	string name = path.filename().string();

	filesystem::path pluginFile = path / "plugin.txt";
	string aboutText;
	string version;
	set<string> authors;
	set<string> tags;
	Plugin::PluginDependencies dependencies;

	// Load plugin metadata from plugin.txt.
	bool hasName = false;
	for(const DataNode &child : DataFile(pluginFile))
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "name" && hasValue)
		{
			name = child.Token(1);
			hasName = true;
		}
		else if(key == "about" && hasValue)
			aboutText += child.Token(1) + '\n';
		else if(key == "version" && hasValue)
			version = child.Token(1);
		else if(key == "authors")
			for(const DataNode &grand : child)
				authors.insert(grand.Token(0));
		else if(key == "tags")
			for(const DataNode &grand : child)
				tags.insert(grand.Token(0));
		else if(key == "dependencies")
		{
			for(const DataNode &grand : child)
			{
				const string &grandKey = grand.Token(0);
				bool grandHasValue = grand.Size() >= 2;
				if(grandKey == "game version" && grandHasValue)
					dependencies.gameVersion = grand.Token(1);
				else if(grandKey == "requires")
					for(const DataNode &great : grand)
						dependencies.required.insert(great.Token(0));
				else if(grandKey == "optional")
					for(const DataNode &great : grand)
						dependencies.optional.insert(great.Token(0));
				else if(grandKey == "conflicts")
					for(const DataNode &great : grand)
						dependencies.conflicted.insert(great.Token(0));
				else
					grand.PrintTrace("Skipping unrecognized attribute:");
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// 'name' is a required field for plugins with a plugin description file.
	if(Files::Exists(pluginFile) && !hasName)
		Logger::Log("Missing required \"name\" field inside plugin.txt.", Logger::Level::WARNING);

	// Plugin names should be unique.
	auto *plugin = plugins.Get(name);
	if(plugin && plugin->IsValid())
	{
		Logger::Log("Skipping plugin located at \"" + path.string()
			+ "\", because another plugin with the same name has already been loaded from: \""
			+ plugin->path.string() + "\".", Logger::Level::WARNING);
		return nullptr;
	}

	// Skip the plugin if the dependencies aren't valid.
	if(!dependencies.IsValid())
	{
		Logger::Log("Skipping plugin located at \"" + path.string() + "\", because it has errors in its dependencies.",
			Logger::Level::WARNING);
		return nullptr;
	}

	plugin->name = std::move(name);
	plugin->path = path;
	// Read the deprecated about.txt content if no about text was specified.
	plugin->aboutText = aboutText.empty() ? Files::Read(path / "about.txt") : std::move(aboutText);
	plugin->version = std::move(version);
	plugin->authors = std::move(authors);
	plugin->tags = std::move(tags);
	plugin->dependencies = std::move(dependencies);

	return plugin;
}



void PluginManager::LoadSettings()
{
	// Global plugin settings
	LoadSettingsFromFile(Files::Resources() / "plugins.txt");
	// Local plugin settings
	LoadSettingsFromFile(Files::Config() / "plugins.txt");
}



void PluginManager::Save()
{
	if(plugins.empty())
		return;
	DataWriter out(Files::Config() / "plugins.txt");

	out.Write("state");
	out.BeginChild();
	{
		for(const auto &it : plugins)
			if(it.second.IsValid())
				out.Write(it.first, it.second.currentState);
	}
	out.EndChild();
}



// Whether the path points to a valid plugin.
bool PluginManager::IsPlugin(const filesystem::path &path)
{
	// A folder is a valid plugin if it contains one (or more) of the assets folders.
	// (They can be empty too).
	return Files::Exists(path / "data") || Files::Exists(path / "images")
		|| Files::Exists(path / "shaders") || Files::Exists(path / "sounds");
}



// Returns true if any plugin enabled or disabled setting has changed since
// launched via user preferences.
bool PluginManager::HasChanged()
{
	for(const auto &it : plugins)
		if(it.second.enabled != it.second.currentState)
			return true;
	return false;
}



// Returns the list of plugins that have been identified by the game.
const Set<Plugin> &PluginManager::Get()
{
	return plugins;
}



// Toggles enabling or disabling a plugin for the next game restart.
void PluginManager::TogglePlugin(const string &name)
{
	auto *plugin = plugins.Get(name);
	plugin->currentState = !plugin->currentState;
}
