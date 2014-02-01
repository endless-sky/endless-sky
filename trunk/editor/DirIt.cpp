/* DirIt.cpp
Michael Zahniser, 26 Dec 2013

Function definitions for the DirIt class.
*/

#include "DirIt.h"

using namespace std;



DirIt::DirIt(const string &path)
	: path(path), dir(nullptr), ent(nullptr)
{
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
	if(ent->d_name[0] == '.' && (ent->d_name[1] == '.' || !ent->d_name[1]))
		return Next();

	return path + ent->d_name;
}
