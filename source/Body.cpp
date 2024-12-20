/* Body.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Body.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "image/Mask.h"
#include "image/MaskManager.h"
#include "pi.h"
#include "Random.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"

#include <algorithm>
#include <cmath>

using namespace std;



// Constructor, based on a Sprite.
Body::Body(const Sprite *sprite, Point position, Point velocity, Angle facing, double zoom)
	: position(position), velocity(velocity), angle(facing), zoom(zoom)
{
	SpriteParameters *spriteState = &sprites[BodyState::FLYING];
	AnimationParameters spriteAnimationParameters;
	spriteAnimationParameters.randomizeStart = true;
	spriteState->SetSprite(0, sprite, spriteAnimationParameters, {});
	anim.randomize = true;
}



// Constructor, based on the animation from another Body object.
Body::Body(const Body &sprite, Point position, Point velocity, Angle facing, double zoom)
{
	*this = sprite;
	this->position = position;
	this->velocity = velocity;
	this->angle = facing;
	this->zoom = zoom;
}



// Check that this Body has a sprite and that the sprite has at least one frame.
bool Body::HasSprite() const
{
	const Sprite *sprite = GetSprite();
	return (sprite && sprite->Frames());
}



// Check that this Body has a sprite for a specific BodyState.
bool Body::HasSpriteFor(BodyState state) const
{
	const Sprite *sprite = sprites[state].GetSprite(0);
	return (sprite && sprite->Frames());
}



const SpriteParameters *Body::GetSpriteParameters(BodyState state) const
{
	BodyState selected = state != BodyState::CURRENT ? state : currentState;
	return &sprites[selected];
}



// Access the underlying Sprite object.
const Sprite *Body::GetSprite(BodyState state) const
{
	BodyState selected = state != BodyState::CURRENT ? state : currentState;
	const SpriteParameters *spriteState = GetSpriteParameters(selected);

	// Return flying sprite if the requested state's sprite does not exist.
	const Sprite *sprite = spriteState->GetSprite();
	if(sprite && !returnDefaultSprite)
		return sprite;
	else
		return sprites[BodyState::FLYING].GetSprite();
}



Body::BodyState Body::GetState() const
{
	return currentState;
}



// Get the width of this object, in world coordinates (i.e. taking zoom and scale into account).
double Body::Width() const
{
	const Sprite *sprite = GetSprite();
	return static_cast<double>(sprite ? (.5f * zoom) * scale * sprite->Width() : 0.f);
}



// Get the height of this object, in world coordinates (i.e. taking zoom and scale into account).
double Body::Height() const
{
	const Sprite *sprite = GetSprite();
	return static_cast<double>(sprite ? (.5f * zoom) * scale * sprite->Height() : 0.f);
}



// Get the farthest a part of this sprite can be from its center.
double Body::Radius() const
{
	return .5 * Point(Width(), Height()).Length();
}



// Which color swizzle should be applied to the sprite?
int Body::GetSwizzle() const
{
	return swizzle;
}



// Get the frame index for the given time step. If no time step is given, this
// will return the frame from the most recently given step.
float Body::GetFrame(int step) const
{
	bool finishStateTransition = false;
	if(step >= 0)
		finishStateTransition = SetStep(step);
	// Choose between frame, random frame and reversed frame, based on the corresponding parameters.
	float returnFrame = 0.;
	if(anim.randomize)
		returnFrame = randomFrame;
	else if(anim.reverse)
		returnFrame = reversedFrame;
	else
		returnFrame = frame;
	// Handle finishing transition after frame selection.
	if(finishStateTransition)
		FinishStateTransition();
	return returnFrame;
}



// Get the mask for the given time step. If no time step is given, this will
// return the mask from the most recently given step.
const Mask &Body::GetMask(int step) const
{
	const Sprite *sprite = GetSprite();

	static const Mask EMPTY;
	int current = round(GetFrame());
	if(!sprite || current < 0)
		return EMPTY;

	const vector<Mask> &masks = GameData::GetMaskManager().GetMasks(sprite, Scale());

	// Assume that if a masks array exists, it has the right number of frames.
	return masks.empty() ? EMPTY : masks[current % masks.size()];
}



// Position, in world coordinates (zero is the system center).
const Point &Body::Position() const
{
	return position;
}



// Velocity, in pixels per second.
const Point &Body::Velocity() const
{
	return velocity;
}



const Point Body::Center() const
{
	return -rotatedCenter + position;
}



// Direction this Body is facing in.
const Angle &Body::Facing() const
{
	return angle;
}



// Unit vector in the direction this body is facing. This represents the scale
// and transform that should be applied to the sprite before drawing it.
Point Body::Unit() const
{
	return angle.Unit() * (.5 * Zoom());
}



// Zoom factor. This controls how big the sprite should be drawn.
double Body::Zoom() const
{
	return max(zoom, 0.f);
}



double Body::Scale() const
{
	return static_cast<double>(scale);
}



// Check if this object is marked for removal from the game.
bool Body::ShouldBeRemoved() const
{
	return shouldBeRemoved;
}



// Store the government here too, so that collision detection that is based
// on the Body class can figure out which objects will collide.
const Government *Body::GetGovernment() const
{
	return government;
}



// Load the sprite specification, including all animation attributes.
bool Body::LoadSprite(const DataNode &node)
{
	if(node.Size() < 2)
		return false;

	// Static map for loading sprites.
	static const map<const string, BodyState> SPRITE_LOAD_PARAMS = {
		{"sprite", BodyState::FLYING},
		{"sprite-flying", BodyState::FLYING},
		{"sprite-firing", BodyState::FIRING},
		{"sprite-launching", BodyState::LAUNCHING},
		{"sprite-landing", BodyState::LANDING},
		{"sprite-jumping", BodyState::JUMPING},
		{"sprite-disabled", BodyState::DISABLED},
		{"hardpoint sprite", BodyState::FLYING},
		{"flare sprite", BodyState::FLYING},
		{"reverse flare sprite", BodyState::FLYING},
		{"steering flare sprite", BodyState::FLYING}
	};

	// Static map for loading transitions
	static const map<const string, TransitionType> TRANSITION_TYPE_PARAMS = {
		{"immediate", TransitionType::IMMEDIATE},
		{"finish", TransitionType::FINISH},
		{"rewind", TransitionType::REWIND}
	};

	// Default to flying state sprite.
	BodyState state = BodyState::FLYING;
	auto flyingPair = SPRITE_LOAD_PARAMS.find(node.Token(0));
	if(flyingPair != SPRITE_LOAD_PARAMS.end())
		state = flyingPair->second;

	const Sprite *sprite = SpriteSet::Get(node.Token(1));
	SpriteParameters *spriteData = &sprites[state];
	AnimationParameters spriteAnimationParameters;
	vector<DataNode> triggerSpriteDefer;

	// The only time the animation does not start on a specific frame is if no
	// start frame is specified and it repeats. Since a frame that does not
	// start at zero starts when the game started, it does not make sense for it
	// to do that unless it is repeating endlessly.
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "trigger" && child.Size() >= 2)
		{
			// Defer loading any trigger sprites until after the main sprite is loaded.
			// Ensures that all "default" parameters are loaded first.
			triggerSpriteDefer.push_back(child);
		}
		else if(child.Token(0) == "center" && child.Size() >= 3)
			spriteAnimationParameters.center = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "frame rate" && child.Size() >= 2 && child.Value(1) >= 0.)
			spriteAnimationParameters.frameRate = child.Value(1) / 60.;
		else if(child.Token(0) == "frame time" && child.Size() >= 2 && child.Value(1) > 0.)
			spriteAnimationParameters.frameRate = 1. / child.Value(1);
		else if(child.Token(0) == "delay" && child.Size() >= 2 && child.Value(1) > 0.)
			spriteAnimationParameters.delay = child.Value(1);
		else if(child.Token(0) == "transition delay" && child.Size() >= 2 && child.Value(1) >= 0.)
			spriteAnimationParameters.transitionDelay = child.Value(1);
		else if(child.Token(0) == "scale" && child.Size() >= 2 && child.Value(1) > 0.)
			spriteAnimationParameters.scale = static_cast<float>(child.Value(1));
		else if(child.Token(0) == "start frame" && child.Size() >= 2)
		{
			spriteAnimationParameters.startFrame = static_cast<float>(child.Value(1));
			spriteAnimationParameters.startAtZero = true;
		}
		else if(child.Token(0) == "ramp" && child.Size() >= 2)
		{
			if(child.Size() == 2)
			{
				spriteAnimationParameters.rampUpRate = child.Value(1) / 3600.;
				spriteAnimationParameters.rampDownRate = child.Value(1) / 3600.;
			}
			else if(child.Size() == 3)
			{
				spriteAnimationParameters.rampUpRate = child.Value(1) / 3600.;
				spriteAnimationParameters.rampDownRate = child.Value(2) / 3600.;
			}
		}
		else if(child.Token(0) == "transition type" && child.Size() >= 2)
		{
			spriteAnimationParameters.transitionType = TransitionType::IMMEDIATE;
			auto transitionPair = TRANSITION_TYPE_PARAMS.find(child.Token(1));
			if(transitionPair != TRANSITION_TYPE_PARAMS.end())
				spriteAnimationParameters.transitionType = transitionPair->second;
		}
		else if(child.Token(0) == "no indicate")
			spriteAnimationParameters.indicateReady = false;
		else if(child.Token(0) == "indicate")
		{
			spriteAnimationParameters.indicateReady = true;
			spriteAnimationParameters.repeat = false;
			spriteAnimationParameters.startAtZero = true;
			spriteAnimationParameters.indicateFrame = 0.f;
			if(child.Size() >= 2 && child.Value(1) > 0.f)
				spriteAnimationParameters.indicateFrame = static_cast<float>(child.Value(1));
		}
		else if(child.Token(0) == "random")
			spriteAnimationParameters.randomize = true;
		else if(child.Token(0) == "random start frame")
			spriteAnimationParameters.randomizeStart = true;
		else if(child.Token(0) == "no repeat")
		{
			spriteAnimationParameters.repeat = false;
			spriteAnimationParameters.startAtZero = true;
		}
		else if(child.Token(0) == "rewind")
			spriteAnimationParameters.rewind = true;
		else if(child.Token(0) == "reverse")
			spriteAnimationParameters.reverse = true;
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}

	// Now load the trigger sprites.
	for(size_t i = 0; i < triggerSpriteDefer.size(); ++i)
	{
		DataNode node = triggerSpriteDefer[i];
		LoadTriggerSprite(node, state, i + 1);
	}
	spriteData->SetSprite(0, sprite, spriteAnimationParameters, {});

	if(scale != 1.f)
		GameData::GetMaskManager().RegisterScale(sprite, Scale());

	return true;
}



// Returns whether the trigger sprite is based on the outfit being used.
void Body::LoadTriggerSprite(const DataNode &node, BodyState state, int index)
{
	if(node.Size() < 2)
		return;

	// Static map for loading transitions
	static const map<const string, TransitionType> TRANSITION_TYPE_PARAMS = {
		{"immediate", TransitionType::IMMEDIATE},
		{"finish", TransitionType::FINISH},
		{"rewind", TransitionType::REWIND}
	};

	const Sprite *sprite = SpriteSet::Get(node.Token(1));

	SpriteParameters *spriteData = &sprites[state];
	AnimationParameters spriteAnimationParameters;
	ConditionSet conditions;

	// The only time the animation does not start on a specific frame is if no
	// start frame is specified and it repeats. Since a frame that does not
	// start at zero starts when the game started, it does not make sense for it
	// to do that unless it is repeating endlessly.
	for(const DataNode &child : node)
		if(child.Token(0) == "conditions")
		{
			conditions.Load(child);
			if(conditions.IsEmpty())
				return;
		}
		else if(child.Token(0) == "center" && child.Size() >= 3)
			spriteAnimationParameters.center = Point(child.Value(1), child.Value(2));
		else if(child.Token(0) == "frame rate" && child.Size() >= 2 && child.Value(1) >= 0.)
			spriteAnimationParameters.frameRate = child.Value(1) / 60.;
		else if(child.Token(0) == "frame time" && child.Size() >= 2 && child.Value(1) > 0.)
			spriteAnimationParameters.frameRate = 1. / child.Value(1);
		else if(child.Token(0) == "delay" && child.Size() >= 2 && child.Value(1) > 0.)
			spriteAnimationParameters.delay = child.Value(1);
		else if(child.Token(0) == "transition delay" && child.Size() >= 2 && child.Value(1) >= 0.)
			spriteAnimationParameters.transitionDelay = child.Value(1);
		else if(child.Token(0) == "scale" && child.Size() >= 2 && child.Value(1) > 0.)
			spriteAnimationParameters.scale = static_cast<float>(child.Value(1));
		else if(child.Token(0) == "start frame" && child.Size() >= 2)
		{
			spriteAnimationParameters.startFrame = static_cast<float>(child.Value(1));
			spriteAnimationParameters.startAtZero = true;
		}
		else if(child.Token(0) == "ramp" && child.Size() >= 2)
		{
			if(child.Size() == 2)
			{
				spriteAnimationParameters.rampUpRate = child.Value(1) / 3600.;
				spriteAnimationParameters.rampDownRate = child.Value(1) / 3600.;
			}
			else if(child.Size() == 3)
			{
				spriteAnimationParameters.rampUpRate = child.Value(1) / 3600.;
				spriteAnimationParameters.rampDownRate = child.Value(2) / 3600.;
			}
		}
		else if(child.Token(0) == "transition type" && child.Size() >= 2)
		{
			spriteAnimationParameters.transitionType = TransitionType::IMMEDIATE;
			auto transitionPair = TRANSITION_TYPE_PARAMS.find(child.Token(1));
			if(transitionPair != TRANSITION_TYPE_PARAMS.end())
				spriteAnimationParameters.transitionType = transitionPair->second;
		}
		else if(child.Token(0) == "no indicate")
		{
			spriteAnimationParameters.indicateReady = false;
			spriteAnimationParameters.indicateFrame = 0.f;
		}
		else if(child.Token(0) == "indicate")
		{
			spriteAnimationParameters.indicateReady = true;
			spriteAnimationParameters.repeat = false;
			spriteAnimationParameters.startAtZero = true;
			spriteAnimationParameters.indicateFrame = 0.f;
			if(child.Size() >= 2 && child.Value(1) > 0.f)
				spriteAnimationParameters.indicateFrame = static_cast<float>(child.Value(1));
		}
		else if(child.Token(0) == "random")
			spriteAnimationParameters.randomize = true;
		else if(child.Token(0) == "random start frame")
			spriteAnimationParameters.randomizeStart = true;
		else if(child.Token(0) == "no repeat")
		{
			spriteAnimationParameters.repeat = false;
			spriteAnimationParameters.startAtZero = true;
		}
		else if(child.Token(0) == "rewind")
			spriteAnimationParameters.rewind = true;
		else if(child.Token(0) == "reverse")
			spriteAnimationParameters.reverse = true;
		else
			child.PrintTrace("Skipping unrecognized attribute:");

	if(!conditions.IsEmpty())
		spriteData->SetSprite(index, sprite, spriteAnimationParameters, conditions);

	if(scale != 1.f)
		GameData::GetMaskManager().RegisterScale(sprite, Scale());
}



// Save the sprite specification, including all animation attributes.
void Body::SaveSprite(DataWriter &out, const string &tag, bool allStates) const
{
	const string tags[BodyState::NUM_STATES] = {allStates ? "sprite" : tag, "sprite-firing",
		"sprite-launching", "sprite-landing",
		"sprite-jumping", "sprite-disabled"};

	for(int i = 0; i < BodyState::NUM_STATES; i++)
	{
		SpriteParameters *spriteState = &sprites[i];
		const Sprite *sprite = spriteState->GetSprite(0);

		if(sprite)
		{
			out.Write(tags[i], sprite->Name());
			out.BeginChild();
			{
				SaveSpriteParameters(out, spriteState, 0);
				// The map of all spriteStates.
				auto spriteStateMap = *(spriteState->GetAllSprites());
				for(auto it : spriteStateMap)
					if(it.first != 0)
					{
						const Sprite *triggerSprite = spriteState->GetSprite(it.first);
						out.Write("trigger", triggerSprite->Name());
						out.BeginChild();
						{
							out.Write("conditions");
							out.BeginChild();
							{
								spriteState->GetConditions(it.first).Save(out);
							}
							out.EndChild();
							SaveSpriteParameters(out, spriteState, it.first);
						}
						out.EndChild();
					}
			}
			out.EndChild();
		}
		if(!allStates)
			return;
	}
}



void Body::SaveSpriteParameters(DataWriter &out, SpriteParameters *state, int index) const
{
	// Static map for saving transitions
	static const map<TransitionType, const string> TRANSITION_TYPE_PARAMS = {
		{TransitionType::IMMEDIATE, "immediate"},
		{TransitionType::FINISH, "finish"},
		{TransitionType::REWIND, "rewind"}
	};

	AnimationParameters exposed = state->GetParameters(index);
	if(exposed.frameRate != static_cast<float>(MIN_FRAME_RATE))
		out.Write("frame rate", exposed.frameRate * 60.);
	if(exposed.delay)
		out.Write("delay", exposed.delay);
	if(exposed.scale != 1.f)
		out.Write("scale", exposed.scale);
	if(exposed.randomizeStart)
		out.Write("random start frame");
	if(exposed.randomize)
		out.Write("random");
	if(!exposed.repeat)
		out.Write("no repeat");
	if(exposed.rewind)
		out.Write("rewind");
	if(exposed.center)
		out.Write("center", exposed.center.X(), exposed.center.Y());
	if(exposed.reverse)
		out.Write("reverse");
	if(exposed.rampUpRate || exposed.rampDownRate)
		out.Write("ramp", exposed.rampUpRate * 3600., exposed.rampDownRate * 3600.);
	if(exposed.indicateReady)
	{
		if(exposed.indicateFrame)
			out.Write("indicate", exposed.indicateFrame);
		else
			out.Write("indicate");
	}
	else
		out.Write("no indicate");
	out.Write("transition type", TRANSITION_TYPE_PARAMS.find(exposed.transitionType)->second);
	if(exposed.transitionDelay)
		out.Write("transition delay", exposed.transitionDelay);
}



// Set the sprite.
void Body::SetSprite(const Sprite *sprite, BodyState state)
{
	AnimationParameters init;
	sprites[state].SetSprite(0, sprite, init, {});
	currentStep = -1;
}



// Set the state.
void Body::SetState(BodyState state)
{
	// If the transition has a delay, ensure that rapid changes from transitions
	// which ignore the delay to transitions that don't (ignore delays)
	// keep smooth animations.
	bool delayIgnoredPreviousStep = transitionState == BodyState::JUMPING ||
		transitionState == BodyState::DISABLED ||
		transitionState == BodyState::LANDING;
	bool delayActiveCurrentStep = (state == BodyState::FLYING ||
		state == BodyState::LAUNCHING || state == BodyState::FIRING)
		&& delayed < anim.transitionDelay;
	if(state == currentState && transitionState != currentState
		&& transitionState != BodyState::TRIGGER)
	{
		// Cancel transition.
		stateTransitionRequested = false;
		// Start replaying animation from current frame.
		integratedFrame = frame;
	}
	else if(delayIgnoredPreviousStep && delayActiveCurrentStep && stateTransitionRequested)
	{
		// Start replaying animation from current frame.
		integratedFrame = frame;
	}
	else if(state != currentState)
	{
		// Set the current frame to be the rewindFrame upon first request to state transition.
		if(!stateTransitionRequested)
		{
			if(anim.transitionType == TransitionType::REWIND)
			{
				rewindFrame = frame;
				// Ensures that rewinding starts from correct frame.
				integratedFrame = rewindFrame;
			}
			stateTransitionRequested = true;
		}
	}

	transitionState = transitionState != BodyState::TRIGGER ? state : BodyState::TRIGGER;

	// If state transition has no animation needed, then immediately transition.
	if(anim.transitionType == TransitionType::IMMEDIATE && stateTransitionRequested)
		FinishStateTransition();
}



// Set the color swizzle.
void Body::SetSwizzle(int swizzle)
{
	this->swizzle = swizzle;
}



double Body::Alpha(const Point &drawCenter) const
{
	return alpha * DistanceAlpha(drawCenter);
}



double Body::DistanceAlpha(const Point &drawCenter) const
{
	if(!distanceInvisible)
		return 1.;
	double distance = (drawCenter - position).Length();
	return clamp<double>((distance - distanceInvisible) / (distanceVisible - distanceInvisible), 0., 1.);
}



bool Body::IsVisible(const Point &drawCenter) const
{
	return DistanceAlpha(drawCenter) > 0.;
}



// Set the frame rate of the sprite. This is used for objects that just specify
// a sprite instead of a full animation data structure.
void Body::SetFrameRate(float framesPerSecond)
{
	frameRate = framesPerSecond / 60.f;
}



// Add the given amount to the frame rate.
void Body::AddFrameRate(float framesPerSecond)
{
	frameRate += framesPerSecond / 60.f;
}



void Body::PauseAnimation()
{
	++pause;
}



void Body::ShowDefaultSprite(bool defaultSprite)
{
	returnDefaultSprite = defaultSprite;
}



// Check the trigger sprites for the current state and initiate state transition for that trigger.
// E.g. if FIRING a certain weapon, check if we have an animation for that weapon, and trigger it.
void Body::CheckTriggers()
{
	for(int i = 0; i < BodyState::NUM_STATES; i++)
		if(HasSpriteFor(static_cast<BodyState>(i)))
		{
			SpriteParameters *spriteState = &sprites[i];
			int triggerID = spriteState->RequestTriggerUpdate(conditions);
			if(triggerID != spriteState->GetExposedID())
			{
				// Conditions met to transition into the trigger state.
				if(i == currentState)
					SetState(BodyState::TRIGGER);
				// Not in state, no need to wait to complete request.
				else
					spriteState->CompleteTriggerRequest();
			}
		}
}



// Indicate whether the body can perform the requested action (depending on its state)
// only if a signal is needed for the action.
bool Body::ReadyForAction() const
{
	// Never ready for action if transitioning between states.
	return anim.indicateReady ? stateReady : (!stateTransitionRequested ||
		transitionState == BodyState::TRIGGER);
}

// Called when the body is ready to transition between states.
void Body::FinishStateTransition() const
{
	if(stateTransitionRequested)
	{
		integratedFrame = 0.f;
		pause = 0;
		// Current transition due to trigger.
		bool triggerTransition = transitionState == BodyState::TRIGGER;
		BodyState requestedTransitionState = triggerTransition ?
			currentState : transitionState;
		// Default to Flying sprite if requested sprite does not exist.
		BodyState trueTransitionState = sprites[requestedTransitionState].GetSprite() ?
			requestedTransitionState : BodyState::FLYING;
		SpriteParameters *transitionedState = &sprites[trueTransitionState];
		float frames = transitionedState->GetSprite()->Frames();
		// Handle hanging trigger requests, ensuring that no ongoing trigger transitions are occurring.
		if(triggerTransition)
			transitionedState->CompleteTriggerRequest();

		// Update animation parameters.
		currentState = requestedTransitionState;
		anim = *(transitionedState->GetExposedParameters());
		if(anim.rampUpRate > 0.)
			frameRate = MIN_FRAME_RATE;
		else
			frameRate = anim.frameRate;

		if(anim.indicateReady && (anim.indicateFrame == 0.f || anim.indicateFrame > frames))
			anim.indicateFrame = frames;
		// No longer need to change states.
		stateTransitionRequested = false;
		transitionState = currentState;
	}
}



// Mark this object to be removed from the game.
void Body::MarkForRemoval()
{
	shouldBeRemoved = true;
}



// Mark this object to not be removed from the game.
void Body::UnmarkForRemoval()
{
	shouldBeRemoved = false;
}



// Turn this object around its center of rotation.
void Body::Turn(double amount)
{
	angle += amount;
	if(!anim.center)
		return;

	auto RotatePointAroundOrigin = [](Point &toRotate, double radians) -> Point {
		float si = sin(radians);
		float co = cos(radians);
		float newX = toRotate.X() * co - toRotate.Y() * si;
		float newY = toRotate.X() * si + toRotate.Y() * co;
		return Point(newX, newY);
	};

	rotatedCenter = -RotatePointAroundOrigin(anim.center, (angle - amount).Degrees() * TO_RAD);

	position -= rotatedCenter;

	rotatedCenter = RotatePointAroundOrigin(rotatedCenter, Angle(amount).Degrees() * TO_RAD);

	position += rotatedCenter;
}



void Body::Turn(const Angle &amount)
{
	Turn(amount.Degrees());
}



// Set the current time step.
bool Body::SetStep(int step) const
{
	const Sprite *sprite = GetSprite();

	// If the animation is paused, reduce the step by however many frames it has
	// been paused for.
	step -= pause;

	// If the step is negative or there is no sprite, do nothing. This updates
	// and caches the mask and the frame so that if further queries are made at
	// this same time step, we don't need to redo the calculations.
	if(step == currentStep || step < 0 || !sprite || !sprite->Frames())
		return false;

	// If the sprite only has one frame, no need to animate anything.
	float frames = sprite->Frames();
	if(frames <= 1.f)
	{
		frame = 0.f;
		return false;
	}
	float lastFrame = frames - 1.f;
	// This is the number of frames per full cycle. If rewinding, a full cycle
	// includes the first and last frames once and every other frame twice.
	float cycle = (anim.rewind ? 2.f * lastFrame : frames) + anim.delay;

	// If this is the very first step, fill in some values that we could not set
	// until we knew the sprite's frame count and the starting step.
	if(anim.randomizeStart)
	{
		anim.randomizeStart = false;
		// The random offset can be a fractional frame.
		integratedFrame = static_cast<float>(Random::Real()) * cycle;
	}
	else if(anim.startAtZero)
	{
		anim.startAtZero = false;
		// Adjust integrated frame to start at the preferred start frame.
		integratedFrame = anim.startFrame;
	}

	// Clamp frameRate from previous calculations.
	if(frameRate < MIN_FRAME_RATE)
		frameRate = MIN_FRAME_RATE;

	if(frameRate > anim.frameRate)
		frameRate = anim.frameRate;

	// Figure out what fraction of the way in between frames we are. Avoid any
	// possible floating-point glitches that might result in a negative frame.
	int prevFrame = frame;
	int nextFrame = -1;
	// Integrate rampRate in order to determine frame.
	integratedFrame += frameRate * (step - currentStep);
	frame = max(0.f, integratedFrame);
	// Flagged for if a state transition has been completed this step.
	bool stateTransitionCompleted = false;
	if(!stateTransitionRequested)
	{
		// Handle any frameRate changes in the animation.
		if(anim.rampUpRate != 0.)
		{
			if(frameRate >= MIN_FRAME_RATE && frameRate <= anim.frameRate)
				frameRate += anim.rampUpRate;
		}
		else
			frameRate = anim.frameRate;
		// For when it needs to be applied to transition.
		delayed = 0.f;

		// If repeating, wrap the frame index by the total cycle time.
		if(anim.repeat)
			frame = fmod(frame, cycle);

		if(!anim.rewind)
		{
			// If not repeating, frame should never go higher than the index of the
			// final frame.
			if(!anim.repeat)
			{
				frame = min(frame, lastFrame);
				if(frame + 1 >= anim.indicateFrame)
					stateReady = anim.indicateReady;
				else
					stateReady = false;
			}
			else if(frame >= frames)
			{
				stateReady = false;
				// If in the delay portion of the loop, set the frame to zero.
				frame = 0.f;
			}
		}
		else if(frame >= lastFrame)
		{
			// In rewind mode, once you get to the last frame, count backwards.
			// Regardless of whether we're repeating, if the frame count gets to
			// be less than 0, clamp it to 0.
			frame = max(0.f, lastFrame * 2.f - frame);
			stateReady = false;
		}
		nextFrame = frame;
		if(nextFrame != prevFrame)
			randomFrame = Random::Real() * frames;
		else
			randomFrame += frame - nextFrame;
	}
	else
	{
		// Override any delay if the ship wants to jump, become disabled, or land.
		bool ignoreDelay = transitionState == BodyState::JUMPING ||
			transitionState == BodyState::DISABLED ||
			transitionState == BodyState::LANDING ||
			transitionState == BodyState::TRIGGER;

		if(delayed >= anim.transitionDelay || ignoreDelay)
		{
			if(anim.rampDownRate != 0.)
			{
				if(frameRate >= MIN_FRAME_RATE && frameRate <= anim.frameRate)
					frameRate -= anim.rampDownRate;
			}
			else
				frameRate = anim.frameRate;
			// Handle transitions.
			if(anim.transitionType == TransitionType::FINISH)
			{
				// Finish the ongoing state's animation, then transition.
				frame = min(frame, lastFrame);

				if(frame >= lastFrame)
					stateTransitionCompleted = true;
			}
			else if(anim.transitionType == TransitionType::REWIND)
			{
				// Rewind the ongoing state's animation, then transition.
				frame = max(0.f, rewindFrame * 2.f - frame);

				if(!frame)
					stateTransitionCompleted = true;
			}
			stateReady = false;
		}
		else
		{
			delayed += (step - currentStep) * anim.frameRate;
			// Maintain last frame of animation in delay.
			frame = min(frame, lastFrame);
			integratedFrame = frame;
			// Rewind frame needs to be set since transition state can change during delay period.
			rewindFrame = frame;
		}
		// Prevent any flickers from transitioning into states with randomized frames.
		randomFrame = frame;
	}
	reversedFrame = lastFrame - frame;
	currentStep = step;
	return stateTransitionCompleted;
}
