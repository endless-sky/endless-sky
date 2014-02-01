/* Effect.h
Michael Zahniser, 20 Jan 2014

Class representing a graphic such as an explosion which is drawn for effect but
has no impact on any other objects in the game.
*/

#ifndef EFFECT_H_INCLUDED
#define EFFECT_H_INCLUDED

#include "Angle.h"
#include "Animation.h"
#include "Point.h"



class Effect {
public:
	Effect();
	
	void Load(const DataFile::Node &node);
	// If creating a new effect, the animation and lifetime are copied,
	// but position, velocity, and angle are specific to this new effect.
	void Place(Point pos, Point vel, Angle angle);
	
	// This returns false if it is time to delete this effect.
	bool Move();
	
	// Get the projectiles characteristics, for drawing.
	const Animation &GetSprite() const;
	const Point &Position() const;
	Point Unit() const;
	
	
private:
	Animation animation;
	
	Point position;
	Point velocity;
	Angle angle;
	Angle spin;
	
	// Parameters used for randomizing spin and velocity. The random angle is
	// added to the parent angle, and then a random velocity in that direction
	// is added to the parent velocity.
	double velocityScale;
	int randomVelocity;
	double randomAngle;
	double randomSpin;
	
	int lifetime;
};



#endif
