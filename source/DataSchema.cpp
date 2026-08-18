#include "DataSchema.h"

#include "audio/Audio.h"
#include "Body.h"
#include "Effect.h"
#include "GameData.h"
#include "audio/Sound.h"
#include "image/SpriteSet.h"

#include <algorithm>

using namespace std;



void ApplySchema(const DataNode &node, const DataSchema &schema)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		auto it = find_if(schema.begin(), schema.end(),
			[&](const DataField &f){ return key == f.key; });
		if(it != schema.end())
			it->apply(child);
	}
}



DataField Field::Double(string_view key, double &target)
{
	return DataField{key, [&target](const DataNode &node){
		if(node.Size() >= 2)
			target = node.Value(1);
	}};
}



DataField Field::EffectAmount(string_view key, map<const Effect *, double> &target)
{
	return DataField{key, [&target](const DataNode &node) {
		if(node.Size() >= 2)
		{
			double amount = (node.Size() >= 3) ? node.Value(2) : 1.;
			target[GameData::Effects().Get(node.Token(1))] += amount;
		}
	}};
}



DataField Field::EffectCount(string_view key, map<const Effect *, int> &target)
{
	return DataField{key, [&target](const DataNode &node) {
		if(node.Size() >= 2)
			++target[GameData::Effects().Get(node.Token(1))];
	}};
}



DataField Field::Int(string_view key, int &target)
{
	return DataField{key, [&target](const DataNode &node){
		if(node.Size() >= 2)
			target = static_cast<int>(node.Value(1));
	}};
}



DataField Field::Int64(string_view key, int64_t &target)
{
	return DataField{key, [&target](const DataNode &node){
		if(node.Size() >= 2)
			target = static_cast<int64_t>(node.Value(1));
	}};
}



DataField Field::SoundCount(string_view key, map<const Sound *, int> &target)
{
	return DataField{key, [&target](const DataNode &node) {
		if(node.Size() >= 2)
			++target[Audio::Get(node.Token(1))];
	}};
}



DataField Field::Sprite(string_view key, const ::Sprite* &target)
{
	return DataField{key, [&target](const DataNode &node) {
		if(node.Size() >= 2) {
			target = SpriteSet::Get(node.Token(1));
		}
	}};
}



DataField Field::SpriteList(string_view key, vector<pair<Body, int>> &target)
{
	return DataField{key, [&target](const DataNode &node) {
		if(node.Size() >= 2) {
			target.emplace_back(Body(), 1);
			target.back().first.LoadSprite(node);
		}
	}};
}



DataField Field::String(string_view key, string &target)
{
	return DataField{key, [&target](const DataNode &node){
		if(node.Size() >= 2)
			target = node.Token(1);
	}};
}
