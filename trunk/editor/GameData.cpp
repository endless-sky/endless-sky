/* GameData.cpp
Michael Zahniser, 5 Nov 2013

Function definitions for the GameData class.
*/

#include "GameData.h"

#include "FontSet.h"
#include "SpriteSet.h"

#include "SpriteShader.h"
#include "LineShader.h"
#include "DotShader.h"

#include <algorithm>
#include <vector>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

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
	// Convert all the given paths to absolute patchs ending in '/".
	vector<string> paths;
	for(const char * const *it = argv; *it; ++it)
	{
		if((*it)[0] == '-')
			continue;
		
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
}



void GameData::LoadShaders()
{
	FontSet::Add(basePath + "images/font/ubuntu14r.png", 14);
	FontSet::Add(basePath + "images/font/ubuntu18r.png", 18);
	
	SpriteShader::Init();
	LineShader::Init();
	DotShader::Init();
}



void GameData::Finish() const
{
	while(queue.Progress() < 1.)
		usleep(100000);
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
