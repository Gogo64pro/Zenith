#include <cctype>

#include "../module.hpp"
#include "../utils/hash.hpp"
#include "error.hpp"
#include "lexer.hpp"

namespace zenith::lexer {

// Keyword map initialization
static const StringHashMap<TokenType> keywords = {
	// Keywords
	{"let", TokenType::LET},
	{"var", TokenType::VAR},
	{"fun", TokenType::FUN},
	{"unsafe", TokenType::UNSAFE},
	{"class", TokenType::CLASS},
	{"struct", TokenType::STRUCT},
	{"union", TokenType::UNION},
	{"new", TokenType::NEW},
	{"hoist", TokenType::HOIST},

	// Access modifiers
	{"public", TokenType::PUBLIC},
	{"private", TokenType::PRIVATE},
	{"protected", TokenType::PROTECTED},
	{"privatew", TokenType::PRIVATEW},
	{"protectedw", TokenType::PROTECTEDW},

	// Modules
	{"import", TokenType::IMPORT},
	{"package", TokenType::PACKAGE},
	{"extern", TokenType::EXTERN},

	// Types
	{"int", TokenType::INT},
	{"long", TokenType::LONG},
	{"short", TokenType::SHORT},
	{"byte", TokenType::BYTE},
	{"float", TokenType::FLOAT},
	{"double", TokenType::DOUBLE},
	{"string", TokenType::STRING},
	{"dynamic", TokenType::DYNAMIC},
	{"freeobj", TokenType::FREEOBJ},
	{"Number", TokenType::NUMBER},
	{"BigInt", TokenType::BIGINT},
	{"BigNumber", TokenType::BIGNUMBER},

	{"const", TokenType::CONST},
	{"java", TokenType::JAVA},
	{"if", TokenType::IF},
	{"for", TokenType::FOR},
	{"while", TokenType::WHILE},
	{"return", TokenType::RETURN},
	{"else", TokenType::ELSE},
	{"do", TokenType::DO},
	{"this", TokenType::THIS},

	// Literals
	{"true", TokenType::TRUE},
	{"false", TokenType::FALSE},
	{"null", TokenType::NULL_LIT},
};

Lexer::Lexer(std::string_view source) : source(source) {}

std::vector<Token> Lexer::tokenize(Module& mod) && {
	try {
		while (!isAtEnd()) {
			scanToken();
		}

		addToken(TokenType::EOF_TOKEN);
		return std::move(tokens);
	}
	catch (const Error& e) {
		mod.report(e.location, e.what());
	}
	return {};
}

bool Lexer::isAtEnd() const {
	return current >= source.length();
}

char Lexer::peek() const {
	if (isAtEnd()) return '\0';
	return source[current];
}

char Lexer::peekNext() const {
	if (current + 1 >= source.length()) return '\0';
	return source[current + 1];
}

char Lexer::advance() {
	return source[current++];
}

bool Lexer::match(char expected) {
	if (isAtEnd()) return false;
	if (source[current] != expected) return false;

	current++;
	return true;
}

void Lexer::addToken(TokenType type) {
	tokens.push_back({type, start, current});
	start = current;
}

void Lexer::error(const std::string& msg) {
	throw Error({current, current}, msg);
}

void Lexer::scanToken() {
	char c = advance();
	switch (c) {
		// Single-character tokens
		case '(': addToken(TokenType::LPAREN); break;
		case ')': addToken(TokenType::RPAREN); break;
		case '{': addToken(TokenType::LBRACE); break;
		case '}': addToken(TokenType::RBRACE); break;
		case '[': addToken(TokenType::LBRACKET); break;
		case ']': addToken(TokenType::RBRACKET); break;
		case ',': addToken(TokenType::COMMA); break;
		case '.': addToken(TokenType::DOT); break;
		case ';': addToken(TokenType::SEMICOLON); break;
		case ':': addToken(TokenType::COLON); break;
		case '@': addToken(TokenType::AT); break;

			// Operators
		case '+':
			if (match('=')) addToken(TokenType::PLUS_EQUALS);
			else addToken(TokenType::PLUS);
			break;
		case '-':
			if (match('>')) addToken(TokenType::ARROW);
			else if(match('=')) addToken(TokenType::MINUS_EQUALS);
			else addToken(TokenType::MINUS);
			break;
		case '*':
			if (match('=')) addToken(TokenType::STAR_EQUALS);
			else addToken(TokenType::STAR);
			break;
		case '/':
			if (match('/')) {
				// Line comment
				while (peek() != '\n' && !isAtEnd()) advance();
			} else if (match('*')) {
				// Block comment
				while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
					advance();
				}
				if (isAtEnd()) {
					error("Unterminated block comment");
				}
				// Consume the '*/'
				advance();
				advance();
			} else if(match('=')) addToken(TokenType::SLASH_EQUALS); else {
				addToken(TokenType::SLASH);
			}
			break;
		case '%':
			if(match('=')) addToken(TokenType::PERCENT_EQUALS);
			else addToken(TokenType::PERCENT); break;
			// Comparisons
		case '=':
			if(match('=')) addToken(TokenType::EQUAL_EQUAL);
			else if (match('>')) addToken(TokenType::LAMBARROW);
			else addToken(TokenType::EQUAL);
			break;
		case '!':
			addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
			break;
		case '&':
			if (match('&')) addToken(TokenType::AND);
			else error("Unexpected character: &");
			break;
		case '|':
			if (match('|')) addToken(TokenType::OR);
			else error("Unexpected character: |");
			break;
		case '<':
			addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
			break;
		case '>':
			addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
			break;

			// Whitespace
		case ' ':
		case '\r':
		case '\t':
		case '\n':
			break;

			// String literals
		case '"': string(); break;
		case '`': templateString(); break;
		case '$':
			if (peek() == '{') {
				advance(); // consume '{'
				addToken(TokenType::DOLLAR_LBRACE);
			} else {
				identifier(); // or handle as regular identifier
			}
			break;
		default:
			if (isdigit(c)) {
				number();
			} else if (isalpha(c) || c == '_') {
				identifier();
			} else {
				error("Unexpected character: " + std::string(1, c));
			}
			break;
	}
	start = current;
}

