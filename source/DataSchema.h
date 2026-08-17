#pragma once

#include "DataNode.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>



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
};
