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

#pragma once

#include "ConditionSet.h"
#include "Point.h"

#include <map>
#include <string>
#include <tuple>

class Sprite;


// Class holding all of the animation parameters required to animate a sprite.
class SpriteParameters {
public:

	// Supported animation transition types from one state to another.
	enum TransitionType {
		IMMEDIATE,
		FINISH,
		REWIND,
		NUM_TRANSITIONS
	};


	// A node defining the parameters for a certain trigger sprite.
	class AnimationParameters {
	public:
		// Animation parameters as found in Body.h .
		// Frames per second.
		float frameRate = 2. / 60.;
		// FPS per second, used to increase (or decrease) rate at which animation plays.
		float rampUpRate = 0.f;
		float rampDownRate = 0.f;
		// Frame number used to track at which frame the animation starts.
		float startFrame = 0.f;
		// Scale of the frame, typically set to 1.
		float scale = 1.f;
		// Frame of animation that needs to be played in order for an action to complete.
		float indicateFrame = 0.f;
		// Delay in the animation starting in number of frames.
		int delay = 0;
		// Delay in the transition of one anim to another (e.g FIRING anim to FLYING anim).
		// In number of frames.
		int transitionDelay = 0;
		// The type of transition to perform.
		TransitionType transitionType = TransitionType::IMMEDIATE;
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
		// Defines whether an animation has to complete in order for a ship to perform an action
		bool indicateReady = false;
		// Center of the body.
		Point center;
	};

	using SpriteDetails = std::tuple<const Sprite*, AnimationParameters, ConditionSet>;
	using SpriteMap = std::map<int, SpriteDetails>;


public:
	SpriteParameters();
	explicit SpriteParameters(const Sprite *sprite);
	// Add a sprite-trigger mapping.
	void SetSprite(int index, const Sprite *sprite, AnimationParameters data,
		ConditionSet triggerConditions);
	// Get the data associated with the current trigger.
	const Sprite *GetSprite(int index = -1) const;
	ConditionSet GetConditions(int index = -1) const;
	AnimationParameters GetParameters(int index = -1) const;
	// Used to verify trigger transitions.
	int GetExposedID() const;
	// Check all conditions and return if one is true.
	int RequestTriggerUpdate(ConditionsStore &store);
	// Complete the switch to a new sprite.
	void CompleteTriggerRequest();
	// Used for saving the sprites.
	const SpriteMap *GetAllSprites() const;
	// Used to get the exposed frame.
	AnimationParameters *GetExposedParameters();


private:
	void Expose(int index);

private:
	// Sprites to be animated.
	SpriteMap sprites;
	SpriteDetails exposedDetails;
	SpriteDetails defaultDetails;
	// Animation parameters exposed to Body.
	AnimationParameters exposed;
	int exposedIndex = 0;
	int requestedIndex = 0;
};

