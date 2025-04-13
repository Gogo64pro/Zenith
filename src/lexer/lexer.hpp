#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define ZENITH_X_TOKENS(X, X_KEYWORD)\
/* Keywords */\
	X_KEYWORD(LET, "let")\
	X_KEYWORD(VAR, "var")\
	X_KEYWORD(FUN, "fun")\
	X_KEYWORD(UNSAFE, "unsafe")\
	X_KEYWORD(CLASS, "class")\
	X_KEYWORD(STRUCT, "struct")\
	X_KEYWORD(UNION, "union")\
	X_KEYWORD(PUBLIC, "public")\
	X_KEYWORD(PRIVATE, "private")\
	X_KEYWORD(PROTECTED, "protected")\
	X_KEYWORD(PRIVATEW, "privatew")\
	X_KEYWORD(PROTECTEDW, "protectedw")\
	X_KEYWORD(CONST, "const")\
	X_KEYWORD(IMPORT, "import")\
	X_KEYWORD(PACKAGE, "package")\
	X_KEYWORD(JAVA, "java")\
	X_KEYWORD(EXTERN, "extern")\
	X_KEYWORD(NEW, "new")\
	X_KEYWORD(HOIST, "hoist")\
	X_KEYWORD(IF, "if")\
	X_KEYWORD(FOR, "for")\
	X_KEYWORD(WHILE, "while")\
	X_KEYWORD(RETURN, "return")\
	X_KEYWORD(ELSE, "else")\
	/*ELIF,*/\
	X_KEYWORD(DO, "do")\
/* Types */\
	X_KEYWORD(INT, "int")\
	X_KEYWORD(LONG, "long")\
	X_KEYWORD(SHORT, "short")\
	X_KEYWORD(BYTE, "byte")\
	X_KEYWORD(FLOAT, "float")\
	X_KEYWORD(DOUBLE, "double")\
	X_KEYWORD(STRING, "string")\
	X_KEYWORD(DYNAMIC, "dynamic")\
	X_KEYWORD(FREEOBJ, "freeobj")\
	X_KEYWORD(NUMBER, "Number")\
	X_KEYWORD(BIGINT, "BigInt")\
	X_KEYWORD(BIGNUMBER, "BigNumber")\
/* Literals */\
	X(IDENTIFIER)\
	X(INTEGER)\
	X(FLOAT_LIT)\
	X(STRING_LIT)\
	X_KEYWORD(TRUE, "true")\
	X_KEYWORD(FALSE, "false")\
	X_KEYWORD(NULL_LIT, "null")\
	X(TEMPLATE_LIT)\
	X(TEMPLATE_PART)\
/* Operators */\
	X(PLUS)\
	X(MINUS)\
	X(STAR)\
	X(SLASH)\
	X(PERCENT)\
	X(EQUAL)\
	X(EQUAL_EQUAL)\
	X(BANG_EQUAL)\
	X(LESS)\
	X(LESS_EQUAL)\
	X(GREATER)\
	X(GREATER_EQUAL)\
	X(BANG)\
	X(PLUS_EQUALS)\
	X(MINUS_EQUALS)\
	X(STAR_EQUALS)\
	X(SLASH_EQUALS)\
	X(PERCENT_EQUALS)\
/* Logical */\
	X(AND)\
	X(OR)\
/* Punctuation */\
	X(LBRACE)\
	X(RBRACE)\
	X(LPAREN)\
	X(RPAREN)\
	X(LBRACKET)\
	X(RBRACKET)\
	X(DOLLAR_LBRACE)\
/*paren skobi, brace kudravi skobi, brackets kvadratni skobi*/\
	X(COMMA)\
	X(DOT)\
	X(SEMICOLON)\
	X(COLON)\
	X(ARROW)\
	X(LAMBARROW)\
	X(BACKTICK)\
/* Special */\
	X(AT)\
/* For annotations */\
	X_KEYWORD(THIS, "this")\
	X(EOF_TOKEN)

namespace zenith {
	class Module;
}

namespace zenith::lexer {

enum class TokenType : std::uint8_t {
#define X(a) a,
#define X_KEYWORD(a, b) a,
	ZENITH_X_TOKENS(X, X_KEYWORD)
#undef X_KEYWORD
#undef X
};

constexpr const char* toString(TokenType tok) noexcept {
	switch (tok) {
#define X(a) case TokenType::a: return #a;
#define X_KEYWORD(a, b) case TokenType::a: return #a;
	ZENITH_X_TOKENS(X, X_KEYWORD)
#undef X_KEYWORD
#undef X
	}
	return "UNKNOWN";
}

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
	std::vector<Token> tokens;
	size_t start = 0;
	size_t current = 0;
};

} // zenith::lexer