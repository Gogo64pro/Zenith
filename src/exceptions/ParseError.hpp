#pragma once

#include <stdexcept>
#include "../core/ASTNode.hpp"

namespace zenith {

	class ParseError : public std::runtime_error {
	public:
		SourceLocation location;
		ParseError(SourceLocation loc, const std::string& msg)
				: std::runtime_error(msg), location(loc) {}
		const char* what() const noexcept override;
		std::string format() const;
	};

} // zenith
