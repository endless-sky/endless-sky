/* GameData.h
Michael Zahniser, 5 Nov 2013

Class storing all the data used in the game: sprites, data files, etc.
*/

#ifndef GAME_DATA_H_INCLUDED
#define GAME_DATA_H_INCLUDED

#include "SpriteQueue.h"

#include <map>
#include <string>



class GameData {
public:
	void BeginLoad(const char * const *argv);
	void LoadShaders();
	void Finish() const;
	
	
private:
	static void FindImages(const std::string &path, int start, std::map<std::string, std::string> &images);
	static std::string Name(const std::string &path);
	
	
private:
	SpriteQueue queue;
	std::string basePath;
};



#endif
