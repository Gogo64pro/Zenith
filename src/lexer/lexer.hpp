// src/lexer/lexer.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "../ast/ASTNode.hpp"

namespace zenith{
	enum class TokenType {
		// Keywords
		LET, VAR, FUN, UNSAFE, CLASS, STRUCT, UNION,
		PUBLIC, PRIVATE, PROTECTED, PRIVATEW, PROTECTEDW,CONST,
		IMPORT, PACKAGE, JAVA, EXTERN, NEW, HOIST,IF,FOR,WHILE,RETURN,ELSE,/*ELIF,*/DO,

		// Types
		INT, LONG, SHORT, BYTE, FLOAT, DOUBLE,
		STRING, DYNAMIC, FREEOBJ, NUMBER, BIGINT, BIGNUMBER,

		// Literals
		IDENTIFIER, INTEGER, FLOAT_LIT, STRING_LIT,TRUE,FALSE,NULL_LIT,TEMPLATE_LIT,TEMPLATE_PART,

		// Operators
		PLUS, MINUS, STAR, SLASH, PERCENT,
		EQUAL, EQUAL_EQUAL, BANG_EQUAL,
		LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,BANG,
		PLUS_EQUALS, MINUS_EQUALS, STAR_EQUALS, SLASH_EQUALS, PERCENT_EQUALS,

		//Logical
		AND,OR,

		// Punctuation
		LBRACE, RBRACE, LPAREN, RPAREN, LBRACKET, RBRACKET, DOLLAR_LBRACE, //paren skobi, brace kudravi skobi, brackets kvadratni skobi
		COMMA, DOT, SEMICOLON, COLON, ARROW, LAMBARROW, BACKTICK,

		// Special
		AT, // For annotations
		THIS,
		EOF_TOKEN
	};

	struct Token {
		TokenType type;
		std::string lexeme;
		SourceLocation loc;
		Token(TokenType type, std::string lexeme, SourceLocation loc): type(type), lexeme(std::move(lexeme)), loc(loc) {}
		Token(TokenType type, std::string lexeme, size_t line, size_t column, size_t length)
				: type(type), lexeme(std::move(lexeme)),
				  loc{line, column, length, 0} {}  // fileOffset=0
	};

	class Lexer {
	public:
		Lexer(const std::string& source, const std::string& name);
		std::vector<Token> tokenize() && ;
		static std::string tokenToString(TokenType type);
		size_t tokenStart = 0;
		size_t startColumn = 1;
	private:
		char advance();
		bool match(char expected);
		void addToken(TokenType type);
		void scanToken();
		void identifier();
		void number();
		void string();
		void templateString();
		bool isAtEnd() const;
		char peekNext() const;
		char peek() const;

		const std::string& source;
		const std::string& fileName;
		std::vector<Token> tokens;
		size_t start = 0;
		size_t current = 0;
		size_t line = 1;
		size_t column = 1;

		static const std::unordered_map<std::string, TokenType> keywords;

	};
}
