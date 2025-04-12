#include <iostream>

#include "lexer/lexer.hpp"
#include "parser/error.hpp"
#include "parser/parser.hpp"
#include "utils/mainargs.hpp"
#include "utils/ReadFile.hpp"

namespace zenith {

std::vector<lexer::Token> lex(utils::Flags &flags) {
	std::string source = utils::readFile(flags.inputFile);
	std::vector<lexer::Token> tokens;
	lexer::Lexer lexer(source, flags.inputFile);
	std::ofstream lexerOut("lexerout.log");
	try {
		tokens = std::move(lexer).tokenize();
		for (const auto &token: tokens) {
			lexerOut << "Line " << token.loc.line
			<< ":" << token.loc.column
			<< " - " << lexer::Lexer::tokenToString(token.type)
			<< " (" << token.lexeme << ")\n";
		}
	} catch (const std::exception &e) {
		lexerOut << "Lexer error: " << e.what() << std::endl;
		throw e; // awkward
	}
	std::cout << "Done Lexing \n";
	return tokens;
}

int parse(std::vector<lexer::Token> tokens, utils::Flags &flags) {
	std::ofstream parserOut("parserout.log");
	try {
		parser::Parser parser(tokens,flags,parserOut);
		parserOut << parser.parse()->toString() << std::endl;
	} catch (const parser::Error &e) {
		parserOut << "Parser error (Error): " << e.format() << std::endl;
		return 1;
	} catch (const std::exception &e) {
		parserOut << "Parser error (std::exception): " << e.what() << std::endl;
		return 1;
	} catch (...) {
		parserOut << "Parser error (unknown type)" << std::endl;
		return 1;
	}
	std::cout << "Done Parsing \n";
	return 0;
}

} // zenith

int main(int argc, char *argv[]) try {
	using namespace zenith;

	auto flags = utils::ArgumentParser::parse(argc, argv);
	if (flags.target != utils::Target::native) {
		std::cerr << "Target not set to native\nNot implemented\n";
		return 0;
	}

	return parse(lex(flags), flags);
}
catch (const std::exception &e) {
	std::cerr << "Error: " << e.what() << '\n';
	return 1;
}
catch (...) {
	std::cerr << "unknown error\n";
	return 1;
}