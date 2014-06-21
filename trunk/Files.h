/* Files.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FILES_H_
#define FILES_H_

#include <string>
#include <vector>



class Files {
public:
	static void Init(const char * const *argv);
	
	static const std::string &Resources();
	static const std::string &Config();
	
	static const std::string &Data();
	static const std::string &Images();
	static const std::string &Saves();
	
	static std::vector<std::string> List(const std::string &directory);
	static void List(const std::string &directory, std::vector<std::string> *list);
	
	static std::vector<std::string> RecursiveList(const std::string &directory);
	static void RecursiveList(const std::string &directory, std::vector<std::string> *list);
	
	static bool Exists(const std::string &filePath);
	static void Copy(const std::string &from, const std::string &to);
	static void Move(const std::string &from, const std::string &to);
	static void Delete(const std::string &filePath);
};



#endif