void Lexer::string() {
	while (peek() != '"' && !isAtEnd()) {
		advance();
	}

	if (isAtEnd()) error("Unterminated string");

	advance();  // Consume closing "
	addToken(TokenType::STRING_LIT);
}

// todo: pretty sure I've screwed this up, but I don't know the spec
void Lexer::templateString() {
	// Add opening backtick
	addToken(TokenType::BACKTICK);
	advance(); // Consume opening backtick
	start = current; // Start of actual template content

	while (peek() != '`' && !isAtEnd()) {
		if (peek() == '\\') {
			// Handle escape sequences...
		}
		else if (peek() == '$' && peekNext() == '{') {
			// Handle interpolation
			if (current > start) {
				addToken(TokenType::TEMPLATE_PART);
			}
			advance(); advance();
			addToken(TokenType::DOLLAR_LBRACE);
			return;  // Return to let parser handle interpolation
		}
		else {
			advance();
		}
	}

	// Handle closing
	if (isAtEnd()) {
		error("Unterminated template string");
	}

	// Add final template part (if any)
	if (current > start) {
		addToken(TokenType::TEMPLATE_PART);
	}

	// Add closing backtick
	addToken(TokenType::BACKTICK);
	advance();  // Consume closing backtick
	start = current;
}

void Lexer::number() {
	bool isFloat = false;

	while (isdigit(peek())) advance();

	// Look for a fractional part
	if (peek() == '.' && isdigit(peekNext())) {
		isFloat = true;
		advance(); // Consume the .

		while (isdigit(peek())) advance();
	}

	// Handle scientific notation
	if ((peek() == 'e' || peek() == 'E') &&
	    (isdigit(peekNext()) || peekNext() == '+' || peekNext() == '-')) {
		isFloat = true;
		advance(); // Consume the e/E

		if (peek() == '+' || peek() == '-') advance();

		while (isdigit(peek())) advance();
	}

	// Handle type suffixes
	if (isFloat) {
		if (peek() == 'f' || peek() == 'F') {
			advance(); // Consume the f
		}
		addToken(TokenType::FLOAT_LIT);
	} else {
		if (peek() == 'l' || peek() == 'L') {
			advance(); // Consume the l
		}
		addToken(TokenType::INTEGER);
	}
}

void Lexer::identifier() {
	while (isalnum(peek()) || peek() == '_') advance();

	// Check if it's a keyword
	const auto text = std::string_view(source.begin() + start, source.begin() + current);
	auto it = keywords.find(text);
	if (it != keywords.end()) {
		addToken(it->second);
	} else {
		addToken(TokenType::IDENTIFIER);
	}
}

