#pragma once

#include <stdexcept>
#include <utility>
#include "../ast/AST.hpp"
namespace zenith {

	class ParseError : public std::runtime_error {
	public:
		SourceLocation location;
		ParseError(SourceLocation loc, const std::string& msg)
				: std::runtime_error(msg), location(std::move(loc)) {}
		[[nodiscard]] const char* what() const noexcept override;
		[[nodiscard]] std::string format() const;
	};

} // zenith
