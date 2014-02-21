/* GameData.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the GameData class.
*/

#include "GameData.h"

#include "FontSet.h"
#include "SpriteSet.h"

#include "SpriteShader.h"
#include "OutlineShader.h"
#include "LineShader.h"
#include "DotShader.h"
#include "PointerShader.h"
#include "FillShader.h"

#include <algorithm>
#include <vector>

#include <sys/stat.h>
#include <dirent.h>

using namespace std;

namespace {
	class DirIt {
	public:
		DirIt(const string &path);
		~DirIt();
		
		string Next();
		
	private:
		string path;
		DIR *dir;
		dirent *ent;
	};

	DirIt::DirIt(const string &path)
		: path(path), dir(nullptr), ent(nullptr)
	{
		if(!path.empty() && path.back() != '/')
			this->path += '/';
		
		dir = opendir(path.c_str());
		if(dir)
			ent = reinterpret_cast<dirent *>(new char[
				sizeof(dirent) - sizeof(dirent::d_name) + PATH_MAX + 1]);
	}

	DirIt::~DirIt()
	{
		if(dir)
			closedir(dir);
		delete [] reinterpret_cast<char *>(ent);
	}

	string DirIt::Next()
	{
		if(!dir)
			return string();
	
		dirent *result = nullptr;
		readdir_r(dir, ent, &result);
		if(!result)
			return string();
	
		// Do not return "." or "..".
		if(ent->d_name[0] == '.')
			return Next();
	
		return path + ent->d_name;
	}
}



void GameData::BeginLoad(const char * const *argv)
{
	showLoad = false;
	
	// Convert all the given paths to absolute patchs ending in '/".
	vector<string> paths;
	for(const char * const *it = argv; *it; ++it)
	{
		if((*it)[0] == '-')
		{
			string arg = *it;
			if(arg == "-l" || arg == "--load")
				showLoad = true;
			continue;
		}
		
		string path;
		if((*it)[0] == '~')
			path = getenv("HOME") + string(*it + 1);
		else
			path = *it;
		if(path.back() != '/')
			path += '/';
		
		struct stat buf;
		if(!stat(path.c_str(), &buf) && S_ISDIR(buf.st_mode))
			paths.push_back(path);
	}
	if(!paths.empty())
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
	
	for(const auto &it : images)
	{
		// From the name, strip out any frame number, plus the extension.
		queue.Add(Name(it.first), it.second);
	}
	
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
}



void GameData::LoadShaders()
{
	FontSet::Add(basePath + "images/font/ubuntu14r.png", 14);
	FontSet::Add(basePath + "images/font/ubuntu18r.png", 18);
	
	// Make sure ~/.config/endless-sky/ exists.
	struct stat buf;
	
	string configPath = getenv("HOME") + string("/.config");
	if(stat(configPath.c_str(), &buf))
		mkdir(configPath.c_str(), 0755);
	
	string prefsPath = configPath + "/endless-sky";
	if(stat(prefsPath.c_str(), &buf))
		mkdir(prefsPath.c_str(), 0700);
	
	// Load the key settings.
	defaultKeys.Load(basePath + "keys.txt");
	string keysPath = getenv("HOME") + string("/.config/endless-sky/keys.txt");
	keys = defaultKeys;
	keys.Load(keysPath);
	
	SpriteShader::Init();
	OutlineShader::Init();
	LineShader::Init();
	DotShader::Init();
	PointerShader::Init();
	FillShader::Init();
	
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
	struct stat buf;
	if(!stat(path.c_str(), &buf) && !S_ISDIR(buf.st_mode))
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
				ships.Get(node.Token(1))->Load(node, outfits, effects);
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
	else
	{
		DirIt it(path);
		while(true)
		{
			string next = it.Next();
			if(next.empty())
				break;
		
			FindFiles(next);
		}
	}
}



void GameData::FindImages(const string &path, int start, map<string, string> &images)
{
	struct stat buf;
	if(!stat(path.c_str(), &buf) && !S_ISDIR(buf.st_mode))
	{
		// This is an ordinary file. Check to see if it is an image.
		if(path.length() >= 4 && (!path.compare(path.length() - 4, 4, ".jpg")
				|| !path.compare(path.length() - 4, 4, ".png")))
		{
			images[path.substr(start)] = path;
		}
		return;
	}
	
	DirIt it(path);
	while(true)
	{
		string next = it.Next();
		if(next.empty())
			break;
		
		FindImages(next, start, images);
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
