/* SpriteParameters.h
Copyright (c) 2022 by XessWaffle

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SPRITE_PARAMETERS_H
#define SPRITE_PARAMETERS_H

#include <string>
#include <map>

class Sprite;

// Class holding all of the animation parameters required to animate a sprite;
class SpriteParameters
{

public:
	SpriteParameters();
	explicit SpriteParameters(const Sprite *sprite);

	void SetSprite(std::string trigger, const Sprite *sprite);
	const Sprite *GetSprite(std::string trigger = "default") const;

public:
	// Act like a struct
	// Animation parameters as found in Body.h
	float frameRate = 2.f / 60.f;
	float startFrame = 0.f;
	float scale = 1.f;
	int delay = 0;
	int transitionDelay = 0;

	bool startAtZero = false;
	bool randomize = false;
	bool repeat = true;
	bool rewind = false;

	// Defines what to do when a state transition is requested (eg. FLYING to LANDING)
	bool transitionFinish = false;
	bool transitionRewind = false;
	bool indicateReady = false;

private:
	// Sprites to be animated
	std::map<std::string, const Sprite*> sprites;
};

#endif
