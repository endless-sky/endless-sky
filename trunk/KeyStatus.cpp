/* KeyStatus.cpp
Michael Zahniser, 26 Oct 2013

Function definitions for the KeyStatus class.
*/

#include "KeyStatus.h"

#include <SDL/SDL.h>



KeyStatus::KeyStatus()
	: status(0), turn(0)
{
}



// Reset the key status (for example, because we're about to create a pop-up
// window that may intercept the key up for any key that we have down).
void KeyStatus::Clear()
{
	status = 0;
	turn = 0;
}



// Set to the given status.
void KeyStatus::Set(int status, int turn)
{
	this->status = status;
	this->turn = turn;
}



// Update based on SDL's tracking of the key states.
void KeyStatus::Update()
{
	int count = 0;
	const Uint8 *state = SDL_GetKeyState(&count);
	
	status = THRUST * state[SDLK_UP];
	turn = state[SDLK_RIGHT] - state[SDLK_LEFT];
	if(turn)
		status |= TURN;
	if(state[SDLK_DOWN])
		status |= BACK | TURN;
	if(state['l'])
		status |= LAND;
	if(state['h'])
		status |= HYPERSPACE;
	if(state['r'])
		status |= TARGET_NEAR;
	if(state[SDLK_TAB])
		status |= PRIMARY;
	if(state['q'])
		status |= SECONDARY;
	if(state['w'])
		status |= SELECT;
}



int KeyStatus::Status() const
{
	return status;
}



double KeyStatus::Thrust() const
{
	return (status & THRUST) ? 1. : 0.;
}



double KeyStatus::Turn() const
{
	return (turn < 0 ? -1. : turn > 0 ? 1. : 0.);
}
