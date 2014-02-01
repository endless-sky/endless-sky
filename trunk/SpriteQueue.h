/* SpriteQueue.h
Michael Zahniser, 24 Oct 2013

Class for queuing up a list of sprites to be loaded from the disk, with a set of
worker threads that begins loading them as soon as they are added.
*/

#ifndef SPRITE_QUEUE_H_INCLUDED
#define SPRITE_QUEUE_H_INCLUDED

#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class Mask;
class SDL_Surface;
class Sprite;



class SpriteQueue {
public:
	SpriteQueue();
	~SpriteQueue();
	
	// Add a sprite to load.
	void Add(const std::string &name, const std::string &path);
	// Find out our percent completion.
	double Progress() const;
	// Finish loading.
	void Finish() const;
	
	// Thread entry point.
	void operator()();
	
	static void Premultiply(SDL_Surface *surface, int additive = 0);
	
	
private:
	double DoLoad(std::unique_lock<std::mutex> &lock) const;
	
	
private:
	class Item {
	public:
		Item(Sprite *sprite, const std::string &name, const std::string &path, int frame);
		
		Sprite *sprite;
		std::string name;
		std::string path;
		SDL_Surface *surface;
		Mask *mask;
		int frame;
	};
	
	
private:
	std::queue<Item> toRead;
	// We must read the value of "added" in const functions, so this mutex must
	// be mutable.
	mutable std::mutex readMutex;
	std::condition_variable readCondition;
	int added;
	std::map<std::string, int> count;
	
	mutable std::queue<Item> toLoad;
	mutable std::mutex loadMutex;
	mutable std::condition_variable loadCondition;
	mutable int completed;
	
	std::vector<std::thread> threads;
};

#endif
