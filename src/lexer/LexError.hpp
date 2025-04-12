//
// Created by gogop on 3/31/2025.
//

#pragma once

#include <stdexcept>
#include "../ast/ASTNode.hpp"

namespace zenith {

	class LexError : public std::runtime_error {
		public:
			SourceLocation location;
			LexError(SourceLocation loc, const std::string& msg)
					: std::runtime_error(msg), location(loc) {}
			const char* what() const noexcept override;
			std::string format() const;
	};

} // zenith
