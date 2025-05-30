#include <iostream>
#include "src/lexer/lexer.hpp"
#include "src/parser/parser.hpp"
#include "src/utils/mainargs.hpp"
#include "src/exceptions/ParseError.hpp"
#include "src/utils/ReadFile.hpp"
//#include "src/SemanticAnalysis/SemanticAnalyzer.hpp"


using namespace zenith;

int main(int argc, char *argv[]) {
	auto flags = ArgumentParser::parse(argc, argv);
	if(flags.target != Target::native){
		std::cerr << "Target not set to native" << std::endl << "Not implemented" << std::endl;
		return 0;
	}


	std::string source = readFile(flags.inputFile);
	std::vector<Token> tokens;
	Lexer lexer(source, flags.inputFile);
	std::ofstream lexerOut("lexerout.log");
	try {
		tokens = std::move(lexer).tokenize();
		for (const auto &token: tokens) {
			lexerOut << "Line " << token.loc.line
			<< ":" << token.loc.column
			<< " - " << Lexer::tokenToString(token.type)
			<< " (" << token.lexeme << ")\n";
		}
	} catch (const std::exception &e) {
		lexerOut << "Lexer error: " << e.what() << std::endl;
		return 1;
	}
	std::cout << "Done Lexing \n";
	lexerOut.close();



	std::ofstream parserOut("parserout.log");
	std_P3019_modified::polymorphic<ProgramNode> programNode;
	try{
		//throw ParseError({0,0,0,0},"STEEPEST"); //Debugger test error
		Parser parser(tokens,flags,parserOut);
		programNode = parser.parse();
		parserOut << programNode->toString() << std::endl;
	}catch (const ParseError &e) {
		parserOut << "Parser error (ParseError): " << e.format() << std::endl;
		return 1;
	} catch (const std::exception &e) {
		parserOut << "Parser error (std::exception): " << e.what() << std::endl;
		return 1;
	} catch (...) {
		parserOut << "Parser error (unknown type)" << std::endl;
		return 1;
	}
	std::cout << "Done Parsing \n";

//	ErrorReporter reporter(std::cout);
//	SemanticAnalyzer semanticAnalyzer(reporter);
//	std::cout << semanticAnalyzer.analyze(*programNode).toString();

}
//	std::string source = R"(
//		class Example {
//			privatew int x;
//			public string name;
//			fun greet() {
//				IO.print("Hello, ${this.name}!");
//			}
//	})";
