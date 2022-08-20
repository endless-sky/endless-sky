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

class Sprite;

// Class holding all of the animation parameters required to animate a sprite;
class SpriteParameters{

    public:
        SpriteParameters();
        explicit SpriteParameters(const Sprite* sprite);

        // Sprite to be animated
        const Sprite* sprite = nullptr;

        // Animation parameters as found in Body.h
        mutable float frameRate = 2.f / 60.f;
        mutable float scale = 1.f;
        mutable int delay = 0;

        mutable bool startAtZero = false;
        mutable bool randomize = false;
        mutable bool repeat = true;
        mutable bool rewind = false;
        
        // Defines what to do when a state transition is requested (eg. FLYING to LANDING)
        mutable bool transitionFinish = false;
        mutable bool transitionRewind = false;
};

#endif