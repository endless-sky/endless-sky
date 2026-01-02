/* Plugins.cpp
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

#include "Plugins.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameData.h"
#include "Logger.h"
#include "Set.h"
#include "TaskQueue.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <sys/stat.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include "text/Utf8.h"

#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;

namespace {
	// These are the installed and available plugins, not all of which will be enabled for use.
	mutex pluginsMutex;
	Set<Plugin> plugins;

	// A list of plugins that can be installed from the online library.
	mutex availablePluginsMutex;
	Set<Plugin> availablePlugins;

	void LoadSettingsFromFile(const filesystem::path &path)
	{
		DataFile prefs(path);
		for(const DataNode &node : prefs)
		{
			if(node.Token(0) != "state")
				continue;

			for(const DataNode &child : node)
				if(child.Size() >= 2)
				{
					auto iPlugins = Plugins::GetPluginsLocked();
					auto *plugin = iPlugins->Get(child.Token(0));
					plugin->SetInUse(child.Value(1));
					plugin->SetDesiredState(child.Value(1));
					plugin->SetVersion(child.Size() > 2 ? child.Token(2) : "???");
				}
		}
	}

	// Keep track of plugins that are undergoing download/file operations to avoid doubling up on activity.
	mutex busyPluginsMutex;
	set<string> busyPlugins;
	std::future<std::string> FutureString(std::string value)
	{
		std::promise<std::string> p;
		p.set_value(std::move(value));
		return p.get_future();
	}

	// The maximum size of a plugin in bytes, this will be 1 GB.
	const size_t MAX_DOWNLOAD_SIZE = 1000000000;
}



// Checks if there are any dependencies of any kind.
bool Plugin::PluginDependencies::IsEmpty() const
{
	return required.empty() && optional.empty() && conflicted.empty();
}



// Checks if there are any duplicate dependencies. E.g. the same dependency in both required and conflicted.
bool Plugin::PluginDependencies::IsValid() const
{
	// We will check every dependency before returning to allow the
	// plugin developer to see all errors and not just the first.
	bool isValid = true;

	string dependencyCollisions;

	// Required dependencies will already be valid due to sets not
	// allowing duplicate values. Therefore we only need to check optional
	// and conflicts.

	// Check and log collisions between optional and required dependencies.
	for(const string &dependency : optional)
	{
		if(required.contains(dependency))
			dependencyCollisions += dependency + ", ";
	}
	if(!dependencyCollisions.empty())
	{
		dependencyCollisions.pop_back();
		dependencyCollisions.pop_back();
		Logger::Log("Dependencies named " + dependencyCollisions
			+ " were found in both the required dependencies list and the optional dependencies list.",
			Logger::Level::WARNING);
		dependencyCollisions.clear();
	}

	// Check and log collisions between conflicted and required dependencies.
	for(const string &dependency : conflicted)
	{
		if(required.contains(dependency))
		{
			isValid = false;
			dependencyCollisions += dependency + ", ";
		}
	}
	if(!dependencyCollisions.empty())
	{
		dependencyCollisions.pop_back();
		dependencyCollisions.pop_back();
		Logger::Log("Dependencies named " + dependencyCollisions
			+ " were found in both the conflicting dependencies list and the required dependencies list.",
			Logger::Level::WARNING);
		dependencyCollisions.clear();
	}

	// Check and log collisions between optional and conflicted dependencies.
	for(const string &dependency : conflicted)
	{
		if(optional.contains(dependency))
		{
			isValid = false;
			dependencyCollisions += dependency + ", ";
		}
	}
	if(!dependencyCollisions.empty())
	{
		dependencyCollisions.pop_back();
		dependencyCollisions.pop_back();
		Logger::Log("Dependencies named " + dependencyCollisions
			+ " were found in both the optional dependencies list and the conflicting dependencies list.",
			Logger::Level::WARNING);
		dependencyCollisions.clear();
	}

	return isValid;
}



// Constructs a description of the plugin from its name, tags, dependencies, etc.
string Plugin::CreateDescription() const
{
	string text;
	static const string EMPTY = "(No description given.)";
	if(!version.empty())
		text += "Version: " + version + '\n';
	if(installedVersion != version)
		text += "Installed Version: " + installedVersion + '\n';
	if(!authors.empty())
	{
		text += "Authors: ";
		for(const string &author : authors)
			text += author + ", ";
		text.pop_back();
		text.pop_back();
		text += '\n';
	}
	if(!license.empty())
		text += "License: " + license + '\n';
	if(!description.empty())
		text += description + '\n';
	if(!homepage.empty())
		// TODO: make url clickable; or copy to buffer; and forcefully wrap the text
		text += "Homepage: " + homepage + '\n';
	if(!tags.empty())
	{
		text += "Tags: ";
		for(const string &tag : tags)
			text += tag + ", ";
		text.pop_back();
		text.pop_back();
		text += '\n';
	}
	if(!dependencies.IsEmpty())
	{
		text += "Dependencies:\n";
		if(!dependencies.gameVersion.empty())
			text += "  Game Version: " + dependencies.gameVersion + '\n';
		if(!dependencies.required.empty())
		{
			text += "  Requires:\n";
			for(const string &dependency : dependencies.required)
				text += "  - " + dependency + '\n';
		}
		if(!dependencies.optional.empty())
		{
			text += "  Optional:\n";
			for(const string &dependency : dependencies.optional)
				text += "  - " + dependency + '\n';
		}
		if(!dependencies.conflicted.empty())
		{
			text += "  Conflicts:\n";
			for(const string &dependency : dependencies.conflicted)
				text += "  - " + dependency + '\n';
		}
		text += '\n';
	}

	return text;
}



string Plugin::GetIconName() const
{
	return string(name) + "-libicon";
}



bool Plugin::InUse() const
{
	return inUse;
}



string Plugin::Name() const
{
	return name;
}



// Checks whether this plugin is valid, i.e. whether it exists.
bool Plugin::IsValid() const
{
	return !name.empty();
}



bool Plugin::IsDownloading() const
{
	lock_guard<mutex> guard(busyPluginsMutex);
	return busyPlugins.contains(name);
}



bool Plugin::HasChanged() const
{
	return inUse != desiredState;
}



void Plugin::SetInUse(bool inUse)
{
	this->inUse = inUse;
}



void Plugin::SetDesiredState(bool desiredState)
{
	this->desiredState = desiredState;
}



void Plugin::SetVersion(string version)
{
	this->version = version;
}



// Attempt to load a plugin at the given path.
const Plugin *Plugins::Load(const filesystem::path &path)
{
	// For consistency between the plugin library and the installed plugins,
	// we will strip the zip extension off any zip files, or else the folder name.
	// Get the name of the folder/zip-file containing the plugin.
	string name = path.filename().string();
	name = path.stem().string();

	// TODO: make sure the same name is used on load as on install (on load we use folder/filename directly)
	// TODO: if outdated, re-get the icon
	// TODO: add find button
	filesystem::path pluginFile = path / "plugin.txt";
	string description;
	string version;
	vector<string> authors;
	set<string> tags;
	Plugin::PluginDependencies dependencies;

	// Load plugin metadata from plugin.txt.
	bool hasName = false;
	string advertisedName;
	for(const DataNode &child : DataFile(pluginFile))
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(key == "name" && hasValue)
		{
			advertisedName = child.Token(1);
			hasName = true;
		}
		else if(key == "about" && hasValue)
			description += child.Token(1) + '\n';
		else if(key == "version" && hasValue)
			version = child.Token(1);
		else if(key == "authors")
			for(const DataNode &grand : child)
				authors.emplace_back(grand.Token(0));
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
	else if(name != advertisedName)
		Logger::Log("Plugin \"name\" (" + advertisedName + ") field inside plugin.txt does not match plugin folder/zip "
			"file name stem (" + name + ").", Logger::Level::WARNING);

	// Plugin names should be unique.
	Plugin *plugin;
	auto iPlugins = GetPluginsLocked();
	plugin = iPlugins->Get(name);
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
	// Read the deprecated about.txt content if no about text was specified in plugin.txt.
	plugin->description = description.empty() ? Files::Read(path / "about.txt") : std::move(description);
	// We will use the version that we have in our plugin management `plugins.txt` file if no version is specified.
	if(!version.empty())
		plugin->version = std::move(version);
	plugin->authors = std::move(authors);
	plugin->tags = std::move(tags);
	plugin->dependencies = std::move(dependencies);

	return plugin;
}



void Plugins::LoadAvailablePlugins(TaskQueue &queue, const std::filesystem::path &pluginsJsonPath)
{
	ifstream pluginlistFile(pluginsJsonPath);
	nlohmann::json pluginInstallList = nlohmann::json::parse(pluginlistFile);
	for(const auto &pluginInstall : pluginInstallList)
	{
		string pluginName = pluginInstall["name"];
		auto iPlugins = GetPluginsLocked();
		auto *installedPlugin = iPlugins->Find(pluginName);
		bool isInstalled = installedPlugin && !installedPlugin->removed;
		string pluginVersion = pluginInstall["version"];
		vector<string> authors;
		for(const auto &author : pluginInstall["authors"])
			authors.emplace_back(author);
		// TODO: test the json structure key that doesn't exist... e.g. what if no homepage?
		// TODO: Un/Related homepage in particular could be clickable, and doesn't wrap.
		auto aPlugins = GetAvailablePluginsLocked();
		Plugin *plugin = aPlugins->Get(pluginName);
		plugin->name = pluginName;
		plugin->url = pluginInstall["url"];
		plugin->version = pluginVersion;
		plugin->description = pluginInstall["description"];
		plugin->authors = authors;
		plugin->homepage = pluginInstall["homepage"];
		plugin->license = pluginInstall["license"];
		bool isOutdated = isInstalled && installedPlugin->version != pluginVersion;
		plugin->outdated = isOutdated;
		plugin->installedVersion = installedPlugin ? installedPlugin->version : "";

		Files::CreateFolder(Files::Config() / "icons/");
		string iconPath = Files::Config() / "icons/" / (pluginName + ".png");
		if(!Files::Exists(iconPath) && pluginInstall.contains("iconUrl"))
			Download(pluginInstall["iconUrl"], iconPath);
		if(Files::Exists(iconPath))
			GameData::RequestSpriteLoad(queue, iconPath, plugin->GetIconName());
	}
}



void Plugins::LoadSettings()
{
	// Global plugin settings
	LoadSettingsFromFile(Files::Resources() / "plugins.txt");
	// Local plugin settings
	LoadSettingsFromFile(Files::Config() / "plugins.txt");
}



void Plugins::Save()
{
	Logger::Log("-Save()----------------------------------", Logger::Level::ERROR);
	auto iPlugins = GetPluginsLocked();
	if(iPlugins->empty())
		return;
	DataWriter out(Files::Config() / "plugins.txt");

	out.Write("state");
	out.BeginChild();
	{
		for(const auto &it : *iPlugins)
			if(it.second.IsValid() && !it.second.removed)
			{
				Logger::Log(it.first + ' ' + to_string(it.second.desiredState) + ' ' + it.second.version, Logger::Level::INFO);
				out.Write(it.first, it.second.desiredState, it.second.version);
			}
	}
	out.EndChild();
	Logger::Log("-Save()'ed-------------------------- ;-P", Logger::Level::ERROR);
}



// Whether the path points to a valid plugin.
bool Plugins::IsPlugin(const filesystem::path &path)
{
	// A folder is a valid plugin if it contains one (or more) of the assets folders.
	// (They can be empty too).
	return Files::Exists(path / "data") || Files::Exists(path / "images")
		|| Files::Exists(path / "shaders") || Files::Exists(path / "sounds");
}



// Returns true if the next start-up will have different plugins inUse than this one.
bool Plugins::HasChanged()
{
	// This compares current state [the set of all installed plugins] to [the set which are .inUse].
	auto iPlugins = GetPluginsLocked();
	for(const auto &it : *iPlugins)
		if(it.second.HasChanged())
			return true;
	return false;
}



bool Plugins::DownloadingInBackground()
{
	lock_guard<mutex> guard(busyPluginsMutex);
	return !busyPlugins.empty();
}



LockedSet<Plugin> Plugins::GetAvailablePluginsLocked()
{
	return {availablePluginsMutex, availablePlugins};
}



LockedSet<Plugin> Plugins::GetPluginsLocked()
{
	return {pluginsMutex, plugins};
}



// Toggles enabling or disabling a plugin for the next game restart.
void Plugins::TogglePlugin(const string &name)
{
	bool changed = false;
	{
		auto iPlugins = GetPluginsLocked();
		auto *plugin = iPlugins->Get(name);
		if(!plugin->removed)
		{
			plugin->desiredState = !plugin->desiredState;
			changed = true;
		}
		else
			plugin->desiredState = false;
	}
	// And write the current state to disk so that it's in effect on the next start. Crashes be damned.
	if(changed)
		Save();
}


future<string> Plugins::InstallOrUpdate(const std::string &name)
{
	{
		auto aPlugins = GetAvailablePluginsLocked();
		// Avoid creating data by name if it doesn't exist and still get a mutable copy Find (immutable) then Get (mutable).
		if(!aPlugins->Find(name))
		{
			Logger::Log("Plugin was selected, but then not found.", Logger::Level::ERROR);
			return FutureString("");
		}
	}

	return async(launch::async, [name]() noexcept -> string
	{
		{
			lock_guard<mutex> guard(busyPluginsMutex);
			if(!busyPlugins.insert(name).second)
				return "Download already in progress for " + name;
		}
		string url;
		{
			auto aPlugins = GetAvailablePluginsLocked();
			const Plugin *installData = aPlugins->Find(name);
			if(!installData)
			{
				string message = "Plugin was selected, but then not found.";
				Logger::Log(message, Logger::Level::ERROR);
				return message;
			}

			if(!(installData && installData->IsValid()))
				return "";

			// Check for malicous paths and bail out if there is one.
			if(installData->name.find("..") != string::npos || installData->name.find("/") != string::npos ||
					installData->name.find("\\") != string::npos)
				return "This plugin cannot be installed because the name contains unsafe path characters.";
			url = installData->url;
		}

		string zipLocation = Files::UserPlugins() / (name + ".zip");
		// Don't hold the lock during the download.
		bool success = Download(url, zipLocation);
		string message;
		if(success)
		{
			auto aPlugins = GetAvailablePluginsLocked();
			Plugin *installData = aPlugins->Get(name);
			// Create a new entry for the plugin.
			Plugin *newPlugin;
			auto iPlugins = GetPluginsLocked();
			newPlugin = iPlugins->Get(installData->name);
			newPlugin->name = installData->name;
			newPlugin->authors = installData->authors;
			newPlugin->homepage = installData->homepage;
			newPlugin->license = installData->license;
			newPlugin->description = installData->description;
			newPlugin->path = zipLocation;
			newPlugin->version = installData->version;
			// Even if this is an update, this new version is not yet in use:
			// TODO: there is a case where the installed plugins list already has this plugin installed
			//  the fact there are changes to the plugin needs to be reflected in the restart required logic
			//  need to rethink and how many and which booleans we need and where they should change.
			newPlugin->inUse = false;
			newPlugin->desiredState = true;
			// Delete and install need to clearly reset the information in the list of available plugins appropriately.
			installData->outdated = false;
			installData->installedVersion = installData->version;
		}
		else
			message = "Could not download and install/update '" + name + "' plugin.";

		{
			lock_guard<mutex> guard(busyPluginsMutex);
			busyPlugins.erase(name);
		}
		return message;
	});
}



string Plugins::DeletePlugin(const std::string &name)
{
	string path;
	{
		auto iPlugins = GetPluginsLocked();
		Plugin *plugin = iPlugins->Get(name);
		if(plugin && plugin->IsValid())
		{
			lock_guard<mutex> guardB(busyPluginsMutex);
			if(busyPlugins.contains(name))
				return "Cannot delete " + name + "' while there related and ongoing downloads.";
			path = plugin->path;
		}
		else
			Logger::Log("Somehow the plugin that was selected isn't there any more.", Logger::Level::ERROR);
	}

	if(!path.empty())
	{
		Files::Delete(path);

		auto iPlugins = GetPluginsLocked();
		Plugin *plugin = iPlugins->Get(name);
		// Note, we will set the desired state to false as this makes HasChanged indicate restart required if inUse.
		plugin->desiredState = false;
		plugin->removed = true;

		if(Files::Exists(path))
			return "Could not delete the '" + name + "' plugin.";

		// There is no need to keep around plugins that are not in use, their deletion does not require a restart.
		if(!plugin->InUse())
			iPlugins->erase(name);
	}

	return "";
}



bool Plugins::Download(const std::string &url, const std::string &location)
{
	CURL *curl = curl_easy_init();
	if(!curl)
		return false;
#if defined _WIN32
	FILE *out = nullptr;
	_wfopen_s(&out, Utf8::ToUTF16(location).c_str(), L"wb");
#else
	FILE *out = fopen(location.c_str(), "wb");
#endif
	if(!out)
	{
		Logger::Log("Unable to write to " + location, Logger::Level::ERROR);
		return false;
	}

	// Set the url that gets downloaded.
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	// Follow redirects.
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1l);
	// How long certificates are cached.
	curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);
	// What is the maximum filesize in bytes.
	curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, MAX_DOWNLOAD_SIZE);
	// Set the write function and the output file used in the write function.
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

	CURLcode result = curl_easy_perform(curl);
	if(result != CURLE_OK)
		Logger::Log(string("curl_easy_perform() failed: ") + curl_easy_strerror(result),
			Logger::Level::ERROR);

	curl_easy_cleanup(curl);
	fclose(out);
	return result == CURLE_OK;
}
