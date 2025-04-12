//
// Created by gogop on 3/30/2025.
//

#include "ReadFile.hpp"

namespace zenith::utils {

std::string readFile(const std::string& filePath){
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filePath);
	}

	// Read the entire file into a string
	std::string content(
			(std::istreambuf_iterator<char>(file)),
			(std::istreambuf_iterator<char>())
	);

	return content;
}

} // zenith::utils