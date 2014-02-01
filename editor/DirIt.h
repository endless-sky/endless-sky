/* DirIt.h
Michael Zahniser, 26 Dec 2013

Class for iterating over a directory.
*/

#ifndef DIR_IT_H_INCLUDED
#define DIR_IT_H_INCLUDED

#include <string>

#include <sys/stat.h>
#include <dirent.h>



class DirIt {
public:
	DirIt(const std::string &path);
	~DirIt();
	
	std::string Next();
	
private:
	std::string path;
	DIR *dir;
	dirent *ent;
};



#endif
