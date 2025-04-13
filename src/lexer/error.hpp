//
// Created by gogop on 3/31/2025.
//

#pragma once

#include <stdexcept>
#include "../ast/Node.hpp"

namespace zenith::lexer {

class Error : public std::runtime_error {
public:
	ast::SourceLocation location;
	[[nodiscard]] Error(ast::SourceLocation loc, const std::string& msg) : std::runtime_error(msg), location(std::move(loc)) {}
};

} // zenith::lexer