std::string Lexer::tokenToString(TokenType type) {
	switch (type) {
		// Keywords
		case TokenType::LET: return "LET";
		case TokenType::VAR: return "VAR";
		case TokenType::FUN: return "FUN";
		case TokenType::UNSAFE: return "UNSAFE";
		case TokenType::CLASS: return "CLASS";
		case TokenType::STRUCT: return "STRUCT";
		case TokenType::UNION: return "UNION";
		case TokenType::NEW: return "NEW";
		case TokenType::HOIST: return "HOIST";

			// Access modifiers
		case TokenType::PUBLIC: return "PUBLIC";
		case TokenType::PRIVATE: return "PRIVATE";
		case TokenType::PROTECTED: return "PROTECTED";
		case TokenType::PRIVATEW: return "PRIVATEW";
		case TokenType::PROTECTEDW: return "PROTECTEDW";

			// Modules
		case TokenType::IMPORT: return "IMPORT";
		case TokenType::PACKAGE: return "PACKAGE";
		case TokenType::EXTERN: return "EXTERN";

			// Types
		case TokenType::INT: return "INT";
		case TokenType::LONG: return "LONG";
		case TokenType::SHORT: return "SHORT";
		case TokenType::BYTE: return "BYTE";
		case TokenType::FLOAT: return "FLOAT";
		case TokenType::DOUBLE: return "DOUBLE";
		case TokenType::STRING: return "STRING";
		case TokenType::DYNAMIC: return "DYNAMIC";
		case TokenType::FREEOBJ: return "FREEOBJ";
		case TokenType::NUMBER: return "NUMBER";
		case TokenType::BIGINT: return "BIGINT";
		case TokenType::BIGNUMBER: return "BIGNUMBER";

			// Literals
		case TokenType::IDENTIFIER: return "IDENTIFIER";
		case TokenType::INTEGER: return "INTEGER";
		case TokenType::FLOAT_LIT: return "FLOAT_LIT";
		case TokenType::STRING_LIT: return "STRING_LIT";
		case TokenType::TRUE: return "TRUE";
		case TokenType::FALSE: return "FALSE";
		case TokenType::NULL_LIT: return "NULL_LIT";

			// Operators
		case TokenType::PLUS: return "PLUS";
		case TokenType::MINUS: return "MINUS";
		case TokenType::STAR: return "STAR";
		case TokenType::SLASH: return "SLASH";
		case TokenType::PERCENT: return "PERCENT";
		case TokenType::EQUAL: return "EQUAL";
		case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
		case TokenType::BANG_EQUAL: return "BANG_EQUAL";
		case TokenType::LESS: return "LESS";
		case TokenType::LESS_EQUAL: return "LESS_EQUAL";
		case TokenType::GREATER: return "GREATER";
		case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";

			// Punctuation
		case TokenType::LBRACE: return "LBRACE";
		case TokenType::RBRACE: return "RBRACE";
		case TokenType::LPAREN: return "LPAREN";
		case TokenType::RPAREN: return "RPAREN";
		case TokenType::LBRACKET: return "LBRACKET";
		case TokenType::RBRACKET: return "RBRACKET";
		case TokenType::DOLLAR_LBRACE: return "DOLLAR_LBRACE";
		case TokenType::COMMA: return "COMMA";
		case TokenType::DOT: return "DOT";
		case TokenType::SEMICOLON: return "SEMICOLON";
		case TokenType::COLON: return "COLON";
		case TokenType::ARROW: return "ARROW";
		case TokenType::LAMBARROW: return "LAMBARROW";

			// Special
		case TokenType::AT: return "AT";
		case TokenType::EOF_TOKEN: return "EOF";
		case TokenType::THIS: return "THIS";

		case TokenType::CONST: return "CONST";
		case TokenType::JAVA: return "JAVA";
		case TokenType::IF: return "IF";
		case TokenType::FOR: return "FOR";
		case TokenType::WHILE: return "WHILE";
		case TokenType::RETURN: return "RETURN";
		case TokenType::ELSE: return "ELSE";
		case TokenType::DO: return "DO";
		case TokenType::TEMPLATE_LIT: return "TEMPLATE_LIT";
		case TokenType::BANG: return "BANG";
		case TokenType::AND: return "AND";
		case TokenType::OR: return "OR";
		case TokenType::BACKTICK: return "BACKTICK";
		case TokenType::TEMPLATE_PART: return "TEMPLATE_PART";

		default: return "UNKNOWN";
	}
}

} // zenith::lexer