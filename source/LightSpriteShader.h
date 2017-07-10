/* LightSpriteShader.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LIGHT_SPRITE_SHADER_H_
#define LIGHT_SPRITE_SHADER_H_

class Point;
class Sprite;

#include <cstdint>


//class used to draw Sprits with light effect
//the luminosity decrease if the distance between
//the object and the light sources increase
class LightSpriteShader{
public:
	static const float DEF_AMBIENT[3];

	// Initialize the shaders.
	static void Init();
	
	static void Bind();

	static void Add(uint32_t tex0, uint32_t tex1, const float position[2], const float transform[4], int swizzle, float clip, float fade, const float blur[2],
		const float posGS[2], const float transformGS[2],
		int nbLights = -1, const float lightAmbiant[3]=DEF_AMBIENT, const float *lightPos = 0, const float *lightEmit = 0, float angCoeff = 0.f,
		float selfLight = 0.f, uint32_t texL = 0);

	static void Unbind();

	static bool IsAvailable();
	
	static int MaxNbLights();
};


#endif
