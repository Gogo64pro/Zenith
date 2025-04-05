//
// Created by gogop on 3/31/2025.
//

#include "RemovePadding.hpp"

namespace zenith{
	std::string removePadUntilNewLine(std::string bef){
		size_t first_non_space = bef.find_first_not_of(' ');
		size_t first_newline = bef.find('\n');

		// Only remove spaces if they appear before the first newline
		if (first_non_space != std::string::npos &&
		    (first_newline == std::string::npos || first_non_space < first_newline)) {
			bef.erase(0, first_non_space);
		}
		return bef;
	}
}