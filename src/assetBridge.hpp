#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "json/json.hpp"

namespace assetBridge {
	std::string readFile(const char* path) {
		std::ifstream file(path, std::ios::in | std::ios::binary);

		if (!file.is_open()) {
			std::cerr << "ERROR OPENING ASSET: " << path << "\n";
			return "";
		}

		std::stringstream buff;
		buff << file.rdbuf();

		return buff.str();
	}

	struct ModelData {
		std::vector<float> vertices;
		std::vector<uint> indices;
	};

	ModelData importJsonModel(const char* path) {
		ModelData data;
		nlohmann::json j;
		std::ifstream f(path);
		f >> j;
		
		return {
			j["vertices"].get<std::vector<float>>(),
			j["indices"].get<std::vector<uint>>()
		};
	}
}
