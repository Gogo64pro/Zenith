#pragma once

#include <stdexcept>
#include "../ast/Node.hpp"

namespace zenith::parser {

class Error : public std::runtime_error {
public:
	ast::SourceLocation location;
	Error(ast::SourceLocation loc, const std::string& msg)
			: std::runtime_error(msg), location(loc) {}
	const char* what() const noexcept override;
	std::string format() const;
};

} // zenith::parser
