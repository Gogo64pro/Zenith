//
// Created by gogop on 11/11/2025.
//
module;
#include <string>
export module zenith.sourceLocation;

export struct SourceLocation {
	size_t line;
	size_t column;
	size_t length;
	[[maybe_unused]] size_t fileOffset;
	std::string file;
};
