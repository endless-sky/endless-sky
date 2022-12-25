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

#include "ConditionSet.h"

#include <map>
#include <string>
#include <tuple>

class Sprite;

// Class holding all of the animation parameters required to animate a sprite
class SpriteParameters {
public:
	// A node defining the parameters for a certain trigger sprite
	class AnimationParameters {
	public:
		// Act like a struct
		// Animation parameters as found in Body.h
		float frameRate = 2. / 60.;
		float rampUpRate = 0.0f;
		float rampDownRate = 0.0f;
		float startFrame = 0.f;
		float scale = 1.f;
		float indicatePercentage = -1.0f;
		int delay = 0;
		int transitionDelay = 0;

		bool startAtZero = false;
		bool randomize = false;
		bool randomizeStart = false;
		bool repeat = true;
		bool rewind = false;
		bool reverse = false;

		// Defines what to do when a state transition is requested (eg. FLYING to LANDING)
		bool transitionFinish = false;
		bool transitionRewind = false;
		bool indicateReady = false;
	};

public:
	typedef std::tuple<const Sprite*, SpriteParameters::AnimationParameters, ConditionSet> SpriteDetails;
	typedef std::map<int, SpriteParameters::SpriteDetails> SpriteMap;

public:
	SpriteParameters();
	explicit SpriteParameters(const Sprite *sprite);
	// Add a sprite-trigger mapping
	void SetSprite(int index, const Sprite *sprite,
				   SpriteParameters::AnimationParameters data, ConditionSet triggerConditions);
	// Get the data associated with the current trigger
	const Sprite *GetSprite(int index = -1) const;
	ConditionSet GetConditions(int index = -1) const;
	AnimationParameters GetParameters(int index = -1) const;
	// Used to verify trigger transitions
	const int GetExposedID() const;
	// Check all conditions and return if one is true
	const int RequestTriggerUpdate(ConditionsStore &store);
	// Complete the switch to a new sprite
	void CompleteTriggerRequest();
	// Used for saving the sprites.
	const SpriteMap *GetAllSprites() const;


public:
	// ID For default sprite
	const static int DEFAULT = 0;
	// Animation parameters exposed to Body
	SpriteParameters::AnimationParameters exposed;

private:
	void Expose(int index);

private:
	// Sprites to be animated
	SpriteParameters::SpriteMap sprites;
	SpriteParameters::SpriteDetails exposedDetails, defaultDetails;
	int exposedIndex = 0, requestedIndex = 0;
};



#endif
