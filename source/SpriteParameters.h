/* SpriteParameters.h
Copyright (c) 2024 by XessWaffle

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
#include "Point.h"

#include <map>
#include <string>
#include <tuple>

class Sprite;



// Class holding all of the animation parameters required to animate a sprite.
class SpriteParameters {
public:
	// A node defining the parameters for a certain trigger sprite.
	class AnimationParameters {
	public:
		// Animation parameters as found in Body.h .
		// Frames per second.
		float frameRate = 2. / 60.;
		// FPS per second, used to increase (or decrease) rate at which animation plays.
		float rampUpRate = 0.0f;
		float rampDownRate = 0.0f;
		// Frame number used to track at which frame the animation starts.
		float startFrame = 0.f;
		// Scale of the frame, typically set to 1.
		float scale = 1.f;
		// Percentage of animation that needs to play in order for an action to complete.
		float indicatePercentage = -1.0f;
		// Delay in the animation starting in number of frames.
		int delay = 0;
		// Delay in the transition of one anim to another (e.g FIRING anim to FLYING anim).
		// In number of frames.
		int transitionDelay = 0;
		// Used to indicate whether the animation should start at startFrame.
		bool startAtZero = false;
		// Used to indicate whether we should randomize the next frame to be played.
		bool randomize = false;
		// Used to indicate whether only the startFrame should be randomized.
		bool randomizeStart = false;
		// Used to indicate whether the animation should be looped.
		bool repeat = true;
		// Used to indicate whether the animation should be rewinded after being played forwards.
		bool rewind = false;
		// Used to indicate whether the animation should be entirely played in reverse.
		bool reverse = false;
		// Defines what to do when a state transition is requested (eg. FLYING to LANDING).
		bool transitionFinish = false;
		bool transitionRewind = false;
		bool indicateReady = false;
		// Center of the body.
		Point center;
	};



public:
	typedef std::tuple<const Sprite*, SpriteParameters::AnimationParameters, ConditionSet> SpriteDetails;
	typedef std::map<int, SpriteParameters::SpriteDetails> SpriteMap;



public:
	SpriteParameters();
	explicit SpriteParameters(const Sprite *sprite);
	// Add a sprite-trigger mapping.
	void SetSprite(int index, const Sprite *sprite, SpriteParameters::AnimationParameters data,
		ConditionSet triggerConditions);
	// Get the data associated with the current trigger.
	const Sprite *GetSprite(int index = -1) const;
	ConditionSet GetConditions(int index = -1) const;
	AnimationParameters GetParameters(int index = -1) const;
	// Used to verify trigger transitions.
	const int GetExposedID() const;
	// Check all conditions and return if one is true.
	const int RequestTriggerUpdate(ConditionsStore &store);
	// Complete the switch to a new sprite.
	void CompleteTriggerRequest();
	// Used for saving the sprites.
	const SpriteMap *GetAllSprites() const;
	// Used to get the exposed frame.
	AnimationParameters *GetExposedParameters();


public:
	// ID For default sprite.
	const static int DEFAULT = 0;

private:
	void Expose(int index);

private:
	// Sprites to be animated.
	SpriteParameters::SpriteMap sprites;
	SpriteParameters::SpriteDetails exposedDetails, defaultDetails;
	// Animation parameters exposed to Body.
	SpriteParameters::AnimationParameters exposed;
	int exposedIndex = 0, requestedIndex = 0;
};



#endif
