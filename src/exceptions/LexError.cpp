//
// Created by gogop on 3/31/2025.
//

#include "LexError.hpp"

namespace zenith {
	std::string LexError::format() const {
		std::string msg = std::runtime_error::what();
		return "Error at " + std::to_string(location.line) +
		       ":" + std::to_string(location.column) +
		       " - " + msg;
	}
	const char *LexError::what() const noexcept {
		// Use thread_local to prevent corruption
		thread_local std::string formatted;
		formatted = format();
		return formatted.c_str();
	}

} // zenith