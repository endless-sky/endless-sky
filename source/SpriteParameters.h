/* SpriteParameters.h
Copyright (c) 2022 by XessWaffle

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SPRITE_PARAMETERS_H
#define SPRITE_PARAMETERS_H

#include <string>
#include <map>
#include <tuple>

class Sprite;

// Used specifically for trigger based sprites
enum Indication{INDICATE, NO_INDICATE, DEFAULT_INDICATE};

// Class holding all of the animation parameters required to animate a sprite;
class SpriteParameters
{

public:
	SpriteParameters();
	explicit SpriteParameters(const Sprite *sprite);

	// Add a sprite-trigger mapping
	void SetSprite(std::string trigger, const Sprite *sprite, Indication indication, float indicatePercentage);
	// Get the sprite associated with the current trigger
	const Sprite *GetSprite() const;
	const Sprite *GetSprite(std::string trigger) const;
	// Get the indication associated with the current trigger
	Indication GetIndication() const;
	Indication GetIndication(std::string trigger) const;
	// Does the selected sprite want to indicate before doing the action
	bool IndicateReady() const;
	float IndicatePercentage() const;

	// Set the current trigger
	void SetTrigger(std::string trigger);
	bool IsTrigger(std::string trigger) const;

	// Used for saving the sprites.
	const std::map<std::string, std::tuple<const Sprite*, Indication, float>> *GetAllSprites() const;

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
	bool randomizeStart = false;
	bool repeat = true;
	bool rewind = false;

	// Defines what to do when a state transition is requested (eg. FLYING to LANDING)
	bool transitionFinish = false;
	bool transitionRewind = false;
	bool indicateReady = false;

private:
	// Used to trigger different animations
	std::string trigger = "default";

	// Sprites to be animated
	std::map<std::string, std::tuple<const Sprite*, Indication, float>> sprites;
};

#endif
