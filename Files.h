//
//  Directories.h
//  Test
//
//  Created by Diagnostic Vision on 6/18/14.
//
//

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
