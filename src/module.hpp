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

public:
	explicit Module(utils::Flags flags, std::ostream& errStream);

	void load();
	void lex();
	void parse();

	void report(lexer::SourceSpan srcSpan, const char*      message, std::string_view errorType = "error") { return report(srcSpan, std::string_view{message, strlen(message)}, errorType); }
	void report(lexer::SourceSpan srcSpan, std::string_view message, std::string_view errorType = "error");

	std::string_view getSourceLine(const ast::SourceLocation& loc);

	ast::SourceLocation getSourceLocation(size_t beg, size_t end) const;
	ast::SourceLocation getSourceLocation(lexer::Token tok) const;
	ast::SourceLocation getSourceLocation(lexer::SourceSpan loc) const { return getSourceLocation(loc.beg, loc.end); }

	std::string_view getLexeme(size_t beg, size_t end) const;
	std::string_view getLexeme(lexer::Token tok) const;
	std::string_view getLexeme(lexer::SourceSpan s) const { return getLexeme(s.beg, s.end); }
};

} // zenith