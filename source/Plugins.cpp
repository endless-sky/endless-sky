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
#include "Logger.h"
#include "Set.h"

#include <curl/curl.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
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
	mutex pluginsMutex;
	Set<Plugin> plugins;

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
					lock_guard<mutex> guard(pluginsMutex);
					auto *plugin = plugins.Get(child.Token(0));
					plugin->enabled = child.Value(1);
					plugin->currentState = child.Value(1);
					plugin->version = child.Size() > 2 ? child.Token(2) : "???";
				}
		}
	}

	bool oldNetworkActivity = false;

	mutex activePluginsMutex;
	set<string> activePlugins;

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
	if(!version.empty())
		text += "Version: " + version + '\n';
	if(!authors.empty())
	{
		text += "Authors: ";
		for(const string &author : authors)
			text += author + ", ";
		text.pop_back();
		text.pop_back();
		text += '\n';
	}
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
	if(!aboutText.empty())
		text += aboutText;

	return text;
}



// Checks whether this plugin is valid, i.e. whether it exists.
bool Plugin::IsValid() const
{
	return !name.empty();
}



// Attempt to load a plugin at the given path.
const Plugin *Plugins::Load(const filesystem::path &path)
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
	Plugin *plugin;
	{
		lock_guard<mutex> guard(pluginsMutex);
		plugin = plugins.Get(name);
	}
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



void Plugins::LoadSettings()
{
	// Global plugin settings
	LoadSettingsFromFile(Files::Resources() / "plugins.txt");
	// Local plugin settings
	LoadSettingsFromFile(Files::Config() / "plugins.txt");
}



void Plugins::Save()
{
	lock_guard<mutex> guard(pluginsMutex);
	if(plugins.empty())
		return;
	DataWriter out(Files::Config() / "plugins.txt");

	out.Write("state");
	out.BeginChild();
	{
		for(const auto &it : plugins)
			if(it.second.IsValid() && !it.second.removed)
				out.Write(it.first, it.second.currentState, it.second.version);
	}
	out.EndChild();
}



// Whether the path points to a valid plugin.
bool Plugins::IsPlugin(const filesystem::path &path)
{
	// A folder is a valid plugin if it contains one (or more) of the assets folders.
	// (They can be empty too).
	return Files::Exists(path / "data") || Files::Exists(path / "images")
		|| Files::Exists(path / "shaders") || Files::Exists(path / "sounds");
}



// Returns true if any plugin enabled or disabled setting has changed since
// launched via user preferences.
bool Plugins::HasChanged()
{
	lock_guard<mutex> guard(pluginsMutex);
	for(const auto &it : plugins)
		if(it.second.enabled != it.second.currentState || it.second.removed)
			return true;
	return oldNetworkActivity;
}



bool Plugins::IsInBackground()
{
	lock_guard<mutex> guard(activePluginsMutex);
	return !activePlugins.empty();
}



// Returns the list of plugins that have been identified by the game.
const Set<Plugin> &Plugins::Get()
{
	return plugins;
}



// Toggles enabling or disabling a plugin for the next game restart.
void Plugins::TogglePlugin(const string &name)
{
	lock_guard<mutex> guard(pluginsMutex);
	auto *plugin = plugins.Get(name);
	plugin->currentState = !plugin->currentState;
}



future<void> Plugins::Install(InstallData *installData, bool update)
{
	oldNetworkActivity = true;
	if(!update)
	{
		lock_guard<mutex> guard(activePluginsMutex);
		if(!activePlugins.insert(installData->name).second)
			return future<void>();
	}

	return async(launch::async, [installData, update]() noexcept -> void
		{
			// Check for malicous paths and bail out if there is one.
			if(installData->name.find("..") != string::npos)
				return;

			string zipLocation = Files::PluginsCache() / installData->name / ".zip";
			bool success = Download(installData->url, zipLocation);
			if(success)
			{
				// TODO: copy it over to the proper place.

				// // We need to change our working directory for the extraction to be able
				// // to check for symlinks or absolute paths without disallowing these things
				// // for the directory itself.
				// auto oldPath = std::filesystem::current_path();
				// std::filesystem::current_path(Files::PluginsCache());
				//
				// success = ExtractZIP(zipLocation, "./", installData->name + "/");
				// std::filesystem::current_path(oldPath);
				//
				// if(success)
				// {

					// Remove old version.
					if(update)
						filesystem::remove_all(Files::PluginsCache() / installData->name);

					// Create a new entry for the plugin.
					Plugin *newPlugin;
					{
						lock_guard<mutex> guard(pluginsMutex);
						newPlugin = plugins.Get(installData->name);
					}
					newPlugin->name = installData->name;
					newPlugin->aboutText = installData->aboutText;
					newPlugin->path = Files::PluginsCache() / installData->name;
					newPlugin->version = installData->version;
					newPlugin->enabled = false;
					newPlugin->currentState = true;

					installData->installed = true;
					installData->outdated = false;

				// }
				// else
				// 	filesystem::remove_all(Files::PluginsCache() / installData->name);
			}
			Files::Delete(zipLocation);
			{
				lock_guard<mutex> guard(activePluginsMutex);
				activePlugins.erase(installData->name);
			}
		});
}



future<void> Plugins::Update(InstallData *installData)
{
	{
		lock_guard<mutex> guard(activePluginsMutex);
		if(!activePlugins.insert(installData->name).second)
			return future<void>();
	}

	{
		lock_guard<mutex> guard(pluginsMutex);
		plugins.Get(installData->name)->version = installData->version;
	}

	return Install(installData, true);
}



void Plugins::DeletePlugin(const std::string &pluginName)
{
	{
		lock_guard<mutex> guard(activePluginsMutex);
		if(activePlugins.count(pluginName))
			return;
	}

	{
		lock_guard<mutex> guard(pluginsMutex);
		plugins.Get(pluginName)->removed = true;
	}

	filesystem::remove_all(Files::PluginsCache() / pluginName);
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
		return false;

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
