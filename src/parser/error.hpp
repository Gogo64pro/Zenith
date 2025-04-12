#pragma once

#include <stdexcept>
#include <string_view>
#include "../ast/Node.hpp"

namespace zenith::parser {

class Error : public std::runtime_error {
public:
	ast::SourceLocation location;
	[[nodiscard]] Error(ast::SourceLocation loc, const char*        msg) : std::runtime_error(msg), location(std::move(loc)) {}
	[[nodiscard]] Error(ast::SourceLocation loc, std::string_view   msg) : Error(std::move(loc), std::string(msg)) {}
	[[nodiscard]] Error(ast::SourceLocation loc, const std::string& msg) : std::runtime_error(msg), location(std::move(loc)) {}
	const char* what() const noexcept override;
	std::string format() const;
};

} // zenith::parser
