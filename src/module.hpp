#pragma once

#include <ostream>
#include <vector>

#include "ast/MainNodes.hpp"
#include "ast/Node.hpp"
#include "lexer/lexer.hpp"
#include "utils/mainargs.hpp"

namespace zenith {

class Module {
private:
	std::string source;
	std::vector<size_t> lineIndex;
	std::vector<lexer::Token> tokens;
	std::unique_ptr<ast::ProgramNode> root;
	utils::Flags flags;

	std::ostream& errStream;

	std::string_view getSourceLine(const ast::SourceLocation& loc);

public:
	explicit Module(utils::Flags flags, std::ostream& errStream);

	void load();
	void lex();
	void parse();

	void report(const ast::SourceLocation& loc, const char*      message, std::string_view errorType = "error") { return report(loc, std::string_view{message, strlen(message)}, errorType); }
	void report(const ast::SourceLocation& loc, std::string_view message, std::string_view errorType = "error");

	std::string_view contents();
	void clear();
};

} // zenith