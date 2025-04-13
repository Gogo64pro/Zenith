#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define ZENITH_X_TOKENS(X)\
/* Keywords */\
	X(LET)\
	X(VAR)\
	X(FUN)\
	X(UNSAFE)\
	X(CLASS)\
	X(STRUCT)\
	X(UNION)\
	X(PUBLIC)\
	X(PRIVATE)\
	X(PROTECTED)\
	X(PRIVATEW)\
	X(PROTECTEDW)\
	X(CONST)\
	X(IMPORT)\
	X(PACKAGE)\
	X(JAVA)\
	X(EXTERN)\
	X(NEW)\
	X(HOIST)\
	X(IF)\
	X(FOR)\
	X(WHILE)\
	X(RETURN)\
	X(ELSE)\
	/*ELIF,*/\
	X(DO)\
/* Types */\
	X(INT)\
	X(LONG)\
	X(SHORT)\
	X(BYTE)\
	X(FLOAT)\
	X(DOUBLE)\
	X(STRING)\
	X(DYNAMIC)\
	X(FREEOBJ)\
	X(NUMBER)\
	X(BIGINT)\
	X(BIGNUMBER)\
/* Literals */\
	X(IDENTIFIER)\
	X(INTEGER)\
	X(FLOAT_LIT)\
	X(STRING_LIT)\
	X(TRUE)\
	X(FALSE)\
	X(NULL_LIT)\
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
	X(THIS)\
	X(EOF_TOKEN)

namespace zenith {
	class Module;
}

namespace zenith::lexer {

enum class TokenType : std::uint8_t {
#define X(a) a,
	ZENITH_X_TOKENS(X)
#undef X
};

constexpr const char* toString(TokenType tok) noexcept {
	switch (tok) {
#define X(a) case TokenType::a: return #a;
	ZENITH_X_TOKENS(X)
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