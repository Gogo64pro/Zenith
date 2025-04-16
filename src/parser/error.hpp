#pragma once

#include <stdexcept>
#include <string_view>
#include "../ast/Node.hpp"

namespace zenith::parser {

class Error : public std::runtime_error {
public:
	lexer::SourceSpan location;
	[[nodiscard]] Error(lexer::SourceSpan loc, const char*        msg) : std::runtime_error(msg), location(loc) {}
	[[nodiscard]] Error(lexer::SourceSpan loc, std::string_view   msg) : Error(loc, std::string(msg)) {}
	[[nodiscard]] Error(lexer::SourceSpan loc, const std::string& msg) : std::runtime_error(msg), location(loc) {}
};

} // zenith::parser
