#include "DataSchema.h"

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



DataField Field::String(string_view key, string &target)
{
	return DataField{key, [&target](const DataNode &node){
		if(node.Size() >= 2)
			target = node.Token(1);
	}};
}
