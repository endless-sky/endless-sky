/* Shaders.cpp
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../source/BatchShader.h"
#include "../../source/FillShader.h"
#include "../../source/FogShader.h"
#include "../../source/LineShader.h"
#include "../../source/OutlineShader.h"
#include "../../source/PointerShader.h"
#include "../../source/RingShader.h"
#include "../../source/SpriteShader.h"

#include <vector>

using namespace std;


void BatchShader::Init() {}
void BatchShader::Add(const Sprite *sprite, bool isHighDPI, const vector<float> &data) {}
void BatchShader::Bind() {}
void BatchShader::Unbind() {}

void FillShader::Init() {}
void FillShader::Fill(const Point &center, const Point &size, const Color &color) {}

void FogShader::Init() {}
void FogShader::Draw(const Point &center, double zoom, const PlayerInfo &player) {}
void FogShader::Redraw() {}

void LineShader::Init() {}
void LineShader::Draw(const Point &from, const Point &to, float width, const Color &color) {}

void OutlineShader::Init() {}
void OutlineShader::Draw(const Sprite *sprite, const Point &pos, const Point &size, const Color &color, const Point &unit, float frame) {}

void PointerShader::Init() {}
void PointerShader::Add(const Point &center, const Point &angle, float width, float height, float offset, const Color &color) {}
void PointerShader::Bind() {}
void PointerShader::Draw(const Point &center, const Point &angle, float width, float height, float offset, const Color &color) {}
void PointerShader::Unbind() {}

void RingShader::Init() {}
void RingShader::Add(const Point &pos, float out, float in, const Color &color) {}
void RingShader::Bind() {}
void RingShader::Draw(const Point &pos, float out, float in, const Color &color) {}
void RingShader::Draw(const Point &pos, float radius, float width, float fraction, const Color &color, float dash, float startAngle) {}
void RingShader::Unbind() {}

void SpriteShader::Init(bool useShaderSwizzle) {}
void SpriteShader::Add(const Item &item, bool withBlur) {}
void SpriteShader::Bind() {}
void SpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle, float frame) {}
void SpriteShader::Unbind() {}
