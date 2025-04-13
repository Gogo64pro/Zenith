#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zenith {
	class Module;
}

namespace zenith::lexer {

enum class TokenType : std::uint8_t {
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

struct SourceSpan {
	size_t beg;
	size_t end;
};

struct Token {
	std::uint32_t kin : 8 ;
	std::uint32_t len : 24;
	std::uint32_t beg : 32;

	Token(TokenType type, SourceSpan span)
		: kin(static_cast<std::uint32_t>(type))
		, beg(span.beg)
		, len(span.end - span.beg)
	{}

	TokenType type() const { return static_cast<TokenType>(kin); }
	SourceSpan span() const { return {beg, beg + len}; }
};
static_assert(std::is_trivially_copyable_v<Token>);
static_assert(std::is_trivially_destructible_v<Token>);
static_assert(sizeof(Token) == 8);

class Lexer {
public:
	Lexer(std::string_view source);
	std::vector<Token> tokenize(Module& mod) &&;
	static std::string tokenToString(TokenType type);
	size_t startColumn = 1;
private:
	bool isAtEnd() const;
	char peekNext() const;
	char peek() const;
	char advance();
	bool match(char expected);
	void addToken(TokenType type);
	[[noreturn]] void error(const std::string& msg);
	void scanToken();

	void identifier();
	void number();
	void string();
	void templateString();

	std::string_view source;
	std::string_view fileName;
	std::vector<Token> tokens;
	size_t start = 0;
	size_t current = 0;
};

} // zenith::lexer