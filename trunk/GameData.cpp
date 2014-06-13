/* GameData.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameData.h"

#include "DotShader.h"
#include "FillShader.h"
#include "FontSet.h"
#include "LineShader.h"
#include "PointerShader.h"
#include "OutlineShader.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

namespace fs = boost::filesystem;

using namespace std;



void GameData::BeginLoad(const char * const *argv)
{
	showLoad = false;
	
	// Convert all the given paths to absolute patchs ending in '/".
	vector<string> paths;
	bool printTable = false;
	for(const char * const *it = argv; *it; ++it)
	{
		if((*it)[0] == '-')
		{
			string arg = *it;
			if(arg == "-l" || arg == "--load")
				showLoad = true;
			if(arg == "-t" || arg == "--table")
				printTable = true;
			continue;
		}
		
		string path;
		if((*it)[0] == '~')
			path = getenv("HOME") + string(*it + 1);
		else
			path = *it;
		if(path.back() != '/')
			path += '/';
		
		if(fs::is_directory(path))
			paths.push_back(path);
	}
	if(paths.empty())
		paths.push_back("./");
	basePath = paths.back();
	
	// Reverse the order: we will start by reading the last directories on the
	// path, and allow them to be overridden by things in earlier ones.
	reverse(paths.begin(), paths.end());
	
	// Now, read all the images in all the path directories. For each unique
	// name, only remember one instance, letting things on the higher priority
	// paths override the default images.
	map<string, string> images;
	for(const string &base : paths)
		FindImages(base + "images/", base.size() + 7, images);
	
	// From the name, strip out any frame number, plus the extension.
	for(const auto &it : images)
		queue.Add(Name(it.first), it.second);
	
	// Iterate through the paths starting with the last directory given. That
	// is, things in folders near the start of the path have the ability to
	// override things in folders later in the path.
	for(const string &base : paths)
		FindFiles(base + "data/");
	
	// Now that all the stars are loaded, update the neighbor lists.
	for(auto &it : systems)
		it.second.UpdateNeighbors(systems);
	// And, update the ships with the outfits we've now finished loading.
	for(auto &it : ships)
		it.second.FinishLoading();
	
	if(printTable)
	{
		cout << "model" << '\t' << "cost" << '\t' << "shields" << '\t' << "hull"
			<< '\t' << "mass" << '\t' << "cargo" << '\t' << "bunks" << '\t' << "fuel"
			<< '\t' << "outfit" << '\t' << "weapon" << '\t' << "engine" << '\t'
			<< "speed" << '\t' << "accel" << '\t' << "turn" << '\n';
		for(auto &it : ships)
		{
			const Ship &ship = it.second;
			cout << ship.ModelName() << '\t';
			cout << ship.Cost() << '\t';
			
			const Outfit &attributes = ship.Attributes();
			cout << attributes.Get("shields") << '\t';
			cout << attributes.Get("hull") << '\t';
			cout << attributes.Get("mass") << '\t';
			cout << attributes.Get("cargo space") << '\t';
			cout << attributes.Get("bunks") << '\t';
			cout << attributes.Get("fuel capacity") << '\t';
			cout << attributes.Get("outfit space") << '\t';
			cout << attributes.Get("weapon capacity") << '\t';
			cout << attributes.Get("engine capacity") << '\t';
			cout << 60. * attributes.Get("thrust") / attributes.Get("drag") << '\t';
			cout << 3600. * attributes.Get("thrust") / attributes.Get("mass") << '\t';
			cout << 60. * attributes.Get("turn") / attributes.Get("mass") << '\n';
		}
	}
}



void GameData::LoadShaders()
{
	FontSet::Add(basePath + "images/font/ubuntu14r.png", 14);
	FontSet::Add(basePath + "images/font/ubuntu18r.png", 18);
	
	// Make sure ~/.config/endless-sky/ exists.
	// TODO: Set the permission bits properly. Boost filesystem 1.48 does not
	// allow that, but the latest version does.
	string configPath = getenv("HOME") + string("/.config");
	if(!fs::exists(configPath))
		fs::create_directory(configPath);
	
	string prefsPath = configPath + "/endless-sky";
	if(!fs::exists(prefsPath))
		fs::create_directory(prefsPath);
	
	string savePath = prefsPath + "/saves";
	if(!fs::exists(savePath))
		fs::create_directory(savePath);
	
	// Load the key settings.
	defaultKeys.Load(basePath + "keys.txt");
	string keysPath = getenv("HOME") + string("/.config/endless-sky/keys.txt");
	keys = defaultKeys;
	keys.Load(keysPath);
	
	DotShader::Init();
	FillShader::Init();
	LineShader::Init();
	OutlineShader::Init();
	PointerShader::Init();
	SpriteShader::Init();
	
	background.Init(16384, 4096);
}



double GameData::Progress() const
{
	return queue.Progress();
}



const Set<Conversation> &GameData::Conversations() const
{
	return conversations;
}



const Set<Effect> &GameData::Effects() const
{
	return effects;
}



const Set<Fleet> &GameData::Fleets() const
{
	return fleets;
}



const Set<Government> &GameData::Governments() const
{
	return governments;
}



const Set<Interface> &GameData::Interfaces() const
{
	return interfaces;
}



const Set<Outfit> &GameData::Outfits() const
{
	return outfits;
}



const Set<Planet> &GameData::Planets() const
{
	return planets;
}



const Set<Ship> &GameData::Ships() const
{
	return ships;
}



const Set<ShipName> &GameData::ShipNames() const
{
	return shipNames;
}



const Set<System> &GameData::Systems() const
{
	return systems;
}



const vector<Trade::Commodity> &GameData::Commodities() const
{
	return trade.Commodities();
}



const StarField &GameData::Background() const
{
	return background;
}



// Get the mapping of keys to commands.
const Key &GameData::Keys() const
{
	return keys;
}



Key &GameData::Keys()
{
	return keys;
}



const Key &GameData::DefaultKeys() const
{
	return defaultKeys;
}



const string &GameData::ResourcePath() const
{
	return basePath;
}



bool GameData::ShouldShowLoad() const
{
	return showLoad;
}



void GameData::FindFiles(const string &path)
{
	if(fs::is_regular_file(path))
	{
		// This is an ordinary file. Check to see if it is an image.
		if(path.length() < 4 || path.compare(path.length() - 4, 4, ".txt"))
			return;
		
		DataFile data(path);
		
		for(const DataFile::Node &node : data)
		{
			const string &key = node.Token(0);
			if(key == "color" && node.Size() >= 6)
				colors.Get(node.Token(1))->Load(
					node.Value(2), node.Value(3), node.Value(4), node.Value(5));
			else if(key == "conversation" && node.Size() >= 2)
				conversations.Get(node.Token(1))->Load(node);
			else if(key == "effect" && node.Size() >= 2)
				effects.Get(node.Token(1))->Load(node);
			else if(key == "fleet" && node.Size() >= 2)
				fleets.Get(node.Token(1))->Load(node, *this);
			else if(key == "government" && node.Size() >= 2)
				governments.Get(node.Token(1))->Load(node, governments);
			else if(key == "interface")
				interfaces.Get(node.Token(1))->Load(node, colors);
			else if(key == "outfit" && node.Size() >= 2)
				outfits.Get(node.Token(1))->Load(node, outfits, effects);
			else if(key == "outfitter" && node.Size() >= 2)
				outfitSales.Get(node.Token(1))->Load(node, outfits);
			else if(key == "planet" && node.Size() >= 2)
				planets.Get(node.Token(1))->Load(node, shipSales, outfitSales);
			else if(key == "ship" && node.Size() >= 2)
				ships.Get(node.Token(1))->Load(node, *this);
			else if(key == "shipyard" && node.Size() >= 2)
				shipSales.Get(node.Token(1))->Load(node, ships);
			else if(key == "shipName" && node.Size() >= 2)
				shipNames.Get(node.Token(1))->Load(node);
			else if(key == "system" && node.Size() >= 2)
				systems.Get(node.Token(1))->Load(node, systems, planets, governments);
			else if(key == "trade")
				trade.Load(node);
		}
	}
	else if(fs::is_directory(path))
	{
		fs::directory_iterator it(path);
		for(fs::directory_iterator end; it != end; ++it)
			FindFiles(it->path().string());
	}
}



void GameData::FindImages(const string &path, int start, map<string, string> &images)
{
	if(fs::is_regular_file(path) && path.length() >= 4)
	{
		bool isJpg = !path.compare(path.length() - 4, 4, ".jpg");
		bool isPng = !path.compare(path.length() - 4, 4, ".png");
		
		// This is an ordinary file. Check to see if it is an image.
		if(isJpg || isPng)
			images[path.substr(start)] = path;
	}
	else if(fs::is_directory(path))
	{
		fs::directory_iterator it(path);
		for(fs::directory_iterator end; it != end; ++it)
			FindImages(it->path().string(), start, images);
	}
}



string GameData::Name(const string &path)
{
	// The path always ends in a three-letter extension, ".png" or ".jpg".
	int end = path.length() - 4;
	while(end--)
		if(path[end] < '0' || path[end] > '9')
			break;
	
	// This should never happen, but just in case someone creates a file named
	// "images/123.jpg" or something:
	if(end < 0)
		end = path.length() - 4;
	else if(path[end] != '-' && path[end] != '~' && path[end] != '+')
		end = path.length() - 4;
	
	return path.substr(0, end);
}
