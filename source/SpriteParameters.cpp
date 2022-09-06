/* SpriteParameters.cpp
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

#include "SpriteParameters.h"

#include "Sprite.h"

SpriteParameters::SpriteParameters()
{

}

SpriteParameters::SpriteParameters(const Sprite* sprite)
{
	auto tuple = std::tuple<const Sprite*, Indication, float>{sprite, Indication::DEFAULT_INDICATE, -1.0f};
	this->sprites.insert(std::pair<std::string, std::tuple<const Sprite*, Indication, float>>("default", tuple));
}

void SpriteParameters::SetSprite(std::string trigger, const Sprite* sprite, Indication indication, float indicatePercentage)
{
	auto tuple = std::tuple<const Sprite*, Indication, float>{sprite, indication, indicatePercentage};
	this->sprites.insert(std::pair<std::string, std::tuple<const Sprite*, Indication, float>>(trigger, tuple));
}

const Sprite *SpriteParameters::GetSprite() const
{
	auto it = this->sprites.find(this->trigger);
	return (it == this->sprites.end()) ? nullptr : std::get<0>(it->second);
}

const Sprite *SpriteParameters::GetSprite(std::string trigger) const
{
	auto it = this->sprites.find(trigger);
	return (it == this->sprites.end()) ? nullptr : std::get<0>(it->second);
}

Indication SpriteParameters::GetIndication() const
{
	auto it = this->sprites.find(this->trigger);
	return (it == this->sprites.end()) ? Indication::DEFAULT_INDICATE : std::get<1>(it->second);
}

Indication SpriteParameters::GetIndication(std::string trigger) const
{
	auto it = this->sprites.find(trigger);
	return (it == this->sprites.end()) ? Indication::DEFAULT_INDICATE : std::get<1>(it->second);
}

bool SpriteParameters::IndicateReady() const
{
	auto it = this->sprites.find(this->trigger);
	return (it == this->sprites.end() || std::get<1>(it->second) == Indication::DEFAULT_INDICATE) ? this->indicateReady : std::get<1>(it->second) != Indication::NO_INDICATE;
}

float SpriteParameters::IndicatePercentage() const
{
	auto it = this->sprites.find(this->trigger);
	auto defIt = this->sprites.find("default");
	float defaultIndicatePercentage = std::get<2>(defIt->second);
	return std::get<2>(it->second) <= 0.0f ? (defaultIndicatePercentage <= 0.0f ? 1.0f : defaultIndicatePercentage) : std::get<2>(it->second);
}

void SpriteParameters::SetTrigger(std::string trigger)
{
	this->trigger = trigger;
}

bool SpriteParameters::IsTrigger(std::string trigger) const
{
	auto it = this->sprites.find(trigger);
	return it != this->sprites.end();
}

const std::map<std::string, std::tuple<const Sprite*, Indication, float>> *SpriteParameters::GetAllSprites() const
{
	return &this->sprites;
}
