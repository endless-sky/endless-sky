#pragma once

#include "DataNode.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

class Body;
class Effect;
class Sound;
class Sprite;



struct DataField {
	std::string_view key;
	std::function<void(const DataNode &)> apply;
};

using DataSchema = std::vector<DataField>;

void ApplySchema(const DataNode &node, const DataSchema &schema);

namespace Field {
	// basic field types that assign to a variable
	DataField Double(std::string_view key, double &target);
	DataField Int(std::string_view key, int &target);
	DataField Int64(std::string_view key, int64_t &target);
	DataField String(std::string_view key, std::string &target);

	// specialist fields
	DataField EffectAmount(std::string_view key, std::map<const Effect *, double> &target);
	DataField EffectCount(std::string_view key, std::map<const Effect *, int> &target);
	DataField SoundCount(std::string_view key, std::map<const Sound *, int> &target);
	DataField Sprite(std::string_view key, const ::Sprite* &target);
	DataField SpriteList(std::string_view key, std::vector<std::pair<Body, int>> &target);
};
