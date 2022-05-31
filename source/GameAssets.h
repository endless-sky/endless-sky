/* GameAssets.h
Copyright (c) 2022 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_ASSETS_H_
#define GAME_ASSETS_H_

#include "SoundSet.h"
#include "SoundQueue.h"
#include "SpriteSet.h"
#include "SpriteQueue.h"
#include "UniverseObjects.h"

#include <future>
#include <map>
#include <string>
#include <vector>



// This class contains the game assets needed to play the game: The game data, the images and the sounds.
class GameAssets {
	// GameData currently is the orchestrating controller for all game definitions.
	friend class GameData;
public:
	static constexpr int None = 0x0;
	static constexpr int Debug = 0x1;
	static constexpr int OnlyData = 0x2;


public:
	GameAssets() noexcept;

	// Asset objects cannot be copied.
	GameAssets(const GameAssets &) = delete;
	GameAssets(GameAssets &&) = delete;
	GameAssets &operator=(const GameAssets &) = delete;
	GameAssets &operator=(GameAssets &&) = delete;

	// Load all the assets from the given sources.
	std::future<void> Load(const std::vector<std::string> &sources, int options = None);
	// Determine the fraction of assets read from disk.
	double GetProgress() const;

	// Check for undefined assets and emit a warning for each of them.
	void CheckReferences();


private:
	void LoadImages(const std::vector<std::string> &sources);
	void LoadSounds(const std::vector<std::string> &sources);


private:
	// The game assets.
	std::map<std::string, std::string> music;
	SoundSet sounds;
	SpriteSet sprites;
	UniverseObjects objects;

	// Helper queues to load assets.
	SoundQueue soundQueue;
	SpriteQueue spriteQueue;
};



#endif
