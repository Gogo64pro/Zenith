#include <fstream>
#include <string>

#include "module.hpp"
#include "parser/parser.hpp"
#include "utils/mainargs.hpp"
#include "utils/ReadFile.hpp"

namespace zenith {

Module::Module(utils::Flags flags, std::ostream& errStream)
	: flags(std::move(flags))
	, errStream(errStream) 
{}

void Module::load() {
	source = utils::readFile(flags.inputFile);
	lineIndex.push_back(0);
	for (size_t i = 0, size = source.size(); i < size; ++i) {
		if (source[i] == '\n')
			lineIndex.push_back(i+1);
	}
	lineIndex.push_back(source.size() + 1);
}

void Module::lex() {
	std::ofstream lexerOut("lexerout.log");
	lexer::Lexer lexer(source);
	tokens = std::move(lexer).tokenize(*this);
	for (const auto &token: tokens) {
		lexerOut << "Line " << token.loc.line
			<< ":" << token.loc.column
			<< " - " << lexer::Lexer::tokenToString(token.type)
			<< " (" << token.lexeme << ")\n";
	}
}

void Module::parse() {
	std::ofstream parserOut("parserout.log");
	parser::Parser parser(*this, tokens, flags, parserOut);
	root = parser.parse();
	parserOut << root->toString() << std::endl;
}

struct Rep {
	std::size_t n;
	char ch;

	friend std::ostream& operator<<(std::ostream& out, Rep r) {
		while (r.n--)
			out.put(r.ch);
		return out;
	}
};

template <std::unsigned_integral T>
static constexpr T digit10count(T v) noexcept {
	static_assert(sizeof(T) <= 8, "type not supported");

	if constexpr (sizeof(T) >= 8) {
		if (v >= 10000000000000000000u) { return 20; }
		if (v >= 1000000000000000000  ) { return 19; }
		if (v >= 100000000000000000   ) { return 18; }
		if (v >= 10000000000000000    ) { return 17; }
		if (v >= 1000000000000000     ) { return 16; }
		if (v >= 100000000000000      ) { return 15; }
		if (v >= 10000000000000       ) { return 14; }
		if (v >= 1000000000000        ) { return 13; }
		if (v >= 100000000000         ) { return 12; }
		if (v >= 10000000000          ) { return 11; }
	}

	if constexpr (sizeof(T) >= 4) {
		if (v >= 1000000000) { return 10; }
		if (v >= 100000000 ) { return 9; }
		if (v >= 10000000  ) { return 8; }
		if (v >= 1000000   ) { return 7; }
		if (v >= 100000    ) { return 6; }
	}

	if constexpr (sizeof(T) >= 2) {
		if (v >= 10000) { return 5; }
		if (v >= 1000 ) { return 4; }
	}

	if (v >= 100) { return 3; }
	if (v >= 10 ) { return 2; }
	return 1;
}

void Module::report(const ast::SourceLocation &loc, std::string_view message, std::string_view errorType) {
	const auto line = getSourceLine(loc);

	// Format the error message
	errStream << "\033[1m" << flags.inputFile << ":" << loc.line << ":" << loc.column << ": "
	          << "\033[1;31m" << errorType << ": \033[0m"
	          << message << "\n";

	// Show the problematic line
	errStream << "  " << loc.line << " | " << line << "\n";

	// Add the underline marker
	errStream << "  " << Rep(digit10count(loc.line), ' ')
		<< " | "
		<< Rep(std::max(1uz, loc.column) - 1, ' ') << "\033[1;31m^" << Rep(std::max(1uz, loc.length) - 1, '~') << "\033[0m\n";
}

std::string_view Module::getSourceLine(const ast::SourceLocation &loc) {
	const auto a = lineIndex[loc.line - 1];
	const auto b = lineIndex[loc.line] - 1;
	return std::string_view(source.begin() + a, source.begin() + b);
}

std::string_view Module::contents() {
	return source;
}

void Module::clear() {
	tokens.clear();
	lineIndex.clear();
	source.clear();
}

} // zenith