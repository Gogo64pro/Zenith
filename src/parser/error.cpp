//
// Created by gogop on 3/29/2025.
//

#include "error.hpp"

#include <string>

namespace zenith::parser {

std::string Error::format() const {
	// Create fresh string every time (no static)
	std::string msg = std::runtime_error::what();
	return "Error at " + std::to_string(location.line) +
	       ":" + std::to_string(location.column) +
	       " - " + msg;
}

const char* Error::what() const noexcept {
	// Use thread_local to prevent corruption
	thread_local std::string formatted;
	formatted = format();
	return formatted.c_str();
}

} // zenith::parser