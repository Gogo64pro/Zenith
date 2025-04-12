#pragma once

#include <stdexcept>
#include "../ast/ASTNode.hpp"

namespace zenith::parser {

class ParseError : public std::runtime_error {
public:
	ast::SourceLocation location;
	ParseError(ast::SourceLocation loc, const std::string& msg)
			: std::runtime_error(msg), location(loc) {}
	const char* what() const noexcept override;
	std::string format() const;
};

} // zenith::parser
